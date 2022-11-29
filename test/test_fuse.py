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
    test_write()
    test_delete()

    shutdown_fuse(fuse_proc) # unmount fuse, delete directory

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
        f"cat {FUSE_MOUNT_POINT}/testread.txt", 
        "this is a file for unit tests, please do not change it"
    )

    # Check that file exists in cache
    run_test( 
        f"cat mnt/ramdisk/testread.txt", 
        "this is a file for unit tests, please do not change it"
    )

def test_write():
    print("\nTesting Write -------------\n")

    # write to new file
    run_test(
        f"echo test > {FUSE_MOUNT_POINT}/test_write",
        ""
    )
    # test file has been written to cache
    run_test(
        f"cat mnt/ramdisk/test_write",
        "test\n"
    )
    # test file has been written to fuse mount point
    run_test(
        f"cat {FUSE_MOUNT_POINT}/test_write",
        "test\n"
    )
    output("rm mnt/ramdisk/test_write")

    # output(f"rm {FUSE_MOUNT_POINT}/test_write_file")

def test_delete():
    print("\nTesting Delete -------------\n")
    """
    1. create file
        touch fusemtpt/test_delete
    2. remove file
        rm fusemtpt/test_delete
    3. check that file is not in fuse, cache
    4. check that file is added to .delete file
    """
    # delete delete.txt file in cache
    output("rm mnt/ramdisk/delete.txt")
    run_test(
        f"echo test > {FUSE_MOUNT_POINT}/unit_test_delete",
        ""
    )
    run_test(
        f"rm {FUSE_MOUNT_POINT}/unit_test_delete",
        ""
    )

    print("Checking file has been removed from fuse mount...")
    file_list = output(f"ls {FUSE_MOUNT_POINT}")
    if "unit_test_delete" not in file_list:
        print("Success\n")
    else:
        print("File exists in mount point")

    print("Checking file has been added to .delete.txt")
    deleted_file_list = output(f"cat mnt/ramdisk/.delete.txt")
    if "unit_test_delete" in deleted_file_list:
        print("Success\n")
    else:
        print("File exists in mount point")

def test_write_dir():
    print("\nTesting new directory -------------\n")
    print("clearing cache")
    output("rm mnt/ramdisk/*")
    run_test(
        f"mkdir {FUSE_MOUNT_POINT}/test_new_dir",
        ""
    )
    print("Checking folder has been added to fuse mount...")
    file_list = output(f"ls {FUSE_MOUNT_POINT}")
    if "test_new_dir" in file_list:
        print("Success\n")
    else:
        print("File exists in mount point")
    
    print("Checking folder has been added to cache...")
    file_list = output(f"ls mnt/ramdisk/")
    if "test_new_dir" in file_list:
        print("Success\n")
    else:
        print("File exists in mount point")




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
    try:
        output = subprocess.check_output(command, shell=True).decode('utf-8')
    except Exception as e:
        output = e
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

if __name__ == '__main__':
    main()