import os
import time
import signal
import sys
import json

def spawn_api(drive_data):

    #Pipe from parent to child
    read_end1, write_end1 = os.pipe()


    #Pipe from child to parent
    read_end2, write_end2 = os.pipe()

    pid = os.fork()
    
    if pid == 0: #child process
        
        #Close pipe ends we don't need
        os.close(write_end1)
        os.close(read_end2)

        os.dup2(read_end1, 0) #Our stdin is now the parent's pipe
        os.dup2(write_end2, 1) #Our stdout will now be sent to the parent
        os.dup2(write_end2, 2) #Our stderr will also be sent to the parent

        #Exec the google drive client
        os.chdir(drive_data[2])
        executable = drive_data[1]
        os.execve(executable, ["/usr/bin/sudo", executable], {})

    else: #parent process
        
        os.close(write_end2)
        os.close(read_end1)

        #Set our global file descriptors to the proper pipes
        drive_data[3] = write_end1
        drive_data[4] = read_end2


def delete_files():
    #Open delete file
    try :
        deleted = open(path + ".delete", "r")
    except :
        print("Nothing to be deleted")
        return

    #Parse through delete file and format a JSON list of files to be deleted
    line = deleted.readline()
    file_list = "["
    while line:
        file_list = file_list + '"' + line[:-1] + '", '
        line = deleted.readline()
    file_list = file_list[:-2] + "]"
    print(file_list)

    #Remove the deleted file to be ready for the next use
    deleted.close()
    os.remove(path + ".delete")

    #Send out delete command to all APIs
    for drive in drives:
        os.write(drive[3], bytes('{"command":"delete","path":"","files":' + file_list + '}\n', 'utf-8'))
        print(os.read(drive[4], 100).decode('utf-8'))

    # TODO --- Not sure if we should wait for output here, I think drives will only write something on an error 
    # but if a file doesn't exist that would be an error.
    # And error data sitting in the pipe could mess up getting the remote filelists
    


def get_file_list(drive):
    os.write(drive[3], bytes('{"command":"list","path":"","files":[]}\n', 'utf-8'))

    temp_buffer = os.read(drive[4], 4000).decode('utf-8')

    if temp_buffer.__contains__("[") : #We recieved JSON formatted output
        if temp_buffer.__contains__("]") : #We recieved the entire output in this one buffer
            
            return json.loads(temp_buffer)

        else :#We need to continue recieving buffered output
            buffer = ""
            while not temp_buffer.__contains__("]"):
                index = temp_buffer.rindex('}')
                buffer = buffer + temp_buffer[:index+1]
                leftover = temp_buffer[index+1:]
                temp_buffer = leftover + os.read(drive[4], 4000).decode('utf-8')

            return json.loads(buffer + temp_buffer)

    #We did not recieve JSON output so we do not have a filelist this time
    return []


def shutdown_api(drive_data):
    os.write(drive_data[3], bytes('{"command":"shutdown"}\n', 'utf-8'))



def sigint_handler(signal, frame):
    print("\nCaptured SIGINT")
    for drive in drives:
        shutdown_api(drive)
    sys.exit(0)



def main():

    global drives

    signal.signal(signal.SIGINT, sigint_handler)

    for drive in drives:
        spawn_api(drive)

    while True:

        delete_files()

        #Creates a list of filenames and the time they were last modified from the given directory
        #Currently only includes files modified after the most recent run 
        #Also excludes files starting with a '.' to avoid uploading the .delete and .lock files
        files = [[x, os.path.getmtime(path + x)] for x in os.listdir(path) if os.path.getmtime(path + x) > time.time() - WAIT_TIME and x[0] != '.']

        #Get list of all files from all drives
        remote_filelists = []
        for drive in drives:

            #Get the file list from this drive and append it to the overall list
            remote_filelists.append(get_file_list(drive))

        #At this point we have a list of all local files which have been modified in 'files', 
        #and a list of files from each drive all stored in 'remote_filelists'

        #Formats a string to hold a json list of the files that need to be uploaded
        if files:
            filenames = "["
            for cur_file in files:
                filenames += ('"' + path + cur_file[0] +'", ')
            filenames = filenames[:-2] + "]"

            #Loops through and writes local files to all remote drives
            for drive in drives:
                os.write(drive[3], bytes('{"command":"upload","path":"","files":' + filenames + '}\n', 'utf-8'))


        #Sleeps for the specified wait time before running again
        time.sleep(WAIT_TIME)


#Path to the test directory we are observing
path = "/home/ubuntu/cache_dir/"

#Current wait time between runs is 5 minutes, or 300 seconds
WAIT_TIME = 300

#Name of drive, drive executable, path to drive executable, to_api filedescriptor, from_api file descriptor
drives = [
    ["google_drive", "./google_drive_client", "../src/API/google_drive/", -1, -1], 
    #["dropbox", "", "../src/API/dropbox/", -1, -1]
    ]


main()