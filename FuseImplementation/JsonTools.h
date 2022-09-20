#ifndef JSON_TOOLS
#define JSON_TOOLS


#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <jansson.h>
#include <sys/stat.h>
const char* getJsonFileName(json_t* file);

int parseJsonString(json_t** fileListAsArray, char stringArray[][255], int numberOfLines);

#endif