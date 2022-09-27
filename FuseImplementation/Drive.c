#include "Drive.h"
#include "logging.h"
#include "ssfs.h"
extern Drive_Object Drives[NUM_DRIVES];

//							Setup / Updating

/**
 * Calls an executable that gets a formatted output of file info
 * If optional_path != NULL, retrieve list of files in subdirectory optional_path instead
 * Serves as a callback for myGetFileList
 */
int __myGetFileList(char lines[][LINE_MAX_BUFFER_SIZE], char *cmd, char * optional_path)
{
	FILE *fp;
	char path[LINE_MAX_BUFFER_SIZE];
	/* Open the command for reading. */

	// Format command as a list query for root directory
	char formatted_command[500];
	if (optional_path == NULL) {
		sprintf(&formatted_command[0], "%s %s", "echo \"{\\\"command\\\":\\\"list\\\",\\\"path\\\":\\\"\\\",\\\"file\\\":\\\"\\\"}\" | ", cmd);
	}
	else {
		sprintf(&formatted_command[0], "echo \"{\\\"command\\\":\\\"list\\\",\\\"path\\\":\\\"%s\\\",\\\"file\\\":\\\"\\\"}\" |  %s", optional_path, cmd);
	}

	fp = popen(&formatted_command[0], "r");
	if (fp == NULL)
	{
		printf("Could not run command");
		return -1;
	}

	int cnt = 0;
	while (fgets(path, sizeof(path), fp) != NULL)
	{
		strcpy(lines[cnt++], path);
	}
	pclose(fp);
	return cnt;
}


/**
 * Calls an executable that gets a formatted output of file info
 */
int myGetFileList(char lines[][LINE_MAX_BUFFER_SIZE], char *cmd)
{
	//FILE *fp;
	//char path[LINE_MAX_BUFFER_SIZE];
	///* Open the command for reading. */

	//// Format command as a list query for root directory
	//char formatted_command[500];
	//sprintf(&formatted_command[0], "%s %s", "echo \"{\\\"command\\\":\\\"list\\\",\\\"path\\\":\\\"\\\",\\\"file\\\":\\\"\\\"}\" | ", cmd);

	//fp = popen(&formatted_command[0], "r");
	//if (fp == NULL)
	//{
		//fuse_log_error("Could not run command");
		//return -1;
	//}

	//int cnt = 0;
	//while (fgets(path, sizeof(path), fp) != NULL)
	//{
		//strcpy(lines[cnt++], path);
	//}
	//pclose(fp);
	//return cnt;
	return __myGetFileList(lines, cmd, NULL);
}

/**
 * Calls update drive on each drive found to populate drives
 * returns > 0 on success
 * <= 0 on failure
 */
int populate_filelists()
{
	int res = 0;
	for (int i = 0; i < NUM_DRIVES; i++)
	{
		res += update_drive(i);
	}
	return res;
}

/**
 * Calls a file list getter and fills all of the drive fields
 * returns 0 on failure
 * 1 on success
 */
int update_drive(int i)
{
	fuse_log("Generating FileList for %s\n", Drives[i].dirname);
	char execOutput[100][LINE_MAX_BUFFER_SIZE];
	int a = myGetFileList(execOutput, Drives[i].exec_path);

	json_t *fileListAsArray;

	if (a < 1)
	{
		fuse_log_error("file list getter was not executed properly or output was empty\n");
		return 0;
	}

	int arraySize = parseJsonString(&fileListAsArray, execOutput, a);

	if (arraySize < 1)
	{
		return 0;
	}
	Drives[i].FileList = json_deep_copy(fileListAsArray);
	Drives[i].num_files = arraySize;
	return 1;
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

// Works for path = drivename, or path = drivename/somefile
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
char * parse_out_drive_name(char * path) {
	char * ret = path + 1;
	while (*ret != '\0' && *ret != '/') {
		ret++;
	}
	return (++ret);
}

int get_file_index(const char *path, int driveIndex)
{
	char * cut_path = parse_out_drive_name((char*) path);
	for (size_t index = 0; index < Drives[driveIndex].num_files; index++)
	{
		const char *fileName = getJsonFileName(json_array_get(Drives[driveIndex].FileList, index));
		if (strcmp(cut_path, fileName) == 0)
		{
			return index;
		}
	}
	fuse_log_error("Could not find index for %s\n", path);
	return -1;
}
//							Subdirectories

