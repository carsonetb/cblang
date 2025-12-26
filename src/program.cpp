#include "program.hpp"
#include "definitions.hpp"
#include "objects.hpp"
#include "parser.hpp"
#include "scanner.hpp"
#include <cassert>
#include <memory>
#include <optional>
#include <ranges>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <stdexcept>
#include <vector>

#define PUSH_EMPTY_SCOPE scope.push_back(std::make_shared<program::Scope>(scope.back()->scope_object))

using namespace cblang;
using namespace cblang::program;

static bool initialized = false;
static std::shared_ptr<spdlog::logger> logger = spdlog::stderr_color_st("cblang::program");

cblang::program::Program::Program(std::shared_ptr<definitions::UserDefinition> p_main_class) : main_class(std::move(p_main_class)) {}

auto cblang::program::Program::create_object(const std::vector<std::shared_ptr<objects::Object>>& params) const -> std::optional<std::shared_ptr<objects::Object>> {
    try {
        return main_class->create_object(scanner::Token(scanner::IDENTIFIER, "PROGRAM ENTRY POINT", -1), {}, params);
    }
    catch (RuntimeException exception) {
        logger->error("Error encountered during execution!");
        return {};
    }
}

cblang::program::ScopeParser::ScopeParser(
    std::vector<std::shared_ptr<parser::Statement>> p_code, 
    std::vector<std::shared_ptr<program::Scope>> p_scope, 
    bool p_returnable
) : code(std::move(p_code)), scope(std::move(p_scope)), returnable(p_returnable) {

}

cblang::program::ScopeParser::ScopeParser(
    std::shared_ptr<parser::Expr> p_expr, 
    std::vector<std::shared_ptr<program::Scope>> p_scope
) : parse_expr(std::move(p_expr)), scope(std::move(p_scope)), returnable(false) {
    
}

auto cblang::program::ScopeParser::binary_operator(const std::shared_ptr<objects::Object>& lhs, const scanner::Token& oper, const std::shared_ptr<objects::Object>& rhs) -> std::shared_ptr<objects::Object> {
    auto out = lhs->call(oper.raw, oper, {}, {rhs});
    if (!out.has_value()) {
        throw handle_error(oper, "Operator '" + lhs->get_templated()->stringify() + "." + oper.raw + "' does not return a value.");
    }
    return out.value();
}

auto cblang::program::ScopeParser::unary_operator(const scanner::Token& oper, const std::shared_ptr<objects::Object>& rhs) -> std::shared_ptr<objects::Object> {
    auto out = rhs->call(oper.raw, oper, {}, {rhs});
    if (!out.has_value()) {
        throw handle_error(oper, "Operator function did not return a value.");
    }
    return out.value();
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
    if (!code) {
        throw std::runtime_error("Can't call process without code.");
    }
    return process_scope(code.value());
}

auto cblang::program::ScopeParser::process_expr() -> std::shared_ptr<objects::Object> {
    if (!parse_expr) {
        throw std::runtime_error("Can't call process_expr without expr.");
    }
    return expression(parse_expr.value());
}

auto cblang::program::ScopeParser::process_scope(const std::vector<std::shared_ptr<parser::Statement>>& statements) -> std::optional<std::shared_ptr<objects::Object>> {
    for (const auto& stmnt : statements) {
        auto ret = statement(stmnt);
        if (ret) {
            return ret;
        }
    }
    return {};
}

