// rtb-json - JSON Parser
// Copyright (C) 2025 Robert Coffey
// Released under the MIT license.

// TODO: Make private functions static.

#include "rtb-json.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// utils -----------------------------------------------------------------------

void print_error(char const *msg) {
    fprintf(stderr, "error: ");
    fprintf(stderr, "%s\n", msg);
}

// JSON specification doesn't include all characters identified as whitespace by
// ctype isspace.
bool char_isspace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}
bool char_isdigit(char c) {
    return '0' <= c && c <= '9';
}

bool str_prefix(char const *str, char const *prefix) {
    while (*prefix)
        if (*(str++) != *(prefix++))
            return false;
    return true;
}
int str_prefix_len(char const *str, char const *prefix) {
    if (str_prefix(str, prefix))
        return strlen(prefix);
    return 0;
}

typedef struct CBuf {
    char *items;
    size_t size;
    size_t capacity;
} CBuf;

void cbuf_clear(CBuf *buf) {
    buf->size = 0;
}

int cbuf_remaining(CBuf const * buf) {
    return buf->capacity - buf->size;
}

bool cbuf_reserve(CBuf *buf, size_t capacity) {
    buf->capacity = capacity;
    char *items = realloc(buf->items, capacity * sizeof(*(buf->items)));
    if (!items) {
        print_error("cbuf_reserve: reallocation failed");
        return false;
    }
    buf->items = items;
    return true;
}

bool cbuf_grow(CBuf *buf) {
    buf->capacity = (buf->capacity == 0) ? 64 : buf->capacity * 2;
    buf->items = realloc(buf->items, buf->capacity * sizeof(*(buf->items)));
    if (!buf->items) {
        print_error("cbuf_grow: reallocation failed");
        return false;
    }
    return true;
}

bool cbuf_append(CBuf *buf, char c) {
    if (buf->size+1 >= buf->capacity)
        cbuf_grow(buf);
    buf->items[buf->size++] = c;
    buf->items[buf->size] = '\0';
    return true;
}

bool cbuf_append_str(CBuf *buf, char const *str) {
    while (*str != '\0') {
        if (!cbuf_append(buf, *str))
            return false;
        ++str;
    }
    return true;
}

char *cbuf_print(CBuf const *buf) {
    char *str = malloc(buf->size + 1);
    if (!str) {
        print_error("cbuf_print: allocation failed");
        return NULL;
    }
    strncpy(str, buf->items, buf->size);
    str[buf->size] = '\0';
    return str;
}

// NOTE: Must still call `free` on caller `buf` if heap allocated.
void cbuf_delete(CBuf *buf) {
    if (buf->items) free(buf->items);
    buf->items = NULL;
    buf->size = buf->capacity = 0;
}

// JSON ------------------------------------------------------------------------

JSON *JSON_Create(JSONType const type) {
    JSON *json = calloc(1, sizeof(JSON));
    if (json) json->type = type;
    return json;
}

JSON *JSON_CreateNull(void) {
    return JSON_Create(JSONNull);
}

JSON *JSON_CreateBool(bool const val) {
    JSON *json = JSON_Create(JSONBool);
    if (json) json->boolval = val;
    return json;
}

JSON *JSON_CreateNumber(double const num) {
    JSON *json = JSON_Create(JSONNumber);
    if (json) json->number = num;
    return json;
}

JSON *JSON_CreateString(char const * const str) {
    size_t str_len = strlen(str);
    char *json_str = malloc((str_len + 1) * sizeof(*str));
    if (!json_str) return NULL;
    strncpy(json_str, str, str_len + 1);
    JSON *json = JSON_Create(JSONString);
    if (json) json->string = json_str;
    return json;
}

JSON *JSON_CreateArray(void) {
    return JSON_Create(JSONArray);
}

static void JSON_AddChild(JSON * const parent, JSON * const child) {
    child->parent = parent;
    if (parent->child == NULL) {
        parent->child = child;
        child->prev = child->next = NULL;
    } else {
        JSON *walk = parent->child;
        while (walk->next != NULL) walk = walk->next;
        walk->next = child;
        child->prev = walk;
        child->next = NULL;
    }
}

JSON *JSON_CreatePair(char * const name, JSON * const val) {
    JSON *pair = JSON_Create(JSONPair);
    if (!pair) return NULL;
    JSON *name_ = JSON_CreateString(name);
    JSON_AddChild(pair, name_);
    JSON_AddChild(pair, val);
    return pair;
}

