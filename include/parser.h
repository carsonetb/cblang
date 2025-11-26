#pragma once 

#include "cblang.h"
#include "scanner.h"
#include <exception>
#include <memory>
#include <utility>
#include <vector>

namespace cblang::parser {
    struct Expr {

    };

    struct Binary : Expr {
        Binary(std::shared_ptr<Expr> p_left, scanner::Token p_op, std::shared_ptr<Expr> p_right) : left(std::move(p_left)), op(std::move(p_op)), right(std::move(p_right)) {}

        std::shared_ptr<Expr> left;
        scanner::Token op;
        std::shared_ptr<Expr> right;
    };

    struct Grouping : Expr {
        Grouping(std::shared_ptr<Expr> p_expression) : expression(std::move(p_expression)) {}

        std::shared_ptr<Expr> expression;
    };

    struct Literal : Expr {
        Literal(std::shared_ptr<scanner::Literal> p_literal) : literal(std::move(p_literal)) {}

        std::shared_ptr<scanner::Literal> literal;
    };

    struct Unary : Expr {
        Unary(scanner::Token p_op, std::shared_ptr<Expr> p_right) : op(std::move(p_op)), right(std::move(p_right)) {}

        scanner::Token op;
        std::shared_ptr<Expr> right;
    };

    class Parser {
        public:
            Parser(std::vector<scanner::Token> p_tokens);
        
        private:
            class ParseException : public std::exception {};

            std::vector<scanner::Token> tokens;
            int current = 0;

            auto match(const std::vector<scanner::TokenType>& types) -> bool;
            auto advance() -> scanner::Token;
            auto consume(const scanner::TokenType& type, const std::string& message) -> scanner::Token;
            auto expression() -> std::shared_ptr<Expr>;
            auto equality() -> std::shared_ptr<Expr>;
            auto comparison() -> std::shared_ptr<Expr>;
            auto term() -> std::shared_ptr<Expr>;
            auto factor() -> std::shared_ptr<Expr>;
            auto unary() -> std::shared_ptr<Expr>;
            auto primary() -> std::shared_ptr<Expr>;
            [[nodiscard]] auto handle_error(const scanner::Token& token, const std::string& message) const -> ParseException;
            [[nodiscard]] auto check(const scanner::TokenType& type) const -> bool;
            [[nodiscard]] auto is_at_end() const -> bool;
            [[nodiscard]] auto peek() const -> scanner::Token;
            [[nodiscard]] auto previous() const -> scanner::Token;
    };
}