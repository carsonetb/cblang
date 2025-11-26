#include "parser.h"

#include "scanner.h"
#include <utility>
#include <vector>

cblang::parser::Parser::Parser(std::vector<scanner::Token> p_tokens) : tokens(std::move(p_tokens)) {
    
}

