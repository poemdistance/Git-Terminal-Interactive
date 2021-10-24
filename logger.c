#include "logger.h"

static FILE *gfp = NULL;

void clear_log_resource(void)
{
    logger("clean logger resource...\n");

    if(gfp != NULL)
        fclose(gfp);
}

void logger(const char *format, ...)
{
    if(gfp == NULL)
        gfp = fopen("./waterLog", "a+");

    va_list valist;
    va_start(valist, format);
    vfprintf(gfp, format, valist);
    va_end(valist);
}

