import os
import time
import signal
import sys

def spawn_api():

    global to_api
    global from_api

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
        os.chdir("../src/API/google_drive/")
        exec_dir = "./google_drive_client"
        os.execve(exec_dir, ["/usr/bin/sudo", exec_dir], {})

    else: #parent process
        
        os.close(write_end2)
        os.close(read_end1)

        #Set our global file descriptors to the proper pipes
        to_api = write_end1
        from_api = read_end2





def shutdown_api():
    os.write(to_api, bytes('{"command":"shutdown"}\n', 'utf-8'))



def sigint_handler(signal, frame):
    print("\nCaptured SIGINT")
    shutdown_api()
    sys.exit(0)



def main():

    signal.signal(signal.SIGINT, sigint_handler)

    spawn_api()

    while True:

        #Creates a list of filenames and the time they were last modified from the given directory
        #Currently only includes files modified within the last five minutes
        files = [[x, os.path.getmtime(path + x)] for x in os.listdir(path) if os.path.getmtime(path + x) > time.time() - 300]

        #Shows the files that will be uploaded
        print(files)

        #Formats a string to hold a json list of the files that need to be uploaded 
        filenames = "["
        for cur_file in files:
            filenames += ('"' + path + cur_file[0] +'", ')
        filenames = filenames[:-2] + "]"

        #Uploads all recently modified files to google drive
        os.write(to_api, bytes('{"command":"upload","path":"","files":' + filenames + '}\n', 'utf-8'))

        #Get list of all files from Google Drive
        #os.write(to_api, bytes('{"command":"list","path":"","files":[]}\n', 'utf-8'))

        #Prints output from google
        output = os.read(from_api, 1024)
        print(output.decode('utf-8'))

        #Runs every 5 minutes
        time.sleep(300)




#Path to the test directory we are observing
path = "/home/ubuntu/cache_dir/"

#Global variables for the pipes
to_api, from_api = -1, -1


main()