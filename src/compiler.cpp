#include "compiler.hpp"
#include "cblang.hpp"
#include "definitions.hpp"
#include "parser.hpp"
#include "program.hpp"
#include "scanner.hpp"
#include "util.hpp"

#include <cassert>
#include <exception>
#include <memory>
#include <optional>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <ranges>
#include <string>
#include <sys/types.h>
#include <vector>

using namespace cblang::compiler;
using namespace cblang;

static bool initialized = false;
static std::shared_ptr<spdlog::logger> logger = spdlog::stderr_color_st("cblang::compiler");

static auto process_templated_definition(const std::shared_ptr<parser::Templated>& input) -> std::vector<std::shared_ptr<TemplateDefinition>> {
    std::vector<std::shared_ptr<TemplateDefinition>> out;
    for (const auto& templdef : input->templates) {
        out.push_back(std::make_shared<TemplateDefinition>(templdef->name));
    }
    return out;
}

auto cblang::compiler::init(bool verbose) -> void {
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [cblang::compiler] %^[%l] %v %$");
    logger->set_level(spdlog::level::warn);

    initialized = true;

    if (verbose) {
        enable_verbose_logs();
    }

    logger->info("Initialization complete!");
}

auto cblang::compiler::enable_verbose_logs() -> void {
    logger->set_level(spdlog::level::debug);
    logger->info("Verbose logs enabled.");
}

auto cblang::compiler::handle_error(const scanner::Token& token, const std::string& error) -> CompileException {
    logger->error("[line " + std::to_string(token.line) + "] [token '" + (token.type == scanner::END_OF_FILE ? "EOF" : token.raw) + "'] " + error);
    return {};
}

auto cblang::compiler::Compiler::get_class(const scanner::Token& token, const std::string& name) const -> std::shared_ptr<ClassDefinition> {
    if (defined_classes.contains(name)) {
        return defined_classes.at(name);
    }
    throw handle_error(token, "Class '" + name + "' not defined yet.");  
}

auto cblang::compiler::Compiler::compile() -> std::optional<std::shared_ptr<program::Program>> {
    logger->info("Compiler started.");

    std::shared_ptr<UserDefinition> main_class;

    try {
        main_class = main();
    }
    catch (CompileException exception) {
        logger->error("Errors compiling.");
        return {};
    }

    auto out = std::make_shared<program::Program>(main_class);

    logger->info("Beginning static analysis.");

    StaticAnalyzer analyzer(out);
    int err = analyzer.perform_analysis();

    if (err == 1) {
        logger->error("Static analysis failed.");
        return {};
    }
    
    logger->info("Static analysis finished successfully.");

    logger->info("Compiler finished successfully.");
    out->valid = true;

    return out;
}

auto cblang::compiler::Compiler::main() -> std::shared_ptr<definitions::UserDefinition> {
    auto members = class_members(source->members);
    auto params = process_params(source->parameters);
    // for (const auto& param : params) {
    //     members.push_back(param);
    // }
    return std::make_shared<definitions::UserDefinition>(TEMPLATED_EMPTY("Main"), params, members);
}

auto cblang::compiler::Compiler::process_class(const std::shared_ptr<parser::Class>& input) -> std::shared_ptr<ClassDefinition> {
    auto params = process_params(input->params);
    auto members = class_members(input->members);
    // for (const auto& param : params) {
    //     members.push_back(param);
    // }
    auto out = std::make_shared<definitions::UserDefinition>(input->name, params, members);
    defined_classes[out->type_name->name.raw] = out;
    return out;
}

auto cblang::compiler::Compiler::process_params(const parser::Parameters& input) -> std::vector<std::shared_ptr<MemberDefinition>> {
    std::vector<std::shared_ptr<MemberDefinition>> out;
    for (const auto& param : input) {
        auto type = process_templated(param.first);
        auto name = param.second;
        std::optional<std::shared_ptr<parser::Expr>> expression; // Expressions need to be supported in Parameters!!!
        out.push_back(std::make_shared<MemberDefinition>(type, name, expression, false, false, false));
    }
    return out;
}

