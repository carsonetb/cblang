#include "cblang.h"

auto main() -> int {
    cblang::init(true);
    cblang::cblang_parse_code("class Main (Alpha beta, Alpha cheta) = (X y = 1, scope function = {}, class Sub : Foo (Ahpla etab) = (X y = 2)) ");
    return 0;
}