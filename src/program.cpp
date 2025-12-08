#include "program.hpp"
#include "definitions.hpp"
#include "objects.hpp"
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