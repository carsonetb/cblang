#include "cblang.h"

auto main() -> int {
    cblang::init(true);
    cblang::cblang_parse_code("class Main (Alpha<Temp> beta, Alpha cheta) = (X<Temp> y = 1, scope function<Templ>(X y, Foo bar) -> Foo = {}, class Sub<Temp2> : Foo<Temp2> (Ahpla etab) = (X y = 2)) ");
    return 0;
}