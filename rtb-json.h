// rtb-json - JSON Parser
// Copyright (C) 2025 Robert Coffey
// Released under the MIT license.

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef enum {
    JSONInvalid,
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

// Construct a JSON object by parsing `str`; must be `JSON_Delete`d.
JSON *JSON_Parse(const char *str);

// Construct a JSON object of type `type` manually; must be `JSON_Delete`d.
JSON *JSON_Create(JSONType type);
JSON *JSON_CreateNull(void);
JSON *JSON_CreateBool(bool const val);
JSON *JSON_CreateNumber(double const num);
JSON *JSON_CreateString(char const *str);
JSON *JSON_CreateArray(void);
JSON *JSON_CreatePair(void);
JSON *JSON_CreateObject(void);

// Delete `json` JSON object and all children.
void JSON_Delete(JSON *json);

// Add `child` JSON object to list of children of `parent`.
void JSON_AddChild(JSON *parent, JSON *child);

// Add name:value pair to `json` object; string, array, and object are parsed.
JSON *JSON_ObjectAddNull(JSON * const json, char const * const name);
JSON *JSON_ObjectAddBool(JSON * const json, char const * const name, bool const val);
JSON *JSON_ObjectAddTrue(JSON * const json, char const * const name);
JSON *JSON_ObjectAddFalse(JSON * const json, char const * const name);
JSON *JSON_ObjectAddNumber(JSON * const json, char const * const name, double const num);
JSON *JSON_ObjectAddString(JSON * const json, char const * const name, char const * const str);
JSON *JSON_ObjectAddArray(JSON * const json, char const * const name, char const * const str);
JSON *JSON_ObjectAddObject(JSON * const json, char const * const name, char const * const str);

// Render `json` as string, string must be `free`d.
char *JSON_Print(JSON *json);

#ifdef __cplusplus
}
#endif
