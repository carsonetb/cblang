#pragma once 

#include "cblang.hpp"
#include "definitions.hpp"
#include "parser.hpp"
#include "program.hpp"
#include "scanner.hpp"
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace cblang::compiler {
    auto init(bool verbose = false) -> void;
    auto enable_verbose_logs() -> void;

    class CompileException : std::exception {};

    auto handle_error(const scanner::Token& token, const std::string& error) -> CompileException;

    static std::unordered_map<std::string, std::shared_ptr<definitions::ClassDefinition>> basic_classes= {
        {"bool", BoolDefinition::generate()},
        {"int", IntDefinition::generate()},
        {"float", FloatDefinition::generate()},
        {"char", CharDefinition::generate()},
        {"string", StringDefinition::generate()},
        {"array", std::make_shared<ArrayDefinition>(TemplateDefinition::generate("value_type"))},
    };

    class Compiler {
        public:
            Compiler(std::shared_ptr<parser::ParsedProgram> p_source, std::optional<StaticScope> p_global_scope, std::optional<StaticClassScope> p_global_classes = {}) : source(std::move(p_source)), global_scope(std::move(p_global_scope)), global_classes(std::move(p_global_classes)) {}

            auto compile() -> std::optional<std::shared_ptr<program::Program>>;

        private:

            std::shared_ptr<parser::ParsedProgram> source;
            std::optional<StaticScope> global_scope;
            std::optional<StaticClassScope> global_classes;

            auto main() -> std::shared_ptr<definitions::UserDefinition>;
            auto process_class(const std::shared_ptr<parser::Class>& input) -> std::shared_ptr<ClassDefinition>;
            auto process_params(const parser::Parameters& input) -> std::vector<std::shared_ptr<MemberDefinition>>;
            auto class_members(const std::vector<std::shared_ptr<parser::Declaration>>& inputs) -> std::vector<std::shared_ptr<MemberDefinition>>;
            auto class_templates(const std::shared_ptr<parser::Class>& input) -> std::vector<std::shared_ptr<TemplateDefinition>>;
            auto process_templated(const std::shared_ptr<parser::Templated>& input) -> std::shared_ptr<TemplatedType>;
            [[nodiscard]] auto get_class(const scanner::Token& token, const std::string& name) const -> std::shared_ptr<ClassDefinition>;
    };

    using StaticScope = std::unordered_map<std::string, std::shared_ptr<MemberDefinition>>;
    using StaticTemplateScope = std::unordered_map<std::string, std::shared_ptr<TemplateDefinition>>;
    using StaticClassScope = std::unordered_map<std::string, std::shared_ptr<ClassDefinition>>;

    class StaticAnalyzer {
        public:
            StaticAnalyzer(std::shared_ptr<program::Program> p_source, const std::optional<StaticScope>& global_scope, const std::optional<StaticClassScope>& global_classes = {}) : source(std::move(p_source)) {
                if (global_scope.has_value()) {
                    main_scope = global_scope.value();
                }
                if (global_classes.has_value()) {
                    for (const auto& [name, cls] : global_classes.value()) {
                        defined_classes[name] = cls;
                    }
                }
            }

            auto perform_analysis() -> int;
        
        private:
            std::shared_ptr<program::Program> source;

            std::unordered_map<std::string, std::shared_ptr<ClassDefinition>> defined_classes = {
                {"bool", BoolDefinition::generate()},
                {"int", IntDefinition::generate()},
                {"float", FloatDefinition::generate()},
                {"char", CharDefinition::generate()},
                {"string", StringDefinition::generate()},
                {"array", std::make_shared<ArrayDefinition>(TemplateDefinition::generate("value_type"))},
            };
            StaticScope main_scope;
            StaticScope current_class;
            StaticTemplateScope current_class_templates;
            std::vector<StaticScope> current_function_scopes;
            std::vector<StaticTemplateScope> current_function_templates;
            std::optional<std::shared_ptr<TemplatedType>> function_returns;

            auto init_members(const std::shared_ptr<definitions::UserDefinition>& definition, StaticScope& scope) -> void;
            auto analyze_class(const std::shared_ptr<definitions::UserDefinition>& definition) -> void;
            auto function(const std::shared_ptr<FunctionMember>& func) -> void;
            auto scope(const std::vector<std::shared_ptr<parser::Statement>>& statements) -> std::optional<std::shared_ptr<TemplatedType>>;
            auto statement(const std::shared_ptr<parser::Statement>& stmnt) -> std::optional<std::shared_ptr<TemplatedType>>;
            auto expression(const std::shared_ptr<parser::Expr>& expr, bool must_evaluate = true) -> void;
            auto accessible(const std::shared_ptr<parser::Accessible>& item, const std::optional<std::shared_ptr<ClassDefinition>>& access_from = {}, bool must_evaluate = true) -> void;
            auto binary(const std::shared_ptr<parser::Expr>& left, const scanner::Token& oper, const std::shared_ptr<parser::Expr>& right) -> std::shared_ptr<TemplatedType>;
            auto assert_function_exists(const scanner::Token& name) const -> std::shared_ptr<FunctionMember>;
            auto assert_var_exists(const scanner::Token& name) const -> std::shared_ptr<MemberDefinition>;
            auto assert_class_exists(const scanner::Token& name) const -> std::shared_ptr<ClassDefinition>;
            auto process_templated(const std::shared_ptr<parser::Templated>& input) const -> std::shared_ptr<TemplatedType>;
    };
}