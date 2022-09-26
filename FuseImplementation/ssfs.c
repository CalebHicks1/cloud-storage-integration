/**
 * Simple & Stupid Filesystem.
 *
 * Mohammed Q. Hussain - http://www.maastaar.net
 *
 * This is an example of using FUSE to build a simple filesystem. It is a part of a tutorial in MQH Blog with the title "Writing a Simple Filesystem Using FUSE in C": http://www.maastaar.net/fuse/linux/filesystem/c/2016/05/21/writing-a-simple-filesystem-using-fuse/
 *
 * License: GNU GPL
 */

#define FUSE_USE_VERSION 30
#define LINE_MAX_BUFFER_SIZE 1024

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

/*Function definitions *******************************************/
int is_drive(const char *path);
struct Drive_Object *get_drive(const char *path);
int myGetFileList(char lines[][LINE_MAX_BUFFER_SIZE], char *cmd);
int get_drive_index(const char *path);
int populate_filelists();
int update_drive(int i);
static int do_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
static int do_getattr(const char *path, struct stat *st);

/*Logging ********************************************************/
//Call me like you would printf
void fuse_log(char * fmt, ...);
void __fuse_log(const char* caller_name, char * fmt, ...);
void fuse_log_error(char * fmt, ...);
#define fuse_log(...) __fuse_log(__func__, __VA_ARGS__)
#define fuse_log_error(...) __fuse_log_error(__func__, 	__VA_ARGS__)
//see: https://stackoverflow.com/questions/16100090/how-can-we-know-the-caller-functions-name

/*Logging ********************************************************/

void __fuse_log(const char* caller_name, char * fmt, ...) {
	//We can add logging to a file here if/when we want to do that
	printf("[%s] ", caller_name);
	va_list arguments;
	va_start(arguments, fmt);
	vprintf(fmt, arguments);
}

void __fuse_log_error(const char* caller_name, char * fmt, ...) {
	printf("---------> [%s] ", caller_name);
	va_list arguments;
	va_start(arguments, fmt);
	vprintf(fmt, arguments);
}

static struct fuse_operations operations = {
	.getattr = do_getattr,
	.readdir = do_readdir,
	// .read		= do_read,
};

/*Global varibales and structs ***********************************/
struct Drive_Object
{
	char dirname[200];
	json_t *FileList;
	int fd;
	char exec_path[200];
	int num_files;
};
typedef struct Drive_Object Drive_Object;
//"echo \"{\\\"command\\\":\\\"list\\\",\\\"path\\\":\\\"\\\",\\\"file\\\":\\\"\\\"}\" | ../src/API/google_drive/quickstart"
#define NUM_DRIVES 2
struct Drive_Object Drives[NUM_DRIVES] =
	{
		{"Test_Dir", NULL, -1, "./getFile", 0},
		// Just have the name of the executable
		// Functions will assume executable has the same API format as the google drive one
		// ie., they will echo a json object into executable's stdin, and expect a json object returned in its stdout
		{"Google_Drive", NULL, -1, "../src/API/google_drive/./google_drive_client" /*quickstart*/, 0}//,

		//{"NFS", NULL, -1, "sudo ../src/API/NFS/nfs_api", 0}
		};

/**********************************************************/

/*Main function and Drive functions ***********************/

/*
 * main calls getFileList, if successful main sets global variables and starts fuse_main
 *
 */
