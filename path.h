#ifndef __PATH_H__
#define __PATH_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <linux/limits.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

bool recursive_create_dir(char *path);

/* get the absolute path of giving relative path, store the result in calloc memory chunk.
 * if return result is not null, you should free it yourself*/
char *absolute_path(char *path);

/* return result is stored in static area, you don't need to free it
 * but you should be care of overwriting issue in subsequent calling*/
char *home_dir();

#endif
