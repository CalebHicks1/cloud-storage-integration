#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <jansson.h>
#include <sys/stat.h>
#include "logging.h"
char * error_log_filename = "errors.txt";
char * subdir_log_name = "subdir_log.txt";
/**
 * Reset the file for __fuse_log_error and note the date and time
 */
void initialize_log(void) {
	//FILE * log = fopen(error_log_filename, "w");
	////https://stackoverflow.com/questions/1442116/how-to-get-the-date-and-time-values-in-a-c-program
	//time_t t = time(NULL);
    //struct tm tm = *localtime(&t);
    //fprintf(log, "Generated at: %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	//fclose(log);
	__initialize_log(error_log_filename);
	__initialize_log(subdir_log_name);
}

void __initialize_log(char * name) 
{
	FILE * log = fopen(name, "w");
	//https://stackoverflow.com/questions/1442116/how-to-get-the-date-and-time-values-in-a-c-program
	time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    fprintf(log, "Generated at: %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	fclose(log);
	
}


void __fuse_log_subdir(const char* caller_name, char * fmt)
{
	
	
	
}

/**
 * Callback for fuse_log macro
 */
void __fuse_log(const char* caller_name, char * fmt, ...) {
	//We can add logging to a file here if/when we want to do that
	printf("[%s] ", caller_name);
	va_list arguments;
	va_start(arguments, fmt);
	vprintf(fmt, arguments);
}

/**
 * Callback for fuse_log_error macro
 * Writes to stdout and to error_log_filename
 */
void __fuse_log_error(const char* caller_name, char * fmt, ...) {
	//Can clean this up later
	FILE * error_log = fopen(error_log_filename, "a");
	if (error_log == NULL) {
		perror("Failed to open error log file\n");
		return;
	}
	printf("---------> [%s] ", caller_name);
	fprintf(error_log, "---------> [%s] ", caller_name);
	va_list arguments;
	va_start(arguments, fmt);
	vprintf(fmt, arguments);
	va_end(arguments);
	va_start(arguments, fmt);
	vfprintf(error_log, fmt, arguments);
	fclose(error_log);
}

