#include "path.h"
#include "str.h"


/* return result is stored in static area, you don't need to free it
 * but you should be care of overwriting issue in subsequent calling*/
char *home_dir()
{
    struct passwd *pw = getpwuid(getuid());
    char *home_dir = pw->pw_dir;
    return home_dir;
}

char *skip_relative_path(char *path)
{
    if(!path)
        return path;

    char *p = path;
    while(*p)
    {
        if(*p == '.')
        {
            while(*p)
            {
                if(*p != '/')
                    p++;
                else
                    break;
            }

            if(*p == '\0')
                return NULL;
        }
        else if(*p == '/')
        {
            while(*p)
            {
                if(*p == '/')
                    p++;
                else
                    break;
            }
            if(*p == '\0')
                return NULL;

            continue;
        }
        else
        {
            return p;
        }

        p++;
    }

    return NULL;
}

/* get the absolute path of giving relative path, store the result in calloc memory chunk.
 * if return result is not null, you should free it yourself*/
char *absolute_path(char *path)
{
    if(!path)
        return path;

    char tmp_path[PATH_MAX] = { '\0' };
    char *abs_path = NULL;
    char *p = NULL;

    switch(*path)
    {
        case '/':
            abs_path = calloc(sizeof(char), strlen(path)+1);
            strcpy(abs_path, path);
            break;
        case '.':
            abs_path = calloc(sizeof(char), PATH_MAX);
            p = skip_relative_path(path);
            strncpy(tmp_path, path, p-path);
            realpath(tmp_path, abs_path);
            strcat(abs_path, "/");
            strcat(abs_path, p);
            break;
        case '~':
            p = home_dir();
            abs_path = calloc(sizeof(char), strlen(p) + strlen(path));
            strcpy(abs_path, p);
            strcat(abs_path, path+1);
            break;
        default:
            abs_path = calloc(sizeof(char), PATH_MAX);
            if(getcwd(abs_path, PATH_MAX)  == NULL)
            {
                fprintf(stderr, "getcwd failed\n");
                exit(1);
            }
            strcat(abs_path, "/");
            strcat(abs_path, path);
            break;
    }

    return abs_path;
}

char *get_parent_dir(char *path)
{
    if(!path)
        return path;

    remove_trailing_slash_character(path);

    /* point to the last character*/
    char *p = point_to_last_char(path);

    /* after remove all redundant slash characters, only left one slash character,
     * it means path is root dir with no more parent directory*/
    if(p == path && *p == '/')
    {
        *p = '\0';
        return path;
    }

    /* locate previous slash character*/
    while(p > path && *p-- != '/');

    if(p == path && *p == '/')
        *(p+1) = '\0';
    else if (p == path && *p != '/')
        *p = '\0';
    else
        *(p+1) = '\0';

    return path;
}

bool recursive_create_dir(char *path)
{
    /* can not create invalid path, just return*/
    if(path == NULL || *path == '\0')
        return true;

    struct stat st;
    if(lstat(path, &st) == 0)
    {
        /* fprintf(stdout, "path: %s already exists\n", path); */
        return true;
    }

    char *dup_path = strdup(path);
    if(!dup_path)
    {
        fprintf(stderr, "strdup %s failed\n", path);
        goto error_exit;
    }

    char *parent_path = get_parent_dir(dup_path);
    recursive_create_dir(parent_path);

    if(mkdir(path, 0755) != 0)
    {
        fprintf(stderr, "create %s failed, errno: %s\n", path, strerror(errno));
        goto error_exit;
    }

normal_exit:

    if(dup_path)
        free(dup_path);

    return true;


error_exit:

    if(dup_path)
        free(dup_path);

    return false;
}

//int main(int argc, char **argv)
//{
//    if(argc <= 1)
//        return 0;
//
//    printf("create dir result: %d\n", recursive_create_dir(argv[1]));
//    return 0;
//}
