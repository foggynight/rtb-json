#include "rtb-json.h"

#include <ctype.h> // TODO: TEMPORARY
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// utils -----------------------------------------------------------------------

// JSON specification doesn't include all characters identified as whitespace by
// ctype isspace.
bool char_is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
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

// Remove whitespace from a string, except for that within double quotes.
// NOTE: Modifies `str`, must not be a string literal.
void remove_spaces_unquoted(char *src) {
    bool in_string = false;
    char *dst = src;
    do {
        while (!in_string && char_is_space(*src))
            ++src;
        if (*src == '"') in_string = !in_string;
    } while (*(dst++) = *(src++));
}

// parser ----------------------------------------------------------------------

char *input_str;
int input_len;
int input_i;

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

bool expect(char expected) {
    if (next() == expected) {
        consume();
        return true;
    }
    printf("error: expect: expected '%c', received '%c'\n", expected, next());
    return false;
}

bool next_null(void) {
    return str_prefix(input_str + input_i, "null");
}

int next_bool(void) {
    const int is_true = str_prefix_len(input_str + input_i, "true");
    return is_true ? is_true : str_prefix_len(input_str + input_i, "false");
}

// TODO
bool parse_number(void) {
    if (isdigit(next())) {
        consume();
        return true;
    }
    return false;
}

// TODO
bool parse_string(void) {
    return parse_number();
}

bool parse_value(void) {
    int match_len;
    if (next_null()) {
        consume_n(sizeof("null")-1);
        return true;
    } else if (match_len = next_bool()) {
        consume_n(match_len);
        return true;
    } else if (parse_number()) {
        // TODO
        return true;
    } else if (parse_string()) {
        // TODO
        return true;
    }
    return false;
}

bool parse_array(void) {
    if (!expect('[')) return false;
    if (!parse_value()) return false;
    while (next() == ',') {
        consume();
        if (!parse_value()) return false;
    }
    if (!expect(']')) return false;
    return true;
}

bool parse_pair(void) {
    if (!parse_string()) return false;
    if (!expect(':')) return false;
    if (!parse_value()) return false;
    return true;
}

bool parse_object(void) {
    if (!expect('{')) return false;
    parse_pair();
    while (next() == ',') {
        consume();
        if (!parse_pair()) return false;
    }
    if (!expect('}')) return false;
    return true;
}

bool json_parse(char *str) {
    // NOTE: Allocates enough memory to store original string, more than
    // necessary for the string with spaces removed, but saves a pass over
    // string spent determining new memory size.
    input_str = malloc((strlen(str) + 1) * sizeof(*str));
    if (!input_str) {
        printf("json_parse: malloc failed\n");
        return false;
    }
    strcpy(input_str, str);
    remove_spaces_unquoted(input_str);
    printf("original:  %s\n", str);
    printf("no spaces: %s\n", input_str);

    input_len = strlen(input_str);
    input_i = 0;

    // TODO: Parse a value, not just object.
    if (!parse_object()) return false;
    if (!expect('\0')) return false;

    free(input_str);

    return true;
}

// main (test) -----------------------------------------------------------------

#include <stdio.h>
int main(void) {
    char *strs[] = {
        "{}",
        "{1 :	2,3 :4}",
        "{1 :	2,3 :null}",
        "{  1 :	true , 3 : null  }",
    };
    for (size_t i = 0; i < sizeof(strs) / sizeof(*strs); ++i)
        printf("%s\n", json_parse(strs[i]) ? "true" : "false");
    printf("DONE\n");
}
