#include "compiler.hpp"
#include "cblang.hpp"
#include "definitions.hpp"
#include "parser.hpp"
#include "scanner.hpp"

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

static auto handle_error(const scanner::Token& token, const std::string& error) -> CompileException {
    logger->error("[line " + std::to_string(token.line) + "] [token " + (token.type == scanner::END_OF_FILE ? "EOF" : token.raw) + "] " + error);
    return {};
}

static auto process_templated_definition(const std::shared_ptr<parser::Templated>& input) -> std::vector<std::shared_ptr<TemplateDefinition>> {
    std::vector<std::shared_ptr<TemplateDefinition>> out;
    for (const auto& templdef : input->templates) {
        out.push_back(std::make_shared<TemplateDefinition>(templdef->name.raw));
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

auto cblang::compiler::Compiler::get_class(const scanner::Token& token, const std::string& name) const -> std::shared_ptr<ClassDefinition> {
    if (defined_classes.contains(name)) {
        return defined_classes.at(name);
    }
    throw handle_error(token, "Class '" + name + "' not defined yet.");  
}

auto cblang::compiler::Compiler::compile() -> Program {
    logger->info("Compiler started.");

    Program out;

    try {
        out.main_class = main();
    }
    catch (CompileException exception) {
        logger->error("Errors compiling.");
        return {};
    }

    logger->info("Compiler finished successfully.");
    out.valid = true;

    return out;
}

auto cblang::compiler::Compiler::main() -> std::shared_ptr<definitions::UserDefinition> {
    auto members = class_members(source->members);
    auto params = process_params(source->parameters);
    for (const auto& param : params) {
        members.push_back(param);
    }
    std::vector<std::shared_ptr<definitions::TemplateDefinition>> templates;
    return std::make_shared<definitions::UserDefinition>("Main", params, members, templates);
}

auto cblang::compiler::Compiler::process_class(const std::shared_ptr<parser::Class>& input) -> std::shared_ptr<ClassDefinition> {
    auto templated_name = parser::debug_templates(input->name);
    auto params = process_params(input->params);
    auto members = class_members(input->members);
    for (const auto& param : params) {
        members.push_back(param);
    }
    auto templates = process_templated_definition(input->name);
    auto out = std::make_shared<ClassDefinition>(templated_name, params, members, templates);
    defined_classes[out->type_name] = out;
    return out;
}

auto cblang::compiler::Compiler::process_params(const parser::Parameters& input) -> std::vector<std::shared_ptr<MemberDefinition>> {
    std::vector<std::shared_ptr<MemberDefinition>> out;
    for (const auto& param : input) {
        auto type = process_templated(param.first);
        auto name = param.second.raw;
        std::optional<std::shared_ptr<parser::Expr>> expression; // Expressions need to be supported in Parameters!!!
        out.push_back(std::make_shared<MemberDefinition>(type, name, expression));
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
            std::string name = as_function->templated_name->name.raw;
            auto params = process_params(as_function->params);
            auto templates = process_templated_definition(as_function->templated_name);
            std::optional<std::shared_ptr<ClassDefinition>> returns;
            if (as_function->returns) {
                returns = get_class(as_function->returns.value(), as_function->returns->raw);
            }
            out.push_back(std::make_shared<FunctionMember>(name, params, templates, returns, as_function->body));
        }
        else if (as_variable) {
            std::string name = as_variable->name.raw;
            auto type = process_templated(as_variable->type);
            out.push_back(std::make_shared<MemberDefinition>(type, name, as_variable->value));
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
        templates.push_back(process_templated(input));
    }
    return std::make_shared<TemplatedType>(cls, templates);
}