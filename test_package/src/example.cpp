#include "cblang.hpp"
#include "definitions.hpp"
#include "objects.hpp"
#include "program.hpp"
#include "scanner.hpp"
#include "standard.hpp"
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
    auto static_stdlib = cblang::standard::get_static_stdlib();
    auto out = cblang::cblang_parse_code(buffer.str(), static_stdlib);
    if (!out.has_value()) {
        return 1;
    }
    if (out) {
        auto obj = out.value()->create_object({std::make_shared<cblang::objects::IntObject>(5), std::make_shared<cblang::objects::StringObject>("input")});
        if (!obj) {
            return 1;
        }
        auto ret = obj.value()->call("fibbonacci", cblang::scanner::Token::create_external("<ext>"), {}, {std::make_shared<cblang::objects::IntObject>(5)}, cblang::standard::get_stdlib_scope(static_stdlib));
        if (ret) {
            auto as_string = std::dynamic_pointer_cast<cblang::objects::IntObject>(ret.value());
            std::cout << as_string->value << "\n";
        }
    }

    return 0;
}