JSON *JSON_CreateObject(void) {
    return JSON_Create(JSONObject);
}

JSON *JSON_ArrayAdd(JSON * const json, JSON * const val) {
    JSON_AddChild(json, val);
    return json;
}

JSON *JSON_ArrayAddParse(JSON * const json, char const * const str) {
    JSON *val = JSON_Parse(str);
    if (!val) return NULL;
    JSON_AddChild(json, val);
    return json;
}

JSON *JSON_ArrayAddNull(JSON * const json) {
    JSON *json_null = JSON_CreateNull();
    if (!json_null) return NULL;
    JSON_AddChild(json, json_null);
    return json;
}

JSON *JSON_ArrayAddBool(JSON * const json, bool const val) {
    JSON *json_bool = JSON_CreateBool(val);
    if (!json_bool) return NULL;
    JSON_AddChild(json, json_bool);
    return json;
}

JSON *JSON_ArrayAddNumber(JSON * const json, double const num) {
    JSON *json_number = JSON_CreateNumber(num);
    if (!json_number) return NULL;
    JSON_AddChild(json, json_number);
    return json;
}

JSON *JSON_ArrayAddString(JSON * const json, char const * const str) {
    JSON *json_str = JSON_CreateString(str);
    if (!json_str) return NULL;
    JSON_AddChild(json, json_str);
    return json;
}

JSON *JSON_ArrayAddArray(JSON * const json, char const * const str) {
    JSON *json_parsed = JSON_Parse(str);
    if (!json_parsed || json_parsed->type != JSONArray)
        return false;
    JSON_AddChild(json, json_parsed);
    return json;
}

JSON *JSON_ArrayAddObject(JSON * const json, char const * const str) {
    JSON *json_parsed = JSON_Parse(str);
    if (!json_parsed || json_parsed->type != JSONObject)
        return false;
    JSON_AddChild(json, json_parsed);
    return json;
}

JSON *JSON_ObjectAdd(JSON * const json, char * const name, JSON * const val) {
    JSON *pair = JSON_CreatePair(name, val);
    if (!pair) return NULL;
    JSON_AddChild(json, pair);
    return json;
}

JSON *JSON_ObjectAddParse(JSON * const json,
        char * const name, char const * const str)
{
    JSON *val = JSON_Parse(str);
    if (!val) return NULL;
    return JSON_ObjectAdd(json, name, val);
}

// TODO: Add ..ObjectAdd.. function definitions.

static bool JSON_Print_(JSON const * const json, CBuf * const buf) {
    JSON *child = NULL;
    switch (json->type) {
    case JSONNull:
        if (!cbuf_append_str(buf, "null")) return false;
        break;
    case JSONBool:
        if (!cbuf_append_str(buf, json->boolval ? "true" : "false"))
            return false;
        break;
    case JSONNumber:
        char const *fmt = "%lg";
        int number_len = snprintf(NULL, 0, fmt, json->number);
        if (number_len < 0) return false;
        while (number_len + 1 > cbuf_remaining(buf)) cbuf_grow(buf);
        if (snprintf(buf->items + buf->size, number_len + 1,
                    fmt, json->number)
                < 1)
            return false;
        buf->size += number_len;
        break;
    case JSONString:
        cbuf_append(buf, '"');
        cbuf_append_str(buf, json->string);
        cbuf_append(buf, '"');
        break;
    case JSONArray:
        cbuf_append(buf, '[');
        child = json->child;
        while (child != NULL) {
            JSON_Print_(child, buf);
            child = child->next;
            if (child != NULL)
                cbuf_append(buf, ',');
        }
        cbuf_append(buf, ']');
        break;
    case JSONPair:
        JSON_Print_(json->child, buf);
        cbuf_append(buf, ':');
        JSON_Print_(json->child->next, buf);
        break;
    case JSONObject:
        cbuf_append(buf, '{');
        child = json->child;
        while (child != NULL) {
            JSON_Print_(child, buf);
            child = child->next;
            if (child != NULL)
                cbuf_append(buf, ',');
        }
        cbuf_append(buf, '}');
        break;
    }
    return true;
}

char *JSON_Print(JSON const * const json) {
    static CBuf buf = {0};
    cbuf_clear(&buf);
    if (!JSON_Print_(json, &buf)) return NULL;
    char *str = malloc(buf.size * sizeof(char));
    if (!str) {
        print_error("JSON_Print: failed to allocate string");
        return NULL;
    }
    strncpy(str, buf.items, buf.size);
    cbuf_delete(&buf);
    return str;
}

