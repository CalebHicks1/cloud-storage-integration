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
#include "cache_utils.h"
#include "path_utils.h"
/*Function definitions *******************************************/

static int do_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
static int do_getattr(const char *path, struct stat *st);
static int do_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi);
// static void split_path_file(char **pathBuffer, char **filenameBuffer, const char *fullPath);
static int xmp_create(const char *path, mode_t mode,
					  struct fuse_file_info *fi);
static int do_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi);
static int xmp_write(const char *path, const char *buf, size_t size,
					 off_t offset, struct fuse_file_info *fi);

/*Global varibales and structs ***********************************/

struct Drive_Object Drives[NUM_DRIVES] = // NumDrives defined in Drive.h
	{
		{
			"",
			NULL,
			//-1,
			//-1,
			//-1,
			//"../src/API/google_drive/google_drive_client",
			//"",
			0,																										// Num Files
			0,																										// Num Sub directories
			2,																									// Num execs
			{"../src/API/google_drive/google_drive_client","../src/API/dropbox/drop_box", "../src/API/one_drive/bin/Debug/net6.0/publish/OneDrive", ""}, // execs
			{"", "", "", ""},																// args
			{-1, -1, -1, -1},																						// in_fds
			{-1, -1, -1, -1},																						// out_fds
			{-1, -1, -1, -1},																						// pids
																													/*   Subdirectory List: NULL*/
		},
		//{"NFS", NULL, -1, -1, -1, "../src/API/NFS/nfs_api", "", 0}

		// {"Test_Dir", NULL, -1, "./getFile", 0, {}, 0}
};
// struct Drive_Object CacheDrive = {"Ramdisk", NULL, "../src/API/ramdisk/ramdisk_client", 0, /*{},*/ 0};
const char *CacheFile = "/mnt/ramdisk/";
const char *AbsoluteCachePath;
const char *CacheDeleteLogName;
const char *AbsoluteLockPath;

/**********************************************************/

/*Main function and Drive functions ***********************/

json_t *create_new_file(const char *path, bool isDir, int size)
{

	json_t *new_file = json_object();
	json_object_set(new_file, "Name", json_string(path));
	json_object_set(new_file, "Size", json_integer(size));
	json_object_set(new_file, "IsDir", json_boolean(isDir));
	return new_file;
}

// This stuff is from the passthrough example, and mostly unmodified
//The "mostly unmodified" thing is less true now lol
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

	char *pathBuffer;
	char *newDirName;
	// split_path_file(&pathBuffer, &newDirName, path);

	int drive_index = get_drive_index(path);

	wait_for_lock();
	char *localPath = form_cache_path(path, true, &newDirName);
	fuse_log(localPath);
	// char *localPath = strcat(cachePath, newDirName);

	// Create the directory in the cachr

	int res = mkdir(localPath, mode);
	if (res == -1)
	{
		//If this is because the directory already exists in the cache, this is ok 
		//(check error code?)
		fuse_log_error("Could not do mkdir in cache for %s\n", localPath);
	}
	delete_lock();
	// Add new directory tofile tree
	SubDirectory *newSub = SubDirectory_create((char *)path);
	newSub->FileList = json_array();
	
	// Get_Result * result = get_subdirectory(drive_index, &(newSub->dirname[0]));
	// result->subdirectory->num_files++;
	Get_Result * get_result = get_subdirectory(drive_index, (char*) path);
	if (get_result->type == ELEMENT)
	{
		if (get_result->subdirectory == NULL)
		{
			fuse_log_error("This shouldn't happen :(\n");
			return -1;
		}
		fuse_log("Need to insert new subdirectory json rep into its parent subdir\n");
		json_t *new_file = create_new_file(parse_out_drive_name(newDirName), true, 0);
		int err = json_list_append(&get_result->subdirectory->FileList, new_file);
		if (err != 0)
		{
			fuse_log_error("Subdirectory json rep not added\n");
			return -1;
		}
		get_result->subdirectory->num_files++;

	}
	else if (get_result->type == ROOT)
	{
		fuse_log("Root case\n");
		json_t *new_file = create_new_file(newDirName, true, 0);
		int err = json_list_append(&Drives[drive_index].FileList, new_file);
		if (err != 0)
		{
			fuse_log_error("Subdirectory json retp not added\n");
			return -1;
		}
		Drives[drive_index].num_files++;
	}
	else {
		fuse_log_error("Error finding location to put subdirectory\n");
		return -1;
	}
	insert_subdirectory(drive_index, newSub);
	

	return 0;
	// json_t *new_file = create_new_file(newDirName, true, 0);
	// int err = json_array_insert(Drives[drive_index].FileList, 0, new_file);
	// if (err != 0)
	// {
	// 	fuse_log_error("Directory not added");
	// }

	// Drives[drive_index].num_files += 1;
	// dump_drive(&Drives[drive_index]);
	// return 0;
}

