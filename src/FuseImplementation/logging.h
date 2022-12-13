/*Logging ********************************************************/
#ifndef LOGGING_H
#define LOGGING_H
//		Call these like you would printf
void fuse_log(char * fmt, ...);
void fuse_log_error(char * fmt, ...);
//		Setup error log file
void initialize_log(void);
void __initialize_log(char * name);
//		Callbacks
void __fuse_log(const char* caller_name, char * fmt, ...);
void __fuse_log_error(const char* caller_name, char * fmt, ...);
#define fuse_log(...) __fuse_log(__func__, __VA_ARGS__)
#define fuse_log_error(...) __fuse_log_error(__func__, 	__VA_ARGS__)
//see: https://stackoverflow.com/questions/16100090/how-can-we-know-the-caller-functions-name
#endif

