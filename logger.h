#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void logger(const char *format, ...);

void clear_log_resource(void);

#endif
