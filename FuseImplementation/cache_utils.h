#ifndef CACHE_UTILS_H
#define CACHE_UTILS_H
#include "Drive.h"
#include "logging.h"
#include "subdirectories/SubDirectory.h"
#include "api.h"
#include "MultiList.h"
int cache_find_or_download(char ** res, char * path);
void addPathToDeleteLog(const char *logPath, char *directory);
int download_file(int fdin, int fdout, char *downloadFile, char *filename, char *cachePath);
int add_subdirectory_to_cache(char* path);

#endif