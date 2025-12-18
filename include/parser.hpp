#pragma once 

#include "scanner.hpp"
#include <exception>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace cblang::definitions {
    class TemplatedType;
}

namespace cblang::parser {
    class ElifStmnt;
    class ElseStmnt;

    struct Templated {
        static auto generate(scanner::Token p_name, std::vector<std::shared_ptr<Templated>> p_templates) -> std::shared_ptr<Templated> {
            return std::make_shared<Templated>(p_name, p_templates);
        }

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
        std::optional<std::shared_ptr<definitions::TemplatedType>> evaluates_to;
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
        Return(scanner::Token p_keyword, std::shared_ptr<Expr> expr) : ret_kw_point(std::move(p_keyword)), return_expression(std::move(expr)) {}

        scanner::Token ret_kw_point;
        std::shared_ptr<Expr> return_expression;
    };

    struct IfStmnt : Statement {
        IfStmnt(
            scanner::Token p_start,
            std::shared_ptr<Expr> check, 
            std::optional<std::shared_ptr<ElifStmnt>> p_elif_following, 
            std::optional<std::shared_ptr<ElseStmnt>> p_else_following,
            std::vector<std::shared_ptr<Statement>> run_if
        ) : start(std::move(p_start)), check_expression(std::move(check)), elif_following(std::move(p_elif_following)), else_following(std::move(p_else_following)), to_run(std::move(run_if)) {}

        scanner::Token start;
        std::shared_ptr<Expr> check_expression;
        std::optional<std::shared_ptr<ElifStmnt>> elif_following;
        std::optional<std::shared_ptr<ElseStmnt>> else_following;
        std::vector<std::shared_ptr<Statement>> to_run;
    };

    struct ElifStmnt : IfStmnt {
        ElifStmnt(
            scanner::Token p_start,
            std::shared_ptr<Expr> check, 
            std::optional<std::shared_ptr<ElifStmnt>> p_elif_following, 
            std::optional<std::shared_ptr<ElseStmnt>> p_else_following,
            std::vector<std::shared_ptr<Statement>> run_if
        ) : IfStmnt(std::move(p_start), std::move(check), std::move(p_elif_following), std::move(p_else_following), std::move(run_if)) {}
    };

    struct ElseStmnt : Statement {
        ElseStmnt(std::vector<std::shared_ptr<Statement>> run_else) : to_run(std::move(run_else)) {}

        std::vector<std::shared_ptr<Statement>> to_run;
    };

    struct WhileStmnt : Statement {
        WhileStmnt(
            scanner::Token p_start,
            std::shared_ptr<Expr> check, 
            std::vector<std::shared_ptr<Statement>> run_while
        ) : start(std::move(p_start)), check_expression(std::move(check)), to_run(std::move(run_while)) {}

        scanner::Token start;
        std::shared_ptr<Expr> check_expression;
        std::vector<std::shared_ptr<Statement>> to_run;
    };

    struct ForStmnt : Statement {
        ForStmnt(
            TypeName p_looper, 
            std::shared_ptr<Expr> p_looped, 
            std::vector<std::shared_ptr<Statement>> run_for
        ) : looper(std::move(p_looper)), looped(std::move(p_looped)), to_run(std::move(run_for)) {}

        TypeName looper;
        std::shared_ptr<Expr> looped;
        std::vector<std::shared_ptr<Statement>> to_run;
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
        Literal(std::shared_ptr<scanner::Literal> p_literal, scanner::Token p_token) : literal(std::move(p_literal)), token(std::move(p_token)) {
            token.literal = literal;
        }

        std::shared_ptr<scanner::Literal> literal;
        scanner::Token token;
    };

    struct Unary : Expr {
        Unary(scanner::Token p_op, std::shared_ptr<Expr> p_right) : op(std::move(p_op)), right(std::move(p_right)) {}

        scanner::Token op;
        std::shared_ptr<Expr> right;
    };

    struct ScopeExpr : Expr {
        ScopeExpr(scanner::Token p_declare_point, std::vector<std::shared_ptr<Statement>> p_statements) : declare_point(std::move(p_declare_point)), statements(std::move(p_statements)) {}

        scanner::Token declare_point;
        std::vector<std::shared_ptr<Statement>> statements;
    };

    struct ArrayExpr : Expr {
        ArrayExpr(scanner::Token p_start_point, std::vector<std::shared_ptr<Expr>> p_items) : start_point(std::move(p_start_point)), items(std::move(p_items)) {}

        scanner::Token start_point;
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
        ) : Accessible(std::move(p_access)), 
            name(std::move(p_name)), 
            args(std::move(p_args))
        {}

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
            std::optional<std::shared_ptr<Templated>> p_returns,
            std::vector<std::shared_ptr<Statement>> p_body,
            bool p_is_cast,
            bool p_is_operator,
            bool p_is_private,
            bool p_is_static,
            bool p_is_const
        ) : templated_name(std::move(p_name)), params(std::move(p_params)), returns(std::move(p_returns)), body(std::move(p_body)),
            is_cast(p_is_cast),
            is_operator(p_is_operator),
            is_private(p_is_private),
            is_static(p_is_static),
            is_const(p_is_const) 
        {}

        std::shared_ptr<Templated> templated_name;
        Parameters params;
        std::optional<std::shared_ptr<Templated>> returns;
        std::vector<std::shared_ptr<Statement>> body;

        bool is_cast;
        bool is_operator;
        bool is_private;
        bool is_static;
        bool is_const;
    };

    struct Variable : Declaration {
        Variable(
            std::shared_ptr<Templated> p_type,
            scanner::Token p_name,
            std::shared_ptr<Expr> p_value,
            bool p_is_private,
            bool p_is_static,
            bool p_is_const
        ) : type(std::move(p_type)), name(std::move(p_name)), value(std::move(p_value)),
            is_private(p_is_private),
            is_static(p_is_static),
            is_const(p_is_const) 
        {}

        std::shared_ptr<Templated> type;
        scanner::Token name;
        std::shared_ptr<Expr> value;

        bool is_private;
        bool is_static;
        bool is_const;
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
            bool invalid = false;

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
            auto if_stmnt() -> std::shared_ptr<IfStmnt>;
            auto elif_stmnt() -> std::shared_ptr<ElifStmnt>;
            auto else_stmnt() -> std::shared_ptr<ElseStmnt>;
            auto while_stmnt() -> std::shared_ptr<WhileStmnt>;
            auto for_stmnt() -> std::shared_ptr<ForStmnt>;
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