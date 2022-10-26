import subprocess
import os
import uuid
import time

"""
TODO:
    1. figure out how to read output from fuse

"""

TEST_ID = uuid.uuid4()
FUSE_MOUNT_POINT = f"fuse_mount_point_{TEST_ID}"

def main():
    fuse_proc = setup_fuse()
    time.sleep(2) # wait for fuse to start
    
    test_read() # run read tests

    shutdown_fuse(fuse_proc) # unmount fuse, delete directory

# Helpers -------------------------------------------------------

def setup_fuse():
    print("\nstartup ------------------\n")
    change_dir("../FuseImplementation")
    os.system(f"mkdir {FUSE_MOUNT_POINT}")
    fuse_proc = subprocess.Popen(["./fuse", "-f", FUSE_MOUNT_POINT], stdout=subprocess.PIPE)
    return fuse_proc

def shutdown_fuse(fuse_process):
    print("shutdown ------------------\n")
    fuse_process.terminate()
    time.sleep(1) # wait for fuse to shutdown
    os.system(f"rm -r {FUSE_MOUNT_POINT}")

def change_dir(path):
    os.chdir(path)
  
# given a command, run it and return the output
def output(command):
    output = subprocess.check_output(command, shell=True).decode('utf-8')
    return output

# given actual and expected strings, compare. If not equal, print message
def compare(actual, expected):
    if actual != expected:
        print(f"Expected: [{expected}]\nActual:   [{actual}]")
    else:
        print("Success")

# run the given command, then check the output against the expected output
def run_test(command, expected):
    print(f"Running command: [{command}]")
    actual = output(command)
    compare(actual, expected)
    print()

# Tests ---------------------------------------------------------

"""
    To add new tests:
    make a call to run_test with a command to be run in the shell
    and expected output.
"""
def test_read():
    print("\nTesting Read -------------\n")

    # Check that we can read a file that does not exist in the cache
    output("rm mnt/ramdisk/testread.txt") # remove file from cache. don't care about output
    run_test(
        f"cat {FUSE_MOUNT_POINT}/Google_Drive/testread.txt", 
        "this is a file for unit tests, please do not change it"
    )

    # Check that file exists in cache
    run_test( 
        f"cat mnt/ramdisk/testread.txt", 
        "this is a file for unit tests, please do not change it"
    )

    
    

if __name__ == '__main__':
    main()