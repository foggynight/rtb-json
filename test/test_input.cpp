// USAGE: ./test_input INPUT RESULT
// Execute JSON parsing function on INPUT string.
// RESULT: 0 -> fail, 1 -> pass.

#include <iostream>

#include "../rtb-json.h"

void error(std::string const& msg, int code = 1) {
    std::cout << "error: " << msg << std::endl;
    exit(code);
}

int main(int argc, char **argv) {
    if (argc < 3) error("missing argument(s)");
    char *input = argv[1];
    int targ = *(argv[2]) - '0';
    bool ret = json_parse(input);
    std::cout << (ret == targ ? "PASS" : "FAIL") << std::endl;
    return 0;
}
