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

    #Remove the deleted file to be ready for the next use
    deleted.close()
    os.remove(path + ".delete")

    #Send out delete command to all APIs
    for drive in drives:
        os.write(drive[3], bytes('{"command":"delete","path":"","files":' + file_list + '}\n', 'utf-8'))
        print(os.read(drive[4], 100).decode('utf-8'))
    


def get_file_list(drive, subdir):
    print(drive)
    os.write(drive[3], bytes('{"command":"list","path":"' + subdir + '","files":[]}\n', 'utf-8'))

    temp_buffer = os.read(drive[4], 4000).decode('utf-8')

    if temp_buffer.__contains__("[") : #We recieved JSON formatted output
        if not temp_buffer.__contains__("]") : #We need to continue recieving buffered output
            buffer = ""
            while not temp_buffer.__contains__("]"):
                index = temp_buffer.rindex('}')
                buffer = buffer + temp_buffer[:index+1]
                leftover = temp_buffer[index+1:]
                temp_buffer = leftover + os.read(drive[4], 4000).decode('utf-8')
            temp_buffer = buffer + temp_buffer

        output = json.loads(temp_buffer)
        for cur_file in output:
            cur_file['Name'] = subdir + "/" + cur_file['Name']

        return output

    #We did not recieve JSON output so we do not have a filelist this time
    return []

#Given a JSON array of files, remove the ones that were not recently modified and parse through all subdirectories
def clean_file_list(drive, file_list):

    file_list = [x for x in file_list if x['Modified'] > (last_ran - 10)]

    files = []
    files.append(file_list)

    for directory in files:
        for cur_file in directory:
            if cur_file['IsDir']: 
                #Create a local directory of the same name, remove it from the list, then get the file list
                try:
                    os.mkdir(path + cur_file['Name'][1:])
                except:
                    print("Directory already exists")
                subdir = get_file_list(drive, cur_file['Name'])
                if len(subdir) > 0:
                    files.append(subdir)
    return files


def push_local_changes(local_files, subdir):
    #Formats a JSON string containing the local files of a given directory that need to be pushed out
    filenames = "["
    for cur_file in local_files:
        filenames = filenames + '"' + path + cur_file + '", '
    filenames = filenames[:-2] + "]"

    #Loops through and writes the specified directory to all remote drives
    for drive in drives:
        os.write(drive[3], bytes('{"command":"upload","path":"' + subdir + '","files":' + filenames + '}\n', 'utf-8'))

    for drive in drives:
        os.read(drive[4], 300).decode('utf-8')


def sync_remote_drives(remote_filelists):
    #Parse through each remote filelist
    #Download the files that were recently changed and add them to the list
    downloaded = {}
    i = 0

    for drive in remote_filelists:
        for file_list in drive:

            #Initialize all variables needed for this iteration
            temp_file_list = []
            temp_file_string = "["
            subdir = ""
            nested_subdirs = []

            #Get subdirectory
            if len(file_list) > 0:
                file = file_list[0]['Name']
                subdir = file[1:file.rindex('/')]
                if not subdir in downloaded:
                    downloaded[subdir] = []
                
            #Format the download string from all files within this directory, excluding subdirectories
            for cur_file in file_list:
                if not cur_file['IsDir']:
                    temp_file_list.append(cur_file['Name'])
                    temp_file_string = temp_file_string + '"' + cur_file['Name'] + '", '
                else:
                    nested_subdirs.append(cur_file['Name'][1:])
            
            #We have to separately track the subdirectories to upload them because they are created locally, not downloaded
            for dir_name in nested_subdirs:
                downloaded[subdir].append(dir_name)
            
            #We have files recently modified so we need to download them
            if len(temp_file_string) > 2:
                temp_file_string = temp_file_string[:-2] + "]"
                os.write(drives[i][3], bytes('{"command":"download","path":"' + path + subdir +'","files":' + temp_file_string + '}\n', 'utf-8'))
                output = os.read(drives[i][4], 300).decode('utf-8')
                print("Downloading output: " +output)

                #Parse through the error response and make note of all files successfully downloaded so they can later be pushed out
                for name in temp_file_list:
                    if not output.__contains__(name) and not name in downloaded[subdir]:
                        downloaded[subdir].append(name)
        i += 1
    
    #At this point all files that were remotely modified have been downloaded and
    #can be pushed out to all drives

    #Nothing was changed remotely that we are able to sync, so just return
    if len(downloaded) == 0:
        return

    #Send a separate upload for each subdirectory so structure is maintained
    for subdir in downloaded:
        
        #Format an upload string containing all files that were downloaded
        upload_string = "["
        for file_name in downloaded[subdir]:
            upload_string = upload_string + '"' + path + file_name + '", '
        upload_string = upload_string[:-2] + "]"

        #Upload this list of files to all remote drives
        for drive in drives:
            os.write(drive[3], bytes('{"command":"upload","path":"' + subdir + '","files":' + upload_string + '}\n', 'utf-8'))

        #For now, just discarding output so it doesn't mess up later reads
        for drive in drives:
            print("Uploading output: " + os.read(drive[4], 300).decode('utf-8'))

    


