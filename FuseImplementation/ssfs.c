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
#include <sys/ioctl.h>
#include "JsonTools.h"
#include "logging.h"
#include "Drive.h"
#include "logging.h"
#include "api.h"
/*Function definitions *******************************************/

static int do_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
static int do_getattr(const char *path, struct stat *st);
static int do_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi);
static void split_path_file(char** pathBuffer, char** filenameBuffer, char *fullPath) ;
static struct fuse_operations operations = {
	.getattr = do_getattr,
	.readdir = do_readdir,
   .read	 = do_read,
};


/*Global varibales and structs ***********************************/

struct Drive_Object Drives[NUM_DRIVES] =	//NumDrives defined in Drive.h
{
		{"Google_Drive", NULL, -1, -1,-1, "../src/API/google_drive/google_drive_client" , 0, {}, 0},
		{"NFS", NULL, -1, -1, -1, "../src/API/NFS/nfs_api", 0}
		
		// {"Test_Dir", NULL, -1, "./getFile", 0, {}, 0}
};
struct Drive_Object CacheDrive = {"Ramdisk", NULL, -1, -1, -1, "../src/API/ramdisk/ramdisk_client", 0, {}, 0};
const char* CacheFile = "/mnt/ramdisk/";

/**********************************************************/

/*Main function and Drive functions ***********************/

/*
 * main calls getFileList, if successful main sets global variables and starts fuse_main
 *
 */
int main(int argc, char *argv[])
{



	initialize_log();
	//start Cache -move this to start process function in api

    int out; // fd to read from executable
	int in; // fd to write to executable

	// 
	fuse_log("running module at %s\n",  CacheDrive.exec_path);
	int success = spawn_module(&in,&out, &CacheDrive.pid, CacheDrive.exec_path);
	if (success == 0){CacheDrive.in = in;
	CacheDrive.out = out;}
	else{
		fuse_log_error("Cache did not start started");
		return -1;
	}
	populate_filelists();



	

	//To-Do:Catch Ctrl-c and kill processes
	return fuse_main(argc, argv, &operations, NULL);
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


/**
 * From https://stackoverflow.com/a/1575314
 * 
 **/
static void split_path_file(char** pathBuffer, char** filenameBuffer, char *fullPath) {
    char *slash = fullPath, *next;
    while ((next = strpbrk(slash + 1, "\\/"))) slash = next;
    if (fullPath != slash) slash++;
    *pathBuffer = strndup(fullPath, slash - fullPath - 1);
	
    *filenameBuffer = strdup(slash);
	
}
static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	int fd;
	int res;

	if(fi == NULL){
		fd = open(path, O_RDONLY);
		fuse_log_error("Open");
	}
	else
		fd = fi->fh;
	
	if (fd == -1){
		fuse_log_error("fd file");
		return -1;
	}

	res = pread(fd, buf, size, offset);
	if (res == -1){
		fuse_log_error("pread fail");
		res = -1;}

	if(fi == NULL)
		close(fd);
	return res;
}
static int do_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi)
{
	printf("--> Trying to read %s, %lu, %lu\n", path, offset, size);
	printf(buffer);
	char cwd[512];
   if (getcwd(cwd, sizeof(cwd)) != NULL) {
       printf("Current working dir: %s\n", cwd);
   } else {
       perror("getcwd() error");
       return -1;
   }


	
	if (is_process_running(CacheDrive.pid)){
		fuse_log_error("Cache not running\n");
		//return -1;
	}
	

	char* downloadFile = strcat(cwd, CacheFile);
 
char* pathcpy = strdup(path);
	char* pathBuffer = calloc(strlen(pathcpy),sizeof(char));
	char* filename= calloc(strlen(pathcpy),sizeof(char));
fuse_log_error("%s\n",pathcpy);
	split_path_file(&pathBuffer, &filename, pathcpy);
		fuse_log_error("%s\n",pathBuffer);
 char* downloadFolder= strcat(downloadFile,filename);

	if (is_drive(pathBuffer) == 0) { //File is requested straight from drive

		int index = get_drive_index(pathBuffer);
		if (index < 0)
		{
			fuse_log_error("Error in get_drive_index\n");
			return -1;
		}
		Drive_Object currDrive = Drives[index];

		//Ensuring we are selecting the proper drive to request the file from
		fuse_log_error("The drive chosen is %s\n", currDrive.dirname);
		if (!is_process_running(currDrive.pid)){
			fuse_log_error("Drive api was closed\n");
			return -1;
		}

		//Send download request to the proper api's stdin
		//  ~/cache_dir is a directory I made, I'm just trying to get it to download and not worrying about ramdisk yet
		dprintf(currDrive.in, "{\"command\":\"download\", \"path\":\"%s\", \"files\":[\"%s\"]}\n",downloadFolder, filename);
		fuse_log_error("{\"command\":\"download\", \"path\":\"%s\", \"files\":[\"%s\"]}\n",downloadFolder, filename);

		//Should wait for output from the api, currently just blocks forever
		char buff[128];
		int count = 0;
		while(1){
		if(read(currDrive.out, buff, 4)> 0) {
			fuse_log_error(buff);
			if (strncmp(buff, "{0}",3) == 0){
					fuse_log_error("Successfully downloaded %s\n", buff);
				
				fuse_log("Getting file from cache");
				const char* cachePath =strcat(downloadFile, filename);
				return xmp_read(cachePath, buffer, size, offset,fi);
				
				

			}
			else if (strncmp(buff, "{0}", 1) == 0){
				fuse_log_error("Unsuccessfully downloaded %s\n", buff);
				return -1;
			}
		}
		}	
	

	}
	

	else if(strlen(pathBuffer) == 0) {
		//File is requested from home directory
		//There currently cannot be any files in home directory so we will implement this later
		fuse_log_error("File does not exist\n");
		
	}
	else {
		//File is requested from subdirector
		fuse_log_error("Subdirectory\n");
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