/** remove subdirectory from tree*/
static int xmp_rmdir(const char *path)
{
	fuse_log("rmdir");

	// char *pathcpy = strdup(path);

	char *newDirName;
	wait_for_lock();
	// int drive_index = get_drive_index(pathBuffer);
	char *localPath = form_cache_path(path, true, &newDirName);

	fuse_log(localPath);
	int res = rmdir(localPath);

	addPathToDeleteLog(CacheDeleteLogName, newDirName);

	delete_lock();
	Drive_delete((char *)path);

	return res;
}

static int xmp_unlink(const char *path)

{
	fuse_log("rm");
	// char *pathcpy = strdup(path);

	char *newDirName;
	wait_for_lock();
	char *localPath = form_cache_path(path, true, &newDirName);
	fuse_log("cache path - %s \n", localPath);

	int res = 0;
	if (access(localPath, F_OK) != -1)
	{
		res = unlink(localPath);
	}

	addPathToDeleteLog(CacheDeleteLogName, newDirName);
Drive_delete((char *)path);
	delete_lock();
	
	return res;
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


	// char *pathcpy = strdup(path);
	char *pathBuffer;
	char *filename;
	split_path_file(&pathBuffer, &filename, path);

	json_t *new_file = create_new_file(filename, false, 0);
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

	wait_for_lock();
	char *filePath = form_cache_path(filename, false, NULL);

	if (access(filePath, F_OK) == -1)
	{
		int fd = open(filePath, O_CREAT);

		close(fd);
		delete_lock();
		return 0;
	}

	delete_lock();
	// Create file with fopen

	return -1;
}
/*
static int xmp_open(const char *path, struct fuse_file_info *fi)
{
	int res;

	res = open(path, fi->flags);
	if (res == -1)
		return -errno;

	fi->fh = res;
	return 0;
}*/
/*
static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res;
	printf("mknod\n");

	 res = mknod_wrapper(AT_FDCWD, path, NULL, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}*/
/*
static int xmp_access(const char *path, int mask)
{
	printf("xmp_access\n");
	int res;

	res = access(path, mask);
	if (res == -1)
		return -errno;

	return 0;
}
*/
/*
static int xmp_release(const char *path, struct fuse_file_info *fi)
{
	printf("release\n");
	(void)path;
	close(fi->fh);
	return 0;
}*/

static int xmp_truncate(const char *path, off_t size)
{
	fuse_log("Truncate called\n");

	// char *pathcpy = strdup(path);
	char *pathBuffer;
	char *filename;
	split_path_file(&pathBuffer, &filename, path);

	wait_for_lock();
	char *fullFileName = form_cache_path(filename, false, NULL); // strcat(writeFilePathLocal, filename);
	int res;

	char *cachePathWithSubs = strip_filename_from_directory(fullFileName);

	if (access(fullFileName, F_OK) == -1)
	{
		int index = get_drive_index(pathBuffer);
		if (index > -1)
		{
			if (cachePathWithSubs != NULL)
			{
				download_file(Drives[index].in_fds[0], Drives[index].out_fds[0], cachePathWithSubs, filename, fullFileName);
			}
			else
				download_file(Drives[index].in_fds[0], Drives[index].out_fds[0], (char *)AbsoluteCachePath, filename, fullFileName);
		}
		// fuse_log("Drive index %d", index);
	}

	res = truncate(fullFileName, size);
	if (res == -1)
	{
		return errno;
		fuse_log_error("Truncate failed %d", errno);
	}

	delete_lock();
	return 0;
}

