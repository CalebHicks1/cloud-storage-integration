#include "JsonTools.h"
#include "logging.h"


int json_list_remove(json_t ** filelist, json_t * file, int num_files)
{
	json_t * match = NULL;
	int index = -1;
	for (int i = 0; i < num_files; i++)
	{
		json_t * curr_file = json_array_get(*filelist, i);
		printf("curr file: %s\n", getJsonFileName(curr_file));
		if (strcmp(getJsonFileName(file), getJsonFileName(curr_file)) == 0)
		{
			match = curr_file;
			index = i;
			break;
		}
	}
	if (match == NULL)
	{
		return -1;
	}
	return json_array_remove(*filelist, index);
}

int json_list_append(json_t ** filelist, json_t * new_file)
{
	return json_array_append_new(*filelist, new_file);
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
	
	else if (json_is_object(file) ){
		
		//printf("\n");
	 	json_t* value = json_object_get(file, "Name");

	 	if (value ==NULL){
			return NULL;
		}
		return json_string_value(value);
	}
	//printf("Not obj\n");
	return NULL;
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
		fuse_log("Line %d - %s \n", i, stringArray[i]);
	}

	json_error_t* errorCheck = NULL;
	*fileListAsJson = json_loads(fileListAsString, 0, errorCheck);
	free(fileListAsString);

	if (errorCheck != NULL){
		fuse_log_error("Json encoding error: %s", errorCheck->text);
		return -1;
	}

	if ( !json_is_array(*fileListAsJson)){
		printf("Json was not of type array");
		return -2;
	}


	return json_array_size(*fileListAsJson);
}
