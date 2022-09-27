#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <jansson.h>
#include <sys/stat.h>
#include "JsonTools.h"

struct Sub_Directory
{
	char dirname[200];
	json_t *FileList;
	int num_files;
};
typedef struct Sub_Directory Sub_Directory;
struct Drive_Object
{
	char dirname[200];
	json_t *FileList;
	int fd;
	char exec_path[200];
	int num_files;
	Sub_Directory sub_directories[20];
	int num_sub_directories;
};
typedef struct Drive_Object Drive_Object;

#define NUM_DRIVES 3
#define LINE_MAX_BUFFER_SIZE 1024

//Important Setup methods

int populate_filelists();
int update_drive(int i);

//Getting FileLists
int __myGetFileList(char lines[][LINE_MAX_BUFFER_SIZE], char *cmd, char * optional_path);
int myGetFileList(char lines[][LINE_MAX_BUFFER_SIZE], char *cmd);

//Interal helpers
int is_drive(const char *path);
struct Drive_Object *get_drive(const char *path);
int get_drive_index(const char *path);
char * parse_out_drive_name(char * path);
int get_file_index(const char *path, int driveIndex);

