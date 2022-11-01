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
#include <errno.h>
#include <libgen.h>
#include "JsonTools.h"
#include "logging.h"
#include "Drive.h"
#include "logging.h"
#include "api.h"
#include "subdirectories/subdir_utils.h"
/*Function definitions *******************************************/

static int do_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
static int do_getattr(const char *path, struct stat *st);
static int do_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi);
static void split_path_file(char **pathBuffer, char **filenameBuffer, char *fullPath);
static int xmp_create(const char *path, mode_t mode,
					  struct fuse_file_info *fi);
static int do_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi);
static int xmp_write(const char *path, const char *buf, size_t size,
					 off_t offset, struct fuse_file_info *fi);

/*Global varibales and structs ***********************************/

struct Drive_Object Drives[NUM_DRIVES] = // NumDrives defined in Drive.h
	{
		{
			"Google_Drive", 
			NULL, 
			//-1, 
			//-1, 
			//-1, 
			//"../src/API/google_drive/google_drive_client",
			//"", 
			0, //Num Files
			0, //Num Sub directories
			1, //Num execs
			{"../src/API/google_drive/google_drive_client", "../src/API/google_drive/google_drive_client", "", ""},	//execs
			{"", "-token_file=secondary.json", "", ""},	//args
			{-1, -1, -1, -1},	//in_fds
			{-1, -1, -1, -1},	//out_fds
			{-1, -1, -1, -1},   //pids
			/*   Subdirectory List: NULL*/
		},
		//{"NFS", NULL, -1, -1, -1, "../src/API/NFS/nfs_api", "", 0}

		// {"Test_Dir", NULL, -1, "./getFile", 0, {}, 0}
};
//struct Drive_Object CacheDrive = {"Ramdisk", NULL, "../src/API/ramdisk/ramdisk_client", 0, /*{},*/ 0};
const char *CacheFile = "/mnt/ramdisk/";

/**********************************************************/

/*Main function and Drive functions ***********************/

json_t *create_new_file(const char *path, bool isDir)
{
	fuse_log("Creating new file %s\n", path);
	json_t *new_file = json_object();
	json_object_set(new_file, "Name", json_string(path));
	json_object_set(new_file, "Size", json_integer(0));
	json_object_set(new_file, "IsDir", json_boolean(isDir));
	return new_file;
}
// This stuff is from the passthrough example, and mostly unmodified
// The key method is xmp_create

#ifdef HAVE_UTIMENSAT
static int xmp_utimens(const char *path, const struct timespec ts[2],
					   struct fuse_file_info *fi)
{
	(void)fi;
	int res;

	/* don't use utime/utimes since they follow symlinks */
	res = utimensat(0, path, ts, AT_SYMLINK_NOFOLLOW);
	if (res == -1)
		return -errno;

	return 0;
}
#endif
// Create new directories
static int xmp_mkdir(const char *path, mode_t mode)
{
	fuse_log("mkdir");
	int res;
	char *pathcpy = strdup(path);
	char *pathBuffer;
	char *newDirName;
	split_path_file(&pathBuffer, &newDirName, pathcpy);

	int drive_index = get_drive_index(pathBuffer);
	char cwd[512];
	getcwd(cwd, sizeof(cwd));

	char *cachePath = strcat(cwd, CacheFile);
	char *localPath = strcat(cachePath, newDirName);
	fuse_log("New folder path  : %s", localPath);

	// trigger new directory in drive
	res = mkdir(localPath, mode);

	if (access(localPath, F_OK) == -1)
	{
		return -1;
	}

	SubDirectory *newSub = SubDirectory_create((char*) path);
	// newSub->rel_dirname = path;
	insert_subdirectory(drive_index, newSub);

	json_t *new_file = create_new_file(newDirName, true);
	

	int err = json_array_insert(Drives[drive_index].FileList, 0, new_file);
	if (err != 0)
	{
		fuse_log_error("Directory not added");
	}

	Drives[drive_index].num_files += 1;

	return 0;
}
static int xmp_rmdir(const char *path)
{
	fuse_log("rmdir");
	int res;
	char *pathcpy = strdup(path);
	char *pathBuffer;
	char *newDirName;

	split_path_file(&pathBuffer, &newDirName, pathcpy);
	int drive_index = get_drive_index(pathBuffer);
	char cwd[512];
	getcwd(cwd, sizeof(cwd));

	char *cachePath = strcat(cwd, CacheFile);
	char *localPath = strcat(cachePath, newDirName);
	fuse_log("New folder path  : %s", localPath);

	res = rmdir(localPath);

	getcwd(cwd, sizeof(cwd));
	char *cachePath2 = strcat(cwd, CacheFile);
	char *cacheDeleteLog = strcat(cachePath2, "delete.txt");

	FILE *fPtr;
	if ((fPtr = fopen(cacheDeleteLog, "a")) == NULL)
	{
		fPtr = fopen(cacheDeleteLog, "w");
	}

	fputs(newDirName, fPtr);
	fputs("\n", fPtr);

	fclose(fPtr);

	return 0;
}


