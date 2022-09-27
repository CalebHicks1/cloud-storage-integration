/*Logging ********************************************************/
//Call me like you would printf
void fuse_log(char * fmt, ...);
void __fuse_log(const char* caller_name, char * fmt, ...);
void __fuse_log_error(const char* caller_name, char * fmt, ...);
void fuse_log_error(char * fmt, ...);
void initialize_log(char * log_name);
char * error_log_filename = "errors.txt";
#define fuse_log(...) __fuse_log(__func__, __VA_ARGS__)
#define fuse_log_error(...) __fuse_log_error(__func__, 	__VA_ARGS__)
//see: https://stackoverflow.com/questions/16100090/how-can-we-know-the-caller-functions-name

/**
 * Reset the file and note the date and time
 */
void initialize_log(char * log_name) {
	FILE * log = fopen(error_log_filename, "w");
	//https://stackoverflow.com/questions/1442116/how-to-get-the-date-and-time-values-in-a-c-program
	time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    fprintf(log, "Generated at: %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	fclose(log);
}


void __fuse_log(const char* caller_name, char * fmt, ...) {
	//We can add logging to a file here if/when we want to do that
	printf("[%s] ", caller_name);
	va_list arguments;
	va_start(arguments, fmt);
	vprintf(fmt, arguments);
}

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
