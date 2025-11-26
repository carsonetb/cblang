#include "parser.h"

#include "scanner.h"
#include <memory>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <utility>
#include <vector>

using namespace cblang::scanner;

static bool initialized = false;
static std::shared_ptr<spdlog::logger> logger = spdlog::stderr_color_mt("cblang::parser");

static auto error(const Token& token, const std::string& message) -> void {
    if (token.type == END_OF_FILE) {
        logger->error("[line " + std::to_string(token.line) + "] [EOF] " + message);
    }
    else {
        logger->error("[line " + std::to_string(token.line) + "] [token '" + token.raw + "] " + message);
    }
}

cblang::parser::Parser::Parser(std::vector<scanner::Token> p_tokens) : tokens(std::move(p_tokens)) {
    
}

auto cblang::parser::Parser::match(const std::vector<scanner::TokenType>& types) -> bool {
    for (const scanner::TokenType& type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

auto cblang::parser::Parser::advance() -> scanner::Token {
    if (!is_at_end()) {
        current++;
    }
    return previous();
}

auto cblang::parser::Parser::consume(const scanner::TokenType& type, const std::string& message) -> scanner::Token {
    if (check(type)) {
        return advance();
    }
    
    error(peek(), message);
}

auto cblang::parser::Parser::expression() -> std::shared_ptr<Expr> {
    return equality();
}

auto cblang::parser::Parser::equality() -> std::shared_ptr<Expr> {
    auto expr = comparison();

    while (match({scanner::BANG_EQUAL, scanner::EQUAL_EQUAL})) {
        scanner::Token oper = previous();
        auto right = comparison();
        expr = std::make_shared<Binary>(expr, oper, right);
    }

    return expr;
}

auto cblang::parser::Parser::comparison() -> std::shared_ptr<Expr> {
    auto expr = term();
    
    while (match({LEFT_ANGLE, LESS_EQUAL, RIGHT_ANGLE, GREATER_EQUAL})) {
        Token oper = previous();
        auto right = term();
        expr = std::make_shared<Binary>(expr, oper, right);
    }

    return expr;
}

auto cblang::parser::Parser::factor() -> std::shared_ptr<Expr> {
    auto expr = unary();

    while (match({SLASH, STAR})) {
        Token oper = previous();
        auto right = unary();
        expr = std::make_shared<Binary>(expr, oper, right);
    }

    return expr;
}

auto cblang::parser::Parser::unary() -> std::shared_ptr<Expr> {
    if (match({BANG, MINUS})) {
        Token oper = previous();
        auto right = unary();
        return std::make_shared<Unary>(oper, right);
    }

    return primary();
}

auto cblang::parser::Parser::primary() -> std::shared_ptr<Expr> {
    if (match({FALSE_KW})) {
        return std::make_shared<Literal>(create_literal(true));
    }
    if (match({TRUE_KW})) {
        return std::make_shared<Literal>(create_literal(false));
    }

    if (match({FLOAT, INT, STRING})) {
        return std::make_shared<Literal>(previous().literal);
    }

    if (match({LEFT_PAREN})) {
        auto expr = expression();
        consume(RIGHT_PAREN, "Expected ')' after expression.");
        return std::make_shared<Grouping>(expr);
    }

    error(peek(), "Expresion must be a literal or grouping expression.");
}

auto cblang::parser::Parser::handle_error(const Token& token, const std::string& message) const -> ParseException {
    error(token, message);
    return {};
}

auto cblang::parser::Parser::check(const scanner::TokenType& type) const -> bool {
    if (is_at_end()) {
        return false;
    }
    return peek().type == type;
}

auto cblang::parser::Parser::is_at_end() const -> bool {
    return peek().type == scanner::END_OF_FILE;
}

auto cblang::parser::Parser::peek() const -> scanner::Token {
    return tokens.at(current);
}

auto cblang::parser::Parser::previous() const -> scanner::Token {
    return tokens.at(current - 1);
}