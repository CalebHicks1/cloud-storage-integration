#define _GNU_SOURCE /* See feature_test_macros(7) */
#include <stdio.h>
#include <fcntl.h> /* Obtain O_* constant definitions */
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include "api.h"


#define WRITE_END 1
#define READ_END 0

int shutdown(pid_t pid, int in, int out)
{
    dprintf(in, "{\"command\":\"shutdown\"}\n");
    // ssize_t read_size = read(out, lines[cnt++], LINE_MAX_BUFFER_SIZE);

    // we don't need these file descriptors anymore
    close(out);
    close(in);
    if (kill(pid, 0) == 0)
    {
        kill(pid, SIGKILL);
    }
    return 0;
}

int is_process_running(pid_t pid)
{
  
  
    if (kill(pid, 0) == 0)
    {
        return 1;
    }
    return 0;
}
/*
params:
    in: file descriptor that child will read from
    out: file descriptor that child will write to
    exec_dir: directory of module executable
    len: length of exec_dir
*/
int spawn_module(int *in, int *out,  pid_t *pid, char *exec_dir, char *exec_arg)
{

    int write_pipe[2]; // pipe that the child writes to
    int read_pipe[2];  // pipe that the child reads from
    pipe(write_pipe);
    pipe(read_pipe);

    int flags = O_CLOEXEC;

    // create child process
    pid_t child_pid = fork();
    if (child_pid != 0)
    { // child process

        // the child writes to write_pipe[WRITE_END]
        if (dup2(write_pipe[WRITE_END], 1) == -1)
        {
            return -1; // kill(child_pid, SIGKILL);
        }
        // the child won't read from the pipe it writes to.
        close(write_pipe[READ_END]);
        // the child reads from read_pipe[READ_END]
        if (dup2(read_pipe[READ_END], 0) == -1)
        {
            return -1; // kill(child_pid, SIGKILL);
        }
        // the child won't write to the pipe it reads from
        close(read_pipe[WRITE_END]);
        // printf("exec...\n");

        char *args[] = { /*"/usr/bin/sudo",*/ exec_dir, exec_arg, NULL};
        if (execv(exec_dir, args) == -1)
        {
            return -1;
        }

        // exit(0);
    }
    else
    { // the parent
        close(write_pipe[WRITE_END]);
        close(read_pipe[READ_END]);
    }

    *in = read_pipe[WRITE_END];
    *out = write_pipe[READ_END];
    *pid = child_pid;

    return 0;
}
