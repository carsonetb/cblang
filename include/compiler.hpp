#pragma once 

#include "cblang.hpp"
#include "definitions.hpp"
#include "parser.hpp"
#include "program.hpp"
#include "scanner.hpp"
#include <exception>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace cblang::compiler {
    auto init(bool verbose = false) -> void;
    auto enable_verbose_logs() -> void;

    class CompileException : std::exception {};

    auto handle_error(const scanner::Token& token, const std::string& error) -> CompileException;

    class Compiler {
        public:
            Compiler(std::shared_ptr<parser::ParsedProgram> p_source) : source(std::move(p_source)) {}

            auto compile() -> std::shared_ptr<program::Program>;

        private:
            std::unordered_map<std::string, std::shared_ptr<definitions::ClassDefinition>> defined_classes = {
                {"bool", std::make_shared<BoolDefinition>()},
                {"int", std::make_shared<IntDefinition>()},
                {"char", std::make_shared<CharDefinition>()},
                {"string", std::make_shared<StringDefinition>()},
                {"array", std::make_shared<ArrayDefinition>()},
            };

            std::shared_ptr<parser::ParsedProgram> source;

            auto main() -> std::shared_ptr<UserDefinition>;
            auto process_class(const std::shared_ptr<parser::Class>& input) -> std::shared_ptr<ClassDefinition>;
            auto process_params(const parser::Parameters& input) -> std::vector<std::shared_ptr<MemberDefinition>>;
            auto class_members(const std::vector<std::shared_ptr<parser::Declaration>>& inputs) -> std::vector<std::shared_ptr<MemberDefinition>>;
            auto class_templates(const std::shared_ptr<parser::Class>& input) -> std::vector<std::shared_ptr<TemplateDefinition>>;
            auto process_templated(const std::shared_ptr<parser::Templated>& input) -> std::shared_ptr<TemplatedType>;
            [[nodiscard]] auto get_class(const scanner::Token& token, const std::string& name) const -> std::shared_ptr<ClassDefinition>;
    };

    struct StaticScope {
        std::unordered_map<std::string, std::shared_ptr<MemberDefinition>> defined_variables;
        std::unordered_map<std::string, std::shared_ptr<ClassDefinition>> defined_classes;
    };

    class StaticAnalyzer {
        public:
            StaticAnalyzer(std::shared_ptr<program::Program> p_source) : source(std::move(p_source)) {}

            auto perform_analysis() -> int;
        
        private:
            std::shared_ptr<program::Program> source;

            StaticScope main_scope;
            StaticScope current_class;
            std::vector<StaticScope> current_function_scopes;

            auto analyze_class(const std::shared_ptr<UserDefinition>& definition) -> void;
            auto function(const std::shared_ptr<FunctionMember>& func) -> void;
            auto statement(const std::shared_ptr<parser::Statement>& stmnt) -> void;
            auto expression(const std::shared_ptr<parser::Expr>& expr) -> void;
            auto assert_function_exists(const scanner::Token& name) const -> std::shared_ptr<FunctionMember>;
            auto assert_var_exists(const scanner::Token& name) const -> std::shared_ptr<MemberDefinition>;
    };
}