auto cblang::compiler::Compiler::class_members(const std::vector<std::shared_ptr<parser::Declaration>>& inputs) -> std::vector<std::shared_ptr<MemberDefinition>> {
    std::vector<std::shared_ptr<MemberDefinition>> out;
    for (const auto& declaration : inputs) {
        auto as_function = std::dynamic_pointer_cast<parser::Function>(declaration);
        auto as_variable = std::dynamic_pointer_cast<parser::Variable>(declaration);
        auto as_class = std::dynamic_pointer_cast<parser::Class>(declaration);
        if (as_function) {
            auto name = as_function->templated_name;
            auto params = process_params(as_function->params);
            auto templates = process_templated_definition(as_function->templated_name);
            std::optional<std::shared_ptr<TemplatedType>> returns;
            if (as_function->returns) {
                returns = process_templated(as_function->returns.value());
            }
            out.push_back(std::make_shared<FunctionMember>(name, params, returns, as_function->body, as_function->is_static, as_function->is_private, as_function->is_const, as_function->is_operator, as_function->is_cast));
        }
        else if (as_variable) {
            auto name = as_variable->name;
            auto type = process_templated(as_variable->type);
            out.push_back(std::make_shared<MemberDefinition>(type, name, as_variable->value, as_variable->is_static, as_variable->is_private, as_variable->is_const));
        }
        else if (as_class) {
            out.push_back(process_class(as_class));
        }
    }
    return out;
}

auto cblang::compiler::Compiler::process_templated(const std::shared_ptr<parser::Templated>& input) -> std::shared_ptr<TemplatedType> {
    auto cls = get_class(input->name, input->name.raw);
    std::vector<std::shared_ptr<TemplatedType>> templates;
    for (const auto& templated : input->templates) {
        templates.push_back(process_templated(templated));
    }
    return std::make_shared<TemplatedType>(cls, templates);
}

auto cblang::compiler::StaticAnalyzer::init_members(const std::shared_ptr<UserDefinition>& definition, StaticScope& scope) -> void {// Next add static member functions.
    for (const auto& member : definition->members) {
        auto as_function = std::dynamic_pointer_cast<FunctionMember>(member);
        if (!as_function || !as_function->is_static) {
            continue;
        }
        if (scope.contains(as_function->function_name.raw)) {
            throw handle_error(as_function->function_name, "(during static analysis) A member variable already exists with this name.");
        }
        scope[as_function->function_name.raw] = as_function;
    }

    // Next add class parameters.
    for (const auto& param : definition->params) {
        scope[param->name.raw] = param;
    }

    // Next check all members and initializers.
    for (const auto& member : definition->members) {
        if (std::dynamic_pointer_cast<ClassDefinition>(member) || std::dynamic_pointer_cast<FunctionMember>(member)) {
            continue; // TODO: Fix
        }
        if (!member->initializer) {
            throw handle_error(member->name, "(during static analysis) Variable initializer is required!");
        }
        std::shared_ptr<parser::Expr> initializer = member->initializer.value();
        expression(initializer);
        if (*initializer->evaluates_to.value() != *member->type) {
            throw handle_error(member->name, "(during static analysis) Initializer expression doesn't evaluate to the same type as the variable.");
        }
        scope[member->name.raw] = member;
    }

    // First register all non-static member functions.
    for (const auto& member : definition->members) {
        auto as_function = std::dynamic_pointer_cast<FunctionMember>(member);
        if (as_function && !as_function->is_static) {
            if (scope.contains(as_function->function_name.raw)) {
                throw handle_error(as_function->function_name, "(during static analysis) A function with this name already exists.");
            }
            scope[as_function->function_name.raw] = as_function;
        }
    }

    // Now check all non-static member functions.
    for (const auto& member : definition->members) {
        auto as_function = std::dynamic_pointer_cast<FunctionMember>(member);
        if (as_function && !as_function->is_static) {
            scope[as_function->function_name.raw] = as_function;
            function(as_function);
        }
    }
}

auto cblang::compiler::StaticAnalyzer::perform_analysis() -> int {
    main_scope = StaticScope();

    try {
        init_members(source->main_class, main_scope);
    }
    catch (CompileException exception) {
        return 1;
    }

    return 0;
}

auto cblang::compiler::StaticAnalyzer::analyze_class(const std::shared_ptr<UserDefinition>& definition) -> void {
    current_class = StaticScope();
    current_class_templates = StaticTemplateScope();

    // First add template types.
    for (const auto& templ : definition->templates) {
        current_class_templates[templ->template_name.raw] = templ;
    }

    init_members(definition, current_class);
}