auto cblang::program::ScopeParser::statement(const std::shared_ptr<parser::Statement>& stmnt) -> std::optional<std::shared_ptr<objects::Object>> {
    auto as_expression = std::dynamic_pointer_cast<parser::Expr>(stmnt);
    if (as_expression) {
        expression(as_expression, false);
        return {};
    }
    auto as_set_var = std::dynamic_pointer_cast<parser::SetVar>(stmnt);
    if (as_set_var) {
        set_var(as_set_var);
        return {};
    }
    auto as_create_var = std::dynamic_pointer_cast<parser::CreateVar>(stmnt);
    if (as_create_var) {
        create_var(as_create_var);
        return {};
    }
    auto as_return = std::dynamic_pointer_cast<parser::Return>(stmnt);
    if (as_return) {
        return expression(as_return->return_expression);
    }
    // TODO: Fix
    auto as_function = std::dynamic_pointer_cast<parser::Function>(stmnt);
    if (as_function) {
        throw program::handle_error(as_function->templated_name->name, "Function declaration not allowed in code scope.");
    }
    auto as_variable = std::dynamic_pointer_cast<parser::Variable>(stmnt);
    if (as_variable) {
        throw program::handle_error(as_variable->name, "(please report) Member variable declaration in code scope.");
    }
    auto as_class = std::dynamic_pointer_cast<parser::Class>(stmnt);
    if (as_class) {
        throw program::handle_error(as_class->name->name, "Class declaration not allowed in code scope.");
    }
    auto as_if_stmnt = std::dynamic_pointer_cast<parser::IfStmnt>(stmnt);
    if (as_if_stmnt) {
        auto eval = std::dynamic_pointer_cast<objects::BoolObject>(expression(as_if_stmnt->check_expression));
        if (eval->value) {
            PUSH_EMPTY_SCOPE;
            auto out = process_scope(as_if_stmnt->to_run);
            scope.pop_back();
            return out;
        }
        if (as_if_stmnt->elif_following) {
            return statement(as_if_stmnt->elif_following.value());
        }
        if (as_if_stmnt->else_following) {
            PUSH_EMPTY_SCOPE; 
            auto out = process_scope(as_if_stmnt->else_following.value()->to_run);
            scope.pop_back();
            return out;
        }
    }
    auto as_while_stmnt = std::dynamic_pointer_cast<parser::WhileStmnt>(stmnt);
    if (as_while_stmnt) {
        while (std::dynamic_pointer_cast<objects::BoolObject>(expression(as_while_stmnt->check_expression))->value) {
            PUSH_EMPTY_SCOPE;
            auto ret = process_scope(as_while_stmnt->to_run);
            scope.pop_back();
            if (ret) {
                return ret;
            }
        }
    }
    auto as_for_stmnt = std::dynamic_pointer_cast<parser::ForStmnt>(stmnt);
    if (as_for_stmnt) {
        auto looped = std::dynamic_pointer_cast<objects::ArrayObject>(expression(as_for_stmnt->looped));
        scanner::Token looper_name = as_for_stmnt->looper.second;
        std::shared_ptr<definitions::TemplatedType> looper_type = looped->defined_templates.at("value_type")->template_used.value();
        for (const auto& object : looped->value) { // Maybe should do some runtime static type checks? Doesn't seem so necessary.
            auto for_scope = std::make_shared<Scope>(scope.back()->scope_object);
            for_scope->defined_variables[looper_name.raw] = std::make_shared<objects::Variable>(looper_name, object, false, false, false);
            scope.push_back(for_scope);
            process_scope(as_for_stmnt->to_run);
            scope.pop_back();
        }
    }
    return {};
}

auto cblang::program::ScopeParser::expression(const std::shared_ptr<parser::Expr>& expr, bool must_evaluate) -> std::shared_ptr<objects::Object> {
    auto as_binary = std::dynamic_pointer_cast<parser::Binary>(expr);
    if (as_binary) {
        return binary_operator(expression(as_binary->left), as_binary->op, expression(as_binary->right));
    }
    auto as_logical = std::dynamic_pointer_cast<parser::Logical>(expr);
    if (as_logical) {
        auto left = std::dynamic_pointer_cast<objects::BoolObject>(expression(as_logical->left));
        auto right = std::dynamic_pointer_cast<objects::BoolObject>(expression(as_logical->right));
        if (as_logical->op.type == scanner::PIPE_PIPE) {
            return std::make_shared<objects::BoolObject>(left->value || right->value);
        }
        if (as_logical->op.type == scanner::AND_AND) {
            return std::make_shared<objects::BoolObject>(left->value && right->value);
        }
        assert(false);
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
        auto out = std::make_shared<objects::FunctionObject>(
            as_scope_expr->declare_point, 
            std::vector<std::shared_ptr<definitions::MemberDefinition>>(), 
            std::vector<std::shared_ptr<definitions::TemplateDefinition>>(),
            std::optional<std::shared_ptr<definitions::TemplatedType>>(),
            as_scope_expr->statements
        );
        out->initialize();
        return out;
    }
    auto as_array_expr = std::dynamic_pointer_cast<parser::ArrayExpr>(expr);
    if (as_array_expr) {
        std::vector<std::shared_ptr<objects::Object>> array_objects;
        for (const auto& inner_expr : as_array_expr->items) {
            array_objects.push_back(expression(inner_expr));
        }
        auto out = std::make_shared<objects::ArrayObject>(array_objects, std::make_shared<definitions::TemplateDefinition>(as_array_expr->evaluates_to.value()->templates[0]));
        out->initialize();
        return out;
    }
    auto as_accessible = std::dynamic_pointer_cast<parser::Accessible>(expr);
    if (as_accessible) {
        auto out = accessible(as_accessible, {}, must_evaluate);
        if (!must_evaluate) {
            return nullptr;
        }
        if (!out.value()) {
            throw handle_error("(please report) Accessible with must evaluate enabled returned a null object without throwing an error.");
        }
        return out.value();
    }
    throw handle_error("(please repot) Invalid expression type.");
}