int main(int argc, char *argv[])
{
	// json_t* fileListAsArray ;
	populate_filelists();

	return fuse_main(argc, argv, &operations, NULL);
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

/**
 *
 * Calls an executable that gets a formatted output of file info
 */
int myGetFileList(char lines[][LINE_MAX_BUFFER_SIZE], char *cmd)
{
	FILE *fp;
	char path[LINE_MAX_BUFFER_SIZE];
	/* Open the command for reading. */

	// Format command as a list query for root directory
	char formatted_command[500];
	sprintf(&formatted_command[0], "%s %s", "echo \"{\\\"command\\\":\\\"list\\\",\\\"path\\\":\\\"\\\",\\\"file\\\":\\\"\\\"}\" | ", cmd);

	fp = popen(&formatted_command[0], "r");
	if (fp == NULL)
	{
		fuse_log_error("Could not run command");
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

int get_file_index(const char *path, int driveIndex)
{

	for (size_t index = 0; index < Drives[driveIndex].num_files; index++)
	{
		const char *fileName = getJsonFileName(json_array_get(Drives[driveIndex].FileList, index));

		if (strstr(path + 1, fileName) != 0)
		{
			fuse_log("%s, %s\n", path + 1, fileName);
			return index;
		}
	}
	return -1;
}

/************************************************/

/*Fuse Implementation **************************/

// GNU's definitions of the attributes (http://www.gnu.org/software/libc/manual/html_node/Attribute-Meanings.html):
// 		st_uid: 	The user ID of the file’s owner.
//		st_gid: 	The group ID of the file.
//		st_atime: 	This is the last access time for the file.
//		st_mtime: 	This is the time of the last modification to the contents of the file.
//		st_mode: 	Specifies the mode of the file. This includes file type information (see Testing File Type) and the file permission bits (see Permission Bits).
//		st_nlink: 	The number of hard links to the file. This count keeps track of how many directories have entries for this file. If the count is ever decremented to zero, then the file itself is discarded as soon
//						as no process still holds it open. Symbolic links are not counted in the total.
//		st_size:	This specifies the size of a regular file in bytes. For files that are really devices this field isn’t usually meaningful. For symbolic links this specifies the length of the file name the link refers to.

static int do_getattr(const char *path, struct stat *st)
{
	fuse_log("\tAttributes of %s requested\n", path);
	int drive;
	if (is_drive(path) == 0)
	{
		fuse_log("Drive found\n");
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2; // Why "two" hardlinks instead of "one"? The answer is here: http://unix.stackexchange.com/a/101536
		return 0;
	}
	else if ((drive = get_drive_index(path)) != -1)
	{
		fuse_log("\tElement in drive\n");

		int index = get_file_index(path, drive);
		// printf("FIle Index: %d\n", index);
		json_t *file = json_array_get(Drives[drive].FileList, index);
		json_t *isDir = json_object_get(file, "IsDir");
		if (isDir != NULL) {
			if (json_is_true(isDir)) {
				st->st_mode = S_IFDIR | 0755;
			}
			else {
				st->st_mode = S_IFREG | 0644;
			}
		}
		else {
			fuse_log_error("IsDir field was not present for %s\n", path);
		}
		 // Check back for different file types
		st->st_nlink = 1;

		json_t *size = json_object_get(file, "Size");
		if (size != NULL)
			st->st_size = (int)json_integer_value(size);
		else
			st->st_size = 1024;

		st->st_uid = getuid();	   // The owner of the file/directory is the user who mounted the filesystem
		st->st_gid = getgid();	   // The group of the file/directory is the same as the group of the user who mounted the filesystem
		st->st_atime = time(NULL); // The last "a"ccess of the file/directory is right now
		st->st_mtime = time(NULL); // The last "m"odification of the file/directory is right now
								   //@alowe Here is where we want API call
								   /*char formatted_command[500];
								   sprintf(&formatted_command[0], "%s %s", "echo \"{\\\"command\\\":\\\"get_attributes\\\",\\\"path\\\":\\\"\\\",\\\"file\\\":\\\"\\\"}\" | ", Drives[get_drive_index((char*) path)].exec_path);
								   printf("\tCommand: %s\n", &formatted_command[0]);
								   //Here is where we would popen the formatted command */
		return 0;
	}

	// GNU's definitions of the attributes (http://www.gnu.org/software/libc/manual/html_node/Attribute-Meanings.html):
	// 		st_uid: 	The user ID of the file’s owner.
	//		st_gid: 	The group ID of the file.
	//		st_atime: 	This is the last access time for the file.
	//		st_mtime: 	This is the time of the last modification to the contents of the file.
	//		st_mode: 	Specifies the mode of the file. This includes file type information (see Testing File Type) and the file permission bits (see Permission Bits).
	//		st_nlink: 	The number of hard links to the file. This count keeps track of how many directories have entries for this file. If the count is ever decremented to zero, then the file itself is discarded as soon
	//						as no process still holds it open. Symbolic links are not counted in the total.
	//		st_size:	This specifies the size of a regular file in bytes. For files that are really devices this field isn’t usually meaningful. For symbolic links this specifies the length of the file name the link refers to.

	st->st_uid = getuid();	   // The owner of the file/directory is the user who mounted the filesystem
	st->st_gid = getgid();	   // The group of the file/directory is the same as the group of the user who mounted the filesystem
	st->st_atime = time(NULL); // The last "a"ccess of the file/directory is right now
	st->st_mtime = time(NULL); // The last "m"odification of the file/directory is right now

	if (strcmp(path, "/") == 0)
	{
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2; // Why "two" hardlinks instead of "one"? The answer is here: http://unix.stackexchange.com/a/101536
	}
	else
	{
		st->st_mode = S_IFREG | 0644;
		st->st_nlink = 1;
		st->st_size = 1024;
	}

	return 0;
}

// "filler": https://www.cs.nmsu.edu/~pfeiffer/fuse-tutorial/html/unclear.html
static int do_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	fuse_log("--> Getting The List of Files of %s\n", path);

	filler(buffer, ".", NULL, 0);  // Current Directory
	filler(buffer, "..", NULL, 0); // Parent Directory
	if (is_drive(path) == 0)
	{
		int index = get_drive_index((char *)path);
		if (index < 0)
		{
			fuse_log_error("Error in get_drive_index\n");
			return -1;
		}
		Drive_Object currDrive = Drives[index];
		for (size_t index = 0; index < currDrive.num_files; index++)
		{
			const char *fileName = getJsonFileName(json_array_get(currDrive.FileList, index));

			if (fileName != NULL)
			{
				filler(buffer, fileName, NULL, 0);
			}
			else
			{
				fuse_log_error("Could not find name");
			}
		}

		return 0;
	}

	if (strcmp(path, "/") == 0) // If the user is trying to show the files/directories of the root directory show the following
	{

		// We want to list our drives
		for (int i = 0; i < NUM_DRIVES; i++)
		{
			filler(buffer, Drives[i].dirname, NULL, 0);
		}
	}

	return 0;
}

static int do_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi)
{
	printf("--> Trying to read %s, %lu, %lu\n", path, offset, size);

	char file54Text[] = "Hello World From File54!";
	char file349Text[] = "Hello World From File349!";
	char *selectedText = NULL;

	// ... //

	if (strcmp(path, "/file54") == 0)
		selectedText = file54Text;
	else if (strcmp(path, "/file349") == 0)
		selectedText = file349Text;
	else
		return -1;

	// ... //

	memcpy(buffer, selectedText + offset, size);

	return strlen(selectedText) - offset;
}


/**
 * Runs an executable (currently ./getFile but can be replaced by any shell command with its path)
 * Input: char list to output command results to
 * Output: Number of lines found in executable output
 * 	-1 -> executable failed to run or was not found.
 *
int getFileList(char lines[][LINE_MAX_BUFFER_SIZE]) {
  FILE *fp;
  char path[LINE_MAX_BUFFER_SIZE];
	//char* cmd = "./getFile";
  char * cmd = Drives[0].exec_path;

  fp = popen(cmd, "r");
  if (fp == NULL) {
	return -1;
  }

  int cnt = 0;

  if (fgets(path, sizeof(path), fp) != NULL) {

	strcpy(lines[cnt++], path);
  }
  pclose(fp);
  return cnt;
}
**/
