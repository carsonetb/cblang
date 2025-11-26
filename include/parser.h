#pragma once 

#include "scanner.h"
#include <memory>
#include <utility>
#include <vector>

namespace cblang::parser {
    struct Expr {

    };

    struct Binary : Expr {
        Binary(Expr p_left, scanner::Token p_op, Expr p_right) : left(p_left), op(std::move(p_op)), right(p_right) {}

        Expr left;
        scanner::Token op;
        Expr right;
    };

    struct Grouping : Expr {
        Grouping(Expr p_expression) : expression(p_expression) {}

        Expr expression;
    };

    struct Literal : Expr {
        std::shared_ptr<parser::Literal> literal;
    };

    struct Unary : Expr {
        Unary(scanner::Token p_op, Expr p_right) : op(std::move(p_op)), right(p_right) {}

        scanner::Token op;
        Expr right;
    };

    class Parser {
        public:
            Parser(std::vector<scanner::Token> p_tokens);
        
        private:
            std::vector<scanner::Token> tokens;
            int current = 0;

            auto match(std::vector<scanner::TokenType> types) -> bool;
            auto check(scanner::TokenType type) -> bool;
            auto advance() -> scanner::Token;
            auto expression() -> Expr;
            auto equality() -> Expr;
            auto comparison() -> Expr;
            auto term() -> Expr;
            auto factor() -> Expr;
            auto primary() -> Expr;
            [[nodiscard]] auto is_at_end() const -> bool;
            [[nodiscard]] auto peek() const -> scanner::Token;
            [[nodiscard]] auto previous() const -> scanner::Token;
    };
}