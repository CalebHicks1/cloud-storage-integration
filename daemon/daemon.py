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

    #print("File descriptors: " + str(drives[0][3]) + " " + str(to_api))
    while True:

        #Creates a list of filenames and the time they were last modified from the given directory
        #Currently only includes files modified within the last five minutes
        files = [[x, os.path.getmtime(path + x)] for x in os.listdir(path) if os.path.getmtime(path + x) > time.time() - 30000000]

        if not files:
            time.sleep(8)
            print("No files this time")
            continue

        #Get list of all files from all drives
        remote_filelists = []
        i = 0
        for drive in drives:
            os.write(drive[3], bytes('{"command":"list","path":"","files":[]}\n', 'utf-8'))

            output = os.read(drive[4], 1024).decode('utf-8')

            if output.__contains__("["): #Parse the JSON returned by the API
                y = json.loads(output)
                print(y)
                remote_filelists.append(y)
                
            else: #Sometimes drives return plain strings which would crash the json parser
                print(output)
            i = i + 1


        #At this point we have a list of all local files which have been modified in 'files', 
        #and a list of files from each drive all stored in 'remote_filelists'


        #Formats a string to hold a json list of the files that need to be uploaded
        if files:
            filenames = "["
            for cur_file in files:
                filenames += ('"' + path + cur_file[0] +'", ')
            filenames = filenames[:-2] + "]"

            #Loops through and writes to all 
            #for drive in drives:
                #os.write(drive[3], bytes('{"command":"upload","path":"","files":' + filenames + '}\n', 'utf-8'))


        #Runs every 5 minutes
        time.sleep(8)


#Path to the test directory we are observing
path = "/home/ubuntu/cache_dir/"

#Name of drive, drive executable, path to drive executable, to_api filedescriptor, from_api file descriptor
drives = [
    ["google_drive", "./google_drive_client", "../src/API/google_drive/", -1, -1], 
    #["dropbox", "", "../src/API/dropbox/", -1, -1]
    ]


main()