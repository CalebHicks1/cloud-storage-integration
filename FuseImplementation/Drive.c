#include "Drive.h"
#include "logging.h"
#include "subdirectories/SubDirectory.h"
//#include "ssfs.h"
#include "api.h"
extern Drive_Object Drives[NUM_DRIVES];

#define BASH

void dump_drive(Drive_Object * drive)
{
	if (drive == NULL) {
		fuse_log_error("Null drive\n");
		return;
	}
	//This Drives attributes
	printf("| %s (%d files)\n", &(drive->dirname[0]), drive->num_files);
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

/**
 * Insert a newly generated file into drive. Path should contain the full path of the file,
 * while the json representation should have the name set to be the relative path
 */
int Drive_insert(int drive_index, char * path, json_t * file)
{
	fuse_log("Inserting %s\n", getJsonFileName(file));
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
		return res;
	}
	else
	{
		fuse_log_error("Cannot find location to put new file %s\n", path);
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
	fuse_log("We found the file %s!!!!!\n", path);
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
		int out = -1; // fd to read from executable
		int in = -1;  // fd to write to executable

		// launch executable
		fuse_log("running module at %s\n", Drives[i].exec_path);
		spawn_module(&in, &out, &Drives[i].pid, Drives[i].exec_path);

		Drives[i].in = in;
		Drives[i].out = out;
		list_init(&(Drives[i].subdirectories_list));
		// Populate FileList
		Drives[i].num_files = listAsArray(&(Drives[i].FileList), Drives[i].exec_path, NULL, Drives[i].in, Drives[i].out);
		if (Drives[i].num_files < 0)
		{
			fuse_log_error("Populating %s failed\n", Drives[i].dirname);
			res = -1;
		}
	}
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
int get_subdirectory_contents(json_t **list, int drive_index, char *path, int in, int out)
{
	fuse_log("Generating filelist for subdirectory %s\n", path);
	return listAsArray(list, Drives[drive_index].exec_path, path, in, out);
}


/**
 * If subdirectory exists, return it. If not, generate it, and add it
 * to the list of the Drive's subdirectories
 */
SubDirectory *handle_subdirectory(char *path)
{
	fuse_log("Called for %s\n", path);
	fuse_log_error("Called for %s\n", path);
	int drive_index = get_drive_index(path);

	dump_drive(&Drives[drive_index]);
	if (drive_index < 0)
	{
		fuse_log_error("Drive not found for subdirectory %s\n", path);
		return NULL;
	}
	
	
	Get_Result * ret = get_subdirectory(drive_index, path);
	//(ret->type == THIS )
	if (ret->subdirectory != NULL && ret->type != ELEMENT)
	{
		if (ret->type == ELEMENT)
		{
			fuse_log("Is element!!!!!!!\n");
		}
		fuse_log("Subdirectory %s already exists, returning it\n", path);
		return ret->subdirectory;
	}
	else
	{
		if (ret->subdirectory == NULL)
		{
			fuse_log_error("was null\n");
			fuse_log_error("type: %d\n", ret->type);
			
		}
		else
		{
			fuse_log_error("wasnt null\n");
			if (ret->type == ELEMENT)
			{
				fuse_log_error("was element\n");
				
			}
			else 
			{
				fuse_log_error("WASNT element\n");
				fuse_log_error("Type: %d\n", ret->type);
				
			}
			
		}
		
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
	
	//New subdirectory version!
	//Create directory
	SubDirectory * new = SubDirectory_create(path);
	//Insert subdirectory
	insert_subdirectory(drive_index, new);
	//Insert files 
	fuse_log("Getting files for new subdirectory structure: %s\n", path);
	new->num_files = get_subdirectory_contents(&(new->FileList), drive_index, relative_path, Drives[drive_index].in, Drives[drive_index].out);
	
	return new;
}

/**
 * Assign *list to be list of files corresponding to Drives[i], returning # of files.
 * If optional_path != NULL, get list of files at the subdirectory optional_path instead
 * Params:
 *
 * list: Pointer to list of json objects, which will be populated with files
 *
 * cmd: executable path to API to use
 *
 * optional_path: if not NULL, corresponds to the subdirectory in which we want to
 * list the files - if NULL, list at root level of the drive
 */
int listAsArray(json_t **list, char *cmd, char *optional_path, int in, int out)
{
	char execOutput[100][LINE_MAX_BUFFER_SIZE] = {};

	int a = myGetFileList(execOutput, cmd, optional_path, in, out);

	json_t *fileListAsArray = NULL;

	if (a < 1)
	{
		printf("file list getter was not executed properly or output was empty\n");
		return 0;
	}

	int arraySize = parseJsonString(&fileListAsArray, execOutput, a);

	if (arraySize < 1)
	{
		return 0;
	}
	*list = fileListAsArray;
	return arraySize;
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
		shutdown(Drives[i].pid, Drives[i].in, Drives[i].out);
	}
	return 0;
}
