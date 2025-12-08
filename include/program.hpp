#pragma once

#include "definitions.hpp"
#include "objects.hpp"
#include <exception>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace cblang::program {
    struct Program {
        Program(std::shared_ptr<definitions::UserDefinition> p_main_class);

        bool valid = false;
        std::shared_ptr<definitions::UserDefinition> main_class;

        [[nodiscard]] auto get_functions() const -> std::vector<std::shared_ptr<definitions::FunctionMember>>;
    };

    struct Scope {
        std::unordered_map<std::string, std::shared_ptr<objects::Variable>> defined_variables;
    };

    class RuntimeException : std::exception {};

    auto init(bool verbose = false) -> void;
    auto enable_verbose_logs() -> void;
    auto run_function(std::shared_ptr<definitions::UserDefinition> run_on, std::shared_ptr<definitions::FunctionMember> to_run) -> std::optional<objects::Object>;
}