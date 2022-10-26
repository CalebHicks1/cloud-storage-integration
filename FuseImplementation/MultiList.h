#ifndef MULTILIST_H
#define MULTILIST_H
#include "JsonTools.h"
#include "Drive.h"



struct ListContainer
{
    json_t * intersection;
    json_t * Union;
};
typedef struct ListContainer ListContainer;

void dump_file_list(json_t * list);
int listAsArrayV2(json_t **list, struct Drive_Object * drive, char *optional_path);


#endif