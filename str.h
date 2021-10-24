#ifndef __STR_H__
#define __STR_H__

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

char *point_to_last_char(char *str);

char *remove_trailing_slash_character(char *str);

/* feature: finding the 'target' sting in 'str' and then replace it with 'replace',
 * return: return new string after processing, you should need to free it while its not null*/
char *str_outside_replace(char *str, char *target, char *replace);

/* replace 'target' with 'replace' inside the 'str'*/
char *str_inside_replace(char *str, char *target, char *replace);

/* find the substring in 'str'.
 * return 'target' sting address if found, otherwise return NULL*/
char *find_substr(char *str, char *target);

/* only check the first 'len' characters and find the substring in 'str'.
 * return 'target' sting address if found, otherwise return NULL*/
char *find_substr_l(char *str, size_t len, char *target);

#endif
