#include "MultiList.h"
#include "logging.h"

// TODO : clean up leaks

void dump_file_list(json_t * list)
{
    if (list == NULL)
    {
        fuse_log("Empty\n");
        return;
    }
    size_t len = json_array_size(list);
    if (len < 1)
    {
        fuse_log_error("Cannot dump list - is empty\n");
        return;
    }

    for (size_t index = 0; index < len; index++)
    {
        json_t * file = json_array_get(list, index);
        printf("(%ld) %s\n", index, getJsonFileName(file));
    }

}

//Files are equal if they have the same name
int file_equals(json_t * a, json_t * b)
{
    return strcmp(getJsonFileName(a), getJsonFileName(b));
}

//Return 0 if file is in list, -1 otherwise
int file_exists_in_list(json_t * file, json_t * list)
{
    for (size_t index = 0; index < json_array_size(list); index++)
    {
        if (file_equals(file, json_array_get(list, index)) == 0)
        {
            return 0;
        }
    }
    return -1;
}

//Untested
json_t * Union(json_t ** lists, int num_lists)
{
    if (num_lists == 0)
    {
        fuse_log_error("You passed me no lists!\n");
        return NULL;
    }
    json_t * Union = json_array();
    //Copy the first list 
    for (size_t index = 0; index < json_array_size(*lists); index++)
    {
        json_list_append(&Union, json_array_get(*lists, index));
    }

    for (int list_index = 1; list_index < num_lists; list_index++)
    {
        for (size_t index = 0; index < json_array_size(*lists + list_index); index++)
        {
            if (file_exists_in_list(json_array_get(*(lists + list_index), index), Union) != 0)
            {
                fuse_log("Adding %s to list\n", getJsonFileName( json_array_get(*(lists + list_index), index) ));
                json_list_append(&Union, json_array_get(*(lists + list_index), index));
            }
        }
    }
    return Union;
}

//Return list of elements that exist in all lists
json_t * Intersection(json_t ** lists, int num_lists)
{
    json_t * intersection = json_array();
    if (num_lists == 0)
    {
        fuse_log_error("You passed me no lists!\n");
        return NULL;
    }
    for (size_t index = 0; index < json_array_size(*lists); index++)
    {
        json_t * curr_file = json_array_get(*lists, index);
        int keep = 0;
        for (int list_index = 1; list_index < num_lists; list_index++)
        {
            if (file_exists_in_list(curr_file, *(lists + list_index)) != 0)
            {
                keep = -1;
                break;
            }
        }
        if (keep == 0)
        {
            json_list_append(&intersection, curr_file);
        }
    }
    return intersection;
}


json_t * list_handler(json_t ** lists, int num_lists) 
{
    return Intersection(lists, num_lists);
}

//Gets filelist and stores it in list
int listAsArrayV2(json_t **list, struct Drive_Object * drive, char *optional_path)
{
    //For each executable, get a file list
    //Pass all these lists into list_handler, which returns us a single list
    //Assign this single list to be the filelist
    fuse_log("Now doing new listAsArray\n");
    //Assemble list of filelists
	json_t **FileLists = calloc(sizeof(json_t *), drive->num_execs);
	for (int exec_index = 0; exec_index < drive->num_execs; exec_index++)
	{
		//json_t * currList = *(FileLists + exec_index);
		char output[100][LINE_MAX_BUFFER_SIZE] = {};
		int a = myGetFileList(output, drive->exec_paths[exec_index], optional_path, drive->in_fds[exec_index], drive->out_fds[exec_index]);
		if (a < 1)
		{
			fuse_log_error("GetFileList failed for executable %s\n", drive->exec_paths[exec_index]);
			*(FileLists + exec_index) = NULL;
			continue;
		}
        
		int arraySize = parseJsonString(FileLists + exec_index, output, a);
		fuse_log("arraysize: %d\n", arraySize);
		dump_file_list(*(FileLists + exec_index));
	}
    //Get list to return
    *list = list_handler(FileLists, drive->num_execs);
    return json_array_size(*list);
    
}