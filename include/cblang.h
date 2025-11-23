#pragma once

#include "definitions.h"
#include "lexical.h"

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace cblang {
    using namespace definitions;

    #ifdef _WIN32
        #define CBLANG_EXPORT __declspec(dllexport)
    #else
        #define CBLANG_EXPORT
    #endif

    class TemplateDefinition;
    class ClassDefinition;

    struct ParseError {
        enum class ErrorType : uint8_t {
            EXPECTED_KEYWORD,
            UNKOWN_TYPE,
            EOF_ERROR,
        };

        ParseError(const lexical::Keyword& keyword, ErrorType error_type, std::string p_message) 
            : line(keyword.line), column(keyword.column), message(std::move(p_message)) 
        {}

        int line;
        int column;
        std::string message;
    };

    class Program {
        public:
            std::vector<ParseError> errors;
            std::shared_ptr<UserDefinition> main_class;
    };

    CBLANG_EXPORT auto init(bool verbose = false) -> void;
    CBLANG_EXPORT auto enable_verbose_logs() -> void;
    CBLANG_EXPORT auto cblang_parse_code(const std::string& code) -> Program;
}