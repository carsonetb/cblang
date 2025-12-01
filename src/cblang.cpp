#include "cblang.hpp"

#include "compiler.hpp"
#include "parser.hpp"
#include "scanner.hpp"

#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

using namespace cblang;

static bool initialized = false;
static std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_mt("cblang");

auto cblang::init(bool verbose) -> void {
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [cblang] %^[%l] %v %$");
    logger->set_level(spdlog::level::warn);

    cblang::scanner::init(verbose);
    cblang::compiler::init(verbose);
    cblang::parser::init(verbose);

    initialized = true;

    if (verbose) {
        enable_verbose_logs();
    }

    logger->info("Initialization complete!");
}

auto cblang::enable_verbose_logs() -> void {
    if (!initialized) {
        logger->error("cblang not initialized (call cblang::init)");
        return;
    }

    logger->set_level(spdlog::level::debug);
    logger->info("Verbose logs enabled.");
}

auto cblang::cblang_parse_code(const std::string& code) -> Program {
    if (!initialized) {
        logger->error("cblang not initialized (call cblang::init)");
        return {};
    }
    logger->info("Request to parse code, beginning.");
    scanner::Scanner scanner(code);
    auto tokens = scanner.scan_tokens();
    parser::Parser parser(tokens);
    auto program = parser.parse();
    if (!program) {
        logger->error("Error parsing code.");
        return {};
    }
    logger->info("Finished parsing code.");

    // if (compiler_out.errors.empty()) {
    //     logger->info("Compiled program has no errors.");
    // }
    // else {
    //     logger->error("Compiled program has errors! (dumping text, find a better way to log)");
    //     for (const auto& error : compiler_out.errors) {
    //         logger->error(error.message);
    //     }
    // }

    // return compiler_out;
    return {};
}