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
#define LINE_MAX_BUFFER_SIZE 255

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <jansson.h>
#include <sys/stat.h>


struct Drive_Object {
	char dirname[200];
	json_t* FileList;
	int fd;
	char exec_path[200];
	int num_files;
	
};
typedef struct Drive_Object Drive_Object;

#define NUM_DRIVES 2
struct Drive_Object Drives[NUM_DRIVES] = 
{ 
	{"Test_Dir", NULL, -1, "./getFile", 0} ,
	{"Google_Drive", NULL, -1, "./getFile", 0}
};


int getFileList(char lines[][LINE_MAX_BUFFER_SIZE]) ;
int parseJsonString(json_t** fileListAsArray, char stringArray[][LINE_MAX_BUFFER_SIZE], int numberOfLines);


json_t* FileList; 
int SizeOfFileList = 0;

int is_drive(const char *path) {
	for (int i = 0; i < NUM_DRIVES; i++) {
		if (strcmp(Drives[i].dirname, path + 1) == 0) {
			return 0;
		}
	}
	return -1;
}

int get_drive_index(char * path) {
	for (int i = 0; i < NUM_DRIVES; i++) {
		if (strcmp(path + 1, Drives[i].dirname) == 0) {
			return i;
		}
	}
	return -1;
	
}