void JSON_Delete(JSON *json) {
    switch (json->type) {
    case JSONString:
        if (json->string != NULL) free(json->string);
        break;
    }
    JSON *walk = json->child;
    while (walk != NULL) {
        JSON_Delete(walk);
        JSON *next = walk->next;
        free(walk);
        walk = next;
    }
    free(json);
}

// parser ----------------------------------------------------------------------
//
// TODO: Explain how parser works and difference between functions which take
// CBuf or JSON as input, and return bool or JSON as output.
//
// TODO: Add more error messages signaling input errors.

// Copy of input string to work with during parsing.
static char const *input_str;
static int input_len;
static int input_i;

// Buffer used as temporary memory during number/string parsing.
static CBuf parse_buf = {0};

char next(void) { return input_str[input_i]; }

bool consume(void) {
    if (input_i < input_len) {
        ++input_i;
        return true;
    }
    return false;
}
bool consume_n(int n) {
    while (n-- > 0)
        if (!consume())
            return false;
    return true;
}
int consume_whitespace(void) {
    int count = 0;
    while (char_isspace(next())) {
        consume();
        ++count;
    }
    return count;
}
bool consume_ifnext(char c) {
    if (next() != c)
        return false;
    consume();
    return true;
}

bool expect(char c) {
    if (next() == c) {
        consume();
        return true;
    }
    fprintf(stderr, "error: expect: expected '%c', received '%c'\n", c, next());
    return false;
}
bool expect_str(char const *str) {
    while (*str) {
        if (!expect(*(str++)))
            return false;
    }
    return true;
}

bool parse_null(JSON *json) {
    if (!expect_str("null")) return false;
    json->type = JSONNull;
    return true;
}

int next_bool(void) {
    if      (next() == 't') return 4; // strlen("true")
    else if (next() == 'f') return 5; // strlen("false")
    else                    return 0;
}
bool parse_bool(JSON *json, int len) {
    if (len == 4) {
        expect_str("true");
        json->boolval = true;
    } else if (len == 5) {
        expect_str("false");
        json->boolval = false;
    } else return false;
    json->type = JSONBool;
    return true;
}

bool next_number(void) {
    char const c = next();
    return c == '-' || char_isdigit(c);
}

bool parse_digit(CBuf *buf) {
    if (!char_isdigit(next())) return false;
    cbuf_append(buf, next());
    consume();
    return true;
}
bool parse_digits(CBuf *buf) {
    bool digit_found = char_isdigit(next());
    while (parse_digit(buf));
    return digit_found;
}

bool parse_natural0(CBuf *buf) {
    if (next() == '0') {
        parse_digit(buf);
        if (char_isdigit(next())) {
            print_error("parse_natural0: digits follow leading zero");
            return false;
        }
        return true;
    }
    return parse_digits(buf);
}

bool parse_exponent(CBuf *buf) {
    if (next() == '-' || next() == '+') {
        cbuf_append(buf, next());
        consume();
    }
    return parse_digits(buf);
}

bool parse_number(JSON *json) {
    cbuf_clear(&parse_buf);
    // Parse integer part of number.
    if (next() == '-') {
        consume();
        cbuf_append(&parse_buf, '-');
    }
    if (!parse_natural0(&parse_buf)) return false;
    // Parse fraction part of number.
    if (next() == '.') {
        consume();
        cbuf_append(&parse_buf, '.');
        if (!parse_digits(&parse_buf)) return false;
    }
    // Parse fraction part of number.
    if (next() == 'e' || next() == 'E') {
        cbuf_append(&parse_buf, next());
        consume();
        if (!parse_exponent(&parse_buf)) return false;
    }

    cbuf_append(&parse_buf, '\0');
    if (sscanf(parse_buf.items, "%lg", &(json->number)) < 1)
        return false;

    json->type = JSONNumber;
    return true;
}

// TODO: Add string escape '\'.
bool parse_string(JSON *json) {
    cbuf_clear(&parse_buf);
    if (!expect('"')) return false;
    while (next() != '"') {
        cbuf_append(&parse_buf, next());
        if (!consume()) return false;
    }
    if (!expect('"')) return false;
    json->string = cbuf_print(&parse_buf);
    json->type = JSONString;
    return true;
}

JSON *parse_value(void);

