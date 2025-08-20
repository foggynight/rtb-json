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

typedef struct JSONDocument {
    JSONType type;
    struct JSONDocument *parent;
    struct JSONDocument *children;    // Head of linked list of children.
    struct JSONDocument *prev, *next; // Prev and next sibling in parent's
                                      // children list.
    union {
        bool boolval;
        char *string;
        struct {
            char *number_integer;
            char *number_fraction;
            char *number_exponent;
        };
    };
} JSONDocument;

void jsondoc_destroy(JSONDocument *doc);

JSONDocument *json_parse(const char *str);

#ifdef __cplusplus
}
#endif
