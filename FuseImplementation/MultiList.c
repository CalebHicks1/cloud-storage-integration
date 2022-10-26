#include "MultiList.h"
#include "logging.h"
void dump_file_list(json_t * list)
{
    if (list == NULL)
    {
        fuse_log_error("Cannot dump list - is null\n");
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