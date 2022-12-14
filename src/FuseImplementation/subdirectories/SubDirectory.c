#include "SubDirectory.h"
#include "../Drive.h"
#include "../logging.h"
#include "subdir_utils.h"
#include "../JsonTools.h"
extern Drive_Object Drives[NUM_DRIVES];
int SubDirectory_find_fileindex_in_dir(SubDirectory *dir, char *path);
/**
 * Set dirname, rel_dirname, and initialize FileList
 */
SubDirectory *SubDirectory_create(char *name)
{
	SubDirectory *ret = calloc(sizeof(SubDirectory), 1);
	strcpy(&(ret->dirname[0]), name);
	list_init(&(ret->subdirectories_list));

	/******* Get relative dirname *********/
	char *cpy = name;
	char *last_slash = NULL;
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
	/**************************************/

	return ret;
}

// Just append the file to the FileList
int SubDirectory_insert(SubDirectory *dir, json_t *file)
{
	//fuse_log(file);
	dump_subdirectory(dir, 0);
	int res = json_list_append(&dir->FileList, file);
	if (res >= 0)
	{
		fuse_log("Inserting file was successful\n");
		dir->num_files++;
	}
	else
	{
		fuse_log_error("Failed to insert new file into subdirectory\n");
	}
	return res;
}

/**
 * Dump contents of subdirectory
 * subdir: directory to dump
 * indent: level of indentation - should reflect depth
 * first level of subdirectories -> 1, second -> 2, etc
 */
void dump_subdirectory(SubDirectory *subdir, int indent)
{
	// Do identation stuff for readability
	char *indents = calloc(sizeof(char), indent + 1);
	for (int i = 0; i < indent; i++)
	{
		*(indents + i) = '\t';
	}
	// Print *this* subdirectories attributes
	printf("%s|--- [%s] (%s) (files: %d)\n", indents, subdir->rel_dirname, subdir->dirname, subdir->num_files);

	// Print files
	for (int i = 0; i < subdir->num_files; i++)
	{
		json_t *curr_file = json_array_get(subdir->FileList, i);
		printf("\t%s|--- %s\n", indents, getJsonFileName(curr_file));
	}

	// dump sub-subdirectories
	struct list_elem *e;
	for (e = list_begin(&(subdir->subdirectories_list));
		 e != list_end(&(subdir->subdirectories_list));
		 e = list_next(e))
	{
		SubDirectory *curr = list_entry(e, struct SubDirectory, elem);
		dump_subdirectory(curr, indent + 1);
	}
	free(indents);
}

/**
 * Recursively search the tree for the specified file
 * (1) Get the subdirectory corresponding to the path
 * (2) Search that subdirectory for the file, if applicable
 */
json_t *Subdirectory_find_file(int drive_index, char *path)
{
	fuse_log("Called for %s\n", path);
	Get_Result *folder = get_subdirectory(drive_index, path);
	if (folder->type == ERROR)
	{
		fuse_log_error("Couldn't find subdirectory while searching for the file: %s\n", path);
		return NULL;
	}
	if (folder->subdirectory == NULL)
	{
		if (folder->type == ROOT)
		{
			// This is what we expect in the NULL case (for now)
			// For files like .hidden, we don't represent it so it fails to find
			return NULL;
		}
		else
		{
			fuse_log_error("Failed in a case where we do not expect failure...\n");
			return NULL;
		}
	}
	if (folder->type == THIS)
	{
		/** When we are trying to find a file that IS a subdirectory **/
		/** ... then we need to search that subdirectories PARENT, not itself **/
		if (folder->parent == NULL)
		{
			fuse_log_error("Error in special case\n");
			return NULL;
		}
		fuse_log("Searching for file in: %s\n", folder->parent->dirname);
		return SubDirectory_find_file_in_dir(folder->parent, path);
	}
	fuse_log("searching %s\n", folder->subdirectory->dirname);
	return SubDirectory_find_file_in_dir(folder->subdirectory, path);
}



void update_subdir_file_size(const char *path, const char *filename, int drive_index, int newsize)
{
	fuse_log("Called for %s\n", path);
	Get_Result *folder = get_subdirectory(drive_index, (char*) path);
	if (folder->type == ERROR)
	{
		fuse_log_error("Couldn't find subdirectory while searching for the file: %s\n", path);
		return;
	}
	if (folder->subdirectory == NULL)
	{
		if (folder->type == ROOT)
		{
			// This is what we expect in the NULL case (for now)
			// For files like .hidden, we don't represent it so it fails to find
			return;
		}
		else
		{
			fuse_log_error("Failed in a case where we do not expect failure...\n");
			return;
		}
	}
	int file_index = -1;
	SubDirectory *dir = folder->parent;
	if (folder->type == THIS)
	{
		/** When we are trying to find a file that IS a subdirectory **/
		/** ... then we need to search that subdirectories PARENT, not itself **/
		if (folder->parent == NULL)
		{
			fuse_log_error("Error in special case\n");
			
		}
		fuse_log("Searching for file in: %s\n", folder->parent->dirname);
		file_index = SubDirectory_find_fileindex_in_dir(folder->parent,(char*) path);
		dir = folder->parent;
	}
	else
	{
		fuse_log("searching %s\n", folder->subdirectory->dirname);
		file_index = SubDirectory_find_fileindex_in_dir(folder->subdirectory, (char*) path);
		dir = folder->subdirectory;
	}

	if (file_index != -1)
	{

		json_array_set(dir->FileList, file_index, create_new_file(filename, false, newsize));
	}
}

