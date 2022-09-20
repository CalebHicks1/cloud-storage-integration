#include <stdio.h>
#include <dirent.h>
#include <jansson.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
int main(void)
{
	struct dirent *de; // Pointer for directory entry
	char *directory = "./testFolderForGetFile";
	// opendir() returns a pointer of DIR type.
	DIR *dr = opendir(directory);

	if (dr == NULL) // opendir returns NULL if couldn't open directory
	{
		printf("Could not open current directory");
		return 0;
	}

	// Refer http://pubs.opengroup.org/onlinepubs/7990989775/xsh/readdir.html
	// for readdir()
	json_t *file_name_array = json_array();
	while ((de = readdir(dr)) != NULL)
	{
		struct stat stats;
		char path[ strlen(directory) + strlen(de->d_name) + 2];
		// if (de->d_type == DT_REG){
		// add error check
		snprintf(path, strlen(directory) + strlen(de->d_name) + 2, "%s/%s", directory, de->d_name);
		if (stat(path, &stats) == 0)
		{

			json_t *obj = json_object();
			if (obj != NULL && strcmp(".", de->d_name) != 0 && strcmp("..", de->d_name) != 0)
			{
				json_object_set(obj, "Name", json_string(de->d_name));
				json_object_set(obj, "Size", json_integer(stats.st_size));
				if (de->d_type == DT_DIR)
				{
					json_object_set(obj, "IsDir", json_string("true"));
				}
				else
				{
					json_object_set(obj, "IsDir", json_string("false"));
				}
				json_array_append(file_name_array, obj);
			}
		}

		//}
	}

	json_dumpfd(file_name_array, 1, 0);

	closedir(dr);
	return 0;
}
