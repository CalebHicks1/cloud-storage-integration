#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <stdio.h>
#include <fcntl.h>              /* Obtain O_* constant definitions */
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>

#define WRITE_END 1
#define READ_END 0
/*
params:
    in: file descriptor that child will read from
    out: file descriptor that child will write to
    exec_dir: directory of module executable
    len: length of exec_dir 
*/
int spawn_module(int *in, int*out, char*exec_dir) {

    int write_pipe[2]; // pipe that the child writes to
    int read_pipe[2]; // pipe that the child reads from
    pipe(write_pipe);
    pipe(read_pipe);

    int flags = O_CLOEXEC;

    // create child process
    pid_t child_pid = fork();
    if (child_pid == 0) { // child process
        
        // the child writes to write_pipe[WRITE_END]
        dup2(write_pipe[WRITE_END], 1);
        // the child won't read from the pipe it writes to.
        close(write_pipe[READ_END]);
        // the child reads from read_pipe[READ_END]
        dup2(read_pipe[READ_END], 0);
        // the child won't write to the pipe it reads from
        close(read_pipe[WRITE_END]);
        //printf("exec...\n");

        char*args[] = {exec_dir, NULL};
        execv(exec_dir, args);

        //exit(0);
    } else { // the parent
        close(write_pipe[WRITE_END]);
        close(read_pipe[READ_END]);
    }

    *in = read_pipe[WRITE_END];
    *out = write_pipe[READ_END];    

    return 0;
}


