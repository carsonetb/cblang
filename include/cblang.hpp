#pragma once

#include "definitions.hpp"
#include "program.hpp"
#include "scanner.hpp"

#include <cstdint>
#include <memory>
#include <utility>

namespace cblang::compiler {
    using StaticScope = std::unordered_map<std::string, std::shared_ptr<definitions::MemberDefinition>>;
    using StaticClassScope = std::unordered_map<std::string, std::shared_ptr<definitions::ClassDefinition>>;
}

namespace cblang::definitions {
    class TemplateDefinition;
    class ClassDefinition;
}

namespace cblang {
    using namespace definitions;

    #ifdef _WIN32
        #define CBLANG_EXPORT __declspec(dllexport)
    #else
        #define CBLANG_EXPORT
    #endif

    struct ParseError {
        enum class ErrorType : uint8_t {
            EXPECTED_KEYWORD,
            UNKNOWN_TYPE,
            EOF_ERROR,
            TOO_MANY_ARGUMENTS,
            INVALID_TYPE,
        };

        ParseError(const scanner::Token& keyword, ErrorType error_type, std::string p_message) 
            : line(keyword.line), column(keyword.column), message(std::move(p_message)) 
        {}

        int line;
        int column;
        std::string message;
    };

    CBLANG_EXPORT auto init(bool verbose = false) -> void;
    CBLANG_EXPORT auto enable_verbose_logs() -> void;
    CBLANG_EXPORT auto cblang_parse_code(const std::string& code, const std::optional<compiler::StaticScope>& global_scope = {}, const std::optional<compiler::StaticClassScope>& global_classes = {}) -> std::optional<std::shared_ptr<program::Program>>;
}