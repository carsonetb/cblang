#pragma once

#include "definitions.hpp"
#include "objects.hpp"
#include "scanner.hpp"
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
        std::unordered_map<std::string, std::shared_ptr<definitions::TemplateDefinition>> defined_templates;
        std::unordered_map<std::string, std::shared_ptr<definitions::ClassDefinition>> defined_classes;
    };

    class RuntimeException : std::exception {
        public:
            RuntimeException(std::string p_error) : error_text(std::move(p_error)) {}

            std::string error_text;
    };

    class ScopeParser {
        public:
            ScopeParser(std::vector<std::shared_ptr<parser::Statement>> p_code, std::vector<std::shared_ptr<program::Scope>> p_scope, bool p_returnable = true);

            auto process() -> std::optional<std::shared_ptr<objects::Object>>;

        private:
            std::vector<std::shared_ptr<parser::Statement>> code;
            std::vector<std::shared_ptr<program::Scope>> scope;
            bool returnable;

            static auto construct_literal(const scanner::Token& token) -> std::shared_ptr<objects::Object>;
            auto statement(const std::shared_ptr<parser::Statement>& statement) -> std::optional<std::shared_ptr<objects::Object>>;
            auto expression(const std::shared_ptr<parser::Expr>& expr) -> std::shared_ptr<objects::Object>;
            auto accessible(const std::shared_ptr<parser::Accessible>& var) -> std::shared_ptr<objects::Object>;
            auto set_var(const std::shared_ptr<parser::SetVar>& statement) -> void;
            auto create_var(const std::shared_ptr<parser::CreateVar>& statement) -> void;
            [[nodiscard]] auto get_class(const std::shared_ptr<parser::Templated>& templated_class) const -> std::shared_ptr<definitions::TemplatedType>;
            [[nodiscard]] auto get_class_definition(const std::string& name) const -> std::optional<std::shared_ptr<definitions::ClassDefinition>>;
            [[nodiscard]] auto get_variable(const std::string& name) const -> std::optional<std::shared_ptr<objects::Variable>>;
            [[nodiscard]] auto binary_operator(const std::shared_ptr<objects::Object>& lhs, const scanner::Token& oper, const std::shared_ptr<objects::Object>& rhs) const -> std::shared_ptr<objects::Object>;
            [[nodiscard]] auto unary_operator(const scanner::Token& oper, const std::shared_ptr<objects::Object>& rhs) -> std::shared_ptr<objects::Object>;
    };

    auto init(bool verbose = false) -> void;
    auto enable_verbose_logs() -> void;
    auto run_function(std::shared_ptr<definitions::UserDefinition> run_on, std::shared_ptr<definitions::FunctionMember> to_run) -> std::optional<objects::Object>;
    auto handle_error(const scanner::Token& token, const std::string& error) -> RuntimeException;
    auto handle_error(const std::string &error) -> RuntimeException;
}