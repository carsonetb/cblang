#pragma once

#include "definitions.h"

#include <cstdint>
#include <memory>
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
        enum : uint8_t {
            UNKOWN_TYPE,
        };

        int line;
        int column;

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