auto cblang::compiler::StaticAnalyzer::function(const std::shared_ptr<FunctionMember>& func) -> void {
    StaticScope this_scope;
    StaticTemplateScope this_templates;
    for (const auto& param : func->parameters) {
        this_scope[param->name.raw] = param;
    }
    for (const auto& templ : func->templates) {
        assert(templ->template_used.has_value());
        this_templates[templ->template_name.raw] = templ;
    }
    current_function_scopes.push_back(this_scope);
    current_function_templates.push_back(this_templates);
    function_returns = func->returns;
    bool all_paths_return = false;

    for (const auto& stmnt : func->code.value()) {
        if (statement(stmnt).has_value()) { // Break because all return types have satisfied.
            all_paths_return = true;
            break;
        }
    }

    current_function_scopes.pop_back();
    current_function_templates.pop_back();

    if (!all_paths_return) {
        throw handle_error(func->function_name, "All code paths must return a value.");
    }
}

auto cblang::compiler::StaticAnalyzer::scope(const std::vector<std::shared_ptr<parser::Statement>>& statements) -> std::optional<std::shared_ptr<TemplatedType>> {
    for (const auto& stmnt : statements) {
        auto ret = statement(stmnt);
        if (ret) {
            return ret;
        }
    }
    return {};
}

auto cblang::compiler::StaticAnalyzer::statement(const std::shared_ptr<parser::Statement>& stmnt) -> std::optional<std::shared_ptr<TemplatedType>> {
    auto as_expr = std::dynamic_pointer_cast<parser::Expr>(stmnt);
    if (as_expr) {
        expression(as_expr, false);
        return {};
    }
    auto as_set_var = std::dynamic_pointer_cast<parser::SetVar>(stmnt);
    if (as_set_var) {
        auto variable = assert_var_exists(as_set_var->name);
        expression(as_set_var->val);
        assert(as_set_var->val->evaluates_to.has_value());
        auto expr_type = as_set_var->val->evaluates_to.value();
        if (*variable->type != *expr_type) { // TODO: Check if the variable can be casted.
            throw handle_error(as_set_var->name, "(during static analysis) Cannot set Object of type '" + expr_type->stringify() + "' to variable of type '" + variable->type->stringify() + "'.");
        }
        return {};
    }
    auto as_create_var = std::dynamic_pointer_cast<parser::CreateVar>(stmnt);
    if (as_create_var) {
        scanner::Token var_name = as_create_var->type_name.second;
        auto var_type = process_templated(as_create_var->type_name.first);
        expression(as_create_var->val);
        assert(as_create_var->val->evaluates_to.has_value());
        auto expr_type = as_create_var->val->evaluates_to.value();
        if (*var_type != *expr_type) { // TODO: Check if the variable can be casted.
            throw handle_error(var_name, "(during static analysis) Cannot set Object of type '" + expr_type->stringify() + "' to variable of type '" + var_type->stringify() + "'");
        }
        current_function_scopes.back()[var_name.raw] = std::make_shared<MemberDefinition>(var_type, var_name, std::optional<std::shared_ptr<parser::Expr>>(), false, false, false);
        return {};
    }
    auto as_return = std::dynamic_pointer_cast<parser::Return>(stmnt);
    if (as_return) {
        expression(as_return->return_expression);
        auto ret_type = as_return->return_expression->evaluates_to;
        if (!function_returns.has_value()) {
            throw handle_error(as_return->ret_kw_point, "(during static analysis) Cannot return a value when there is no function return type.");
        }
        if (*ret_type.value() != *function_returns.value()) {
            throw handle_error(as_return->ret_kw_point, "(during static analysis) Return type doesn't match the function return type.");
        }
        return as_return->return_expression->evaluates_to;
    }
    auto as_if_stmnt = std::dynamic_pointer_cast<parser::IfStmnt>(stmnt);
    if (as_if_stmnt) {
        expression(as_if_stmnt->check_expression);
        assert(as_if_stmnt->check_expression->evaluates_to.has_value());
        if (as_if_stmnt->check_expression->evaluates_to.value()->cls->pretty_name != "bool") { // skull emoji
            throw handle_error(as_if_stmnt->start, "Expression inside of if statement must evaluate to a bool type.");
        }
        current_function_scopes.emplace_back();
        auto ret_type = scope(as_if_stmnt->to_run);
        current_function_scopes.pop_back();
        if (as_if_stmnt->elif_following.has_value()) {
            auto elif_ret = statement(as_if_stmnt->elif_following.value());
            if (ret_type.has_value()) {
                ret_type = elif_ret;
            }
        }
        if (as_if_stmnt->else_following.has_value()) {
            auto else_ret = statement(as_if_stmnt->else_following.value());
            if (ret_type.has_value()) {
                ret_type = else_ret;
            }
        }
        if (!as_if_stmnt->elif_following.has_value() && !as_if_stmnt->else_following.has_value()) {
            ret_type = {}; // This was the last if/elif and there is no else.
        }
        return ret_type;
    }
    auto as_else_statement = std::dynamic_pointer_cast<parser::ElseStmnt>(stmnt);
    if (as_else_statement) {
        current_function_scopes.emplace_back();
        auto ret_type = scope(as_else_statement->to_run);
        current_function_scopes.pop_back();
        return ret_type;
    }
    auto as_while_stmnt = std::dynamic_pointer_cast<parser::WhileStmnt>(stmnt);
    if (as_while_stmnt) {
        expression(as_while_stmnt->check_expression);
        assert(as_while_stmnt->check_expression->evaluates_to.has_value());
        if (as_while_stmnt->check_expression->evaluates_to.value()->cls->pretty_name != "bool") { // skull emoji
            throw handle_error(as_while_stmnt->start, "Expression inside of while statement must evaluate to a bool type.");
        }
        current_function_scopes.emplace_back();
        auto ret_type = scope(as_while_stmnt->to_run); // This would have to return on the first loop always.
        current_function_scopes.pop_back();
        return ret_type;
    }
    auto as_for_stmnt = std::dynamic_pointer_cast<parser::ForStmnt>(stmnt);
    if (as_for_stmnt) {
        expression(as_for_stmnt->looped);
        assert(as_for_stmnt->looped->evaluates_to.has_value());
        auto looped_type = as_for_stmnt->looped->evaluates_to.value();
        scanner::Token looper_name = as_for_stmnt->looper.second;
        if (looped_type->cls->name.raw != "array") {
            throw handle_error(looper_name, "Looper can only be constructed from an array.");
        }
        auto looper_type = process_templated(as_for_stmnt->looper.first);
        auto looped_item_type = looped_type->templates[0];
        if (*looped_item_type != *looper_type) { // TODO: Check if variable can be casted
            throw handle_error(as_for_stmnt->looper.first->name, "Incorrect looper type (should be '" + looped_item_type->cls->pretty_name + "'.");
        }
        current_function_scopes.emplace_back();
        current_function_scopes.back()[looper_name.raw] = std::make_shared<MemberDefinition>(looper_type, looper_name, std::optional<std::shared_ptr<parser::Expr>>(), false, false, false);
        auto ret_type = scope(as_for_stmnt->to_run);
        current_function_scopes.pop_back();
        return ret_type;
    }
    throw std::exception();
}

