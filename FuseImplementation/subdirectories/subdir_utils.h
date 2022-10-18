#ifndef SUBDIR_UTILS_H
#define SUBDIR_UTILS_H

char ** split(char * path);
void split_free(char ** split);
int count_tokens(char ** split);

#endif