bool parse_array(JSON *json) {
    if (!expect('[')) return false;
    consume_whitespace();
    if (next() == ']') {
        consume();
        goto done;
    }
    while (true) {
        JSON *child = parse_value();
        if (!child) return false;
        JSON_AddChild(json, child);
        if (next() != ',')
            break;
        consume();
    }
    if (!expect(']')) return false;
done:
    json->type = JSONArray;
    return true;
}

JSON *parse_pair(void) {
    JSON *pair = calloc(1, sizeof(JSON));
    if (!pair) {
        print_error("parse_pair: failed to allocate JSON");
        return NULL;
    }
    JSON *key = calloc(1, sizeof(JSON));
    if (!key) {
        print_error("parse_pair: failed to allocate JSON");
        return NULL;
    }
    JSON *val; // set by `parse_value`
    consume_whitespace();
    if (!parse_string(key)) goto fail;
    consume_whitespace();
    if (!expect(':')) goto fail;
    if (!(val = parse_value())) goto fail;
    JSON_AddChild(pair, key);
    JSON_AddChild(pair, val);
    pair->type = JSONPair;
    return pair;
fail:
    free(pair);
    return NULL;
}

bool parse_object(JSON *json) {
    if (!expect('{')) return false;
    consume_whitespace();
    if (next() == '}') {
        consume();
        goto done;
    }
    while (true) {
        JSON *pair = parse_pair();
        if (!pair) return false;
        JSON_AddChild(json, pair);
        if (next() != ',')
            break;
        consume();
    }
    if (!expect('}')) return false;
done:
    json->type = JSONObject;
    return true;
}

JSON *parse_value(void) {
    JSON *json = calloc(1, sizeof(JSON));
    if (!json) {
        print_error("parse_value: failed to allocate JSON");
        return NULL;
    }
    consume_whitespace();
    int match_len;
    if (next() == 'n') {
        if (!parse_null(json)) goto fail;
    } else if (match_len = next_bool()) {
        if (!parse_bool(json, match_len)) goto fail;
    } else if (next_number()) {
        if (!parse_number(json)) goto fail;
    } else if (next() == '"') {
        if (!parse_string(json)) goto fail;
    } else if (next() == '[') {
        if (!parse_array(json)) goto fail;
    } else if (next() == '{') {
        if (!parse_object(json)) goto fail;
    } else {
        goto fail;
    }
    consume_whitespace();
    return json;
fail:
    free(json);
    return NULL;
}

JSON *JSON_Parse(char const * const str) {
    input_str = str;
    input_len = strlen(input_str);
    input_i = 0;
    JSON *json = parse_value();
    cbuf_delete(&parse_buf);
    if (!json || !expect('\0')) return NULL;
    return json;
}

// main (test) -----------------------------------------------------------------

void test_parser(void) {
    char const *strs[] = {
        //"1",
        //"-1",
        //"1.2e5",
        //"-1.2e+5",
        //"\"this is a test\"",
        //"{\"a\": 1, \"b\": 2}",
        //"null",
        //"true",
        //"false",
        //"[null, true, false]",
        //"[null, true, false, 0, -1.0e2, \"test\"]",
        "{\"a\": null, \"b\": 0, \"c\": [1,-2.0e0,3.0]}",
    };
    size_t len = sizeof(strs) / sizeof(*strs);
    for (size_t i = 0; i < len; ++i) {
        char const *json_str = strs[i];
        JSON *json = JSON_Parse(json_str);
        if (!json) {
            print_error("test_parser: failed to parse JSON");
        } else {
            char *str = JSON_Print(json);
            printf("TYPE = %d: ", json->type);
            printf("%s -> %s\n", json_str, str);
            free(str);
            JSON_Delete(json);
            free(json);
        }
    }
}

void test_JSON(void) {
    //JSON *json = JSON_CreateObject();
    //JSON_ObjectAddParse(json, "a", "1");
    //JSON_ObjectAddParse(json, "b", "2.5");
    //JSON_ObjectAddParse(json, "c", "3e2");
    JSON *json = JSON_CreateArray();
    JSON_ArrayAddParse(json, "1");
    JSON_ArrayAddParse(json, "2.5");
    JSON_ArrayAddParse(json, "3e2");
    JSON_ArrayAddParse(json, "4e6");
    JSON_ArrayAddArray(json, "[1,2,3]");
    char *str = JSON_Print(json);
    printf("%s\n", str);
    free(str);
}

int main(void) {
    //test_parser();
    test_JSON();
    return 0;
}