/**
 * Search a SINGLE subdirectory for file specified by "path",
 * where "path" is an absolute path
 */
json_t *SubDirectory_find_file_in_dir(SubDirectory *dir, char *path)
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

int SubDirectory_find_fileindex_in_dir(SubDirectory *dir, char *path)
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
			return i;
		}
	}
	return -1;
}

/**
 * Given a list of subdirectories, return the subdirectory with rel_dirname
 * corresponding to relative_path
 */
SubDirectory *find_subdirectory(struct list *subdirectory_list, char *relative_path)
{
	struct list_elem *e;
	// fuse_log("starting\n");
	for (e = list_begin(subdirectory_list); e != list_end(subdirectory_list); e = list_next(e))
	{
		SubDirectory *curr = list_entry(e, struct SubDirectory, elem);
		// fuse_log("curr: %s\n", curr->dirname);
		if (strcmp(curr->rel_dirname, relative_path) == 0)
		{

			return curr;
		}
	}
	return NULL;
}

void __get_subdirectory(Get_Result *result, SubDirectory *dir, char **tokens, SubDirectory *prev)
{
	if (*tokens == NULL)
	{
		fuse_log("this case\n");
		result->type = THIS;
		result->subdirectory = dir;
		result->parent = prev;
		return;
	}
	// dump_subdirectory(dir, 0);
	SubDirectory *next = find_subdirectory(&(dir->subdirectories_list), *tokens);
	if (next == NULL)
	{
		fuse_log("%s\n", *tokens);
		fuse_log("element case\n");
		if (*(tokens + 1) != NULL)
		{
			// The next subdirectory is null but...
			// There is another directory in the path...
			fuse_log("New case: this means the subdirectory we should get doesn't exist yet\n");
			result->type = ERROR;
			return;
		}
		result->type = ELEMENT;
		result->subdirectory = dir;
		return;
	}
	// Keep searching
	__get_subdirectory(result, next, tokens + 1, dir);
}

/**
 * Return subdirectory identified by path or the subdirectory that does/would
 * contain the file specified by path
 * (SEE HEADER FOR DETAILS)
 */
Get_Result *get_subdirectory(int drive_index, char *path)
{
	fuse_log("getting subdirectory for %s\n", path);
	Get_Result *result = calloc(sizeof(Get_Result), 1);
	// Divide path into tokens

	char **tokens = split(path);

	if (count_tokens(tokens) <= 1) // This is at root level
	{
		fuse_log("Determined it should go at root directory\n");
		result->type = ROOT;
		if (count_tokens(tokens) == 1)
		{
			fuse_log("geting from root directory\n");
			result->subdirectory = find_subdirectory(&Drives[drive_index].subdirectories_list, *(tokens));
			if (result->subdirectory != NULL)
			{
				fuse_log("Successfully found subdirectory for %s\n", path);
			}
			else
			{
				fuse_log("Failed to find subdirectory for %s\n", path);
			}
			return result;
		}
		return result;
	}
	// Find the first subdirectory in the path
	char *first_name = *(tokens);
	fuse_log(first_name);
	// fuse_log("first_name: %s\n", first_name);
	SubDirectory *first = find_subdirectory(&(Drives[drive_index].subdirectories_list), first_name);
	if (first == NULL)
	{
		fuse_log_error(":(\n");
		result->type = ERROR;
		return result;
	}
	else
	{
		__get_subdirectory(result, first, tokens + 1, NULL);
		return result;
	}
}
extern char *AbsoluteCachePath;

// Mimic the file tree in the cache - called in insert_subdirectory
int insert_subdirectory_in_cache(SubDirectory *subdir)
{
	char *folder_path = calloc(sizeof(char), strlen(AbsoluteCachePath) + strlen(subdir->dirname) + 1);
	strcat(folder_path, AbsoluteCachePath);
	strcat(folder_path, subdir->dirname);
	// int res = mkdir(folder_path ,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

/**
 * Place subdir in Drive - determine location to put it at
 */
int insert_subdirectory(int drive_index, SubDirectory *subdir)
{
	Get_Result *result = get_subdirectory(drive_index, &(subdir->dirname[0]));

	if (result->type == ROOT)
	{
		fuse_log("Inserting subdirectory %s into root directory\n", &(subdir->dirname[0]));
		list_push_front(&(Drives[drive_index].subdirectories_list), &(subdir->elem));
		// Drives[drive_index].num_files ++;
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
		// result->subdirectory->num_files++;
	}
	else if (result->type == ERROR)
	{
		fuse_log_error("Location for subdirectory %s not found\n", subdir->dirname);
		return -1;
	}
	// This is a good spot to check stuff:
	// dump_drive(&(Drives[drive_index]));
	insert_subdirectory_in_cache(subdir);
	return 0;
}
