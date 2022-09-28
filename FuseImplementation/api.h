/*
params:
    in: file descriptor that child will read from
    out: file descriptor that child will write to
    exec_dir: directory of module executable
    len: length of exec_dir 
*/
int spawn_module(int *in, int*out, char*exec_dir);