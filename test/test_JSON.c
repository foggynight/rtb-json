#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../rtb-json.h"

bool has_child(JSON const *json) {
    return json->child != NULL;
}

bool has_sibling(JSON const *json) {
    return json->prev != NULL
        || json->next != NULL;
}

bool test_JSONNull(void) {
    JSON *json = JSON_CreateNull();
    if (json == NULL
            || json->type != JSONNull
            || has_child(json)
            || has_sibling(json))
        return false;

    char *str = JSON_Print(json);
    if (strcmp(str, "null") != 0)
        return false;

    JSON_Delete(json);
    free(str);
    return true;
}

bool test_JSONBool(void) {
    JSON *json = JSON_CreateBool(false);
    if (json == NULL
            || json->type != JSONBool
            || has_child(json)
            || has_sibling(json)
            || json->boolval != false)
        return false;
    char *str = JSON_Print(json);
    if (strcmp(str, "false") != 0) {
        printf("error: invalid JSONBool string from JSON_Parse: '%s'\n", str);
        return false;
    }
    JSON_Delete(json);
    free(str);

    json = JSON_CreateBool(true);
    if (json == NULL
            || json->type != JSONBool
            || has_child(json)
            || has_sibling(json)
            || json->boolval != true)
        return false;
    str = JSON_Print(json);
    if (strcmp(str, "true") != 0) {
        printf("error: invalid JSONBool string from JSON_Parse: '%s'\n", str);
        return false;
    }
    JSON_Delete(json);
    free(str);

    return true;
}

// TODO
bool test_JSONNumber(void) { return true; }
bool test_JSONString(void) { return true; }
bool test_JSONArray(void) { return true; }
bool test_JSONPair(void) { return true; }
bool test_JSONObject(void) { return true; }

// TODO: Print info which cases failed.
int main(void) {
    bool (*test_funcs[])(void) = {
        test_JSONNull,
        test_JSONBool,
        test_JSONNumber,
        test_JSONString,
        test_JSONArray,
        test_JSONPair,
        test_JSONObject,
    };
    for (size_t i = 0; i < sizeof(test_funcs) / sizeof(*test_funcs); ++i) {
        bool (*test_func)(void) = test_funcs[i];
        if (!test_func()) {
            printf("FAIL\n");
            return 1;
        }
    }
    printf("PASS\n");
    return 0;
}
