// USAGE: ./test_cases FILE...
// Execute JSON parsing function on the contents of each FILE and print results.
// Desired case result is determined by filename prefix.
//   y -> success
//   n -> failure
//   i -> either is acceptable
// e.g. y_object_empty.json -> should pass.

#include <libgen.h>
#include <cstddef>
#include <format>
#include <fstream>
#include <iostream>
#include <sstream>

#include "../rtb-json.h"

void error(std::string const& msg, int code = 1) {
    std::cout << "error: " << msg << std::endl;
    exit(code);
}

std::string read_file(char const *path) {
    std::ifstream file_stream(path);
    if (!file_stream) { error(std::format("failed to open file '{}'", path)); }
    std::stringstream str_stream;
    str_stream << file_stream.rdbuf();
    return str_stream.str();
}

int main(int argc, char **argv) {
    for (size_t i = 1; i < argc; ++i) {
        char *path = *(argv + i);
        char const *base = basename(path);

        int targ;
        if      (*base == 'y') targ =  1;
        else if (*base == 'n') targ = -1;
        else if (*base == 'i') targ =  0;
        else error(std::format("invalid test case: {}", base));

        std::string json = read_file(path);
        bool ret = json_parse(json.c_str());
        bool pass = false;
        if      (targ == 0)        pass = true;
        else if (targ > 0 && ret)  pass = true;
        else if (targ < 0 && !ret) pass = true;

        std::cout << "case: " << base << ' '
            << (pass ? "PASS" : "FAIL") << std::endl;
    }
    return 0;
}
