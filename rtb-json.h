// rtb-json - JSON Parser
// Copyright (C) 2025 Robert Coffey
// Released under the MIT license.

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef enum {
    JSONNull,
    JSONBool,
    JSONNumber,
    JSONString,
    JSONArray,
    JSONPair,
    JSONObject,
} JSONType;

typedef struct JSON {
    JSONType type;
    struct JSON *parent;
    struct JSON *child;       // Head of linked list of children.
    struct JSON *prev, *next; // Prev and next sibling in parent's child list.
    union {
        bool boolval;
        char *string;
        struct {
            char *number_integer;
            char *number_fraction;
            char *number_exponent;
        };
    };
} JSON;

JSON *json_parse(const char *str);
void json_delete(JSON *json);

#ifdef __cplusplus
}
#endif
