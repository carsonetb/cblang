#include "parser.hpp"

#include "scanner.hpp"
#include "util.hpp"
#include <memory>
#include <optional>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <string>
#include <utility>
#include <vector>

#define __TABBING repeat_string("\u2502   ", tabs) + "\u251C\u2500\u2500\u2500" 
#define NEWLINE (!out.empty() ? (out.back() == '\n' ? '\0' : '\n') : '\n')

using namespace cblang::scanner;
using namespace cblang::parser;

static bool initialized = false;
static std::shared_ptr<spdlog::logger> logger = spdlog::stderr_color_mt("cblang::parser");

static auto error(const Token& token, const std::string& message) -> void {
    if (token.type == END_OF_FILE) {
        logger->error("[line " + std::to_string(token.line) + "] [EOF] " + message);
    }
    else {
        logger->error("[line " + std::to_string(token.line) + "] [token '" + token.raw + "'] " + message);
    }
}

static auto handle_error(const Token& token, const std::string& message) -> ParseException {
    error(token, message);
    return {};
}

cblang::parser::Statement::~Statement() = default;

cblang::parser::Parser::Parser(std::vector<scanner::Token> p_tokens) : tokens(std::move(p_tokens)) {
    
}

auto cblang::parser::Parser::parse() -> std::optional<std::shared_ptr<ParsedProgram>> {
    logger->debug("Parser started.");
    try {
        auto out = program();
        if (invalid) {
            return {};
        }
        return out;
    }
    catch (ParseException exception) {
        return {};
    } 
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
    
    throw handle_error(peek(), message);
}

auto cblang::parser::Parser::synchronize() -> void {
    advance();

    while (!is_at_end()) {
        if (previous().type == SEMICOLON) {
            return;
        }

        switch (peek().type) {
            case CLASS_KW:
            case SCOPE_KW:
            case PRIVATE_KW:
            case STATIC_KW:
            case CONST_KW:
            case OPERATOR_KW:
            case CAST_KW:
            case RIGHT_CURLY:
                return;
            default:
                break;
        }

        advance();
    }
}

auto cblang::parser::Parser::program() -> std::shared_ptr<ParsedProgram> {
    consume(CLASS_KW, "Program must start with the 'class' keyword (for Main class definition.)");
    auto name = consume(IDENTIFIER, "Expected 'Main' identifier after 'class' keyword.");
    if (name.raw != "Main") { throw handle_error(name, "First class must be named 'Main'."); }
    auto params = parameters();
    consume(EQUAL, "Expected '=' between parameters and members.");
    auto mems = members();
    return std::make_shared<ParsedProgram>(params, mems);
}

auto cblang::parser::Parser::templated(const std::string& scope, const bool& definition) -> std::shared_ptr<Templated> {
    auto name = consume(IDENTIFIER, "Expected a class name after " + scope + ".");
    std::vector<std::shared_ptr<Templated>> templates;
    if (match({LEFT_ANGLE})) {
        while (true) {
            templates.push_back(templated("template scope seperator", definition));
            if (!match({COMMA})) {
                break;
            }
        }
        consume(RIGHT_ANGLE, "Expected '>' after template definition.");
    }
    return std::make_shared<Templated>(name, templates);
}

auto cblang::parser::Parser::function() -> std::shared_ptr<Function> {
    bool is_cast = false;
    bool is_operator = false;
    bool is_private = false;
    bool is_static = false;
    bool is_const = false;

    // Kinda silly
    if (match({CAST_KW})) { is_cast = true; }
    if (match({OPERATOR_KW})) { is_operator = true; }
    if (!is_cast && !is_operator) {
        if (match({PRIVATE_KW})) { is_private = true; }
        if (match({STATIC_KW})) { is_static = true; }
        if (match({PRIVATE_KW}) && !is_private) { is_private = true; }
        if (match({CONST_KW}) && !is_static) { is_const = true; }
        if (match({PRIVATE_KW}) && !is_private) { is_private = true; }
    } 

    consume(SCOPE_KW, "Expected 'scope' keyword.");

    auto name = templated("'scope' keyword", true);
    auto params = parameters();
    std::optional<std::shared_ptr<Templated>> returns;
    if (match({RETURN})) {
        returns = templated("function return");
    }
    consume(EQUAL, "Expected '=' between function declaration and code.");
    consume(LEFT_CURLY, "Expected '{' after '='");
    auto body = scope();
    return std::make_shared<Function>(name, params, returns, body, is_cast, is_operator, is_private, is_static, is_const);
}

