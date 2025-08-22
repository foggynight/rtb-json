// rtb-json - JSON Parser
// Copyright (C) 2025 Robert Coffey
// Released under the MIT license.

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

// NOTE: Must still call `free` on caller `buf` if heap allocated.
void cbuf_delete(CBuf *buf) {
    if (buf->items) free(buf->items);
    buf->items = NULL;
    buf->size = buf->capacity = 0;
}

void cbuf_clear(CBuf *buf) {
    buf->size = 0;
}

bool cbuf_append(CBuf *buf, char c) {
    if (buf->size >= buf->capacity) {
        buf->capacity = (buf->capacity == 0) ? 64 : buf->capacity * 2;
        buf->items = realloc(buf->items, buf->capacity * sizeof(*(buf->items)));
        if (!buf->items) {
            print_error("cbuf_append: reallocation failed");
            return false;
        }
    }
    buf->items[buf->size++] = c;
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

bool cbuf_reserve(CBuf *buf, size_t capacity) {
    char *items = realloc(buf->items, capacity * sizeof(*(buf->items)));
    if (!items) {
        print_error("cbuf_reserve: reallocation failed");
        return false;
    }
    buf->items = items;
    buf->capacity = capacity;
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

// JSON ------------------------------------------------------------------------

// NOTE: Must still call `free` on caller `json` if heap allocated.
void JSON_Delete(JSON *json) {
    switch (json->type) {
    case JSONString:
        if (json->string != NULL) free(json->string);
        break;
    case JSONNumber:
        if (json->number_integer != NULL) free(json->number_integer);
        if (json->number_fraction != NULL) free(json->number_fraction);
        if (json->number_exponent != NULL) free(json->number_exponent);
        break;
    }
    JSON *walk = json->child;
    while (walk != NULL) {
        JSON_Delete(walk);
        JSON *next = walk->next;
        free(walk);
        walk = next;
    }
}

void JSON_AddChild(JSON *parent, JSON *child) {
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

bool JSON_Print_(JSON *json, CBuf *buf) {
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
        if (!cbuf_append_str(buf, json->number_integer)) return false;
        if (*(json->number_fraction) != '\0') {
            if (!cbuf_append(buf, '.')) return false;
            if (!cbuf_append_str(buf, json->number_fraction)) return false;
        }
        if (*(json->number_exponent) != '\0') {
            if (!cbuf_append(buf, 'e')) return false;
            if (!cbuf_append_str(buf, json->number_exponent)) return false;
        }
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

char *JSON_Print(JSON *json) {
    static CBuf buf = {0};
    cbuf_clear(&buf);
    if (!JSON_Print_(json, &buf)) return NULL;
    char *str = malloc(buf.size * sizeof(char));
    if (!str) {
        print_error("json_str: failed to allocate string");
        return NULL;
    }
    strncpy(str, buf.items, buf.size);
    return str;
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
    // Parse integer part of number.
    cbuf_clear(&parse_buf);
    if (next() == '-') {
        consume();
        cbuf_append(&parse_buf, '-');
    }
    if (!parse_natural0(&parse_buf)) return false;
    json->number_integer = cbuf_print(&parse_buf);

    // Parse fraction part of number.
    cbuf_clear(&parse_buf);
    if (next() == '.') {
        consume();
        if (!parse_digits(&parse_buf)) return false;
    }
    json->number_fraction = cbuf_print(&parse_buf);

    // Parse fraction part of number.
    cbuf_clear(&parse_buf);
    if (next() == 'e' || next() == 'E') {
        consume();
        if (!parse_exponent(&parse_buf)) return false;
    }
    json->number_exponent = cbuf_print(&parse_buf);

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

JSON *JSON_Parse(const char *str) {
    input_str = str;
    input_len = strlen(input_str);
    input_i = 0;
    JSON *json = parse_value();
    cbuf_delete(&parse_buf);
    if (!json || !expect('\0')) return NULL;
    return json;
}

// main (test) -----------------------------------------------------------------

int main(void) {
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
            print_error("JSON_Parse: failed to parse JSON");
        } else {
            char *str = JSON_Print(json);
            printf("TYPE = %d: ", json->type);
            printf("%s -> %s\n", json_str, str);
            free(str);
            JSON_Delete(json);
            free(json);
        }
    }
    return 0;
}
