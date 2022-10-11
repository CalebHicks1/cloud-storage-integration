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
	
	//Old subdirectories
	/*
	printf("Subdirectories\n");
	for (int i = 0; i < drive->num_sub_directories; i++) {
		printf("\t%s: num_files: %d\n", drive->sub_directories[i].dirname, drive->sub_directories[i].num_files);
	}
	*/
	
	//New subdirectories
	//printf("New subdirectories\n");
	struct list_elem * e;
	for (e = list_begin(&(drive->subdirectories_list)); e != list_end(&(drive->subdirectories_list)); e = list_next(e))
	{
		struct SubDirectory * currSubdir = list_entry(e, struct SubDirectory, elem);
		//printf("|--- ");
		dump_subdirectory(currSubdir, 0);
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
	
	json_t * file = subdir_find_file(drive_index, path);
	if (file == NULL)
	{
		fuse_log_error("Could not find file %s\n", path);
		return NULL;
	}
	fuse_log("We found the file!!!!!\n");
	return file;
	//Sub_Directory *dir = __get_subdirectory_for_path(drive_index, path);
	//if (dir != NULL)
	//{
		//json_t *ret = __get_file_subdirectory(dir, path);
		//if (ret != NULL)
		//{
			//Get_Result * folder = get_subdirectory(drive_index, path);
			//if (folder->type == ERROR)
			//{
				//fuse_log_error("--------------------------> NEW FILESYSTEM: could not find %s\n", path);
				
			//}
			//else
			//{
				//fuse_log("--------------------------> apparently, found the correct subdirectory\n");
				
			//}
			//json_t * ret = SubDirectory_find_file(folder->subdirectory, path);
			//if (ret != NULL)
			//{
				//fuse_log("... and we found it\n");
				
			//}
			//else 
			//{
				//fuse_log_error("We did not find it :(((((\n");
				
			//}
			//return ret;
		//}
		//else
		//{
			//fuse_log_error("%s should have been in subdirectory %s, but wasn't\n", path, dir->dirname);
		//}
	//}
	//fuse_log_error("Could not find file %s\n", path);
	//return NULL;
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
Sub_Directory *handle_subdirectory(char *path)
{
	fuse_log("Called for %s\n", path);
	int drive_index = get_drive_index(path);

	if (drive_index < 0)
	{
		fuse_log_error("Drive not found for subdirectory %s\n", path);
		return NULL;
	}

	for (int i = 0; i < Drives[drive_index].num_sub_directories; i++)
	{
		if (strcmp(Drives[drive_index].sub_directories[i].dirname, path) == 0)
		{
			fuse_log("Subdirectory %s already exists... returning it\n", path);
			return &Drives[drive_index].sub_directories[i];
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
	Sub_Directory *new_directory = &(Drives[drive_index].sub_directories[Drives[drive_index].num_sub_directories]);
	int num_files = get_subdirectory_contents(&(new_directory->FileList), drive_index, relative_path, Drives[drive_index].in, Drives[drive_index].out);
	if (num_files < 0)
	{
		fuse_log_error("Problem getting subdirectory contents\n");
	}
	new_directory->num_files = num_files;
	sprintf(&(new_directory->dirname[0]), "%s", path);
	new_directory->self = json_object();
	json_object_set(new_directory->self, "Name", json_string(path));
	json_object_set(new_directory->self, "Size", json_integer(0));
	json_object_set(new_directory->self, "IsDir", json_string("true"));
	Drives[drive_index].num_sub_directories++;
	
	//New subdirectory version!
	//Create directory
	SubDirectory * new = SubDirectory_create(path);
	//Insert subdirectory
	insert_subdirectory(drive_index, new);
	//Insert files 
	fuse_log("Getting files for new subdirectory structure\n");
	new->num_files = get_subdirectory_contents(&(new->FileList), drive_index, relative_path, Drives[drive_index].in, Drives[drive_index].out);
	
	return new_directory;
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