auto cblang::parser::Parser::class_decl() -> std::shared_ptr<Class> {
    auto name = templated("'class' keyword", true);
    std::vector<std::shared_ptr<Templated>> inherits;
    if (match({COLON})) {
        while (true) {
            inherits.push_back(templated("inheritor scope"));
            if (!check(COMMA)) {
                break;
            }
        }
    }
    auto params = parameters();
    consume(EQUAL, "Expected '=' between class declaration and members.");
    auto mems = members();
    return std::make_shared<Class>(name, inherits, params, mems);
}

auto cblang::parser::Parser::variable() -> std::shared_ptr<Variable> {
    bool is_private = false;
    bool is_static = false;
    bool is_const = false;
    if (match({PRIVATE_KW})) { is_private = true; }
    if (match({STATIC_KW})) { is_static = true; }
    if (match({PRIVATE_KW}) && !is_private) { is_private = true; }
    if (match({CONST_KW}) && !is_static) { is_const = true; }
    if (match({PRIVATE_KW}) && !is_private) { is_private = true; }

    auto type = templated("variable type");
    auto name = consume(IDENTIFIER, "Expected variable name.");
    consume(EQUAL, "Expected '=' to denote variable definition.");
    auto expr = expression();
    return std::make_shared<Variable>(type, name, expr, is_private, is_static, is_const);
}

auto cblang::parser::Parser::parameters(bool optional) -> Parameters {
    std::vector<std::pair<std::shared_ptr<Templated>, Token>> out;
    if (!check(LEFT_PAREN) && optional) {
        return {};
    }
    consume(LEFT_PAREN, "Expected '('");
    if (!check(RIGHT_PAREN)) {
        while (true) {
            auto type = templated("parameter scope");
            auto name = consume(IDENTIFIER, "Expected parameter name.");
            out.emplace_back(type, name);
            if (!match({COMMA})) {
                break;
            }
        }
    }
    consume(RIGHT_PAREN, "Expected ')' or ',' after a parameter.");
    return out;
}

auto cblang::parser::Parser::members() -> std::vector<std::shared_ptr<Declaration>> {
    std::vector<std::shared_ptr<Declaration>> out;
    consume(LEFT_PAREN, "Expected '(' to begin a member scope.");
    if (match({RIGHT_PAREN})) {
        return {};
    }
    while (true) {
        bool scope_ahead = check(SCOPE_KW, 1) || check(SCOPE_KW, 2) || check(SCOPE_KW, 3);
        std::shared_ptr<Declaration> decl;
        if (!scope_ahead && (check(IDENTIFIER) || check(PRIVATE_KW) || check(STATIC_KW) || check(CONST_KW))) {
            decl = variable();
        }
        else if (scope_ahead && (check(SCOPE_KW) || check(PRIVATE_KW) ||
                                 check(STATIC_KW) || check(CONST_KW) ||
                                 check(CAST_KW) || check(OPERATOR_KW))) 
        {
            decl = function();
        }
        else if (match({CLASS_KW})) {
            decl = class_decl();
        }
        else {
            throw handle_error(peek(), "Expected a declaration after ',' (variable, scope, or class).");
        }
        out.push_back(decl);
        if (!match({COMMA})) {
            break;
        }
    }
    consume(RIGHT_PAREN, "Expected ')' or ',' after a member.");
    return out;
}

auto cblang::parser::Parser::scope() -> std::vector<std::shared_ptr<Statement>> {
    std::vector<std::shared_ptr<Statement>> statements;

    while (!check(RIGHT_CURLY) && !is_at_end()) {
        try {
            if (match({SEMICOLON})) {
                continue;
            }
            statements.push_back(statement());
        }
        catch (ParseException exception) {
            invalid = true;
            synchronize();
        }
    }

    consume(RIGHT_CURLY, "Expected '}' after scope.");
    return statements;
}

auto cblang::parser::Parser::statement() -> std::shared_ptr<Statement> {
    try {
        if (check(IDENTIFIER) && check(EQUAL, 2)) {
            auto name = consume(IDENTIFIER, "");
            consume(EQUAL, "");
            auto expr = expression();
            consume(SEMICOLON, "Expected ';' after statement.");
            return std::make_shared<SetVar>(name, expr);
        }
        if (check(IDENTIFIER) && (check(IDENTIFIER, 2) || check(LEFT_ANGLE, 2))) {
            auto type = templated("create var");
            auto name = consume(IDENTIFIER, "Expected name after variable type.");
            consume(EQUAL, "Expected '=' after variable name");
            auto expr = expression();
            consume(SEMICOLON, "Expected ';' after statement.");
            return std::make_shared<CreateVar>(TypeName(type, name), expr);
        }
        if (match({LEFT_CURLY})) {
            return std::make_shared<ScopeExpr>(peek(), scope());
        }
        if (check(SCOPE_KW)) {
            return function();
        }
        if (match({RETURN_KW})) {
            auto out = std::make_shared<Return>(previous(), expression());
            consume(SEMICOLON, "Expected ';' after statement.");
            return out;
        }
        if (match({IF_KW})) {
            return if_stmnt();
        }
        if (match({WHILE_KW})) {
            return while_stmnt();
        }
        if (match({FOR_KW})) {
            return for_stmnt();
        }
        return expression();
    }
    catch (ParseException exception) {
        invalid = true;
        synchronize();
        return nullptr; // TODO: We don't like nullptrs.
    }
    // throw handle_error(peek(), "Expected expression, variable set, variable create, function create, scope init, or return.");
}

