#include "SubDirectory.h"
#include "../Drive.h"
#include "../logging.h"
#include "subdir_utils.h"
extern Drive_Object Drives[NUM_DRIVES];
SubDirectory * SubDirectory_create(char * name)
{
	SubDirectory * ret = calloc(sizeof(SubDirectory), 1);
	strcpy(&(ret->dirname[0]), name);
	list_init(&(ret->subdirectories_list));
	
	//Find relative dirname
	char * cpy = name;
	char * last_slash = NULL;
	while (*cpy != '\0')
	{
		if (*cpy == '/')
		{
			last_slash = cpy + 1;
		}
		cpy++;
		
	}
	if (last_slash == NULL)
	{
		fuse_log_error("Could not get relative path\n");
		ret->rel_dirname[0] = '\0';
	}
	else
	{
		strcpy(&(ret->rel_dirname[0]), last_slash);
	}
	//-----------------------
	return ret;
	
}

/**
 * Dump contents of subdirectory
 * subdir: directory to dump
 * indent: level of indentation - should reflect depth
 * first level of subdirectories -> 1, second -> 2, etc
 */
void dump_subdirectory(SubDirectory * subdir, int indent)
{
	//Do identation stuff for readability
	char * indents = calloc(sizeof(char), indent + 1);
	for (int i = 0; i < indent; i++) {
		*(indents + i) = '\t';
	}
	//Print *this* subdirectories attributes
	printf("%s|--- [%s] (%s) (files: %d)\n", indents, subdir->rel_dirname, subdir->dirname, subdir->num_files);
	
	//Print files
	for (int i = 0; i < subdir->num_files; i++) {
		json_t * curr_file = json_array_get(subdir->FileList, i);
		printf("\t%s|--- %s\n", indents, getJsonFileName(curr_file));
	}
	
	//dump sub-subdirectories
	struct list_elem * e;
	for (e = list_begin(&(subdir->subdirectories_list)); 
		e != list_end(&(subdir->subdirectories_list)); 
		e = list_next(e))
	{
		SubDirectory * curr = list_entry(e, struct SubDirectory, elem);
		dump_subdirectory(curr, indent + 1);
	}
	free(indents);
}

json_t * SubDirectory_find_file(SubDirectory * dir, char * path)
{
	for (int i = 0; i < dir->num_files; i++)
	{
		json_t *curr = json_array_get(dir->FileList, i);
		char fullname[500];
		strcpy(fullname, "\0");
		strcat(&fullname[0], dir->dirname);
		strcat(&fullname[0], "/");
		strcat(&fullname[0], getJsonFileName(curr));
		if (strcmp(fullname, path) == 0)
		{
			return curr;
		}
	}
	return NULL;
}

SubDirectory * find_subdirectory(struct list * subdirectory_list, char * relative_path)
{
	struct list_elem *e;
	for (e = list_begin(subdirectory_list); e != list_end(subdirectory_list); e = list_next(e))
	{
		SubDirectory * curr = list_entry(e, struct SubDirectory, elem);
		if (strcmp(curr->rel_dirname, relative_path) == 0)
		{
			return curr;
		}
		
	}
	return NULL;
}




void __get_subdirectory(Get_Result * result, SubDirectory * dir, char ** tokens)
{
	//fuse_log("In __get_subdirectory!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	//fuse_log("token: %s\n", *tokens);
	if (*tokens == NULL)
	{
		result->type = THIS;
		result->subdirectory = dir;
		//fuse_log("tokens was null\n");
		return;
	}
	SubDirectory * next = find_subdirectory(&(dir->subdirectories_list), *tokens);
	if (next == NULL)
	{
		//fuse_log("next was null\n");
		result->type = ELEMENT;
		result->subdirectory = dir;
		return;
	}
	//Keep searching
	__get_subdirectory(result, next, tokens + 1);
	
}

/**
 * Return subdirectory identified by path or the subdirectory that does/would
 * contain the file specified by path
 */
Get_Result * get_subdirectory(int drive_index, char * path)
{
	fuse_log("getting subdirectory for %s\n", path);
	Get_Result * result = calloc(sizeof(Get_Result), 1);
	//Divide path into tokens
	char ** tokens = split(path);
	
	if (count_tokens(tokens) <= 2)	//This is at root level
	{
		fuse_log("Determined it should go at root directory\n");
		result->type = ROOT;
		return result;
	}
	
	//This is not at root level
	fuse_log("Is not at root level\n");
	//Find the first subdirectory in the path
	char * first_name = *(tokens + 1);
	//fuse_log("first_name: %s\n", first_name);
	SubDirectory * first = find_subdirectory(&(Drives[drive_index].subdirectories_list), first_name);
	if (first == NULL)
	{
		fuse_log_error(":(\n");
		result->type = ERROR;
		return result;
	}
	else {
		fuse_log("Found the first subdirectory!!!\n");
		__get_subdirectory(result, first, tokens + 2);
	}
	
	
	
	
}


/**
 * Place subdir in Drive - determine location to put it at
 */
int insert_subdirectory(int drive_index, SubDirectory * subdir)
{
	Get_Result * result = get_subdirectory(drive_index, &(subdir->dirname[0]));
	
	if (result->type == ROOT)
	{
		fuse_log("Inserting subdirectory %s into root directory\n", &(subdir->dirname[0]));
		list_push_front(&(Drives[drive_index].subdirectories_list), &(subdir->elem));
	}
	else if (result->type == THIS)
	{
		fuse_log_error("Subdirectory already exists\n");
		return -1;
	}
	else if (result->type == ELEMENT) 
	{
		fuse_log("Going to insert %s into subdirectory %s\n", subdir->dirname, result->subdirectory->dirname);
		list_push_front(&(result->subdirectory->subdirectories_list), &(subdir->elem));
	}
	else if (result->type == ERROR)
	{
		fuse_log_error("Location for subdirectory %s not found\n", subdir->dirname);
		return -1;
		
	}
	fuse_log("dumping drive post insertion\n");
	dump_drive(&(Drives[drive_index]));
	return 0;
	
	
}