static int xmp_unlink(const char *path)

{
	fuse_log("rm");
	char *pathcpy = strdup(path);
	char *pathBuffer;
	char *newDirName;
	split_path_file(&pathBuffer, &newDirName, pathcpy);

	char cwd[512];
	getcwd(cwd, sizeof(cwd));

	char *cachePath = strcat(cwd, CacheFile);
	char *localPath = strcat(cachePath, newDirName);

	int res;
	if (access(localPath, F_OK) != -1)
	{
		res = unlink(localPath);
	}

	getcwd(cwd, sizeof(cwd));
	char *cachePath2 = strcat(cwd, CacheFile);
	char *cacheDeleteLog = strcat(cachePath2, "delete.txt");

	FILE *fPtr;
	if ((fPtr = fopen(cacheDeleteLog, "a")) == NULL)
	{

		fPtr = fopen(cacheDeleteLog, "w");
	}

	fputs(newDirName, fPtr);
	fputs("\n", fPtr);

	fclose(fPtr);
	Drive_delete((char*) path);
	return 0;
} 


// This handles adding new files
// If you "touch test.txt", it should go:
// get_attr: Could not find file (return -2)
// xmp_create gets called
// get_attr: Found file :)
static int xmp_create(const char *path, mode_t mode,
					  struct fuse_file_info *fi)
{
	fuse_log("create\n");
	int res;

	char *pathcpy = strdup(path);
	char *pathBuffer;
	char *filename;
	split_path_file(&pathBuffer, &filename, pathcpy);

	json_t *new_file = create_new_file(filename, false);
	int drive_index = get_drive_index(path);
	if (drive_index < 0)
	{
		fuse_log_error("Could not find drive for %s\n", path);
		return -1;
	}

	res = Drive_insert(drive_index, (char *)path, new_file);
	// res = open(path, fi->flags, mode);
	if (res == -1)
	{
		fuse_log_error("Insert failed\n");
		return -errno;
	}

	char cwd[512];
	if (getcwd(cwd, sizeof(cwd)) != NULL)
	{
		fuse_log_error("Current working dir: %s\n", cwd);
	}
	else
	{
		perror("getcwd() error");
		return -1;
	}

	char *cachePath = strcat(cwd, CacheFile);
	char *filePath = strcat(cachePath, filename);
	FILE *file = fopen(filePath, "a");

	// res = open(writeFilePathLocal, fi->flags, mode);
	if (file == NULL)
	{
		fuse_log_error("Insert failed\n");
		return -errno;
	}

	//	char *remoteFilePath = parse_out_drive_name(pathBuffer);

	/*
		dprintf(Drives[drive_index].in, "{\"command\":\"upload\", \"path\":\"%s\", \"files\":[\"%s\"]}\n", remoteFilePath, filePath);
		fuse_log_error("{\"command\":\"upload\", \"path\":\"%s\", \"files\":[\"%s\"]}\n", remoteFilePath, filePath);
	*/
	// fi->fh = res;
	return 0;
}