auto cblang::parser::Parser::if_stmnt() -> std::shared_ptr<IfStmnt> {
    scanner::Token start = consume(LEFT_PAREN, "Expected '(' after 'if' keyword.");
    auto expr = expression();
    consume(RIGHT_PAREN, "Expected ')' after expression in if statement.");
    consume(LEFT_CURLY, "Expected '{' after ')' in if statement.");
    auto scope_expr = scope();

    std::optional<std::shared_ptr<ElifStmnt>> elif_following;
    std::optional<std::shared_ptr<ElseStmnt>> else_following;
    if (match({ELIF_KW})) {
        elif_following = elif_stmnt();
    }
    else if (match({ELSE_KW})) {
        else_following = else_stmnt();
    }

    return std::make_shared<IfStmnt>(start, expr, elif_following, else_following, scope_expr);
}

auto cblang::parser::Parser::elif_stmnt() -> std::shared_ptr<ElifStmnt> {
    scanner::Token start = consume(LEFT_PAREN, "Expected '(' after 'elif' keyword.");
    auto expr = expression();
    consume(RIGHT_PAREN, "Expected ')' after expression in elif statement.");
    consume(LEFT_CURLY, "Expected '{' after ')' in elif statement.");
    auto scope_expr = scope();

    std::optional<std::shared_ptr<ElifStmnt>> elif_following;
    std::optional<std::shared_ptr<ElseStmnt>> else_following;
    if (match({ELIF_KW})) {
        elif_following = elif_stmnt();
    }
    else if (match({ELSE_KW})) {
        else_following = else_stmnt();
    }

    return std::make_shared<ElifStmnt>(start, expr, elif_following, else_following, scope_expr);
}

auto cblang::parser::Parser::else_stmnt() -> std::shared_ptr<ElseStmnt> {
    consume(LEFT_CURLY, "Expected '{' after 'else' keyword.");
    auto scope_expr = scope();
    return std::make_shared<ElseStmnt>(scope_expr);
}

auto cblang::parser::Parser::while_stmnt() -> std::shared_ptr<WhileStmnt> {
    scanner::Token start = consume(LEFT_PAREN, "Expected '(' after 'while' keyword.");
    auto expr = expression();
    consume(RIGHT_PAREN, "Expected ')' after expression in while statement.");
    consume(LEFT_CURLY, "Expected '{' after ')' in while statement.");
    auto scope_expr = scope();
    return std::make_shared<WhileStmnt>(start, expr, scope_expr);
}

auto cblang::parser::Parser::for_stmnt() -> std::shared_ptr<ForStmnt> {
    consume(LEFT_PAREN, "Expected '(' after 'for' keyword.");
    auto looper = TypeName(templated("for statement"), consume(IDENTIFIER, "Expected identifier after type of looper."));
    consume(IN_KW, "Expected 'in' keyword after type and name of looper.");
    auto looped = expression();
    consume(RIGHT_PAREN, "Expected ')' after expression.");
    consume(LEFT_CURLY, "Expected '{' after ')' in for statement.");
    auto scope_expr = scope();
    return std::make_shared<ForStmnt>(looper, looped, scope_expr);
}

auto cblang::parser::Parser::expression() -> std::shared_ptr<Expr> {
    return logic_or();
}

auto cblang::parser::Parser::logic_or() -> std::shared_ptr<Expr> {
    auto expr = logic_and();

    while (match({scanner::PIPE_PIPE})) {
        scanner::Token oper = previous();
        auto right = logic_and();
        expr = std::make_shared<Logical>(expr, oper, right);
    }

    return expr;
}

