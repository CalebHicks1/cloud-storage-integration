#include "Drive.h"
#include "logging.h"
#include "subdirectories/SubDirectory.h"
//#include "ssfs.h"
#include "api.h"
#include "MultiList.h"
#include "cache_utils.h"
extern Drive_Object Drives[NUM_DRIVES];
extern char *AbsoluteCachePath;
#define BASH


void dump_file_descriptors(Drive_Object * drive)
{
	printf("| in: {");
	for (int i = 0; i < MAX_EXECS; i++)
	{
		printf("%d, ", drive->in_fds[i]);
	}
	printf("}\n");
	
	printf("| out: {");
	for (int i = 0; i < MAX_EXECS; i++)
	{
		printf("%d, ", drive->out_fds[i]);
	}
	printf("}\n");
}

void dump_drive(Drive_Object * drive)
{
	if (drive == NULL) {
		fuse_log_error("Null drive\n");
		return;
	}
	//This Drives attributes
	printf("| %s (%d files)\n", &(drive->dirname[0]), drive->num_files);
	dump_file_descriptors(drive);
	//Files
	for (int i = 0; i < drive->num_files; i++) {
		json_t * curr_file = json_array_get(drive->FileList, i);
		printf("|--- %s\n", getJsonFileName(curr_file));
	}

	struct list_elem * e;
	for (e = list_begin(&(drive->subdirectories_list)); e != list_end(&(drive->subdirectories_list)); e = list_next(e))
	{
		struct SubDirectory * currSubdir = list_entry(e, struct SubDirectory, elem);
		//printf("|--- ");
		dump_subdirectory(currSubdir, 0);
	}
	
}



//Path is the absolute path
int Drive_delete(char * path)
{
	fuse_log("Called for %s\n", path);
	int drive_index = get_drive_index(path);
	if (drive_index < 0)
	{
		fuse_log_error("Could not find drive for %s\n", path);
	}
	json_t * file = get_file(drive_index, path);
	if (file == NULL)
	{
		fuse_log_error("Could not find file %s\n", path);
		fuse_log_error("Above error message is OK if called by Drive_insert\n");
		return -1;
	}
	
	Get_Result * result = get_subdirectory(drive_index, path);
	if (result->type == ROOT)
	{
		fuse_log("Determined to be in root directory\n");
		int file_index = get_file_index(path, drive_index);
		int ret = json_list_remove(&Drives[drive_index].FileList, file, Drives[drive_index].num_files);
		if (ret >= 0)
		{
			//Success
			Drives[drive_index].num_files--;
			return 0;
		}
		else
		{
			fuse_log_error("Deleting file %s failed\n", path);
			return -1;
		}
		
	}
	//File is in a subdirectory...
	
	if (result->type == ELEMENT && result->subdirectory != NULL)
	{
		fuse_log("Determined to be in a subdirectory\n");
		//This is the only case we support - removing directories will be another function
		int ret = json_list_remove(&result->subdirectory->FileList, file, result->subdirectory->num_files);
		if (ret >= 0)
		{
			//Success
			result->subdirectory->num_files--;
			return 0;
		}
		else
		{
			fuse_log_error("Failed to remove file %s from subdirectory\n", path);
			return -1;
		}
	}
	else if (result->type == THIS && result->parent != NULL && result->subdirectory != NULL)
	{
		//We are deleting a subdirectory in a subdirectory
		fuse_log("We are deleting a subdirectory in a subdirectory... what fun\n");
		SubDirectory * parent = result->parent;
		SubDirectory * child = result->subdirectory;
		int removed_file = json_list_remove(&parent->FileList, file, parent->num_files);
		if (removed_file < 0)
		{
			fuse_log_error("Could not remove file from subdirectory\n");
			return -1;
		}
		parent->num_files--;
		size_t start_size = list_size(&parent->subdirectories_list);
		list_remove(&child->elem);
		size_t end_size = list_size(&parent->subdirectories_list);
		if (end_size != start_size - 1)
		{
			fuse_log_error("Did not remove child subdirectory from parent subdirectory's list\n");
			return -1;
		}
		return 0;
	}
	else
	{
		fuse_log_error("Unexpected error\n");
		return -1;
	}
}

