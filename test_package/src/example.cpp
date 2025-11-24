#include "cblang.h"

auto main() -> int {
    cblang::init(true);
    cblang::cblang_parse_code("class Main (array<char> alpha, char beta) = (Alpha beta, scope test(Alpha beta, Alpha beta) -> x = {Alpha beta = Alpha;})");
    return 0;
}