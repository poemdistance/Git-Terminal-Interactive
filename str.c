#include "str.h"

char *point_to_last_char(char *str)
{
    if(!str)
        return str;

    char *p = str;
    while(*p++);

    return p-2;
}

char *remove_trailing_slash_character(char *str)
{
    if(!str)
        return str;

    char *p = point_to_last_char(str);

    /* erase multiple trailing slash characters*/
    while(p > str && *p == '/' && ((*p-- = '\0') || true));

    return str;
}

/* do not pass const string to 'str', or program will crash while modifying it*/
char *str_inside_replace(char *str, char *target, char *replace)
{
    if(!str || !target || !replace)
        return str;

    if(*target == '\0' || *str == '\0')
        return str;

    /* length of target string more than source string, it's impossible find the target string*/
    size_t target_len = strlen(target);
    size_t replace_len = strlen(replace);
    size_t src_str_len = strlen(str);
    if(target_len > src_str_len)
        return str;

    if(replace_len > target_len)
    {
        fprintf(stderr, "replace string cannot larger than target string\n");
        exit(1);
    }

    char *left = str;
    char *right = str;
    char *t = target;
    size_t compare_len = 0;
    while(*right)
    {
        if(*right == *t)
        {
            while(*right && *t && (*right++ == *t++));

            /* only case match the 'target' string in 'str'*/
            if(*t == '\0' && *(right-1) == *(t-1))
            {
                strncpy(left, replace, replace_len);
                left += replace_len;
                t = target;
                continue;
            }

            compare_len = t - target;
            strncpy(left, right-compare_len, compare_len);
            t = target;
            left += compare_len;
            continue;
        }

        *left++ = *right++;
    }

    *left = '\0';

    return str;
}

char *str_outside_replace(char *str, char *target, char *replace)
{
    if(!str || !target || !replace)
        return str;

    if(*target == '\0' || *str == '\0')
        return strdup(str);

    /* length of target string more than source string, it's impossible find the target string*/
    size_t target_len = strlen(target);
    size_t replace_len = strlen(replace);
    size_t src_str_len = strlen(str);
    if(target_len > src_str_len)
        return strdup(str);

    char *new = NULL;
    size_t max_new_str_len = src_str_len + replace_len;
    new = calloc(sizeof(char), max_new_str_len + 1);

    char *s = str;
    char *t = target;
    char *n = new;
    size_t compare_len = 0;
    size_t new_str_len = 0;
    while(*s)
    {
        if(*s == *t)
        {
            while(*s && *t && (*s++ == *t++));

            /* only case match the 'target' string in 'str'*/
            if(*t == '\0' && *(s-1) == *(t-1))
            {
                if(new_str_len + replace_len >= max_new_str_len)
                {
                    max_new_str_len = max_new_str_len * 2;
                    new = realloc(new, max_new_str_len);
                    n = new + new_str_len;
                }

                strcpy(n, replace);
                n += replace_len;
                new_str_len += replace_len;
                t = target;
                continue;
            }

            /* mismatch target string,
             * copying origin string into new buffer*/

            compare_len = t - target;
            if(new_str_len + compare_len >= max_new_str_len)
            {
                max_new_str_len = max_new_str_len * 2;
                new = realloc(new, max_new_str_len);
                n = new + compare_len;
            }
            strncat(new, s-compare_len, compare_len);
            new_str_len += compare_len;
            n += compare_len;
            t = target;
            continue;
        }

        *n++ = *s++;
        new_str_len++;
    }

    *n = '\0';

    return new;
}

/* int main() */
/* { */
/*     char *new = str_outside_replace("月がきれいてす", "てす", "てすね"); */
/*     if(new) */
/*     { */
/*         printf("outside replace new string: %s\n", new); */
/*         free(new); */
/*     } */

/*     char buf[] = "あなたのことがだいすきてす"; */
/*     printf("inside replace new string: %s\n", str_inside_replace(buf, "だいすき", "すき")); */

/*     return 0; */
/* } */
