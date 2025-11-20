#include "cblang.h"

#include "lexical.h"

#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

using namespace cblang;

static bool initialized = false;
static std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_mt("cblang");

auto cblang::init(bool verbose) -> void {
    logger->set_pattern("[%Y-%m-%d %H:%M:%S] [cblang] %^[%l] %v %$");
    logger->set_level(spdlog::level::warn);

    lexical::init(verbose);

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
    auto lexical_parse_out = lexical::parse(code, lexical::KeywordMap::default_map());
    logger->info("Finished parsing code, exiting.");
    return {};
}