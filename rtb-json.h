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

// Construct a JSON struct by parsing a string; must be `JSON_Delete`d.
JSON *JSON_Parse(char const * const str);

// Construct a JSON struct of a given type manually; must be `JSON_Delete`d.
JSON *JSON_Create(JSONType const type);
JSON *JSON_CreateNull(void);
JSON *JSON_CreateBool(bool const val);
JSON *JSON_CreateNumber(double const num);
JSON *JSON_CreateString(char * const str);
JSON *JSON_CreateArray(void);
JSON *JSON_CreatePair(char * const name, JSON * const value);
JSON *JSON_CreateObject(void);

// Add child JSON struct to parent JSON struct's list of children.
void JSON_AddChild(JSON * const parent, JSON * const child);

// Add value to JSON struct of JSONType "array".
// NOTE: `JSON_ArrayAdd` parses `str` argument using `JSON_Parse`.
// NOTE: `JSON_ArrayAdd{String,Array,Object}` also parse `str` argument, but
//       additionally return NULL if parsed type is not the type called for.
JSON *JSON_ArrayAdd(JSON * const json, char const * const str);
JSON *JSON_ArrayAddNull(JSON * const json);
JSON *JSON_ArrayAddTrue(JSON * const json);
JSON *JSON_ArrayAddFalse(JSON * const json);
JSON *JSON_ArrayAddBool(JSON * const json, bool const val);
JSON *JSON_ArrayAddNumber(JSON * const json, double const num);
JSON *JSON_ArrayAddString(JSON * const json, char const * const str);
JSON *JSON_ArrayAddArray(JSON * const json, char const * const str);
JSON *JSON_ArrayAddObject(JSON * const json, char const * const str);

// Add name:value pair to JSON struct of JSONType "object".
// NOTE: `JSON_ObjectAdd` parses `str` argument using `JSON_Parse`.
// NOTE: `JSON_ObjectAdd{String,Array,Object}` also parse `str` argument, but
//       additionally return NULL if parsed type is not the type called for.
JSON *JSON_ObjectAdd(JSON * const json, char * const name, char const * const str);
JSON *JSON_ObjectAddNull(JSON * const json, char const * const name);
JSON *JSON_ObjectAddTrue(JSON * const json, char const * const name);
JSON *JSON_ObjectAddFalse(JSON * const json, char const * const name);
JSON *JSON_ObjectAddBool(JSON * const json, char const * const name, bool const val);
JSON *JSON_ObjectAddNumber(JSON * const json, char const * const name, double const num);
JSON *JSON_ObjectAddString(JSON * const json, char const * const name, char const * const str);
JSON *JSON_ObjectAddArray(JSON * const json, char const * const name, char const * const str);
JSON *JSON_ObjectAddObject(JSON * const json, char const * const name, char const * const str);

// Render JSON struct as string, string must be `free`d.
char *JSON_Print(JSON const * const json);

// Delete JSON struct and all children.
void JSON_Delete(JSON * const json);

#ifdef __cplusplus
}
#endif
