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
char *str_replace(char *str, char *target, char *replace);

#endif