static int xmp_rename(const char *from, const char *to /*, unsigned int flags */)
{
	int res = 0;
	fuse_log("from: %s, to: %s\n", from, to);
	int from_drive_index = get_drive_index(from);
	if (from_drive_index < 0)
	{
		fuse_log_error("Could not find Drive for source file %s\n", from);
		return -1;
	}

	json_t *file = get_file(from_drive_index, (char *)from);
	if (file == NULL)
	{
		fuse_log_error("Could not find the source file %s\n", from);
		return -1;
	}

	int to_drive_index = get_drive_index(to);
	char *tolocal;

	wait_for_lock();
	char *to_cache_path = form_cache_path(to, true, &tolocal);

	// char *pathcpy = strdup(path);
	char *from_pathBuffer;
	char *from_filename;
	split_path_file(&from_pathBuffer, &from_filename, from);

	char *from_cache_path = form_cache_path(from_filename, false, NULL); // strcat(writeFilePathLocal, filename);

	// if file being copied is not downloaded, download file
	char *cachePathWithSubs = strip_filename_from_directory(from_cache_path);

	if (access(from_cache_path, F_OK) == -1)
	{

		if (cachePathWithSubs != NULL)
		{
			download_file(Drives[from_drive_index].in_fds[0], Drives[from_drive_index].out_fds[0], cachePathWithSubs, from_filename, from_cache_path);
		}
		else
			download_file(Drives[from_drive_index].in_fds[0], Drives[from_drive_index].out_fds[0], (char *)AbsoluteCachePath, from_filename, from_cache_path);

		// fuse_log("Drive index %d", index);
	}

	// now that file is downloaded, get size
	struct stat st;
	int size = 0;
	if (stat(from_cache_path, &st) == 0)
	{
		size = st.st_size;
	}
	// reflect changes in cache
	res = rename(from_cache_path, to_cache_path);
	fuse_log("renamed - from %s to %s \n", from_cache_path, to_cache_path);
	

	
	
	/*	Fix to bug */
	//Naming the file with 'tolocal' gives the file the whole path as the name, which 
	//includes the name of a subdirectory. We need to have just the name of the file
	//Sorry to make what is probably more redundant, path modifying code xD
	char * last_slash = tolocal;
	char * ptr = tolocal;
	while (*ptr != '\0')
	{
		if (*ptr == '/')
		{
			last_slash = ptr;
		}
		ptr++;
	}
	if (last_slash != tolocal)
		last_slash++;
	// Reflect changes in file directory
	if (Drive_insert(to_drive_index, (char *)to, create_new_file(last_slash, false, size)) < 0)
	{
		fuse_log_error("Could not insert new file\n");
		res = -1;
	}
	if (Drive_delete((char *)from) < 0)
	{
		fuse_log_error("Could not delete old file\n");
		res = -1;
	}

	addPathToDeleteLog(CacheDeleteLogName, from_filename);
	//int fd = open(to_cache_path, O_CREAT | S_IRUSR | S_IWUSR);
	struct stat foo;
	time_t mtime;
  	struct utimbuf new_times;

    stat(to_cache_path, &foo);
  mtime = foo.st_mtime; /* seconds since the epoch */

  new_times.actime = time(NULL); /* keep atime unchanged */
  new_times.modtime = time(NULL);    /* set mtime to current time */
  utime(to_cache_path, &new_times);

	/*if (fd != -1) {
        close(fd);
    }*/
	delete_lock();
	return res;
}
// woo wuz here// // do not ctrl z this bo
static struct fuse_operations operations = {
	.getattr = do_getattr,
	.readdir = do_readdir,
	.read = do_read,
	.rename = xmp_rename,
	.truncate = xmp_truncate,
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

	// int out; // fd to read from executable
	// int in;	 // fd to write to executable

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

	char cwd[512];
	if (getcwd(cwd, sizeof(cwd)) == NULL)
	{
		perror("getcwd() error");
		return -1;
	}
	AbsoluteCachePath = strcat(cwd, CacheFile);
	// fuse_log(AbsoluteCachePath);

	char *AbsoluteCachePathCpy = calloc(strlen(AbsoluteCachePath) + 11, sizeof(char));
	AbsoluteCachePathCpy = memcpy(AbsoluteCachePathCpy, AbsoluteCachePath, strlen(AbsoluteCachePath) * sizeof(char));

	CacheDeleteLogName = strcat(AbsoluteCachePathCpy, ".delete");

	char *AbsoluteCachePathCpy2 = calloc(strlen(AbsoluteCachePath) + 6, sizeof(char));
	AbsoluteCachePathCpy2 = memcpy(AbsoluteCachePathCpy2, AbsoluteCachePath, strlen(AbsoluteCachePath) * sizeof(char));
	AbsoluteLockPath= strcat(AbsoluteCachePathCpy2, ".lock");
	// fuse_log(AbsoluteCachePath);
	//  dump_drive(&(Drives[0]));
	//   To-Do:Catch Ctrl-c and kill processes
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

