#ifndef SUBDIRECTORY_H
#define SUBDIRECTORY_H
#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <jansson.h>
#include <sys/stat.h>
#include "../JsonTools.h"
#include "../list/list.h"
#define LEN_DIRNAME 200

/**
 * Example:
 * dirname = /drive/folder/folder/this
 * rel_dirname = this
 * FileList = 
 * {
 * 		file.txt,
 * 		file.txt,
 * 		another_dir1
 * 		another_dir2
 * }
 * num_files = 4
 * subdirectories_list = 
 * {
 * {dirname = another_dir1, ...}
 * {dirname = another_dir2, ...}
 * }
 */
struct SubDirectory 
{
	struct list_elem elem;
	char dirname[LEN_DIRNAME];
	char rel_dirname[LEN_DIRNAME];
	json_t *FileList;
	int num_files;
	struct list subdirectories_list;
	
};
typedef struct SubDirectory SubDirectory;

//-----------------------Get--------------------------
enum Result_Type
{
	THIS,	//The path specified IS the subdirectory returned
	ELEMENT, //The path specified is an ELEMENT of the subdirectory returned
	ROOT,	//The path specified is at the ROOT level of the drive
	ERROR	// :)
};

struct Get_Result
{
	enum Result_Type type;
	SubDirectory * subdirectory;
	SubDirectory * parent; //Case (1) below
	
};
typedef struct Get_Result Get_Result;
//Finding things
json_t * Subdirectory_find_file(int drive_index, char * path);	//Find a file that exists in some subdirectory in drive 
json_t * SubDirectory_find_file_in_dir(SubDirectory * dir, char * path);	//Find a file residiing within a single subdirectory (not its children)
void update_subdir_file_size(const char *path, const char* filename, int drive_index, int newsize);
/**
 * For the file specified by path, find 
 * (1) The subdirectory that IS that file
 * 		Set Result->parent to be the parent of said subdirectory
 * (2) The subdirectory CONTAINING that file
 * or
 * (3) Return NULL if the file resides in the root directory of the drive, not 
 * in a subdirectory
 */
Get_Result * get_subdirectory(int drive_index, char * path);	

//Insert a newly created file into a subdirectory
int SubDirectory_insert(SubDirectory * dir, json_t * file);

//Place a newly generated subdirectory in a drive
int insert_subdirectory(int drive_index, SubDirectory * subdir);

//"Constructor" - just initializes names and list, nothing else
SubDirectory * SubDirectory_create(char * name);

//Essentially gives the output of "tree" (which honestly is probably more useful for debugging)
void dump_subdirectory(SubDirectory * subdir, int indent);

#endif
