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
#include "logging.h"
#include "Drive.h"
#include "logging.h"
#include "api.h"
/*Function definitions *******************************************/

static int do_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
static int do_getattr(const char *path, struct stat *st);
static int do_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi);

static struct fuse_operations operations = {
	.getattr = do_getattr,
	.readdir = do_readdir,
    .read	 = do_read,
};


/*Global varibales and structs ***********************************/

struct Drive_Object Drives[NUM_DRIVES] =	//NumDrives defined in Drive.h
{
		{"Google_Drive", NULL, -1, -1,-1, "../src/API/google_drive/google_drive_client" /*quickstart*/, 0, {}, 0},
		{"NFS", NULL, -1, -1, -1, "../src/API/NFS/nfs_api", 0}
		// {"Test_Dir", NULL, -1, "./getFile", 0, {}, 0}
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
	initialize_log();
	populate_filelists();

	fuse_main(argc, argv, &operations, NULL);
	return kill_all_processes();
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
	
	if (is_drive(path) == 0)
	{
		fuse_log("Drive found\n");
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2; // Why "two" hardlinks instead of "one"? The answer is here: http://unix.stackexchange.com/a/101536
		return 0;
	}
	 int drive = get_drive_index(path);
	if (drive > -1)
	{
		fuse_log("\tElement in drive\n");
		int index = get_file_index(path, drive);
		//-------------------	Request file from drive
		json_t * file = get_file(drive, (char*) path);
		
		if (file == NULL) {
			fuse_log_error("Could not find file %s\n", path);
			return -1;
		}
		
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
			return -1;
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
	else if (strcmp(path, "/") == 0) // If the user is trying to show the files/directories of the root directory show the following
	{

		// We want to list our drives
		for (int i = 0; i < NUM_DRIVES; i++)
		{
			filler(buffer, Drives[i].dirname, NULL, 0);
		}
	}
	else {	//Then this *should* be a subdirectory
		fuse_log("This should be a subdirectory: %s\n", path);
		Sub_Directory * dir = handle_subdirectory((char*) path);
		if (dir != NULL) {
			for (size_t index = 0; index < dir->num_files; index++) {
				const char *fileName = getJsonFileName(json_array_get(dir->FileList, index));
				if (fileName != NULL)
				{
					filler(buffer, fileName, NULL, 0);
				}
				else
				{
					fuse_log_error("Could not find name");
				}
			}
		}
		else {
			fuse_log_error("Subdirectory not generated\n");
		}
		
	}
	

	return 0;
}

static int do_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi)
{
	printf("--> Trying to read %s, %lu, %lu\n", path, offset, size);


	//Separates the path into the directories/subdirectories and then the actual filename
	int lastslash = 0;
	char filename[64];
	char my_path[128];

	for (int i = 0; path[i] != '\0'; i++) {
		if(path[i] == '/'){
			lastslash = i;
		}
	}

	//Copies filename into temp buffer
	for (int i = 0; path[i+lastslash-1] != '\0' && i < 64; i++){
		filename[i] = path[i+lastslash];
	}
	strncpy(my_path, path, 128);
	if (lastslash != 0) {
		my_path[lastslash] = '\0';
	}
	else{
		my_path[1] = '\0';
	}

	if (is_drive(my_path) == 0) {
		//File is requested straight from drive
		int index = get_drive_index((char *)path);
		if (index < 0)
		{
			fuse_log_error("Error in get_drive_index\n");
			return -1;
		}
		Drive_Object currDrive = Drives[index];

		dprintf(currDrive.in, "{\"command\":\"download\", \"path\":\"%s\", \"file\":\"%s\"}\n", my_path, filename);

	}
	else if(strcmp(my_path, "/") == 0) {
		//File is requested from home directory
		//There currently cannot be any files in home directory so we will implement this later
		fuse_log_error("File does not exist\n");
	}
	else {
		//File is requested from subdirectory
	}

	return 0;

	/*
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

	return strlen(selectedText) - offset;*/
}

