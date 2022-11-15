#include "path_utils.h"
extern char *AbsoluteCachePath;
/**
 * Builds the absolute path to directory, if stripRoot = true, the
 * root directory is removed from the path before the absolute path
 * is made.
 * if root should be removed, providing a char ** for strippedFile
 * to return the new path.
 */
char *form_cache_path(const char *directory, bool stripRoot, char **strippedFile)
{
	char *dir = strdup(directory);
	if (stripRoot)
	{
		char *dircpy = strdup(directory);
		dir = parse_out_drive_name(dircpy);
		if (strippedFile != NULL)
		{
			*strippedFile = dir;
		}
	}
	//char *absCpy = strdup(AbsoluteCachePath);
		char *absCpy = calloc(strlen(AbsoluteCachePath)+strlen(dir) +1, sizeof(char));
	absCpy = memcpy(absCpy, AbsoluteCachePath, strlen(AbsoluteCachePath) * sizeof(char));
	char *localPath = strcat(absCpy, dir);
}


/*
Removes the last file/directory from a directory
Ex. Google_Drive/Directory1/file.txt ->Google_Drive/Directory1
*/
char*  strip_filename_from_directory(char* directory){
	char* dir = strdup(directory);
	
	char* after = strrchr(dir, '/');
	
	if (after != NULL && strlen(after) > 0){
		
char* before = calloc(strlen(dir)- strlen(after) + 1, sizeof(char));
	 before = strncpy(before, dir, strlen(dir)- strlen(after));
	 
	return before;
	}
	else 
	return NULL;
	
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