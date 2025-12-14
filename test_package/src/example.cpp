#include "cblang.hpp"
#include "definitions.hpp"
#include "objects.hpp"
#include "program.hpp"
#include "scanner.hpp"
#include <fstream>
#include <iostream>
#include <span>
#include <sstream>

auto main(int argc, char *argv[]) -> int {
    auto args = std::span(argv, size_t(argc));

    if (argc < 2) {
        std::cerr << "Expected file to parse!\n";
        return 0;
    }

    std::ifstream file(args[1]);

    if (!file.is_open()) {
        std::cerr << "Error opening file!\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    cblang::init(true);
    auto out = cblang::cblang_parse_code(buffer.str());
    if (out) {
        auto func = out.value()->main_class->get_functions()[0];
        auto obj = out.value()->create_object({std::make_shared<cblang::objects::IntObject>(5), std::make_shared<cblang::objects::StringObject>("input")});
        auto ret = obj->call(func->name.raw, cblang::scanner::Token(cblang::scanner::IDENTIFIER, "begin", -1), {}, {});
        if (ret) {
            auto as_string = std::dynamic_pointer_cast<cblang::objects::StringObject>(ret.value());
            std::cout << as_string->value << "\n";
        }
    }

    return 0;
}