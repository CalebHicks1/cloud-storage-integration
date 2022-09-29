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

#define LEN_DIRNAME 200
#define LEN_EXEC_PATH 200
#define NUM_SUBDIRS 20
struct Sub_Directory
{
	char dirname[LEN_DIRNAME];
	json_t *FileList;
	int num_files;
	json_t * self;	//"self" is actually unecessary but will leave it in
					//in case it's needed later
};
typedef struct Sub_Directory Sub_Directory;
struct Drive_Object
{
	char dirname[LEN_DIRNAME];
	json_t *FileList;
	int fd;
	char exec_path[LEN_EXEC_PATH];
	int num_files;
	Sub_Directory sub_directories[NUM_SUBDIRS];
	int num_sub_directories;
};
typedef struct Drive_Object Drive_Object;

#define NUM_DRIVES 2
#define LINE_MAX_BUFFER_SIZE 1024

//Important Setup methods
int populate_filelists();
int update_drive(int i);

//Access files
json_t * get_file(int drive_index, char * path) ;

//Getting FileLists
int __myGetFileList(char lines[][LINE_MAX_BUFFER_SIZE], char *cmd, char * optional_path);
int myGetFileList(char lines[][LINE_MAX_BUFFER_SIZE], char *cmd);
int listAsArray(json_t** list, char* cmd, char * optional_path);

//Subdirectories
int get_subdirectory_contents(json_t ** list, int drive_index,  char *path);
Sub_Directory * handle_subdirectory(char * path);

//Interal helpers
int is_drive(const char *path);
struct Drive_Object *get_drive(const char *path);
int get_drive_index(const char *path);
char * parse_out_drive_name(char * path);
int get_file_index(const char *path, int driveIndex);
Sub_Directory * __get_subdirectory_for_path(int drive_index, char * path);
json_t * __get_file_subdirectory(Sub_Directory * subdir, char * path);