auto cblang::compiler::StaticAnalyzer::expression(const std::shared_ptr<parser::Expr>& expr, bool must_evaluate) -> void {
    auto as_binary = std::dynamic_pointer_cast<parser::Binary>(expr);
    if (as_binary) {
        as_binary->evaluates_to = binary(as_binary->left, as_binary->op, as_binary->right);
        return;
    }
    auto as_logical = std::dynamic_pointer_cast<parser::Logical>(expr);
    if (as_logical) {
        as_logical->evaluates_to = binary(as_logical->left, as_logical->op, as_logical->right);
        return;
    }
    auto as_grouping = std::dynamic_pointer_cast<parser::Grouping>(expr);
    if (as_grouping) {
        expression(as_grouping->expression);
        as_grouping->evaluates_to = as_grouping->expression->evaluates_to;
        return;
    }
    auto as_literal = std::dynamic_pointer_cast<parser::Literal>(expr);
    if (as_literal) {
        auto literal = as_literal->literal;
        auto as_bool = std::dynamic_pointer_cast<scanner::BoolLiteral>(literal);
        auto as_int = std::dynamic_pointer_cast<scanner::IntLiteral>(literal);
        auto as_float = std::dynamic_pointer_cast<scanner::FloatLiteral>(literal);
        auto as_char = std::dynamic_pointer_cast<scanner::CharLiteral>(literal);
        auto as_string = std::dynamic_pointer_cast<scanner::StringLiteral>(literal);
        if (as_bool) { as_literal->evaluates_to = std::make_shared<TemplatedType>(BoolDefinition::generate()); }
        if (as_int) { as_literal->evaluates_to = std::make_shared<TemplatedType>(IntDefinition::generate()); }
        if (as_float) { as_literal->evaluates_to = std::make_shared<TemplatedType>(FloatDefinition::generate()); }
        if (as_char) { as_literal->evaluates_to = std::make_shared<TemplatedType>(CharDefinition::generate()); }
        if (as_string) { as_literal->evaluates_to = std::make_shared<TemplatedType>(StringDefinition::generate()); }
        if (!as_literal->evaluates_to.has_value()) {
            throw handle_error(as_literal->token, "(during static analysis) Unkown or base type Literal.");
        }
        return;
    }
    auto as_unary = std::dynamic_pointer_cast<parser::Unary>(expr);
    if (as_unary) {
        expression(as_unary->right);
        assert(as_unary->right->evaluates_to.has_value());
        auto def = as_unary->right->evaluates_to.value();
        if (!def->cls->members_by_name.contains(as_unary->op.raw)) {
            throw handle_error(as_unary->op, "(during static analysis) Object of type " + def->stringify() + " has no operator overload for '" + as_unary->op.raw + "'.");
        }
        auto func = std::dynamic_pointer_cast<FunctionMember>(def->cls->members_by_name.at(as_unary->op.raw)); // Should always succeed
        func->validate_call(as_unary->op, {}, {});
        if (!func->returns.has_value()) {
            throw handle_error(func->function_name, "(during static analysis) Operator functions must return a value.");
        }
        as_unary->evaluates_to = func->returns.value();
        return;
    }
    auto as_scope_expr = std::dynamic_pointer_cast<parser::ScopeExpr>(expr);
    if (as_scope_expr) {
        for (const auto& stmnt : as_scope_expr->statements) {
            statement(stmnt);
        }
        as_scope_expr->evaluates_to = std::make_shared<TemplatedType>(std::make_shared<FunctionDefinition>());
        return;
    }
    auto as_array_expr = std::dynamic_pointer_cast<parser::ArrayExpr>(expr);
    if (as_array_expr) {
        std::shared_ptr<TemplatedType> contained_type = nullptr;
        for (const auto& item_expr : as_array_expr->items) {
            expression(item_expr);
            assert(item_expr->evaluates_to.has_value());
            if (!contained_type) {
                contained_type = item_expr->evaluates_to.value();
            }
            if (*item_expr->evaluates_to.value() != *contained_type) {
                throw handle_error(as_array_expr->start_point, "(during static analysis) Array literal does not contain objects of a consistent type.");
            }
        }
        if (!contained_type) {
            throw handle_error(as_array_expr->start_point, "(during static analysis) Cannot deduce type of empty array, use array<type>() constructor instead.");
        }
        as_array_expr->evaluates_to = TemplatedType::generate(std::make_shared<ArrayDefinition>(TemplateDefinition::generate(contained_type)), {contained_type});
        return;
    }
    auto as_accessible = std::dynamic_pointer_cast<parser::Accessible>(expr);
    if (as_accessible) {
        accessible(as_accessible, {}, must_evaluate);
        return;
    }
    assert(false);
}

