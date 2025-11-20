#include "cblang.h"

auto main() -> int {
    cblang::init(true);
    cblang::cblang_parse_code("class ");
    return 0;
} 