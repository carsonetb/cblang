#pragma once 

#include "scanner.h"
#include <exception>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace cblang::parser {
    using Parameters = std::vector<std::pair<scanner::Token, scanner::Token>>;

    struct Expr {
        virtual ~Expr();
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

    struct Statement {

    };

    struct Declaration {
        virtual ~Declaration();
    };

    struct Function : Declaration {
        Function(
            scanner::Token p_name, 
            Parameters p_params, 
            std::optional<scanner::Token> p_returns,
            std::vector<std::shared_ptr<Statement>> p_body
        ) : name(std::move(p_name)), params(std::move(p_params)), returns(std::move(p_returns)), body(std::move(p_body)) {}

        scanner::Token name;
        Parameters params;
        std::optional<scanner::Token> returns;
        std::vector<std::shared_ptr<Statement>> body;
    };

    struct Variable : Declaration {
        Variable(
            scanner::Token p_type,
            scanner::Token p_name,
            std::optional<std::shared_ptr<Expr>> p_value
        ) : type(std::move(p_type)), name(std::move(p_name)), value(std::move(p_value)) {}

        scanner::Token type;
        scanner::Token name;
        std::optional<std::shared_ptr<Expr>> value;
    };

    struct Class : Declaration {
        Class(
            scanner::Token p_name,
            std::vector<scanner::Token> p_inherits,
            Parameters p_params,
            std::vector<std::shared_ptr<Declaration>> p_members
        ) : name(std::move(p_name)), inherits(std::move(p_inherits)), params(std::move(p_params)), members(std::move(p_members)) {}

        scanner::Token name;
        std::vector<scanner::Token> inherits;
        Parameters params;
        std::vector<std::shared_ptr<Declaration>> members;
    };

    struct Program {
        Program(
            Parameters p_params, 
            std::vector<std::shared_ptr<Declaration>> p_members
        ) : parameters(std::move(p_params)), members(std::move(p_members)) {}

        Parameters parameters;
        std::vector<std::shared_ptr<Declaration>> members;
    };
    
    class ParseException : public std::exception {};

    class Parser {
        public:
            Parser(std::vector<scanner::Token> p_tokens);

            auto parse() -> std::optional<std::shared_ptr<Program>>;
        
        private:

            std::vector<scanner::Token> tokens;
            int current = 0;

            auto match(const std::vector<scanner::TokenType>& types) -> bool;
            auto advance() -> scanner::Token;
            auto consume(const scanner::TokenType& type, const std::string& message) -> scanner::Token;
            auto synchronize() -> void;
            auto program() -> std::shared_ptr<Program>;
            auto function() -> std::shared_ptr<Function>;
            auto class_decl() -> std::shared_ptr<Class>;
            auto variable() -> std::shared_ptr<Variable>;
            auto parameters(bool optional = true) -> Parameters;
            auto members() -> std::vector<std::shared_ptr<Declaration>>;
            auto scope() -> std::vector<std::shared_ptr<Statement>>;
            auto statement() -> std::shared_ptr<Statement>;
            auto expression() -> std::shared_ptr<Expr>;
            auto equality() -> std::shared_ptr<Expr>;
            auto comparison() -> std::shared_ptr<Expr>;
            auto term() -> std::shared_ptr<Expr>;
            auto factor() -> std::shared_ptr<Expr>;
            auto unary() -> std::shared_ptr<Expr>;
            auto primary() -> std::shared_ptr<Expr>;
            [[nodiscard]] auto check(const scanner::TokenType& type) const -> bool;
            [[nodiscard]] auto is_at_end() const -> bool;
            [[nodiscard]] auto peek() const -> scanner::Token;
            [[nodiscard]] auto previous() const -> scanner::Token;
    };

    auto init(bool verbose = false) -> void;
    auto enable_verbose_logs() -> void;
    auto debug_program(const std::shared_ptr<Program>& program) -> std::string;
    auto debug_members(const std::vector<std::shared_ptr<Declaration>>& decls, const int& tabs = 0) -> std::string;
}