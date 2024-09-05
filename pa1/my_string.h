#ifndef MY_STRING
#define MY_STRING

#include <sys/types.h>

size_t strlength(char *str);
char* itoa(int num);
int is_identical(char *str1, char *str2);
int is_matching(char *str1, char *str2);
int is_matching_char(char char1, char char2);

#endif