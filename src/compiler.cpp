#include "compiler.h"
#include "cblang.h"

#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>

using namespace cblang::compiler;

static bool initialized = false;
static std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_st("cblang::compiler");

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

auto cblang::compiler::compile(const std::vector<lexical::Keyword> &keywords) -> Program {
    logger->info("Compiler started.");

    logger->info("Compiler finished.");

    return {};
}