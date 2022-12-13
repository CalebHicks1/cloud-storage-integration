#include <stdlib.h>
#include <unistd.h>
#include "cache_utils.h"

extern const char *CacheFile;
extern Drive_Object Drives[NUM_DRIVES];
extern char *AbsoluteCachePath;
extern char *AbsoluteLockPath;

void wait_for_lock(){
	FILE *fPtr;
	fuse_log("wait for lock - %s\n", AbsoluteLockPath);
	struct stat buffer;
    int exist;
	
	while((exist = stat(AbsoluteLockPath, &buffer)) == 0)
	{
	

	}
	fPtr = fopen(AbsoluteLockPath, "w");
	fclose(fPtr);
}

void delete_lock(){
	
	fuse_log("delete for locl - %s\n", AbsoluteLockPath);
		struct stat buffer;
    int exist;
	
	if((exist = stat(AbsoluteLockPath, &buffer)) == 0)
	{
		remove(AbsoluteLockPath);

	}
	
}
//char * cache_location;

// char * assemble_cache_path(char *path)
// {
//     char cwd[512];
//     if (getcwd(cwd, sizeof(cwd)) == NULL)
//     {
//         fuse_log_error("Could not get current directory\n");
//         return NULL;
//     }
//     char * cacheDir = calloc(sizeof(char), strlen(CacheFile) + strlen(cwd));
//     strcat(cacheDir, cwd);
//     strcat(cacheDir, CacheFile);
    
//     // char * ptr = path + 1;
//     // while (*ptr != '\0' && *ptr != '/') { ptr++; }
//     // if (*ptr != '/')
//     // {
//     //     fuse_log_error("%s is not a valid path\n");
//     //     return NULL;
//     // }
//     //ptr++;
//     char *rel_filename = parse_out_drive_name(path);
//     char * fullpath = calloc(sizeof(char), strlen(cacheDir) + strlen(rel_filename) + 1);
//     strcat(fullpath, cacheDir);
//     strcat(fullpath, rel_filename);
//     free(cacheDir);
//     return fullpath;
// }

// int cache_find(char *res, char * path)
// {
//     char * fullpath = assemble_cache_path(path);
//     if (fullpath == NULL)
//     {
//         return -1;
//     }
//     else
//     {
//         memcpy(res, fullpath, strlen(fullpath));
//         int ret = access(fullpath, F_OK);
//         free(fullpath);
//         return ret;
//     }
// }



int add_subdirectory_to_cache(char* path){
 fuse_log("add cache dir called %s\n", path);
char* cacheDir  =  form_cache_path(path, true, NULL);
 return mkdir(cacheDir, 0755);
 
}
/**
 * Checks if delete log exists, if log does not exist create log
 * otherwise append to log
 */
void addPathToDeleteLog(const char *logPath, char *directory)
{
	FILE *fPtr;
	if ((fPtr = fopen(logPath, "a")) == NULL)
	{
		fPtr = fopen(logPath, "w");
	}

	fputs(directory, fPtr);
	fputs("\n", fPtr);

	fclose(fPtr);
}

/**int init_cache_location()
{
    char cwd[512];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        fuse_log_error("Could not get current directory\n");
        return -1;
    }
    cache_location = calloc(sizeof(char), strlen(CacheFile) + strlen(cwd) + 1);
    strcat(cache_location, cwd);
    strcat(cache_location, CacheFile);
    return 0;
}**/

int cache_find_or_download(char ** res, char * path)
{
    /*if (cache_location == NULL)
    {
        if (init_cache_location() < 0)
        {
            return -1;
        }
    }*/
    //char * rel_filename = parse_out_drive_name(path);
   // *res = calloc(sizeof(char), strlen(rel_filename) + strlen(cache_location) + 1);
   // strcat(*res, cache_location);
   // strcat(*res, rel_filename);
   char * rel_filename;
   *res =  form_cache_path(path, true, &rel_filename);

  
    if (access(*res, F_OK) >= 0)
    {
        return 0;
    }
    fuse_log("Need to download %s\n", path);
    int drive_index = get_drive_index(path);
    if (drive_index < 0)
    {
        fuse_log_error("Could not get drive for path %s\n", path);
        return -1;
    }
    Drive_Object currDrive = Drives[drive_index];
    char *cachePathWithSubs = strip_filename_from_directory(rel_filename);

int downloaded =-1;
	if (access(*res, F_OK) == -1)
	{

		if (cachePathWithSubs != NULL )
		{
			downloaded = download_file(currDrive.in_fds[0], currDrive.out_fds[0], cachePathWithSubs, rel_filename, *res);
		}
		else
			downloaded = download_file(currDrive.in_fds[0], currDrive.out_fds[0], AbsoluteCachePath, rel_filename, *res);

		// fuse_log("Drive index %d", index);
	}
   // int downloaded = download_file(currDrive.in_fds[0], currDrive.out_fds[0], cache_location, rel_filename, *res);
  
    if (access(*res, F_OK) >= 0)
    {
        return 0;
    }
    fuse_log_error("Downloading %s failed\n", path);
    return -1;

        
    


}

int download_file(int fdin, int fdout, char *downloadFile, char *filename, char *cachePath)
{
	dprintf(fdin, "{\"command\":\"download\", \"path\":\"%s\", \"files\":[\"%s\"]}\n", downloadFile, filename);
	fuse_log("{\"command\":\"download\", \"path\":\"%s\", \"files\":[\"%s\"]}\n", downloadFile, filename);
	// Should wait for output from the api, currently just blocks forever
	char buff[11];
	int i = 1;
	while (1)
	/*{
		i++;
		if (access(cachePath, F_OK) != -1){
			return 1;
		}
	}*/
		// TODO : see TODO above
		if (read(fdout, buff, 10) > 0)
		{

			if (strncmp(buff, "{\"code\":0,\"message\":\"No Error\"}", 9) == 0)
			{

				fuse_log_error("Successfully downloaded %s\n", buff);
				return 1;

				return 1;
			}
			else if (strncmp(buff, "{\"code\":0,\"message\":\"No Error\"}", 1) == 0)
			{
				fuse_log_error("Unsuccessfully downloaded %s\n", buff);
				return -1;
			}
		
		}

	return -1;
}