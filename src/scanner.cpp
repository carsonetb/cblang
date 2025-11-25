#include "scanner.h"
#include <memory>
#include <vector>

using namespace cblang::scanner;

cblang::scanner::Scanner::Scanner(std::string p_source) : source(std::move(p_source)) {}

auto cblang::scanner::Scanner::scan_tokens() -> std::vector<Token> {
    while (!is_at_end()) {
        start = current;
        scan_token();
    }

    tokens.emplace_back(TokenType::END_OF_FILE, "", line);
    return tokens;
}

auto cblang::scanner::Scanner::scan_token() -> void {
    char character = advance();
    switch (character) {
        case '(': add_token(LEFT_PAREN); break;
        case ')'
    }
}

auto cblang::scanner::Scanner::advance() -> char {
    current++;
    return source.at(current);
}

auto cblang::scanner::Scanner::add_token(TokenType type, const std::shared_ptr<Literal>& literal) -> void {
    std::string text = source.substr(start, current);
    tokens.emplace_back(type, text, line, literal);
}

auto cblang::scanner::Scanner::is_at_end() const -> bool {
    return current >= source.size();
}
