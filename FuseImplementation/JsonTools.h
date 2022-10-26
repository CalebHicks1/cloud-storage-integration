#ifndef JSON_TOOLS
#define JSON_TOOLS


#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <jansson.h>
#include <sys/stat.h>

int json_list_append(json_t ** filelist, json_t * new_file);
int json_list_remove(json_t ** filelist, json_t * file, int num_files);
const char* getJsonFileName(json_t* file);

int parseJsonString(json_t** fileListAsArray, char stringArray[][1024], int numberOfLines);

#endif