char * path_minus_filename(char * path)
{
	char * ptr = path;
	char * last_slash = NULL;
	while (*ptr != '\0')
	{
		if (*ptr == '/')
			last_slash = ptr;
		ptr++;
	}
	if (last_slash == NULL)
	{
		fuse_log_error("Found no slashes in file path %s\n", path);
		return NULL;
	}
	int new_length = strlen(path) - strlen(last_slash);
	char * res = calloc(sizeof(char*), new_length + 1);
	memcpy(res, path, new_length);
	return res;
}

char * filename_minus_path(char * path)
{
	char * ptr = path;
	char * last_slash = NULL;
	while (*ptr != '\0')
	{
		if (*ptr == '/')
			last_slash = ptr;
		ptr++;
	}
	if (last_slash == NULL)
	{
		fuse_log_error("Found no slashes in file path %s\n", path);
		return NULL;
	}
	last_slash++;
	int new_length = strlen(last_slash);
	char * res = calloc(sizeof(char*), new_length + 1);
	memcpy(res, last_slash, new_length);
	return res;
}

/**
 * Insert a newly generated file into drive. Path should contain the full path of the file,
 * while the json representation should have the name set to be the relative path
 */
int Drive_insert(int drive_index, char * path, json_t * file)
{
	//If we rename a file to an already existing one
	//we need to delete the old one
	fuse_log("Try deleting path to see if it already exists...");
	fuse_log("(it's ok if this causes an error)\n");
	int did_delete = Drive_delete(path);
	if (did_delete == 0)
	{
		fuse_log("Deleted the original\n");
	}


	Get_Result * get_result = get_subdirectory(drive_index, (char*) path);
	if (get_result->type == ROOT)
	{
		fuse_log("Inserting %s in root directory\n", getJsonFileName(file));
		int res =  json_list_append(&(Drives[drive_index].FileList), file);
		if (res >= 0)
		{
			fuse_log("Sucessfully inserted\n");
			Drives[drive_index].num_files++;
		}
		else {
			fuse_log_error("Insert failed\n");
		}
		return res;
	}
	else if (get_result->subdirectory != NULL || get_result->type == ELEMENT)
	{
		fuse_log("New file %s should be inserted in a subdirectory\n", path);
		int res = SubDirectory_insert(get_result->subdirectory, file);
		dump_subdirectory(get_result->subdirectory, 0);
		return res;
	}
	else
	{
		//This is for when we try to move a file into a subdirectory we 
		//have not populated yet
		fuse_log_error("Cannot find location to put new file %s\n", path);
		fuse_log("Lets see if there's a subdirectory to put it in that doesn't exist yet\n");
		//char * path_minus_name = parse_out_drive_name(path);
		char * path_minus_name = strip_filename_from_directory(path);
		json_t * subdir = get_file(get_drive_index(path_minus_name), path_minus_name);
		//char * name = getJsonFileName(subdir);
		if (subdir == NULL)
		{
			fuse_log_error("Directory to put file in does not exist");
		}
		json_t *isDir = json_object_get(subdir, "IsDir");
		if (isDir != NULL)
		{
			if (json_is_true(isDir))
			{
				fuse_log("Is directory!\n");
				SubDirectory * generated_subdir = handle_subdirectory(path_minus_name);
				if (generated_subdir == NULL)
				{
					fuse_log_error("Generating subdirectory failed\n");
					return -1;
				}
				int ret = SubDirectory_insert(generated_subdir, file);
				return ret;
			}
			else
			{
				fuse_log_error("%s is not a folder\n", path_minus_name);
				return -1;
			}
		}
		return -1;
	}
	
	
}

/**
 * Return json object if file exists anywhere in drive, NULL if not
 * drive_index: index of drive in Drives
 * path: path of the file
 */