auto cblang::parser::Parser::logic_and() -> std::shared_ptr<Expr> {
    auto expr = equality();

    while (match({AND_AND})) {
        scanner::Token oper = previous();
        auto right = equality();
        expr = std::make_shared<Logical>(expr, oper, right);
    }

    return expr;
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

auto cblang::parser::Parser::term() -> std::shared_ptr<Expr> {
    auto expr = factor();

    while (match({PLUS, MINUS})) {
        Token oper = previous();
        auto right = factor();
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
    if (match({TRUE_KW})) {
        return std::make_shared<Literal>(create_literal(true), previous());
    }
    if (match({FALSE_KW})) {
        return std::make_shared<Literal>(create_literal(false), previous());
    }

    if (match({FLOAT, INT, STRING, CHARACTER})) {
        if (!previous().literal.has_value()) {
            throw handle_error(previous(), "(please report) reported type is a literal but has no literal value.");
        }
        return std::make_shared<Literal>(previous().literal.value(), previous());
    }

    if (check(IDENTIFIER)) {
        return function_or_variable();
    }

    if (match({LEFT_CURLY})) {
        return std::make_shared<ScopeExpr>(peek(), scope());
    }

    if (match({LEFT_PAREN})) {
        auto expr = expression();
        consume(RIGHT_PAREN, "Expected ')' after expression.");
        return std::make_shared<Grouping>(expr);
    }

    if (match({LEFT_BRACKET})) {
        auto start = previous();
        std::vector<std::shared_ptr<Expr>> items;
        if (!check(RIGHT_BRACKET)) {
            while (true) {
                items.push_back(expression());
                if (!match({COMMA})) {
                    break;
                }
            }
        }
        consume(RIGHT_BRACKET, "Expected ']' after array items.");
        return std::make_shared<ArrayExpr>(start, items);
    }

    throw handle_error(peek(), "Expected literal, identifier, '{', or '('.");
}

auto cblang::parser::Parser::function_or_variable() -> std::shared_ptr<Accessible> {
    if (check(LEFT_ANGLE, 2) || check(LEFT_PAREN, 2)) {
        auto name = templated("function call");
        std::vector<std::shared_ptr<Expr>> args;
        consume(LEFT_PAREN, "Expected '(' after templated function name.");
        if (!check(RIGHT_PAREN)) {
            while (true) {
                args.push_back(expression());
                if (!match({COMMA})) {
                    break;
                }
            }
        }
        consume(RIGHT_PAREN, "Expected ')' after parameter list.");
        std::optional<std::shared_ptr<Accessible>> access;
        if (match({DOT})) {
            access = function_or_variable();
        }
        return std::make_shared<CallExpr>(access, name, args);
    }
    auto name = consume(IDENTIFIER, "Expected variable name.");
    std::optional<std::shared_ptr<Accessible>> access;
    if (match({DOT})) {
        access = function_or_variable();
    }
    return std::make_shared<VarExpr>(access, name);
}

auto cblang::parser::Parser::check(const scanner::TokenType& type, const int& ahead) const -> bool {
    if (is_at_end()) {
        return false;
    }
    return peek(ahead).type == type;
}

auto cblang::parser::Parser::is_at_end(const int& ahead) const -> bool {
    return (current + ahead - 1 >= tokens.size() || peek(ahead).type == scanner::END_OF_FILE);
}

auto cblang::parser::Parser::peek(const int& ahead) const -> scanner::Token {
    if (current + ahead - 1 >= tokens.size()) {
        return tokens.back();
    }
    return tokens.at(current + ahead - 1);
}

auto cblang::parser::Parser::previous() const -> scanner::Token {
    return tokens.at(current - 1);
}

auto cblang::parser::init(bool verbose) -> void {
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [cblang::parser] %^[%l] %v %$");
    logger->set_level(spdlog::level::warn);

    initialized = true;

    if (verbose) {
        enable_verbose_logs();
    }

    logger->info("Initialization complete!");
}

auto cblang::parser::enable_verbose_logs() -> void {
    logger->set_level(spdlog::level::debug);
    logger->info("Verbose logs enabled.");
}

// TODO: Delete everything downwards (safely one of these functions is used for error handling)

auto debug_expression(const std::shared_ptr<Expr>& expr, const int& tabs) -> std::string {
    std::string out;
    auto as_binary = std::dynamic_pointer_cast<Binary>(expr);
    if (as_binary) {
        out += "Binary expression\n";
        out += __TABBING + "Left hand side: " + debug_expression(as_binary->left, tabs + 1); out += NEWLINE;
        out += __TABBING + "Operator: " + as_binary->op.raw; out += NEWLINE;
        out += __TABBING + "Right hand side: " + debug_expression(as_binary->right, tabs + 1); out += NEWLINE;
    }
    auto as_grouping = std::dynamic_pointer_cast<Grouping>(expr);
    if (as_grouping) {
        out += "Grouped expression\n";
        out += __TABBING + "Expression: " + debug_expression(as_grouping->expression, tabs + 1); out += NEWLINE;
    }
    auto as_literal = std::dynamic_pointer_cast<cblang::parser::Literal>(expr);
    if (as_literal) {
        out += "(literal) " + get_literal_string(as_literal);
    }
    auto as_unary = std::dynamic_pointer_cast<Unary>(expr);
    if (as_unary) {
        out += "Unary expression\n";
        out += __TABBING + "Operator: " + as_unary->op.raw; out += NEWLINE;
        out += __TABBING + "Expression: " + debug_expression(as_unary->right, tabs + 1); out += NEWLINE;
    }
    if (!out.empty()) {
        return out;
    }
    return "This type needs debugging!";
}

auto debug_parameters(const Parameters& params, const int& tabs = 0) -> std::string {
    std::string out;
    for (const auto& param : params) {
        out += __TABBING + "Parameter: " + debug_templates(param.first, tabs + 1, true) + " " + param.second.raw; out += NEWLINE;
    }
    return out;
}

auto debug_inherits(const std::vector<std::shared_ptr<Templated>>& types, const int& tabs = 0) -> std::string {
    std::string out;
    if (types.empty()) {
        return "(does not inherit)";
    }
    for (const auto& type : types) {
        out += "\n" + __TABBING + debug_templates(type, tabs + 1);
    }
    return out;
}

auto debug_member(const std::shared_ptr<Declaration>& decl, const int& tabs = 0) -> std::string {
    // std::string out;
    // auto as_var = std::dynamic_pointer_cast<Variable>(decl);
    // if (as_var) {
    //     out += "Variable declaration: \n";
    //     out += __TABBING + "Type: " + debug_templates(as_var->type, tabs + 1);
    //     out += __TABBING + "Name: " + as_var->name.raw; out += NEWLINE;
    //     if (as_var->value) {
    //         out += __TABBING + "Value: " + debug_expression(as_var->value, tabs + 1); out += NEWLINE;
    //     }
    //     else {
    //         out += __TABBING + "(no value set)\n";
    //     }
    // }
    // auto as_function = std::dynamic_pointer_cast<Function>(decl);
    // if (as_function) {
    //     out += "Function declaration: \n";
    //     out += __TABBING + "Name: " + debug_templates(as_function->templated_name, tabs + 1);
    //     out += __TABBING + "Parameters: \n" + debug_parameters(as_function->params, tabs + 1); out += NEWLINE;
    //     if (as_function->returns) {
    //         out += __TABBING + "Returns: " + as_function->returns.value().raw; out += NEWLINE;
    //     }
    //     else {
    //         out += __TABBING + "(void return)\n";
    //     }
    // }
    // auto as_class = std::dynamic_pointer_cast<Class>(decl);
    // if (as_class) {
    //     out += "Class declaration: \n";
    //     out += __TABBING + "Name: " + debug_templates(as_class->name, tabs + 1);
    //     out += __TABBING + "Inherits: " + debug_inherits(as_class->inherits, tabs + 1);
    //     out += __TABBING + "Parameters: \n" + debug_parameters(as_class->params, tabs + 1); out += NEWLINE;
    //     out += __TABBING + "Members: \n" + debug_members(as_class->members, tabs + 1); out += NEWLINE;
    // }
    // if (out.empty()) {
    //     return "This type needs debugging!";
    // }
    // return out;
    return "";
}

auto cblang::parser::debug_templates(const std::shared_ptr<Templated>& decl, const int& tabs, const bool& one_line) -> std::string {
    std::string out = decl->name.raw + (one_line ? "" : "\n");
    for (const auto& templated : decl->templates) {
        out += (one_line ? "<" : (__TABBING + "Template: ")) + debug_templates(templated, tabs + 1, one_line) + (one_line ? ">" : "");
        if (!one_line) { out += NEWLINE; }
    }
    return out;
}

auto cblang::parser::debug_members(const std::vector<std::shared_ptr<Declaration>>& decls, const int& tabs) -> std::string {
    std::string out;
    for (const auto& decl : decls) {
        out += __TABBING + debug_member(decl, tabs + 1); out += NEWLINE; 
    }
    return out;
}

auto cblang::parser::debug_program(const std::shared_ptr<ParsedProgram>& program) -> std::string {
    int tabs = 0;
    std::string out;
    out += "Program: \n";
    out += __TABBING + "Parameters: \n" + debug_parameters(program->parameters, tabs + 1); out += NEWLINE;
    out += __TABBING + "Members: \n" + debug_members(program->members, tabs + 1); out += NEWLINE;
    return out;
}
