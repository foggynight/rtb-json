//------------------------------------------------------------------------------
// --- USAGE ---
// run_test MODE ...
//
// run_test input INPUT
//   Execute JSON parsing function on INPUT string, print result.
//
// run_test cases [FILE...]
//   Execute JSON parsing function on the contents of each FILE and print
//   results. Desired case result is determined by filename prefix.
//     y -> success
//     n -> failure
//     i -> either is acceptable
//   e.g. y_object_empty.json -> should pass.
//------------------------------------------------------------------------------

#include <libgen.h>
#include <cstddef>
#include <cstring>
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

void test_input(char const *input) {
    JSON *json = JSON_Parse(input);
    std::cout << (json ? "PASS" : "FAIL") << std::endl;
    if (json) JSON_Delete(json);
}

void test_cases(int argc, char **argv) {
    for (size_t i = 2; i < argc; ++i) {
        char *path = *(argv + i);
        char const *filename = basename(path);

        int targ;
        if      (*filename == 'y') targ =  1;
        else if (*filename == 'n') targ = -1;
        else if (*filename == 'i') targ =  0;
        else error(std::format("invalid test case: {}", filename));

        std::string str = read_file(path);
        JSON *json = JSON_Parse(str.c_str());
        bool pass = false;
        if      (targ == 0)                pass = true;
        else if (targ > 0 && json != NULL) pass = true;
        else if (targ < 0 && json == NULL) pass = true;

        std::cout << "case: " << filename << ' '
            << (pass ? "PASS" : "FAIL") << std::endl;
    }
}

int main(int argc, char **argv) {
    if (argc < 3) error("missing arguments");
    if (!strcmp(argv[1], "input"))
        test_input(argv[2]);
    else if (!strcmp(argv[1], "cases"))
        test_cases(argc, argv);
    else std::cout << "error: invalid MODE argument: " << argv[1] << std::endl;
    return 0;
}