def shutdown_api(drive_data):
    os.write(drive_data[3], bytes('{"command":"shutdown"}\n', 'utf-8'))



def sigint_handler(signal, frame):
    print("\nCaptured SIGINT")
    for drive in drives:
        shutdown_api(drive)
    sys.exit(0)



def main():

    global drives
    global last_ran

    signal.signal(signal.SIGINT, sigint_handler)

    for drive in drives:
        spawn_api(drive)

    while True:

        #Checks to ensure that FUSE is not touching the cache currently

        try : #If the lock file exists then FUSE is working, and we must wait
            lock = open(path + ".lock", "r")
            lock.close()
            print("Locked out")
            time.sleep(5)
            continue
        except : #If the lock file doesn't exist then we create it and continue our syncing
            lock = open(path + ".lock", "x")
            lock.close()
            print("I have the lock")
            

        last_ran = time.time()
        #Creates a list of filenames and the time they were last modified from the given directory
        #Currently only includes files modified after the most recent run 
        #Also excludes files starting with a '.' to avoid uploading the .delete and .lock files

        local_files = []
        local_subdirectories = []

        local_files.append([x for x in os.listdir(path) if os.path.getatime(path + x) > (last_ran - 10) and x[0] != '.'])
        local_subdirectories.append("")

        #Parses through all subdirectories repeating this process until all locally modified files are logged
        for directory in local_files:
            for file_name in directory:
                if os.path.isdir(path + file_name):
                    temp_path = path + file_name + "/"
                    dir_list = [file_name + "/" + x for x in os.listdir(temp_path) if os.path.getatime(temp_path + x) > (last_ran - 10) and x[0] != '.']
                    if len(dir_list) > 0:
                        local_files.append(dir_list)
                        local_subdirectories.append(file_name)

        #Get list of all files from all drives
        remote_filelists = []
        for drive in drives:
            #Get the file list from this drive, clean it and format into subdirectories, and append it to the overall list
            json = get_file_list(drive, "")
            all_files = clean_file_list(drive, json)
            remote_filelists.append(all_files)

        #At this point we have a list of all local files which have been modified in 'files', 
        #and a list of files from each drive all stored in 'remote_filelists'

        #Push out all locally modified files so they take precedence and aren't overwritten by syncing
        i = 0
        for directory in local_files:
            push_local_changes(directory, local_subdirectories[i])
            i = i + 1

        #Sync the remote drives with each other first so that local changes have priority
        if remote_filelists:
            sync_remote_drives(remote_filelists)

        
        
        print("Done pushing")

        #Parses the .delete file and deletes all files listed
        delete_files()

        #Removes the lock file so FUSE can resume, and sleeps for the specified wait time before running again
        os.remove(path + ".lock")
       
        time.sleep(WAIT_TIME)


#Path to the test directory we are observing
import os
CURR_DIR = os.getcwd()+ "/temp/"
print(CURR_DIR)

path = CURR_DIR

#Current wait time between runs is 5 minutes, or 300 seconds
WAIT_TIME = 15

#Keeps track of the last time the daemon was ran, starts at 0 to sync all files on the first run
last_ran = 0

#Name of drive, drive executable, path to drive executable, to_api filedescriptor, from_api file descriptor
drives = [
    ["google_drive", "./google_drive_client", "../API/google_drive/", -1, -1], 
    ["dropbox", "./drop_box", "../API/dropbox/", -1, -1]
]


main()