#include "cblang.h"

auto main() -> int {
    cblang::init(true);
    cblang::cblang_parse_code("class Name : Test (Alpha<x> beta, Alpha beta) = (Alpha beta, scope test(Alpha beta, Alpha beta) -> x = {Alpha beta = \"cinturon\"})");
    return 0;
} 