static int do_getattr( const char *path, struct stat *st )
{
	printf( "[getattr] Called\n" );
	printf( "\tAttributes of %s requested\n", path );
	if (is_drive(path) == 0) {
		printf("Drive found\n");
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2; // Why "two" hardlinks instead of "one"? The answer is here: http://unix.stackexchange.com/a/101536
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
	
	st->st_uid = getuid(); // The owner of the file/directory is the user who mounted the filesystem
	st->st_gid = getgid(); // The group of the file/directory is the same as the group of the user who mounted the filesystem
	st->st_atime = time( NULL ); // The last "a"ccess of the file/directory is right now
	st->st_mtime = time( NULL ); // The last "m"odification of the file/directory is right now
	
	if ( strcmp( path, "/" ) == 0 )
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

/*
* passing in a json object, function returns a string value representation of 
file name.
If json object is a json string -  dump object to string
If json object is not a string - check object for a filename 
*/
const char* getJsonFileName(json_t* file){
	if (json_is_string(file) ){
		return  json_string_value(file);
	}
	else{
		//To-do implement json object (kv pairs) to get file name
		return json_dumps(file, 0);
	}
}

// "filler": https://www.cs.nmsu.edu/~pfeiffer/fuse-tutorial/html/unclear.html
static int do_readdir( const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi )
{
	printf( "--> Getting The List of Files of %s\n", path );
	
	filler( buffer, ".", NULL, 0 ); // Current Directory
	filler( buffer, "..", NULL, 0 ); // Parent Directory
	if (is_drive(path) == 0) {
		int index = get_drive_index((char*) path);
		if (index < 0) {
			printf("Error in get_drive_index\n");
			return -1;
		}
		Drive_Object currDrive = Drives[index];
		for (size_t index = 0; index < (currDrive.num_files*sizeof(json_t)); index += sizeof(json_t)) {
			const char* fileName = getJsonFileName(json_array_get(currDrive.FileList, 0));
			
			if (fileName != NULL){
			filler( buffer, fileName, NULL, 0 );
			}
			else{
				printf("NULL");
			}
		}
		
		return 0;
	}
	
	if ( strcmp( path, "/" ) == 0 ) // If the user is trying to show the files/directories of the root directory show the following
	{
		//printf("readd dump %s\n", json_dumps(FileList, 0));
		//for(size_t index = 0; index < (SizeOfFileList*sizeof(json_t)); index+= sizeof(json_t)){
		
			//const char* fileName = getJsonFileName(json_array_get(FileList, 0));
			
			//if (fileName != NULL){
			//filler( buffer, fileName, NULL, 0 );
			//}
			//else{
				//printf("NULL");
			//}
		
		//}
		//We want to list our drives
		for (int i = 0; i < NUM_DRIVES; i++) {
			filler(buffer, Drives[i].dirname, NULL, 0);
		}
		
	}
	
	return 0;
}

static int do_read( const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi )
{
	printf( "--> Trying to read %s, %lu, %lu\n", path, offset, size );
	
	char file54Text[] = "Hello World From File54!";
	char file349Text[] = "Hello World From File349!";
	char *selectedText = NULL;
	
	// ... //
	
	if ( strcmp( path, "/file54" ) == 0 )
		selectedText = file54Text;
	else if ( strcmp( path, "/file349" ) == 0 )
		selectedText = file349Text;
	else
		return -1;
	
	// ... //
	
	memcpy( buffer, selectedText + offset, size );
		
	return strlen( selectedText ) - offset;
}

static struct fuse_operations operations = {
    .getattr	= do_getattr,
    .readdir	= do_readdir,
   // .read		= do_read,
};

int myGetFileList(char lines[][LINE_MAX_BUFFER_SIZE], char * cmd) {
  FILE *fp;
  char path[LINE_MAX_BUFFER_SIZE];
  /* Open the command for reading. */
  fp = popen(cmd, "r");
  if (fp == NULL) {
    return -1;
  }
 
  int cnt = 0;
  while (fgets(path, sizeof(path), fp) != NULL) {
    strcpy(lines[cnt++], path);
  }
  pclose(fp);
  return cnt;  
}

int populate_filelists() {
	for (int i = 0; i < NUM_DRIVES; i++) {
		printf("Generating FileList for %s\n", Drives[i].dirname);
		char execOutput[100][LINE_MAX_BUFFER_SIZE];
		int a = myGetFileList(execOutput, Drives[i].exec_path);
		
		json_t* fileListAsArray ;
		

		if (a < 1){
			printf("file list getter was not executed properly or output was empty\n");
			return -1;	
		}

   
		int arraySize = parseJsonString(&fileListAsArray, execOutput, a);
	
		if (arraySize < 1){
			return arraySize;
		}
		Drives[i].FileList = json_deep_copy(fileListAsArray);
		Drives[i].num_files = arraySize;
		printf("iteration complete\n");
	}
}



/*
* main calls getFileList, if successful main sets global variables and starts fuse_main
*
*/
int main( int argc, char *argv[] )
{
	char execOutput[100][LINE_MAX_BUFFER_SIZE];
	int a = getFileList(execOutput);
	json_t* fileListAsArray ;
	populate_filelists();

	if (a < 1){
		printf("file list getter was not executed properly or output was empty\n");
		return -1;	
	 }

   
	int arraySize = parseJsonString(&fileListAsArray, execOutput, a);
	
	if (arraySize < 1){
		return arraySize;
	}
	FileList = json_deep_copy(fileListAsArray);
	SizeOfFileList = arraySize;

	return fuse_main( argc, argv, &operations, NULL );
}

/**
 * Runs an executable (currently ./getFile but can be replaced by any shell command with its path)
 * Input: char list to output command results to
 * Output: Number of lines found in executable output
 * 	-1 -> executable failed to run or was not found.
 **/
int getFileList(char lines[][LINE_MAX_BUFFER_SIZE]) {
  FILE *fp;
  char path[LINE_MAX_BUFFER_SIZE];
	//char* cmd = "./getFile";
  char * cmd = Drives[0].exec_path;
  /* Open the command for reading. */
  fp = popen(cmd, "r");
  if (fp == NULL) {
    return -1;
  }
 
  int cnt = 0;
  while (fgets(path, sizeof(path), fp) != NULL) {
    strcpy(lines[cnt++], path);
  }
  pclose(fp);
  return cnt;  
}

/**
 * Gets a string array and returns a json array.
 * Input: json_t** json object to be modified
 * char string array
 * int number of lines in the string array
 * 
 * Return: <1 error occurred in parsing or no array items were found
 * 
 * >=1 number of items in json array.
 * 
 **/
int parseJsonString(json_t** fileListAsJson, char stringArray[][LINE_MAX_BUFFER_SIZE], int numberOfLines){

	char* fileListAsString = calloc((numberOfLines * LINE_MAX_BUFFER_SIZE) +1, sizeof(char));
	for( int i = 0; i < numberOfLines; i++){
		fileListAsString = strncat(fileListAsString, stringArray[i], strlen(stringArray[i]));
	}


	json_error_t* errorCheck = NULL;
	*fileListAsJson = json_loads(fileListAsString, 0, errorCheck);

	if (errorCheck != NULL){
		printf("Json encoding error: %s", errorCheck->text);
		return -1;
	}

	if ( !json_is_array(*fileListAsJson)){
		printf("Json was not of type array");
		return -2;
	}


	return json_array_size(*fileListAsJson);
}