static int xmp_open(const char *path, struct fuse_file_info *fi)
{
	int res;

	res = open(path, fi->flags);
	if (res == -1)
		return -errno;

	fi->fh = res;
	return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res;
	printf("mknod\n");

	// res = mknod_wrapper(AT_FDCWD, path, NULL, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_access(const char *path, int mask)
{
	printf("xmp_access\n");
	int res;

	res = access(path, mask);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_release(const char *path, struct fuse_file_info *fi)
{
	printf("release\n");
	(void)path;
	close(fi->fh);
	return 0;
}

static int xmp_rename(const char *from, const char *to /*, unsigned int flags */)
{
	int res = 0;
	fuse_log("from: %s, to: %s\n", from, to);
	int drive_index = get_drive_index(from);
	if (drive_index < 0)
	{
		fuse_log_error("Could not find Drive for source file %s\n", from);
		return -1;
	}
	json_t * file = get_file(drive_index, (char*) from);
	if (file == NULL)
	{
		fuse_log_error("Could not find the source file %s\n", from);
		return -1;
	}
	char * prefix;
	char * local;
	char * filename = strdup(to);
	split_path_file(&prefix, &local, filename);
	fuse_log("local: %s\n", local);
	if (Drive_insert(drive_index, (char*) to, create_new_file(local, false)) < 0)
	{
		fuse_log_error("Could not insert new file\n");
		res = -1;
	}
	if (Drive_delete((char*) from) < 0)
	{
		fuse_log_error("Could not delete old file\n");
		res = -1;
	}
	return res;
}

static struct fuse_operations operations = {
	.getattr = do_getattr,
	.readdir = do_readdir,
	.read = do_read,
	.rename	= xmp_rename,
#ifdef HAVE_UTIMENSAT
	.utimens = xmp_utimens,
#endif
	.mkdir = xmp_mkdir,
	.rmdir = xmp_rmdir,
	.unlink = xmp_unlink,

	//	.open = xmp_open,
	.create = xmp_create,
	//	.mknod = xmp_mknod,
	//.access = xmp_access,
	//.release = xmp_release,
	.write = do_write,
};
/*
 * main calls getFileList, if successful main sets global variables and starts fuse_main
 *
 */
int main(int argc, char *argv[])
{

	initialize_log();
	// start Cache -move this to start process function in api

	int out; // fd to read from executable
	int in;	 // fd to write to executable

	//
	/*fuse_log("running module at %s\n", CacheDrive.exec_path);
	int success = spawn_module(&in, &out, &CacheDrive.pid, CacheDrive.exec_path);
	if (success == 0)
	{
		CacheDrive.in = in;
		CacheDrive.out = out;
	}
	else
	{
		fuse_log_error("Cache did not start started");
		return -1;
	}*/
	populate_filelists();
	// dump_drive(&(Drives[0]));
	//  To-Do:Catch Ctrl-c and kill processes
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
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2; // Why "two" hardlinks instead of "one"? The answer is here: http://unix.stackexchange.com/a/101536
		return 0;
	}
	int drive = get_drive_index(path);
	if (drive > -1)
	{
		int index = get_file_index(path, drive);
		//-------------------	Request file from drive
		json_t *file = get_file(drive, (char *)path);

		if (file == NULL)
		{
			fuse_log_error("Could not find file %s\n", path);
			return -2;
		}

		json_t *isDir = json_object_get(file, "IsDir");
		if (isDir != NULL)
		{
			if (json_is_true(isDir))
			{
				st->st_mode = S_IFDIR | 0755;
			}
			else
			{
				st->st_mode = S_IFREG | 0644;
			}
		}
		else
		{
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
	else
	{ // Then this *should* be a subdirectory
		fuse_log("This should be a subdirectory: %s\n", path);
		SubDirectory *dir = handle_subdirectory((char *)path);
		if (dir != NULL)
		{
			fuse_log("Directory returned: %s\n", dir->dirname);
			fuse_log("Dir needed: %s\n", path);
			if (strcmp(dir->dirname, path) != 0)
			{
				fuse_log_error("fatal\n");
				exit(1);
			}
			for (size_t index = 0; index < dir->num_files; index++)
			{
				const char *fileName = getJsonFileName(json_array_get(dir->FileList, index));
				if (fileName != NULL)
				{
					fuse_log("Adding %s\n", fileName);
					filler(buffer, fileName, NULL, 0);
				}
				else
				{
					fuse_log_error("Could not find name");
				}
			}
		}
		else
		{
			fuse_log_error("Subdirectory not generated\n");
		}
	}

	return 0;
}

/**
 * From https://stackoverflow.com/a/1575314
 *
 **/
static void split_path_file(char **pathBuffer, char **filenameBuffer, char *localFile)
{
	char* ts1 = strdup(localFile);
char* ts2 = strdup(localFile);
	
char* tempdir = dirname(ts1);
*pathBuffer = calloc(strlen(tempdir)+1, sizeof(char));
strncpy(*pathBuffer, tempdir, strlen(tempdir));

char* tempname = basename(ts2);
*filenameBuffer= calloc(strlen(tempname)+1, sizeof(char));
strncpy(*filenameBuffer, tempname, strlen(tempname));

free(ts1);
free(ts2);
}

/**
 * Fuse "passthrough" - completes a read function
 **/
static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
					struct fuse_file_info *fi)
{
	int fd;
	int res;

	fd = open(path, O_RDONLY);
	fuse_log_error("Open");

	if (fd == -1)
	{
		fuse_log_error("fd file\n");
		return -1;
	}

fuse_log("offset - %d \n",offset);
	res = pread(fd, buf, size, offset);
	//res = read(fd, buf, size);
	
	if (res == -1)
		fuse_log_error("pread fail\n");

	//if (fi == NULL)
		close(fd);
	return res;
}

static int do_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi)
{
	fuse_log("--> Trying to read %s, %lu, %lu\n", path, offset, size);
	
	char cwd[512];
	if (getcwd(cwd, sizeof(cwd)) != NULL)
	{
		// printf("Current working dir: %s\n", cwd);
	}
	else
	{
		perror("getcwd() error");
		return -1;
	}

	char *downloadFile = strcat(cwd, CacheFile);

	char *pathcpy = strdup(path);
	char *pathBuffer ;
	char *filename ;

	split_path_file(&pathBuffer, &filename, pathcpy);

	// char* downloadFolder= strcat(downloadFile,filename);

	if (is_drive(pathBuffer) == 0)
	{ // File is requested straight from drive

		int index = get_drive_index(pathBuffer);
		if (index < 0)
		{
			fuse_log_error("Error in get_drive_index\n");
			free(pathBuffer);
			free(filename);
			return -1;
		}
		Drive_Object currDrive = Drives[index];

		// Ensuring we are selecting the proper drive to request the file from
		fuse_log_error("The drive chosen is %s\n", currDrive.dirname);
	/*	if (!is_process_running(currDrive.pid))
		{
			fuse_log_error("Drive api was closed\n");
			free(pathBuffer);
			free(filename);
			return -1;
		}*/
		//fuse_log("Before cachePath - %s\n", downloadFile);

		char *downloadFile2 =  calloc (strlen(downloadFile)+strlen(filename)+1, sizeof(char));
		memcpy(downloadFile2, downloadFile, strlen(downloadFile));
		char *cachePath = strcat(downloadFile2, filename);
		//fuse_log("After cachePath - %s\n", cachePath);
		if (access(cachePath, F_OK) == -1)
				{
		// do nothing - making sure file is found
 		fuse_log("Before send command\n");
		// Send download request to the proper api's stdin
		// TODO - right now just asks first executable - should be updated to do most recent version of file?
		dprintf(currDrive.in_fds[0], "{\"command\":\"download\", \"path\":\"%s\", \"files\":[\"%s\"]}\n", downloadFile, filename);
		fuse_log_error("{\"command\":\"download\", \"path\":\"%s\", \"files\":[\"%s\"]}\n", downloadFile, filename);

		// Should wait for output from the api, currently just blocks forever
		char buff[4];
		int count = 0;
		while (1)
		{
			// TODO : see TODO above
			if (read(currDrive.out_fds[0], buff, 4) > 0)
			{

			//	if (strncmp(buff, "{0}", 3) == 0)
				//{

					fuse_log_error("Successfully downloaded %s\n", buff);

					while (access(cachePath, F_OK) == -1)
					{
						// do nothing - making sure file is found
					}

					return xmp_read(cachePath, buffer, size, offset, fi);
			//	}
			/*else if (strncmp(buff, "{", 1) == 0)
				{
					fuse_log_error("Unsuccessfully downloaded %s\n", buff);
					return -1;
				}*/
			}
		}
		}
		else
		{
			fuse_log("File found");
			//fuse_log_error(cachePath);
			return xmp_read(cachePath, buffer, size, offset, fi);
		}
		
	return -1;
	}

	else if (strlen(pathBuffer) == 0)
	{
		// File is requested from home directory
		// There currently cannot be any files in home directory so we will implement this later
		fuse_log_error("File does not exist\n");
	}
	else
	{
		// File is requested from subdirector
		fuse_log_error("Subdirectory\n");
	}

	return 0;
}

static int do_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi)
{
	fuse_log("--> Trying to write %s, %lu, %lu\n", path, offset, size);

	char cwd[512];
	if (getcwd(cwd, sizeof(cwd)) != NULL)
	{
		fuse_log_error("Current working dir: %s\n", cwd);
	}
	else
	{
		perror("getcwd() error");
		return -1;
	}

	char *writeFilePathLocal = strcat(cwd, CacheFile);
	fuse_log_error("Write file path - %s\n", writeFilePathLocal);

	char *pathcpy = strdup(path);
	char *pathBuffer;
	char *filename;
	split_path_file(&pathBuffer, &filename, pathcpy);

	char *remoteFilePath = parse_out_drive_name(pathBuffer);
	fuse_log_error("%s\n", remoteFilePath);

	// char *cachePath = strcat(writeFilePath, filename);

	fuse_log_error("path buffer -%s\n", pathBuffer);
	char *fullFileName = strcat(writeFilePathLocal, filename);
	//	fuse_log_error("Cache path - %s\n",cachePath);
	// char* downloadFolder= strcat(downloadFile,filename);
	int i = xmp_write(fullFileName, buffer, size, offset, fi);

	if (is_drive(pathBuffer) == 0)
	{ // File is requested straight from drive

		int index = get_drive_index(pathBuffer);
		if (index < 0)
		{
			fuse_log_error("Error in get_drive_index\n");
			free(pathBuffer);
			free(filename);
			return -1;
		}
		Drive_Object currDrive = Drives[index];

		// Ensuring we are selecting the proper drive to request the file from
		fuse_log_error("The drive chosen is %s\n", currDrive.dirname);
		//TODO: currently downloads from first executable in list. Should change to download most recent version of file?
		if (!is_process_running(currDrive.pids[0]))
		{
			fuse_log_error("Drive api was closed\n");
			free(pathBuffer);
			free(filename);
			return -1;
		}

		// Send download request to the proper api's stdin
		//{"command":"upload","path":"<valid_path>","files":["<filename1>", "<filename2>", ...]}
		
		//dprintf(currDrive.in_fds[0], "{\"command\":\"upload\", \"path\":\"%s\", \"files\":[\"%s\"]}\n", remoteFilePath, fullFileName);
		//fuse_log_error("{\"command\":\"upload\", \"path\":\"%s\", \"files\":[\"%s\"]}\n", remoteFilePath, fullFileName);
	}
	return i;

	return 0;
}

static int xmp_write(const char *path, const char *buf, size_t size,
					 off_t offset, struct fuse_file_info *fi)
{
	int fd;
	int res;

	(void)fi;
	fd = open(path, O_WRONLY | O_CREAT);
	if (fd == -1)
		return -errno;

	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}