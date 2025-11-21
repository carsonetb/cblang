#include "cblang.h"

auto main() -> int {
    cblang::init(true);
    cblang::cblang_parse_code("class Name : Test (Alpha beta, Alpha beta)");
    return 0;
} 