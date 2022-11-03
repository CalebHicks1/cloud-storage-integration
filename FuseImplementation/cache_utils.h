#ifndef CACHE_UTILS_H
#define CACHE_UTILS_H
#include "Drive.h"
#include "logging.h"
#include "subdirectories/SubDirectory.h"
#include "api.h"
#include "MultiList.h"
int cache_find_or_download(char ** res, char * path);

int download_file(int fdin, int fdout, char *downloadFile, char *filename, char *cachePath);

#endif