auto cblang::compiler::StaticAnalyzer::accessible(const std::shared_ptr<parser::Accessible>& item, const std::optional<std::shared_ptr<ClassDefinition>>& access_from, bool must_evaluate) -> void {
    auto as_call_expr = std::dynamic_pointer_cast<parser::CallExpr>(item);
    if (as_call_expr) {
        std::shared_ptr<MemberDefinition> as_member = nullptr;
        scanner::Token func_name = as_call_expr->name->name;
        if (access_from) {
            if (!access_from.value()->members_by_name.contains(func_name.raw)) {
                throw handle_error(func_name, "(during static analysis) No function named '" + func_name.raw + " exists in class '" + access_from.value()->pretty_name + "'.");
            }
            as_member = access_from.value()->members_by_name.at(func_name.raw);
        }
        else {
            as_member = assert_function_exists(func_name);
        }
        auto function = std::dynamic_pointer_cast<FunctionMember>(as_member);
        if (!function) {
            throw handle_error(func_name, "(during static analysis) Expected a function to call but found a variable instead.");
        }
        std::vector<std::shared_ptr<TemplateDefinition>> passed_templates;
        for (const auto& templated : as_call_expr->name->templates) {
            passed_templates.push_back(std::make_shared<TemplateDefinition>(process_templated(templated)));
        }
        std::vector<std::shared_ptr<TemplatedType>> argument_types;
        for (const auto& arg : as_call_expr->args) {
            expression(arg);
            assert(arg->evaluates_to.has_value());
            argument_types.push_back(arg->evaluates_to.value());
        }
        function->validate_call(func_name, passed_templates, argument_types);
        if (must_evaluate && !function->returns.has_value()) {
            throw handle_error(func_name, "(during static analysis) This function must return a value.");
        }
        as_call_expr->evaluates_to = function->returns;
        return;
    }
    auto as_var_expr = std::dynamic_pointer_cast<parser::VarExpr>(item);
    if (as_var_expr) {
        std::shared_ptr<MemberDefinition> as_member = nullptr;
        scanner::Token var_name = as_var_expr->name;
        if (access_from) {
            if (!access_from.value()->members_by_name.contains(var_name.raw)) {
                throw handle_error(var_name, "(during static analysis) No function named '" + var_name.raw + " exists in class '" + access_from.value()->pretty_name + "'.");
            }
            as_member = access_from.value()->members_by_name.at(var_name.raw);
        }
        else {
            as_member = assert_var_exists(var_name);
        }
        as_var_expr->evaluates_to = as_member->type;
        return;
    }
    assert(false);
}

