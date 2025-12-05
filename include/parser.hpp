#pragma once 

#include "scanner.hpp"
#include <exception>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace cblang::parser {
    struct Templated {
        Templated(
            scanner::Token p_name,
            std::vector<std::shared_ptr<Templated>> p_templates
        ) : name(std::move(p_name)), templates(std::move(p_templates)) {}

        scanner::Token name;
        std::vector<std::shared_ptr<Templated>> templates;
    };

    using TypeName = std::pair<std::shared_ptr<Templated>, scanner::Token>;
    using Parameters = std::vector<TypeName>;

    struct Statement {
        virtual ~Statement();
    };

    struct Expr : Statement {

    };

    struct SetVar : Statement {
        SetVar(scanner::Token p_name, std::shared_ptr<Expr> p_val) : name(std::move(p_name)), val(std::move(p_val)) {}

        scanner::Token name;
        std::shared_ptr<Expr> val;
    };

    struct CreateVar : Statement {
        CreateVar(TypeName p_type_name, std::shared_ptr<Expr> p_val) : type_name(std::move(p_type_name)), val(std::move(p_val)) {}

        TypeName type_name;
        std::shared_ptr<Expr> val;
    };

    struct Return : Statement {
        Return(std::shared_ptr<Expr> expr) : return_expression(std::move(expr)) {}

        std::shared_ptr<Expr> return_expression;
    };

    struct Binary : Expr {
        Binary(std::shared_ptr<Expr> p_left, scanner::Token p_op, std::shared_ptr<Expr> p_right) : left(std::move(p_left)), op(std::move(p_op)), right(std::move(p_right)) {}

        std::shared_ptr<Expr> left;
        scanner::Token op;
        std::shared_ptr<Expr> right;
    };

    struct Logical : Expr {
        Logical(std::shared_ptr<Expr> p_left, scanner::Token p_op, std::shared_ptr<Expr> p_right) : left(std::move(p_left)), op(std::move(p_op)), right(std::move(p_right)) {}

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

    struct ScopeExpr : Expr {
        ScopeExpr(std::vector<std::shared_ptr<Statement>> p_statements) : statements(std::move(p_statements)) {}

        std::vector<std::shared_ptr<Statement>> statements;
    };

    struct ArrayExpr : Expr {
        ArrayExpr(std::vector<std::shared_ptr<Expr>> p_items) : items(std::move(p_items)) {}

        std::vector<std::shared_ptr<Expr>> items;
    };

    struct Accessible : Expr {
        Accessible(std::optional<std::shared_ptr<Accessible>> p_access) : access(std::move(p_access)) {}

        std::optional<std::shared_ptr<Accessible>> access;
    };

    struct CallExpr : Accessible {
        CallExpr(
            std::optional<std::shared_ptr<Accessible>> p_access, 
            std::shared_ptr<Templated> p_name, 
            std::vector<std::shared_ptr<Expr>> p_args
        ) : Accessible(std::move(p_access)), name(std::move(p_name)), args(std::move(p_args)) {}

        std::shared_ptr<Templated> name;
        std::vector<std::shared_ptr<Expr>> args;
    };

    struct VarExpr : Accessible {
        VarExpr(std::optional<std::shared_ptr<Accessible>> p_access, scanner::Token p_name) : Accessible(std::move(p_access)), name(std::move(p_name)) {}

        scanner::Token name;
    };

    struct Declaration : Statement {

    };

    struct Function : Declaration {
        Function(
            std::shared_ptr<Templated> p_name, 
            Parameters p_params, 
            std::optional<scanner::Token> p_returns,
            std::vector<std::shared_ptr<Statement>> p_body
        ) : templated_name(std::move(p_name)), params(std::move(p_params)), returns(std::move(p_returns)), body(std::move(p_body)) {}

        std::shared_ptr<Templated> templated_name;
        Parameters params;
        std::optional<scanner::Token> returns;
        std::vector<std::shared_ptr<Statement>> body;
    };

    struct Variable : Declaration {
        Variable(
            std::shared_ptr<Templated> p_type,
            scanner::Token p_name,
            std::optional<std::shared_ptr<Expr>> p_value
        ) : type(std::move(p_type)), name(std::move(p_name)), value(std::move(p_value)) {}

        std::shared_ptr<Templated> type;
        scanner::Token name;
        std::optional<std::shared_ptr<Expr>> value;
    };

    struct Class : Declaration {
        Class(
            std::shared_ptr<Templated> p_name,
            std::vector<std::shared_ptr<Templated>> p_inherits,
            Parameters p_params,
            std::vector<std::shared_ptr<Declaration>> p_members
        ) : name(std::move(p_name)), inherits(std::move(p_inherits)), params(std::move(p_params)), members(std::move(p_members)) {}

        std::shared_ptr<Templated> name;
        std::vector<std::shared_ptr<Templated>> inherits;
        Parameters params;
        std::vector<std::shared_ptr<Declaration>> members;
    };

    struct ParsedProgram {
        ParsedProgram(
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

            auto parse() -> std::optional<std::shared_ptr<ParsedProgram>>;
        
        private:
            std::vector<scanner::Token> tokens;
            int current = 0;

            auto match(const std::vector<scanner::TokenType>& types) -> bool;
            auto advance() -> scanner::Token;
            auto consume(const scanner::TokenType& type, const std::string& message) -> scanner::Token;
            auto synchronize() -> void;
            auto program() -> std::shared_ptr<ParsedProgram>;
            auto templated(const std::string& scope, const bool& definition = false) -> std::shared_ptr<Templated>;
            auto function() -> std::shared_ptr<Function>;
            auto class_decl() -> std::shared_ptr<Class>;
            auto variable() -> std::shared_ptr<Variable>;
            auto parameters(bool optional = true) -> Parameters;
            auto members() -> std::vector<std::shared_ptr<Declaration>>;
            auto scope() -> std::vector<std::shared_ptr<Statement>>;
            auto statement() -> std::shared_ptr<Statement>;
            auto expression() -> std::shared_ptr<Expr>;
            auto logic_or() -> std::shared_ptr<Expr>;
            auto logic_and() -> std::shared_ptr<Expr>;
            auto equality() -> std::shared_ptr<Expr>;
            auto comparison() -> std::shared_ptr<Expr>;
            auto term() -> std::shared_ptr<Expr>;
            auto factor() -> std::shared_ptr<Expr>;
            auto unary() -> std::shared_ptr<Expr>;
            auto primary() -> std::shared_ptr<Expr>;
            auto function_or_variable() -> std::shared_ptr<Accessible>;
            [[nodiscard]] auto check(const scanner::TokenType& type, const int& ahead = 1) const -> bool;
            [[nodiscard]] auto is_at_end(const int& ahead = 1) const -> bool;
            [[nodiscard]] auto peek(const int& ahead = 1) const -> scanner::Token;
            [[nodiscard]] auto previous() const -> scanner::Token;
    };

    auto init(bool verbose = false) -> void;
    auto enable_verbose_logs() -> void;
    auto debug_templates(const std::shared_ptr<Templated>& decl, const int& tabs = 0, const bool& one_line = true) -> std::string;
    auto debug_program(const std::shared_ptr<ParsedProgram>& program) -> std::string;
    auto debug_members(const std::vector<std::shared_ptr<Declaration>>& decls, const int& tabs = 0) -> std::string;
}