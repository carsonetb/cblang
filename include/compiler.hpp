#pragma once 

#include "cblang.hpp"
#include "definitions.hpp"
#include "parser.hpp"
#include "scanner.hpp"
#include <exception>
#include <memory>
#include <vector>

namespace cblang::compiler {
    auto init(bool verbose = false) -> void;
    auto enable_verbose_logs() -> void;

    class CompileException : std::exception {};

    class Compiler {
        public:
            Compiler(std::shared_ptr<parser::ParsedProgram> p_source) : source(std::move(p_source)) {}

            auto compile() -> Program;

        private:
            std::unordered_map<std::string, std::shared_ptr<definitions::ClassDefinition>> defined_classes = {
                {"bool", std::make_shared<BoolDefinition>()},
                {"int", std::make_shared<IntDefinition>()},
                {"char", std::make_shared<CharDefinition>()},
                {"string", std::make_shared<StringDefinition>()},
                {"array", std::make_shared<ArrayDefinition>()},
            };

            std::shared_ptr<parser::ParsedProgram> source;

            auto get_class(const scanner::Token& token, const std::string& name) const -> std::shared_ptr<ClassDefinition>;

            auto main() -> std::shared_ptr<UserDefinition>;
            auto process_class(const std::shared_ptr<parser::Class>& input) -> std::shared_ptr<ClassDefinition>;
            auto process_params(const parser::Parameters& input) -> std::vector<std::shared_ptr<MemberDefinition>>;
            auto class_members(const std::vector<std::shared_ptr<parser::Declaration>>& inputs) -> std::vector<std::shared_ptr<MemberDefinition>>;
            auto class_templates(const std::shared_ptr<parser::Class>& input) -> std::vector<std::shared_ptr<TemplateDefinition>>;
            auto process_templated(const std::shared_ptr<parser::Templated>& input) -> std::shared_ptr<TemplatedType>;
    };
}