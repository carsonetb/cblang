#include "cblang.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

auto main(int argc, char *argv[]) -> int {
    if (argc < 2) {
        std::cerr << "Expected file to parse!\n";
        return 1;
    }

    std::ifstream file(argv[1]);

    if (!file.is_open()) {
        std::cerr << "Error opening file!\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    cblang::init(true);
    cblang::cblang_parse_code(buffer.str());
    return 0;
}