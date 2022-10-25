import subprocess
import os
import uuid
import time

TEST_ID = uuid.uuid4()
FUSE_MOUNT_POINT = f"fuse_mount_point_{TEST_ID}"

# Helpers -------------------------------------------------------

def setup_fuse():
    print("\nstartup ------------------\n")
    change_dir("../FuseImplementation")
    os.system(f"mkdir {FUSE_MOUNT_POINT}")
    fuse_proc = subprocess.Popen(["./fuse", "-f", FUSE_MOUNT_POINT], stdout=subprocess.PIPE)
    return fuse_proc

def shutdown_fuse(fuse_process):
    fuse_process.terminate()
    time.sleep(1) # wait for fuse to shutdown
    os.system(f"rm -r {FUSE_MOUNT_POINT}")

def change_dir(path):
    os.chdir(path)

def log_result(results, test_case, passed):
    line_length = 20
    passed_string = "FAIL" 
    if passed:
        passed_string = "PASS"
    filler = line_length-len(test_case)
    new_line = f"{test_case} {'-'*filler} {passed_string}\n" 
    results+= new_line
    return results
  
def command(command):
    print(f"Running command: {command}")

    output = subprocess.check_output(command, shell=True).decode('utf-8')

    print("output ---\n", output, "\n----------")

    print()

# Tests ---------------------------------------------------------

def test_read(fuse_proc):
    results = ""
    print("\ntesting read -------------\n")
    command(f"cat {FUSE_MOUNT_POINT}/Google_Drive/testread.txt")
    # command(["cat", f"{FUSE_MOUNT_POINT}/Google_Drive/testread.txt"])
    results = log_result(results, "hello world", True)
    # print_output(fuse_proc)
    print(results)
    

def main():
    fuse_proc = setup_fuse()
    time.sleep(2) # wait for fuse to start
    test_read(fuse_proc)
    shutdown_fuse(fuse_proc)
    

if __name__ == '__main__':
    main()