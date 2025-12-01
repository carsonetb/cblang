#include "compiler.hpp"
#include "cblang.hpp"
#include "definitions.hpp"
#include "parser.hpp"

#include <memory>
#include <optional>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <string>
#include <sys/types.h>
#include <vector>

#define COMPILE_ERROR(type, message) out.errors.emplace_back(keyword, ParseError::ErrorType::type, message)

using namespace cblang::compiler;
using namespace cblang;

static bool initialized = false;
static std::shared_ptr<spdlog::logger> logger = spdlog::stderr_color_st("cblang::compiler");

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

auto cblang::compiler::Compiler::compile() -> Program {
    logger->info("Compiler started.");

    Program out;

    out.main_class = main();

    logger->info("Compiler finished successfully.");

    return out;
}

auto cblang::compiler::Compiler::main() -> std::shared_ptr<definitions::UserDefinition> {
    auto members = class_members(source->members);
    auto params = class_params(source->parameters);
    for (const auto& param : params) {
        members.push_back(param);
    }
    std::vector<std::shared_ptr<definitions::TemplateDefinition>> templates;
    return std::make_shared<definitions::UserDefinition>("Main", params, members, templates);
}

auto cblang::compiler::Compiler::class_params(const parser::Parameters& input) -> std::vector<std::shared_ptr<MemberDefinition>> {
    std::vector<std::shared_ptr<MemberDefinition>> out;
    for (const auto& param : input) {
        auto type = process_templated(param.first);
        auto name = param.second.raw;
        std::optional<std::shared_ptr<parser::Expr>> expression; // Expressions need to be supported in Parameters!!!
        out.push_back(std::make_shared<MemberDefinition>(type, name, expression));
    }
    return out;
}