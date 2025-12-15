#include "compiler.hpp"
#include "cblang.hpp"
#include "definitions.hpp"
#include "parser.hpp"
#include "program.hpp"
#include "scanner.hpp"
#include "util.hpp"

#include <memory>
#include <optional>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
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
    logger->error("[line " + std::to_string(token.line) + "] [token " + (token.type == scanner::END_OF_FILE ? "EOF" : token.raw) + "] " + error);
    return {};
}

auto cblang::compiler::Compiler::get_class(const scanner::Token& token, const std::string& name) const -> std::shared_ptr<ClassDefinition> {
    if (defined_classes.contains(name)) {
        return defined_classes.at(name);
    }
    throw handle_error(token, "Class '" + name + "' not defined yet.");  
}

auto cblang::compiler::Compiler::compile() -> std::shared_ptr<program::Program> {
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
            std::optional<std::shared_ptr<ClassDefinition>> returns;
            if (as_function->returns) {
                returns = get_class(as_function->returns.value(), as_function->returns->raw);
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

auto cblang::compiler::StaticAnalyzer::perform_analysis() -> int {
    return 0;
}

auto cblang::compiler::StaticAnalyzer::analyze_class(const std::shared_ptr<UserDefinition>& definition) -> void {

}

auto cblang::compiler::StaticAnalyzer::function(const std::shared_ptr<FunctionMember>& func) -> void {

}

auto cblang::compiler::StaticAnalyzer::statement(const std::shared_ptr<parser::Statement>& stmnt) -> void {

}

auto cblang::compiler::StaticAnalyzer::expression(const std::shared_ptr<parser::Expr>& expr) -> void {

}

auto cblang::compiler::StaticAnalyzer::assert_function_exists(const scanner::Token& name) const -> std::shared_ptr<FunctionMember> {
    auto as_function = std::dynamic_pointer_cast<FunctionMember>(assert_var_exists(name));
    if (!as_function) {
        throw handle_error(name, "(during static analysis) Expected '" + name.raw + "' to be a function but found a variable.");
    }
    return as_function;
}

auto cblang::compiler::StaticAnalyzer::assert_var_exists(const scanner::Token& name) const -> std::shared_ptr<MemberDefinition> {
    for (unsigned long i = current_function_scopes.size() - 1; i >= 0; i--) {
        const auto& scope = current_function_scopes[i];
        if (!scope.defined_variables.contains(name.raw)) {
            continue;
        }
        return scope.defined_variables.at(name.raw);
    }
    if (current_class.defined_variables.contains(name.raw)) {
        return current_class.defined_variables.at(name.raw);
    }
    if (main_scope.defined_variables.contains(name.raw)) {
        return main_scope.defined_variables.at(name.raw);
    }
    throw handle_error(name, "(during static analysis) Variable " + name.raw + " not found in the current scope.");
}