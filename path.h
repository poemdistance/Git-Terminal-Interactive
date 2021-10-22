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

bool recursive_create_dir(char *path);
char *absolute_path(char *path);

#endif
