#include <stdio.h>
#include <dirent.h>
#include <jansson.h>
#include <string.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
int main(void)
{
	struct dirent *de; // Pointer for directory entry
    json_t* one = json_array();
	// opendir() returns a pointer of DIR type.
	DIR *dr = opendir("./testFolderForGetFile");

	if (dr == NULL) // opendir returns NULL if couldn't open directory
	{
		printf("Could not open current directory" );
		return 0;
	}

	// Refer http://pubs.opengroup.org/onlinepubs/7990989775/xsh/readdir.html
	// for readdir()
    json_t* file_name_array = json_array();
	while ((de = readdir(dr)) != NULL){
        //if (de->d_type == DT_REG){
			//add error check
			json_t* obj = json_object();
			if (obj != NULL){
				json_object_set(obj, "Name", json_string(de->d_name));
				json_object_set(obj, "Size", json_integer(de->d_reclen));
				if (de->d_type == DT_DIR){
					json_object_set(obj, "IsDir", json_string("true"));
				}
				else{
					json_object_set(obj, "IsDir", json_string("false"));
				}
            	json_array_append(file_name_array, obj);
			}

		//}
    }
	

    json_dumpfd(file_name_array, 1, 0);


	closedir(dr);	
	return 0;
}