 //fuse_log("is drive\n");
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2; // Why "two" hardlinks instead of "one"? The answer is here: http://unix.stackexchange.com/a/101536
		return 0;
	}
	int drive = get_drive_index(path);
	if (drive > -1)
	{

		// int index = get_file_index(path, drive);
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
		int index = get_drive_index((char *)"");
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
	/*else if (strcmp(path, "/") == 0) // If the user is trying to show the files/directories of the root directory show the following
	{

		// We want to list our drives
		for (int i = 0; i < NUM_DRIVES; i++)
		{
			filler(buffer, Drives[i].dirname, NULL, 0);
		}
	}*/
	else
	{ // Then this *should* be a subdirectory
		fuse_log("This should be a subdirectory: %s\n", path);
		SubDirectory *dir = handle_subdirectory((char *)path);
		if (dir != NULL)
		{

			if (strcmp(dir->dirname, path) != 0)
			{
				fuse_log_error("fatal\n");
				exit(1);
			}
			fuse_log("Going to dump subdirectory here\n");
			dump_subdirectory(dir, 0);
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
static void split_path_file(char **pathBuffer, char **filenameBuffer, const char *fullPath)
{
	char *localFile = strdup(fullPath);
	char *ts1 = strdup(fullPath);
	char *ts2 = strdup(fullPath);

	char *tempdir = dirname(ts1);
	*pathBuffer = calloc(strlen(tempdir) + 1, sizeof(char));
	strncpy(*pathBuffer, tempdir, strlen(tempdir));

	char *tempname = basename(ts2);
	*filenameBuffer = calloc(strlen(tempname) + 1, sizeof(char));
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

	res = pread(fd, buf, size, offset);

	if (res == -1)
		fuse_log_error("pread fail\n");

	close(fd);
	return res;
}

static int do_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi)
{
	fuse_log("--> Trying to read %s, %lu, %lu, %d\n", path, offset, size, buffer);

	char cwd[512];
	if (getcwd(cwd, sizeof(cwd)) == NULL)
	{
		perror("getcwd() error");
		return -1;
	}

	wait_for_lock();

	char *downloadFile = strcat(cwd, CacheFile);

	/*char *pathBuffe;
	char *filenam;
	split_path_file(&pathBuffe, &filenam, path);*/
	

	if (get_drive_index(path) >= 0)
	{ // File is requested straight from drive

		fuse_log("In drive\n");
		char *cachePath;
		if (cache_find_or_download(&cachePath, (char *)path) == 0)
		{
			int res = xmp_read(cachePath, buffer, size, offset, fi);
			fuse_log("\n\nStat %s \n", cachePath);
		struct stat foo;
	time_t mtime;
  	struct utimbuf new_times;

    stat(cachePath, &foo);
  mtime = foo.st_mtime; /* seconds since the epoch */

  new_times.actime = time(NULL); /* keep atime unchanged */
  new_times.modtime = time(NULL);    /* set mtime to current time */
  utime(cachePath, &new_times);
		delete_lock();
			return res;
		}
		fuse_log_error("Failed to retrieve or download %s\n", path);
		delete_lock();
		return -1;
	}

/*	else if (strlen(pathBuffer) == 0)
	{
		// File is requested from home directory
		// There currently cannot be any files in home directory so we will implement this later
		fuse_log_error("File does not exist\n");
	}*/
	


	delete_lock();
	return 0;
}

static int do_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi)
{
	fuse_log("--> Trying to write %s, %lu, %lu\n", path, offset, size);
	
	char *pathBuffer;
	char *filename;
	split_path_file(&pathBuffer, &filename, path);

	wait_for_lock();
	char *fullFileName = form_cache_path(path, true, NULL);

	// char *fullFileName = strcat(writeFilePathLocal, filename);

	int i = -1;
	int index;
	if ((index = get_drive_index(path)) >= 0)
	{ // File is requested straight from drive

		fuse_log("In drive\n");
		char *cachePath;
		if (cache_find_or_download(&cachePath, (char *)path) == 0)
		{

			i = xmp_write(fullFileName, buffer, size, offset, fi);

			///	Drive_Object currDrive = Drives[index];
			struct stat st;
			int size = 0;
			if (stat(fullFileName, &st) == 0)
			{
				size = st.st_size;
			}
			int file_index = get_file_index(path, index);
			json_array_set(Drives[index].FileList, file_index, create_new_file(filename, false, size));
			delete_lock();
			return i;
		}

		fuse_log_error("Failed to retrieve or download %s\n", path);
		delete_lock();
		return -1;

		// dump_drive(&Drives[index]);
	}
	fuse_log_error("Error in get_drive_index\n");
	delete_lock();
	return -1;
}

static int xmp_write(const char *path, const char *buf, size_t size,
					 off_t offset, struct fuse_file_info *fi)
{

	int fd;
	int res;
	

	(void)fi;
	// if(fi == NULL)
	fd = open(path, O_WRONLY);

	if (fd == -1)
		return -errno;

	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	// if(fi == NULL)
	close(fd);

	return res;
}