json_t *get_file(int drive_index, char *path)
{
	// Check FileList
	int file_index = get_file_index(path, drive_index);
	if (file_index > -1)
	{
		return json_array_get(Drives[drive_index].FileList, file_index);
	}
	// Check subdirectories
	
	json_t * file = Subdirectory_find_file(drive_index, path);
	if (file == NULL)
	{
		fuse_log_error("Could not find file %s\n", path);
		dump_drive(&Drives[0]);
		return NULL;
	}
	return file;
}
//							Setup / Updating

/**
 * Populates each drive with FileLists
 * returns > 0 on success
 * <= 0 on failure
 */
int populate_filelists()
{
	int res = 0;
	for (int i = 0; i < NUM_DRIVES; i++)
	{
		
		//int out = -1; // fd to read from executable
		//int in = -1;  // fd to write to executable

		// launch executable
		//fuse_log("running module at %s\n", Drives[i].exec_path);
		//spawn_module(&in, &out, &Drives[i].pid, Drives[i].exec_path, Drives[i].exec_arg);

		for (int exec_index = 0; exec_index < Drives[i].num_execs; exec_index++)
		{
			fuse_log("New version: spawning executable #%d for drive %s", exec_index, Drives[i].dirname);
			spawn_module(&Drives[i].in_fds[exec_index], &Drives[i].out_fds[exec_index], &Drives[i].pids[exec_index],
				Drives[i].exec_paths[exec_index], Drives[i].exec_args[exec_index]);
		}

		//Drives[i].in = in;
		//Drives[i].out = out;
		list_init(&(Drives[i].subdirectories_list));
		// Populate FileList
		Drives[i].num_files = listAsArray(&(Drives[i].FileList), &Drives[i], NULL);
		if (Drives[i].num_files < 0)
		{
			fuse_log_error("Populating %s failed\n", Drives[i].dirname);
			res = -1;
		}

		dump_drive(&Drives[i]);
	}
	//int ret = Drive_delete("/Google_Drive/whatever");
	return res;
}

/**
 * Calls an executable that gets a formatted output of file info
 * If optional_path != NULL, retrieve list of files in subdirectory optional_path instead
 * Serves as a callback for myGetFileList
 */
int myGetFileList(char lines[][LINE_MAX_BUFFER_SIZE], char *cmd, char *optional_path, int in, int out)
{

	int cnt = 0;
	if (in != -1 && out != -1)
	{

		// Format command as a list query for root directory
		if (optional_path == NULL)
		{
			dprintf(in, "{\"command\":\"list\", \"path\":\"/\", \"file\":\"example\"}\n");
		}
		else
		{
			dprintf(in, "{\"command\":\"list\", \"path\":\"%s\", \"file\":\"example\"}\n", optional_path);
		}

		// read last line from executable

		ssize_t read_size = read(out, lines[cnt++], LINE_MAX_BUFFER_SIZE);
	}
	/*
	// shut down api
	dprintf(in, "{\"command\":\"shutdown\"}\n");

	// we don't need these file descriptors anymore
	close(out);
	close(in);
	*/
	return cnt;
}

/*Subdirectories *************************************************/

/**
 * Uses API to get contents of a subdirectory, placing this contents in param list
 */
 //int get_subdirectory_contents(json_t **list, int drive_index, char *path, int in, int out)
int get_subdirectory_contents(json_t **list, int drive_index, char *path)
{
	fuse_log("Generating filelist for subdirectory %s\n", path);
	return listAsArray(list, &Drives[drive_index], path);
}


/**
 * If subdirectory exists, return it. If not, generate it, and add it
 * to the list of the Drive's subdirectories
 */
