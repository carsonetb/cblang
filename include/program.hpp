#pragma once

#include "definitions.hpp"
#include "objects.hpp"
#include "parser.hpp"
#include "scanner.hpp"
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace cblang::program {
    struct Program {
        Program(std::shared_ptr<definitions::UserDefinition> p_main_class);

        bool valid = false;
        std::shared_ptr<definitions::UserDefinition> main_class;

        [[nodiscard]] auto create_object(const std::vector<std::shared_ptr<objects::Object>>& params) const -> std::optional<std::shared_ptr<objects::Object>>;
    };

    struct Scope {
        Scope(std::shared_ptr<objects::Object> p_scope_object) : scope_object(std::move(p_scope_object)) {}

        std::unordered_map<std::string, std::shared_ptr<objects::Variable>> defined_variables;
        std::unordered_map<std::string, std::shared_ptr<definitions::TemplateDefinition>> defined_templates;
        std::unordered_map<std::string, std::shared_ptr<definitions::ClassDefinition>> defined_classes;
        std::shared_ptr<objects::Object> scope_object;
    };

    class RuntimeException : std::exception {
        public:
            RuntimeException(std::string p_error) : error_text(std::move(p_error)) {}

            std::string error_text;
    };

    class ScopeParser {
        public:
            ScopeParser(std::vector<std::shared_ptr<parser::Statement>> p_code, std::vector<std::shared_ptr<program::Scope>> p_scope, bool p_returnable = true);
            ScopeParser(std::shared_ptr<parser::Expr> p_expr, std::vector<std::shared_ptr<program::Scope>> p_scope);

            auto process() -> std::optional<std::shared_ptr<objects::Object>>;
            auto process_expr() -> std::shared_ptr<objects::Object>;

        private:
            std::optional<std::vector<std::shared_ptr<parser::Statement>>> code;
            std::optional<std::shared_ptr<parser::Expr>> parse_expr;
            std::vector<std::shared_ptr<program::Scope>> scope;
            bool returnable;

            static auto construct_literal(const scanner::Token& token) -> std::shared_ptr<objects::Object>;
            static auto unary_operator(const scanner::Token& oper, const std::shared_ptr<objects::Object>& rhs) -> std::shared_ptr<objects::Object>;
            static auto binary_operator(const std::shared_ptr<objects::Object>& lhs, const scanner::Token& oper, const std::shared_ptr<objects::Object>& rhs) -> std::shared_ptr<objects::Object>;
            auto process_scope(const std::vector<std::shared_ptr<parser::Statement>>& statements) -> std::optional<std::shared_ptr<objects::Object>>;
            auto statement(const std::shared_ptr<parser::Statement>& statement) -> std::optional<std::shared_ptr<objects::Object>>;
            auto expression(const std::shared_ptr<parser::Expr>& expr) -> std::shared_ptr<objects::Object>;
            auto accessible(const std::shared_ptr<parser::Accessible>& var, std::optional<std::shared_ptr<objects::Object>> call_on = {}, bool must_evaluate = false) -> std::optional<std::shared_ptr<objects::Object>>;
            auto set_var(const std::shared_ptr<parser::SetVar>& statement) -> void;
            auto create_var(const std::shared_ptr<parser::CreateVar>& statement) -> void;
            [[nodiscard]] auto get_class(const std::shared_ptr<parser::Templated>& templated_class) const -> std::shared_ptr<definitions::TemplatedType>;
            [[nodiscard]] auto get_class_definition(const std::string& name) const -> std::optional<std::shared_ptr<definitions::ClassDefinition>>;
            [[nodiscard]] auto get_variable(const std::string& name) const -> std::optional<std::shared_ptr<objects::Variable>>;
    };

    auto init(bool verbose = false) -> void;
    auto enable_verbose_logs() -> void;
    auto handle_error(const scanner::Token& token, const std::string& error) -> RuntimeException;
    auto handle_error(const std::string &error) -> RuntimeException;
}