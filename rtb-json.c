#include "rtb-json.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// utils -----------------------------------------------------------------------

void print_error(char const *msg) {
    printf("error: ");
    printf("%s\n", msg);
}

// JSON specification doesn't include all characters identified as whitespace by
// ctype isspace.
bool char_is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

bool char_is_digit(char c) {
    return c >= '0' && c <= '9';
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
int consume_whitespace(void) {
    int count = 0;
    while (char_is_space(next())) {
        consume();
        ++count;
    }
    return count;
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

bool next_number(void) {
    char const c = next();
    return c == '-' || char_is_digit(c);
}

bool parse_digits(void) {
    bool digit_found = char_is_digit(next());
    while (char_is_digit(next())) {
        consume();
    }
    return digit_found;
}

bool parse_natural0(void) {
    if (next() == '0') {
        consume();
        if (char_is_digit(next())) {
            print_error("parse_natural0: digits follow leading zero");
            return false;
        }
    } else if (char_is_digit(next())) {
        if (!parse_digits()) return false;
    }
    return true;
}

bool parse_exponent(void) {
    int sign = 1;
    if (next() == '-') {
        consume();
        sign = -1;
    } else if (next() == '+') {
        consume();
    }
    return parse_digits();
}

bool parse_number(void) {
    int sign = 1;
    if (next() == '-') {
        sign = -1;
        consume();
    }
    if (!parse_natural0()) return false;
    if (next() == '.') {
        consume();
        if (!parse_digits()) return false;
    }
    if (next() == 'e' || next() == 'E') {
        consume();
        if (!parse_exponent()) return false;
    }
    return true;
}

bool parse_string(void) {
    if (!expect('"')) return false;
    while (next() != '"')
        if (!consume()) return false;
    if (!expect('"')) return false;
    return true;
}

bool parse_value(void);

bool parse_array(void) {
    if (!expect('[')) return false;
    consume_whitespace();
    if (next() == ']') {
        consume();
        return true;
    }
    if (!parse_value()) return false;
    while (next() == ',') {
        consume();
        if (!parse_value()) return false;
    }
    if (!expect(']')) return false;
    return true;
}

bool parse_pair(void) {
    consume_whitespace();
    if (!parse_string()) return false;
    consume_whitespace();
    if (!expect(':')) return false;
    if (!parse_value()) return false;
    return true;
}

bool parse_object(void) {
    if (!expect('{')) return false;
    consume_whitespace();
    if (next() == '}') {
        consume();
        return true;
    }
    parse_pair();
    while (next() == ',') {
        consume();
        if (!parse_pair()) return false;
    }
    if (!expect('}')) return false;
    return true;
}

bool parse_value(void) {
    consume_whitespace();
    int match_len;
    if (next_null()) {
        return consume_n(sizeof("null")-1);
    } else if (match_len = next_bool()) {
        return consume_n(match_len);
    } else if (next_number()) {
        return parse_number();
    } else if (next() == '"') {
        return parse_string();
    } else if (next() == '[') {
        return parse_array();
    } else if (next() == '{') {
        return parse_object();
    }
    return false;
    consume_whitespace();
}

bool json_parse(char *str) {
    input_str = str;
    input_len = strlen(input_str);
    input_i = 0;
    if (!parse_value()) return false;
    if (!expect('\0')) return false;
    return true;
}

// main (test) -----------------------------------------------------------------

#include <stdio.h>
int main(void) {
    char *strs[] = {
        "1",
        "{\"this\": 1}",
        "[1, -2, -12.15e-015, \"this\", true, false, \" test\", null]",
        "[  ]",
        "{   }",
        "1  23", // FALSE
    };
    for (size_t i = 0; i < sizeof(strs) / sizeof(*strs); ++i)
        printf("%s\n", json_parse(strs[i]) ? "true" : "false");
}