// TODO: This whole function is kinda messy, and badly implemented.
auto cblang::program::ScopeParser::accessible(const std::shared_ptr<parser::Accessible>& var, std::optional<std::shared_ptr<objects::Object>> call_on, bool must_evaluate) -> std::optional<std::shared_ptr<objects::Object>> {
    auto as_call_expr = std::dynamic_pointer_cast<parser::CallExpr>(var);
    if (as_call_expr) {
        scanner::Token name_token = as_call_expr->name->name;
        if (!call_on.has_value()) {
            for (const auto& item : std::ranges::reverse_view(scope)) {
                if (item->scope_object->members_by_name.contains(name_token.raw)) {
                    call_on = item->scope_object;
                    break;
                }
            }
            if (!call_on.has_value()) {
                throw handle_error(name_token, "Function does not exist in the current scope.");
            }
        }
        std::vector<std::shared_ptr<objects::Object>> arguments;
        for (const auto& argument_expr : as_call_expr->args) {
            arguments.push_back(expression(argument_expr));
        }
        std::vector<std::shared_ptr<definitions::TemplateDefinition>> templates;
        for (const auto& template_def : as_call_expr->name->templates) {
            templates.push_back(std::make_shared<definitions::TemplateDefinition>(get_class(template_def)));
        }
        auto out = call_on.value()->call(name_token.raw, name_token, templates, arguments); // TODO: Function call stack.
        if (!out.has_value() && must_evaluate) {
            throw handle_error(name_token, "Function must return a value.");
        }
        return out;
    }

    auto as_var_expr = std::dynamic_pointer_cast<parser::VarExpr>(var);
    if (as_var_expr) {
        if (call_on.has_value()) {
            return call_on.value()->get_var(as_var_expr->name);
        }
        auto optional_var = get_variable(as_var_expr->name.raw);
        if (!optional_var.has_value()) {
            throw handle_error(as_var_expr->name, "Variable doesn't exist in the current scope.");
        }
        return optional_var.value()->object;
    }

    throw handle_error("(please report) Invalid Accessible type (or base class).");
}

auto cblang::program::ScopeParser::set_var(const std::shared_ptr<parser::SetVar>& statement) -> void {
    auto this_scope = scope.back();
    auto variable = get_variable(statement->name.raw);
    if (!variable) {
        throw program::handle_error(statement->name, "'" + statement->name.raw + "' does not exist in the current scope.");
    }
    if (variable.value()->is_const || variable.value()->is_static) {
        throw program::handle_error(statement->name, "Cannot modify a const/static variable.");
    }
    variable.value()->object = expression(statement->val);
}

auto cblang::program::ScopeParser::create_var(const std::shared_ptr<parser::CreateVar>& statement) -> void {
    scanner::Token name = statement->type_name.second;
    scope.back()->defined_variables[name.raw] = std::make_shared<objects::Variable>(name, expression(statement->val), false, false, false); // TODO: Const and static in code.
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
    for (const auto& item : std::ranges::reverse_view(scope)) {
        if (item->defined_classes.contains(name)) {
            return item->defined_classes.at(name);
        }
    }
    return {};
}

auto cblang::program::ScopeParser::get_variable(const std::string& name) const -> std::optional<std::shared_ptr<objects::Variable>> {
    for (const auto& item : std::ranges::reverse_view(scope)) {
        if (item->defined_variables.contains(name)) {
            return item->defined_variables.at(name);
        }
    }
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

auto cblang::program::handle_error(const scanner::Token &token, const std::string &error) -> RuntimeException {
    logger->error("[line " + std::to_string(token.line) + "] [token " + (token.type == scanner::END_OF_FILE ? "EOF" : token.raw) + "] " + error);
    return {error};
}

auto cblang::program::handle_error(const std::string &error) -> RuntimeException {
    logger->error("[unknown line or token] " + error);
    return {error};
}