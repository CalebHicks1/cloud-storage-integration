#include "JsonTools.h"

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
		
		 printf("\n");
	 	json_t* value = json_object_get(file, "Name");

	 	if (value ==NULL){
			return NULL;
		}
		return json_string_value(value);
	}
	printf("Not obj\n");
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
int parseJsonString(json_t** fileListAsJson, char stringArray[][1024], int numberOfLines){

	char* fileListAsString = calloc((numberOfLines * 1024) +1, sizeof(char));
	for( int i = 0; i < numberOfLines; i++){
		fileListAsString = strncat(fileListAsString, stringArray[0], strlen(stringArray[0]));
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