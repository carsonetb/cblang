#include "cblang.hpp"
#include <fstream>
#include <iostream>
#include <span>
#include <sstream>

auto main(int argc, char *argv[]) -> int {
    auto args = std::span(argv, size_t(argc));

    if (argc < 2) {
        std::cerr << "Expected file to parse!\n";
        return 1;
    }

    std::ifstream file(args[1]);

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