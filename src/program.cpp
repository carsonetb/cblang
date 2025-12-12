#include "program.hpp"
#include "definitions.hpp"
#include "objects.hpp"
#include "parser.hpp"
#include <memory>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <vector>

using namespace cblang;
using namespace cblang::program;

static bool initialized = false;
static std::shared_ptr<spdlog::logger> logger = spdlog::stderr_color_mt("cblang::compiler");

cblang::program::Program::Program(std::shared_ptr<definitions::UserDefinition> p_main_class) : main_class(std::move(p_main_class)) {}

auto cblang::program::Program::get_functions() const -> std::vector<std::shared_ptr<definitions::FunctionMember>> {
    return {};
}

auto cblang::program::init(bool verbose) -> void {
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [cblang::program] %^[%l] %v %$");
    logger->set_level(spdlog::level::warn);

    initialized = true;

    if (verbose) {
        enable_verbose_logs();
    }

    logger->info("Initialization complete!");
}

auto cblang::program::enable_verbose_logs() -> void {
    logger->set_level(spdlog::level::debug);
    logger->info("Verbose logs enabled.");
}

auto cblang::program::run_function(std::shared_ptr<definitions::UserDefinition> run_on, std::shared_ptr<definitions::FunctionMember> to_run) -> std::optional<objects::Object> {
    try {
        // TODO: Run the function.
    }
    catch (RuntimeException exception) {
        logger->error("Program exited early because of an error.");
    }

    logger->info("Program finished successfully.");

    return {};
} 

auto cblang::program::handle_error(const scanner::Token &token, const std::string &error) -> RuntimeException {
    logger->error("[line " + std::to_string(token.line) + "] [token " + (token.type == scanner::END_OF_FILE ? "EOF" : token.raw) + "] " + error);
    return {error};
}

auto cblang::program::handle_error(const std::string &error) -> RuntimeException {
    logger->error("[unknown line or token] " + error);
    return {error};
}

cblang::program::ScopeParser::ScopeParser(
    std::vector<std::shared_ptr<parser::Statement>> p_code, 
    std::vector<std::shared_ptr<program::Scope>> p_scope, 
    bool p_returnable
) : code(std::move(p_code)), scope(std::move(p_scope)), returnable(p_returnable) {

}

auto cblang::program::ScopeParser::construct_literal(const scanner::Token& token) -> std::shared_ptr<objects::Object> {
    if (!token.literal) {
        throw program::handle_error(token, "(please report) token is not a literal but passed to construct_literal.");
    }

    auto literal = token.literal.value();

    auto as_bool = std::dynamic_pointer_cast<scanner::BoolLiteral>(literal);
    auto as_int = std::dynamic_pointer_cast<scanner::IntLiteral>(literal);
    auto as_float = std::dynamic_pointer_cast<scanner::FloatLiteral>(literal);
    auto as_char = std::dynamic_pointer_cast<scanner::CharLiteral>(literal);
    auto as_string = std::dynamic_pointer_cast<scanner::StringLiteral>(literal);

    if (as_bool) { return std::make_shared<objects::BoolObject>(as_bool->val); }
    if (as_int) { return std::make_shared<objects::IntObject>(as_int->val); }
    if (as_float) { return std::make_shared<objects::FloatObject>(as_float->val); }
    if (as_char) { return std::make_shared<objects::CharObject>(as_char->val); }
    if (as_string) { return std::make_shared<objects::StringObject>(as_string->val); }

    throw program::handle_error(token, "(please report) base class Literal passed to construct_literal.");
}

auto cblang::program::ScopeParser::process() -> std::optional<std::shared_ptr<objects::Object>> {
    for (const auto& line : code) {
        auto out = statement(line);
        if (out) {
            return out;
        }
    }
    return {};
}

auto cblang::program::ScopeParser::statement(const std::shared_ptr<parser::Statement>& statement) -> std::optional<std::shared_ptr<objects::Object>> {
    auto as_expression = std::dynamic_pointer_cast<parser::Expr>(statement);
    if (as_expression) {
        expression(as_expression);
        return {};
    }
    auto as_set_var = std::dynamic_pointer_cast<parser::SetVar>(statement);
    if (as_set_var) {
        set_var(as_set_var);
        return {};
    }
    auto as_create_var = std::dynamic_pointer_cast<parser::CreateVar>(statement);
    if (as_create_var) {
        create_var(as_create_var);
        return {};
    }
    auto as_return = std::dynamic_pointer_cast<parser::Return>(statement);
    if (as_return) {
        return expression(as_return->return_expression);
    }
    // TODO: Fix
    auto as_function = std::dynamic_pointer_cast<parser::Function>(statement);
    if (as_function) {
        throw program::handle_error(as_function->templated_name->name, "Function declaration not allowed in code scope.");
    }
    auto as_variable = std::dynamic_pointer_cast<parser::Variable>(statement);
    if (as_variable) {
        throw program::handle_error(as_variable->name, "(please report) Member variable declaration in code scope.");
    }
    auto as_class = std::dynamic_pointer_cast<parser::Class>(statement);
    if (as_class) {
        throw program::handle_error(as_class->name->name, "Class declaration not allowed in code scope.");
    }
    return {};
}