auto cblang::compiler::StaticAnalyzer::binary(const std::shared_ptr<parser::Expr>& left, const scanner::Token& oper, const std::shared_ptr<parser::Expr>& right) -> std::shared_ptr<TemplatedType> {
    expression(left);
    expression(right);
    assert(left->evaluates_to.has_value());
    assert(right->evaluates_to.has_value());
    auto left_def = left->evaluates_to.value();
    auto right_def = right->evaluates_to.value();
    if (!left_def->cls->members_by_name.contains(oper.raw)) {
        throw handle_error(oper, "(during static analysis) Object of type " + left_def->stringify() + " has no operator overload for '" + oper.raw + "'.");
    }
    auto func = std::dynamic_pointer_cast<FunctionMember>(left_def->cls->members_by_name.at(oper.raw)); // Should always succeed
    func->validate_call(oper, {}, {right_def});
    if (!func->returns.has_value()) {
        throw handle_error(func->function_name, "(during static analysis) Operator functions must return a value.");
    }
    return func->returns.value();
}

auto cblang::compiler::StaticAnalyzer::assert_function_exists(const scanner::Token& name) const -> std::shared_ptr<FunctionMember> {
    auto as_function = std::dynamic_pointer_cast<FunctionMember>(assert_var_exists(name));
    if (!as_function) {
        throw handle_error(name, "(during static analysis) Expected '" + name.raw + "' to be a function but found a variable.");
    }
    return as_function;
}

auto cblang::compiler::StaticAnalyzer::assert_var_exists(const scanner::Token& name) const -> std::shared_ptr<MemberDefinition> {
    for (const auto & scope : std::ranges::reverse_view(current_function_scopes)) {
         if (!scope.contains(name.raw)) {
            continue;
        }
        return scope.at(name.raw);
    }
    if (current_class.contains(name.raw)) {
        return current_class.at(name.raw);
    }
    if (main_scope.contains(name.raw)) {
        return main_scope.at(name.raw);
    }
    throw handle_error(name, "(during static analysis) Variable " + name.raw + " not found in the current scope.");
}

auto cblang::compiler::StaticAnalyzer::assert_class_exists(const scanner::Token& name) const -> std::shared_ptr<ClassDefinition> {
    if (defined_classes.contains(name.raw)) {
        return defined_classes.at(name.raw);
    }
    throw handle_error(name, "(during static analysis) Class does not exist.");
}

auto cblang::compiler::StaticAnalyzer::process_templated(const std::shared_ptr<parser::Templated>& input) const -> std::shared_ptr<TemplatedType> {
    auto cls = assert_class_exists(input->name);
    std::vector<std::shared_ptr<TemplatedType>> templates;
    for (const auto& templated : input->templates) {
        templates.push_back(process_templated(templated));
    }
    return std::make_shared<TemplatedType>(cls, templates);
}