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

bool cbuf_appendstr(CBuf *buf, char const *str) {
    while (*str != '\0') {
        if (!cbuf_append(buf, *str))
            return false;
        ++str;
    }
    return true;
}

void cbuf_clear(CBuf *buf) {
    buf->size = 0;
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

char *cbuf_str(CBuf const *buf) {
    char *str = malloc(buf->size + 1);
    if (!str) {
        print_error("cbuf_str: allocation failed");
        return NULL;
    }
    strncpy(str, buf->items, buf->size);
    str[buf->size] = '\0';
    return str;
}

// JSONDocument ----------------------------------------------------------------

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

void jsondoc_addchild(JSONDocument *parent, JSONDocument *child) {
    child->parent = parent;
    if (parent->children == NULL) {
        parent->children = child;
        child->prev = child->next = NULL;
    } else {
        JSONDocument *walk = parent->children;
        while (walk->next != NULL) walk = walk->next;
        walk->next = child;
        child->prev = walk;
        child->next = NULL;
    }
}

bool jsondoc_str_(JSONDocument *doc, CBuf *buf) {
    JSONDocument *child = NULL;
    switch (doc->type) {
    case JSONNull:
        if (!cbuf_appendstr(buf, "null")) return false;
        break;
    case JSONBool:
        if (!cbuf_appendstr(buf, doc->boolval ? "true" : "false"))
            return false;
        break;
    case JSONNumber:
        if (!cbuf_appendstr(buf, doc->number_integer)) return false;
        if (*(doc->number_fraction) != '\0') {
            if (!cbuf_append(buf, '.')) return false;
            if (!cbuf_appendstr(buf, doc->number_fraction)) return false;
        }
        if (*(doc->number_exponent) != '\0') {
            if (!cbuf_append(buf, 'e')) return false;
            if (!cbuf_appendstr(buf, doc->number_exponent)) return false;
        }
        break;
    case JSONString:
        cbuf_append(buf, '"');
        cbuf_appendstr(buf, doc->string);
        cbuf_append(buf, '"');
        break;
    case JSONArray:
        cbuf_append(buf, '[');
        child = doc->children;
        while (child != NULL) {
            jsondoc_str_(child, buf);
            child = child->next;
            if (child != NULL)
                cbuf_append(buf, ',');
        }
        cbuf_append(buf, ']');
        break;
    case JSONPair:
        jsondoc_str_(doc->children, buf);
        cbuf_append(buf, ':');
        jsondoc_str_(doc->children->next, buf);
        break;
    case JSONObject:
        cbuf_append(buf, '{');
        child = doc->children;
        while (child != NULL) {
            jsondoc_str_(child, buf);
            child = child->next;
            if (child != NULL)
                cbuf_append(buf, ',');
        }
        cbuf_append(buf, '}');
        break;
    }
    return true;
}

char *jsondoc_str(JSONDocument *doc) {
    static CBuf buf = {0};
    cbuf_clear(&buf);
    if (!jsondoc_str_(doc, &buf)) return NULL;
    char *str = malloc(buf.size * sizeof(char));
    if (!str) {
        print_error("jsondoc_str: failed to allocate string");
        return NULL;
    }
    strncpy(str, buf.items, buf.size);
    return str;
}

// parser ----------------------------------------------------------------------
//
// TODO: Explain how parser works and difference between functions which take
// CBuf or JSONDocument as input, and return bool or JSONDocument as output.
//
// TODO: Add more error messages signaling input errors.

// Copy of input string to work with during parsing.
static char *input_str;
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

bool parse_null(JSONDocument *doc) {
    if (!expect_str("null")) return false;
    doc->type = JSONNull;
    return true;
}

int next_bool(void) {
    if      (next() == 't') return 4; // strlen("true")
    else if (next() == 'f') return 5; // strlen("false")
    else                    return 0;
}
bool parse_bool(JSONDocument *doc, int len) {
    if (len == 4) {
        expect_str("true");
        doc->boolval = true;
    } else if (len == 5) {
        expect_str("false");
        doc->boolval = false;
    } else return false;
    doc->type = JSONBool;
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

bool parse_number(JSONDocument *doc) {
    // Parse integer part of number.
    cbuf_clear(&parse_buf);
    if (next() == '-') {
        consume();
        cbuf_append(&parse_buf, '-');
    }
    if (!parse_natural0(&parse_buf)) return false;
    doc->number_integer = cbuf_str(&parse_buf);

    // Parse fraction part of number.
    cbuf_clear(&parse_buf);
    if (next() == '.') {
        consume();
        if (!parse_digits(&parse_buf)) return false;
    }
    doc->number_fraction = cbuf_str(&parse_buf);

    // Parse fraction part of number.
    cbuf_clear(&parse_buf);
    if (next() == 'e' || next() == 'E') {
        consume();
        if (!parse_exponent(&parse_buf)) return false;
    }
    doc->number_exponent = cbuf_str(&parse_buf);

    doc->type = JSONNumber;
    return true;
}

// TODO: Add string escape '\'.
bool parse_string(JSONDocument *doc) {
    cbuf_clear(&parse_buf);
    if (!expect('"')) return false;
    while (next() != '"') {
        cbuf_append(&parse_buf, next());
        if (!consume()) return false;
    }
    if (!expect('"')) return false;
    doc->string = cbuf_str(&parse_buf);
    doc->type = JSONString;
    return true;
}

JSONDocument *parse_value(void);

bool parse_array(JSONDocument *doc) {
    if (!expect('[')) return false;
    consume_whitespace();
    if (next() == ']') {
        consume();
        goto done;
    }
    while (true) {
        JSONDocument *child = parse_value();
        if (!child) return false;
        jsondoc_addchild(doc, child);
        if (next() != ',')
            break;
        consume();
    }
    if (!expect(']')) return false;
done:
    doc->type = JSONArray;
    return true;
}

JSONDocument *parse_pair(void) {
    JSONDocument *pair = calloc(1, sizeof(JSONDocument));
    if (!pair) {
        print_error("parse_pair: failed to allocate JSONDocument");
        return NULL;
    }
    JSONDocument *key = calloc(1, sizeof(JSONDocument));
    if (!key) {
        print_error("parse_pair: failed to allocate JSONDocument");
        return NULL;
    }
    JSONDocument *val; // set by `parse_value`
    consume_whitespace();
    if (!parse_string(key)) goto fail;
    consume_whitespace();
    if (!expect(':')) goto fail;
    if (!(val = parse_value())) goto fail;
    jsondoc_addchild(pair, key);
    jsondoc_addchild(pair, val);
    pair->type = JSONPair;
    return pair;
fail:
    free(pair);
    return NULL;
}

bool parse_object(JSONDocument *doc) {
    if (!expect('{')) return false;
    consume_whitespace();
    if (next() == '}') {
        consume();
        goto done;
    }
    while (true) {
        JSONDocument *pair = parse_pair();
        if (!pair) return false;
        jsondoc_addchild(doc, pair);
        if (next() != ',')
            break;
        consume();
    }
    if (!expect('}')) return false;
done:
    doc->type = JSONObject;
    return true;
}

JSONDocument *parse_value(void) {
    JSONDocument *doc = calloc(1, sizeof(JSONDocument));
    if (!doc) {
        print_error("parse_value: failed to allocate JSONDocument");
        return NULL;
    }
    consume_whitespace();
    int match_len;
    if (next() == 'n') {
        if (!parse_null(doc)) goto fail;
    } else if (match_len = next_bool()) {
        if (!parse_bool(doc, match_len)) goto fail;
    } else if (next_number()) {
        if (!parse_number(doc)) goto fail;
    } else if (next() == '"') {
        if (!parse_string(doc)) goto fail;
    } else if (next() == '[') {
        if (!parse_array(doc)) goto fail;
    } else if (next() == '{') {
        if (!parse_object(doc)) goto fail;
    } else {
        goto fail;
    }
    consume_whitespace();
    return doc;
fail:
    free(doc);
    return NULL;
}

JSONDocument *json_parse(const char *str) {
    input_str = malloc((strlen(str) + 1));
    if (!input_str) {
        print_error("json_parse: malloc failed to allocate input buffer");
        return false;
    }
    strcpy(input_str, str);
    input_len = strlen(input_str);
    input_i = 0;

    JSONDocument *doc = parse_value();
    if (!doc || !expect('\0')) return NULL;
    return doc;
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
        "{\"a\": null, \"b\": 0, \"c\": [1.0,2e0,3]}",
    };
    size_t len = sizeof(strs) / sizeof(*strs);
    for (size_t i = 0; i < len; ++i) {
        char const *json = strs[i];
        JSONDocument *doc = json_parse(json);
        if (!doc) {
            print_error("json_parse: failed to parse JSON");
        } else {
            printf("TYPE = %d: ", doc->type);
            printf("%s -> %s\n", json, jsondoc_str(doc));
        }
    }
    return 0;
}
