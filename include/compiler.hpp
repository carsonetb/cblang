#pragma once 

#include "cblang.hpp"
#include "definitions.hpp"
#include "parser.hpp"
#include <memory>
#include <vector>

namespace cblang::compiler {
    auto init(bool verbose = false) -> void;
    auto enable_verbose_logs() -> void;

    class Compiler {
        public:
            Compiler(std::shared_ptr<parser::ParsedProgram> p_source) : source(std::move(p_source)) {}

            auto compile() -> Program;

        private:
            std::unordered_map<std::string, std::shared_ptr<definitions::ClassDefinition>> defined_classes = {
                {"bool", std::make_shared<definitions::BoolDefinition>()},
                {"int", std::make_shared<definitions::StringDefinition>()},
                {"char", std::make_shared<definitions::CharDefinition>()},
                {"string", std::make_shared<definitions::StringDefinition>()},
                {"array", std::make_shared<definitions::ArrayDefinition>()},
            };

            std::shared_ptr<parser::ParsedProgram> source;

            auto main() -> std::shared_ptr<UserDefinition>;
            auto class_params(const parser::Parameters& input) -> std::vector<std::shared_ptr<MemberDefinition>>;
            auto class_members(const std::vector<std::shared_ptr<parser::Declaration>>& input) -> std::vector<std::shared_ptr<MemberDefinition>>;
            auto class_templates(const std::shared_ptr<parser::Class>& input) -> std::vector<std::shared_ptr<TemplateDefinition>>;
    };
}