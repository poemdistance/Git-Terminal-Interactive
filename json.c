#include "json.h"


typedef struct json_obj
{
    int key_type;
    int value_type;
    char *key;
    void *normal_value;
    struct json_obj **obj_value;

} JsonObj;

typedef JsonObj* JsonObjPtr;

JsonObjPtr *init_json_obj()
{
    JsonObjPtr json_obj = calloc(sizeof(JsonObj), 1);
    return json_obj;
}

void load_from_string(char *json)
{
    if(!json)
        return NULL;

    if(*json == '\0')
        return json;

    char *p = json;
    while(*p)
    {

    }
}

int main(int argc, char **argv)
{
    char *json_str = "{\"a\": 1, \"b\": [1, 2, 3]}";
    load_from_string(json_str);
    return 0;
}
