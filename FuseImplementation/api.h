
#ifndef API
#define API
/*
params:
    in: file descriptor that child will read from
    out: file descriptor that child will write to
    exec_dir: directory of module executable
    len: length of exec_dir 
*/
#include <signal.h>
int spawn_module(int *in, int *out,  pid_t *pid, char *exec_dir);
int shutdown(pid_t pid, int in, int out);
int is_process_running(pid_t pid);

#endif