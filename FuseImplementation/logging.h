/*Logging ********************************************************/
//Call me like you would printf
#ifndef LOGGING_H
#define LOGGING_H
void fuse_log(char * fmt, ...);
void __fuse_log(const char* caller_name, char * fmt, ...);
void __fuse_log_error(const char* caller_name, char * fmt, ...);
void fuse_log_error(char * fmt, ...);
void initialize_log(void);
#define fuse_log(...) __fuse_log(__func__, __VA_ARGS__)
#define fuse_log_error(...) __fuse_log_error(__func__, 	__VA_ARGS__)
//see: https://stackoverflow.com/questions/16100090/how-can-we-know-the-caller-functions-name
#endif

