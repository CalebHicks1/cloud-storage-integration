#ifndef PATH_UTILS_H
#define PATH_UTILS_H
#include <libgen.h>
#include "Drive.h"
#include "logging.h"
char *form_cache_path(const char *directory, bool stripRoot, char **strippedFile);
void addPathToDeleteLog(const char *logPath, char *directory);
char*  strip_filename_from_directory(char* directory);
static void split_path_file(char **pathBuffer, char **filenameBuffer, const char *fullPath);
#endif