SubDirectory *handle_subdirectory(char *path)
{
	//fuse_log("Called for %s\n", path);
	//fuse_log_error("Called for %s\n", path);
	int drive_index = get_drive_index(path);

	//dump_drive(&Drives[drive_index]);
	if (drive_index < 0)
	{
		fuse_log_error("Drive not found for subdirectory %s\n", path);
		return NULL;
	}
	
	
	Get_Result * ret = get_subdirectory(drive_index, path);
	//(ret->type == THIS )
	if (ret->subdirectory != NULL && ret->type != ELEMENT)
	{
		fuse_log("Subdirectory %s already exists, returning it\n", path);
		return ret->subdirectory;
	}
	
	// Generate the subdirectory
	fuse_log("Need to generate subdirectory %s...\n", path);

	char *relative_path = path + 1;
	while (*relative_path != '\0' && *relative_path != '/')
	{
		relative_path++;
	}
	if (*relative_path == '\0')
	{
		fuse_log_error("Could not parse relative path for %s\n", path);
		return NULL;
	}
	
	//Create directory
	SubDirectory * new = SubDirectory_create(path);
	//Insert subdirectory
	insert_subdirectory(drive_index, new);

	add_subdirectory_to_cache(path);
	//Insert files 
	fuse_log("Getting files for new subdirectory structure: %s\n", path);
	new->num_files = get_subdirectory_contents(&(new->FileList), drive_index, relative_path);
	
	return new;
}

/**
 * Assign *list to be list of files corresponding to Drives[i], returning # of files.
 * If optional_path != NULL, get list of files at the subdirectory optional_path instead
 * Params:
 * list: list to store contents in
 * Drive: Drive the files reside in (passed in to get executable info)
 * optional_path: if not NULL, corresponds to the subdirectory in which we want to
 * list the files - if NULL, list at root level of the drive
 */
int listAsArray(json_t **list, struct Drive_Object * drive, char *optional_path)
{
	return listAsArrayV2(list, drive, optional_path);
}

/*****************************************************************************/

/*****Helper functions ******************************************************/
int is_drive(const char *path)
{
	for (int i = 0; i < NUM_DRIVES; i++)
	{
		if (strcmp(Drives[i].dirname, path + 1) == 0)
		{
			return 0;
		}
	}
	return -1;
}

/**
 * Gets drive PRECISELY corresponding to path
 */
struct Drive_Object *get_drive(const char *path)
{
	for (int i = 0; i < NUM_DRIVES; i++)
	{
		if (strncmp(Drives[i].dirname, path + 1, strlen(Drives[i].dirname)) == 0)
		{
			return &Drives[i];
		}
	}
	return NULL;
}

/**
 * Gets index of drive that "path" would live in
 * Works for path = drivename, or path = drivename/somefile
 */
int get_drive_index(const char *path)
{

	for (int i = 0; i < NUM_DRIVES; i++)
	{
		if (strncmp(path + 1, Drives[i].dirname, strlen(Drives[i].dirname)) == 0)
		{

			return i;
		}
	}
	return -1;
}

/**
 * /<GoogleDrive, NFS, etc>/filename -> filename
 */
char *parse_out_drive_name(char *path)
{
	char *ret = path + 1;
	while (*ret != '\0' && *ret != '/')
	{
		ret++;
	}
	return (++ret);
}

/**
 * Gets the index of "path" in ROOT level of drive
 */
int get_file_index(const char *path, int driveIndex)
{
	char *cut_path = parse_out_drive_name((char *)path);
	for (size_t index = 0; index < Drives[driveIndex].num_files; index++)
	{
		const char *fileName = getJsonFileName(json_array_get(Drives[driveIndex].FileList, index));
		if (strcmp(cut_path, fileName) == 0)
		{
			return index;
		}
	}
	return -1;
}

// teardown
int kill_all_processes()
{
	for (int i = 0; i < NUM_DRIVES; i++)
	{
		//shutdown(Drives[i].pid, Drives[i].in, Drives[i].out);
		for (int exec_index = 0; exec_index < Drives[i].num_execs; exec_index++)
		{
			shutdown(Drives[i].pids[exec_index], Drives[i].in_fds[exec_index], Drives[i].out_fds[exec_index]);
		}
	}
	return 0;
}
