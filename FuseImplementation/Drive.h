#ifndef DRIVE_H
#define DRIVE_H
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
#include "list/list.h"
#include "subdirectories/SubDirectory.h"

#define LEN_DIRNAME 200
#define LEN_EXEC_PATH 400
#define LEN_EXEC_ARG 200
#define MAX_EXECUTABLES 4
//#define NUM_SUBDIRS 20
//struct Sub_Directory
//{
	//char dirname[LEN_DIRNAME];
	//json_t *FileList;
	//int num_files;
	//json_t * self;	//"self" is actually unecessary but will leave it in
					////in case it's needed later
//};
//typedef struct Sub_Directory Sub_Directory;

//-------------------------- New subdirectory

//----------------------------------

struct Drive_Object
{
	char dirname[LEN_DIRNAME];
	json_t *FileList;
	int num_executables;
	int in[MAX_EXECUTABLES];
	int out[MAX_EXECUTABLES];
	pid_t pid[MAX_EXECUTABLES];
	char exec_path[MAX_EXECUTABLES][LEN_EXEC_PATH];
	char exec_arg[MAX_EXECUTABLES][LEN_EXEC_ARG];
	int num_files;
	//Sub_Directory sub_directories[NUM_SUBDIRS];
	int num_sub_directories;
	
	
	struct list subdirectories_list;
};
typedef struct Drive_Object Drive_Object;

#define NUM_DRIVES 2
#define LINE_MAX_BUFFER_SIZE 1024

int Drive_insert(int drive_index, char * path, json_t * file);
//Important Setup methods
int populate_filelists();
int update_drive(int i);

//Access files
json_t * get_file(int drive_index, char * path) ;

//Getting FileLists
int myGetFileList(char lines[][LINE_MAX_BUFFER_SIZE], char *cmd, char * optional_path, int in, int out);
//int myGetFileList(char lines[][LINE_MAX_BUFFER_SIZE], char *cmd);
int listAsArray(json_t** list, char cmd[MAX_EXECUTABLES][LEN_EXEC_PATH], char * optional_path, int in[MAX_EXECUTABLES], int out[MAX_EXECUTABLES]);

//Subdirectories
//int get_subdirectory_contents(json_t ** list, int drive_index,  char *path, int in, int out);
//int get_subdirectory_contents(json_t **list, int drive_index, char path[MAX_EXECUTABLES][LEN_EXEC_PATH], int in[MAX_EXECUTABLES], int out[MAX_EXECUTABLES]);
int get_subdirectory_contents(json_t **list, int drive_index, char * path, int in[MAX_EXECUTABLES], int out[MAX_EXECUTABLES]);
SubDirectory * handle_subdirectory(char * path);

//Interal helpers
int is_drive(const char *path);
struct Drive_Object *get_drive(const char *path);
int get_drive_index(const char *path);
char * parse_out_drive_name(char * path);
int get_file_index(const char *path, int driveIndex);
int kill_all_processes();


//Debug
void dump_drive(Drive_Object * drive);
#endif

