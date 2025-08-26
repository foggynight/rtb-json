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
        double number;
        char *string;
    };
} JSON;

// Construct a JSON struct by parsing a string; must be `JSON_Delete`d.
JSON *JSON_Parse(char const * const str);

// Construct a JSON struct of a given type manually; must be `JSON_Delete`d.
JSON *JSON_Create(JSONType const type);
JSON *JSON_CreateNull(void);
JSON *JSON_CreateBool(bool const val);
JSON *JSON_CreateNumber(double const num);
JSON *JSON_CreateString(char const * const str);
JSON *JSON_CreateArray(void);
JSON *JSON_CreatePair(char * const name, JSON * const val);
JSON *JSON_CreateObject(void);

// Add value to JSON struct of JSONType "array".
// NOTE: `JSON_ArrayAddParse` parses `str` argument using `JSON_Parse`.
// NOTE: `JSON_ArrayAdd{Array,Object}` also parse `str` argument, but
//       additionally return NULL if parsed type is not the type called for.
JSON *JSON_ArrayAdd(JSON * const json, JSON * const val);
JSON *JSON_ArrayAddParse(JSON * const json, char const * const str);
JSON *JSON_ArrayAddNull(JSON * const json);
JSON *JSON_ArrayAddBool(JSON * const json, bool const val);
JSON *JSON_ArrayAddNumber(JSON * const json, double const num);
JSON *JSON_ArrayAddString(JSON * const json, char const * const str);
JSON *JSON_ArrayAddArray(JSON * const json, char const * const str);
JSON *JSON_ArrayAddObject(JSON * const json, char const * const str);

// Add name:value pair to JSON struct of JSONType "object".
// NOTE: `JSON_ObjectAddParse` parses `str` argument using `JSON_Parse`.
// NOTE: `JSON_ObjectAdd{Array,Object}` also parse `str` argument, but
//       additionally return NULL if parsed type is not the type called for.
JSON *JSON_ObjectAdd(JSON * const json, char * const name, JSON * const val);
JSON *JSON_ObjectAddParse(JSON * const json, char * const name, char const * const str);
JSON *JSON_ObjectAddNull(JSON * const json, char * const name);
JSON *JSON_ObjectAddBool(JSON * const json, char * const name, bool const val);
JSON *JSON_ObjectAddNumber(JSON * const json, char * const name, double const num);
JSON *JSON_ObjectAddString(JSON * const json, char * const name, char const * const str);
JSON *JSON_ObjectAddArray(JSON * const json, char * const name, char const * const str);
JSON *JSON_ObjectAddObject(JSON * const json, char * const name, char const * const str);

// Render JSON struct as string, string must be `free`d.
char *JSON_Print(JSON const * const json);

// Delete JSON struct and all children.
void JSON_Delete(JSON * const json);

#ifdef __cplusplus
}
#endif