auto cblang::program::ScopeParser::expression(const std::shared_ptr<parser::Expr>& expr) -> std::shared_ptr<objects::Object> {
    auto as_binary = std::dynamic_pointer_cast<parser::Binary>(expr);
    if (as_binary) {
        return binary_operator(expression(as_binary->left), as_binary->op, expression(as_binary->right));
    }
    auto as_logical = std::dynamic_pointer_cast<parser::Logical>(expr);
    if (as_logical) {
        return binary_operator(expression(as_logical->left), as_logical->op, expression(as_logical->right));
    }
    auto as_grouping = std::dynamic_pointer_cast<parser::Grouping>(expr);
    if (as_grouping) {
        return expression(as_grouping->expression);
    }
    auto as_literal = std::dynamic_pointer_cast<parser::Literal>(expr);
    if (as_literal) {
        return construct_literal(as_literal->token);
    }
    auto as_unary = std::dynamic_pointer_cast<parser::Unary>(expr);
    if (as_unary) {
        return unary_operator(as_unary->op, expression(as_unary->right));
    }
    auto as_scope_expr = std::dynamic_pointer_cast<parser::ScopeExpr>(expr);
    if (as_scope_expr) {
        return std::make_shared<objects::FunctionObject>(
            as_scope_expr->declare_point, 
            std::vector<std::shared_ptr<definitions::MemberDefinition>>(), 
            std::vector<std::shared_ptr<definitions::TemplateDefinition>>(),
            std::optional<std::shared_ptr<definitions::ClassDefinition>>(),
            as_scope_expr->statements
        );
    }
    auto as_array_expr = std::dynamic_pointer_cast<parser::ArrayExpr>(expr);
    if (as_array_expr) {
        std::vector<std::shared_ptr<objects::Object>> array_objects;
        for (const auto& inner_expr : as_array_expr->items) {
            array_objects.push_back(expression(inner_expr));
        }
        // TODO: Add checks to make sure these variables exist.
        return std::make_shared<objects::ArrayObject>(array_objects, std::make_shared<definitions::TemplateDefinition>(as_array_expr->evaluates_to.value()->templates[0]->cls));
    }
    auto as_accessible = std::dynamic_pointer_cast<parser::Accessible>(expr);
    if (as_accessible) {
        return accessible(as_accessible);
    }
    throw handle_error("(please repot) Invalid expression type.");
}

auto cblang::program::ScopeParser::accessible(const std::shared_ptr<parser::Accessible>& var) -> std::shared_ptr<objects::Object> {

}

auto cblang::program::ScopeParser::set_var(const std::shared_ptr<parser::SetVar>& statement) -> void {
    auto this_scope = scope.back();
    auto variable = get_variable(statement->name.raw);
    if (!variable) {
        throw program::handle_error(statement->name, "'" + statement->name.raw + "' does not exist in the current scope.");
    }
    variable.value()->object = expression(statement->val);
}

auto cblang::program::ScopeParser::create_var(const std::shared_ptr<parser::CreateVar>& statement) -> void {
    scanner::Token name = statement->type_name.second;
    scope.back()->defined_variables[name.raw] = std::make_shared<objects::Variable>(name, expression(statement->val));
}

auto cblang::program::ScopeParser::get_class(const std::shared_ptr<parser::Templated>& templated_class) const -> std::shared_ptr<definitions::TemplatedType> {
    auto this_definition = get_class_definition(templated_class->name.raw);
    if (!this_definition) {
        throw handle_error(templated_class->name, "Class doesn't exist in the current scope.");
    }
    std::vector<std::shared_ptr<definitions::TemplatedType>> inner_templates;
    for (const auto& passed_template : templated_class->templates) {
        inner_templates.push_back(get_class(passed_template));
    }
    return std::make_shared<definitions::TemplatedType>(this_definition.value(), inner_templates);
}

auto cblang::program::ScopeParser::get_class_definition(const std::string& name) const -> std::optional<std::shared_ptr<definitions::ClassDefinition>> {
    for (unsigned long i = scope.size() - 1; i >= 0; i--) {
        const auto& item = scope[i];
        if (item->defined_classes.contains(name)) {
            return item->defined_classes.at(name);
        }
    }
    return {};
}

auto cblang::program::ScopeParser::get_variable(const std::string& name) const -> std::optional<std::shared_ptr<objects::Variable>> {
    for (unsigned long i = scope.size() - 1; i >= 0; i--) {
        const auto& item = scope[i];
        if (item->defined_variables.contains(name)) {
            return item->defined_variables.at(name);
        }
    }
    return {};
}

auto cblang::program::ScopeParser::binary_operator(const std::shared_ptr<objects::Object>& lhs, const scanner::Token& oper, const std::shared_ptr<objects::Object>& rhs) const -> std::shared_ptr<objects::Object> {

}

auto cblang::program::ScopeParser::unary_operator(const scanner::Token& oper, const std::shared_ptr<objects::Object>& rhs) -> std::shared_ptr<objects::Object> {
    
}