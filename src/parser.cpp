#include "parser.h"

#include "scanner.h"
#include "util.h"
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

cblang::parser::Expr::~Expr() = default;

cblang::parser::Declaration::~Declaration() = default;

cblang::parser::Parser::Parser(std::vector<scanner::Token> p_tokens) : tokens(std::move(p_tokens)) {
    
}

auto cblang::parser::Parser::parse() -> std::optional<std::shared_ptr<Program>> {
    logger->debug("Parser started.");
    try {
        return program();
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
                return;
            default:
                break;
        }

        advance();
    }
}

auto cblang::parser::Parser::program() -> std::shared_ptr<Program> {
    consume(CLASS_KW, "Program must start with the 'class' keyword (for Main class definition.)");
    auto name = consume(IDENTIFIER, "Expected 'Main' identifier after 'class' keyword.");
    if (name.raw != "Main") { throw handle_error(name, "First class must be named 'Main'."); }
    auto params = parameters();
    consume(EQUAL, "Expected '=' between parameters and members.");
    auto mems = members();
    return std::make_shared<Program>(params, mems);
}

auto cblang::parser::Parser::function() -> std::shared_ptr<Function> {
    auto name = consume(IDENTIFIER, "Expected function name after 'scope' keyword.");
    auto params = parameters();
    std::optional<Token> returns;
    if (match({RETURN})) {
        returns = consume(IDENTIFIER, "Expected return type after '->'");
    }
    consume(EQUAL, "Expected '=' between function declaration and code.");
    consume(LEFT_CURLY, "Expected '{' after '='");
    auto body = scope();
    return std::make_shared<Function>(name, params, returns, body);
}

auto cblang::parser::Parser::class_decl() -> std::shared_ptr<Class> {
    auto name = consume(IDENTIFIER, "Expected class name after 'class' keyword.");
    std::vector<Token> inherits;
    if (match({COLON})) {
        while (true) {
            inherits.push_back(consume(IDENTIFIER, "Expected name of class to inherit after ':'."));
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
    auto type = consume(IDENTIFIER, "Expected variable type.");
    auto name = consume(IDENTIFIER, "Expected variable name.");
    std::optional<std::shared_ptr<Expr>> expr;
    if (match({EQUAL})) {
        expr = expression();
    }
    return std::make_shared<Variable>(type, name, expr);
}

auto cblang::parser::Parser::parameters(bool optional) -> std::vector<std::pair<Token, Token>> {
    std::vector<std::pair<Token, Token>> out;
    if (!check(LEFT_PAREN) && optional) {
        return {};
    }
    consume(LEFT_PAREN, "Expected '('");
    if (!check(RIGHT_PAREN)) {
        while (true) {
            auto type = consume(IDENTIFIER, "Expected parameter type.");
            auto name = consume(IDENTIFIER, "Expected parameter name.");
            out.emplace_back(type, name);
            if (!match({COMMA})) {
                break;
            }
        }
    }
    consume(RIGHT_PAREN, "Expected ')' after parameters.");
    return out;
}

auto cblang::parser::Parser::members() -> std::vector<std::shared_ptr<Declaration>> {
    std::vector<std::shared_ptr<Declaration>> out;
    consume(LEFT_PAREN, "Expected '(' to begin a member scope.");
    if (match({RIGHT_PAREN})) {
        return {};
    }
    while (true) {
        std::shared_ptr<Declaration> decl;
        if (check(IDENTIFIER)) {
            decl = variable();
        }
        else if (match({SCOPE_KW})) {
            decl = function();
        }
        else if (match({CLASS_KW})) {
            decl = class_decl();
        }
        else {
            throw handle_error(peek(), "Expected a declaration (variable, scope, or class).");
        }
        out.push_back(decl);
        if (!match({COMMA})) {
            break;
        }
    }
    consume(RIGHT_PAREN, "Expected ')' after members.");
    return out;
}

auto cblang::parser::Parser::scope() -> std::vector<std::shared_ptr<Statement>> {
    std::vector<std::shared_ptr<Statement>> statements;

    while (!check(RIGHT_CURLY) && !is_at_end()) {
        statements.push_back(statement());
    }

    consume(RIGHT_CURLY, "Expected '}' after scope.");
    return statements;
}

auto cblang::parser::Parser::statement() -> std::shared_ptr<Statement> {
    // TODO
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

    throw handle_error(peek(), "Expected expression.");
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
        out += __TABBING + "Parameter: " + param.first.raw + " " + param.second.raw; out += NEWLINE;
    }
    return out;
}

auto inherits(const std::vector<Token> types, const int& tabs = 0) -> std::string {
    std::string out;
    if (types.empty()) {
        return "(does not inherit)";
    }
    for (const auto& types : types) {
        out += "\n" + __TABBING + types.raw;
    }
    return out;
}

auto debug_member(const std::shared_ptr<Declaration>& decl, const int& tabs = 0) -> std::string {
    std::string out;
    auto as_var = std::dynamic_pointer_cast<Variable>(decl);
    if (as_var) {
        out += "Variable declaration: \n";
        out += __TABBING + "Type: " + as_var->type.raw; out += NEWLINE;
        out += __TABBING + "Name: " + as_var->name.raw; out += NEWLINE;
        if (as_var->value) {
            out += __TABBING + "Value: " + debug_expression(as_var->value.value(), tabs + 1); out += NEWLINE;
        }
        else {
            out += __TABBING + "(no value set)\n";
        }
    }
    auto as_function = std::dynamic_pointer_cast<Function>(decl);
    if (as_function) {
        out += "Function declaration: \n";
        out += __TABBING + "Name: " + as_function->name.raw; out += NEWLINE;
        out += __TABBING + "Parameters: \n" + debug_parameters(as_function->params, tabs + 1); out += NEWLINE;
        if (as_function->returns) {
            out += __TABBING + "Returns: " + as_function->returns.value().raw; out += NEWLINE;
        }
        else {
            out += __TABBING + "(void return)\n";
        }
    }
    auto as_class = std::dynamic_pointer_cast<Class>(decl);
    if (as_class) {
        out += "Class declaration: \n";
        out += __TABBING + "Name: " + as_class->name.raw; out += NEWLINE;
        out += __TABBING + "Inherits: " + inherits(as_class->inherits, tabs + 1); out += NEWLINE;
        out += __TABBING + "Parameters: \n" + debug_parameters(as_class->params, tabs + 1); out += NEWLINE;
        out += __TABBING + "Members: \n" + debug_members(as_class->members, tabs + 1); out += NEWLINE;
    }
    if (out.empty()) {
        return "This type needs debugging!";
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

auto cblang::parser::debug_program(const std::shared_ptr<Program>& program) -> std::string {
    int tabs = 0;
    std::string out;
    out += "Program: \n";
    out += __TABBING + "Parameters: \n" + debug_parameters(program->parameters, tabs + 1); out += NEWLINE;
    out += __TABBING + "Members: \n" + debug_members(program->members, tabs + 1); out += NEWLINE;
    return out;
}
