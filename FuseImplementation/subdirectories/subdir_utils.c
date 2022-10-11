#include "SubDirectory.h"
#include "subdir_utils.h"
#include "../logging.h"
#define NUM_SPLIT 20
char ** split(char * path)
{
	//fuse_log("Split %s\n", path);
	//So we don't wreck the variable 
	char * cpy_path = calloc(sizeof(char), strlen(path) + 1);
	strcpy(cpy_path, path);
	
	char ** ret = calloc(sizeof(char*), NUM_SPLIT);
	char ** cpy_ret = ret;
	
	char * token = strtok(cpy_path, "/");
	
	while (token != NULL) {
		*(cpy_ret) = calloc(sizeof(char), strlen(token) + 1);
		strcpy(*cpy_ret, token);
		cpy_ret++;
		//fuse_log("token: %s\n", token);
		token = strtok(NULL, "/");
	}
	
	free(cpy_path);
	return ret;
	
}

int count_tokens(char ** split) {
	int ret = 0;
	while (*(split + ret) != NULL) { ret++; }
	return ret;
}

void split_free(char ** split) 
{
	
}

