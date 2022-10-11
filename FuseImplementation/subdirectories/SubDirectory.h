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
	SubDirectory * prev;	//special case for subdir_find_file
	
};
typedef struct Get_Result Get_Result;
json_t * SubDirectory_find_file(SubDirectory * dir, char * path);
Get_Result * get_subdirectory(int drive_index, char * path);

int insert_subdirectory(int drive_index, SubDirectory * subdir);

json_t * subdir_find_file(int drive_index, char * path);
SubDirectory * SubDirectory_create(char * name);
void dump_subdirectory(SubDirectory * subdir, int indent);
