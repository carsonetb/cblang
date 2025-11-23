#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

namespace cblang::definitions {
    class Object;
    class ClassDefinition;
    class TemplateDefinition;
    class Scope;

    class MemberDefinition {
        public:
            MemberDefinition(std::string name);
            virtual ~MemberDefinition();

            std::string name;
            std::shared_ptr<ClassDefinition> type;

            bool is_static = false;
            bool is_private = false;
            bool is_const = false;
            bool has_initializer = false;
    };

    class ParameterDefinition : public MemberDefinition {
        public:
            ParameterDefinition(std::string name);
    };

    class FunctionDefinition : public MemberDefinition {
        public:
            FunctionDefinition(
                std::string name, 
                std::vector<std::shared_ptr<ParameterDefinition>> p_parameters, 
                std::vector<std::shared_ptr<TemplateDefinition>> p_templates, 
                std::shared_ptr<ClassDefinition> p_returns
            );

            std::vector<std::shared_ptr<ParameterDefinition>> parameters;
            std::unordered_map<std::string, std::shared_ptr<TemplateDefinition>> templates_by_name;
            std::vector<std::shared_ptr<TemplateDefinition>> templates;
            std::shared_ptr<ClassDefinition> returns;

            auto validate_call(std::vector<std::shared_ptr<TemplateDefinition>> templates, std::vector<std::shared_ptr<ClassDefinition>> args) -> bool;
    };

    class ClassDefinition {
        public:
            ClassDefinition();
            virtual ~ClassDefinition();

            bool invalid = false;

            std::string type_name;
            std::string templated_type_name;
            std::vector<std::shared_ptr<MemberDefinition>> params;
            std::unordered_map<std::string, std::shared_ptr<MemberDefinition>> members;
            std::unordered_map<std::string, std::shared_ptr<TemplateDefinition>> templates_by_name;
            std::vector<std::shared_ptr<TemplateDefinition>> templates;

            virtual auto is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool = 0;
            virtual auto can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool = 0;
    };

    class UserDefinition : public ClassDefinition {
        public:
            UserDefinition();
            UserDefinition(
                std::string name, 
                const std::vector<std::shared_ptr<MemberDefinition>>& p_members, 
                std::vector<std::shared_ptr<TemplateDefinition>> p_templates
            );

            auto is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool override;
            auto can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool override;
    };

    class BoolDefinition : public ClassDefinition {
        public:
            BoolDefinition();

            auto is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool override;
            auto can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool override;
    };

    class IntDefinition : public ClassDefinition {
        public:
            IntDefinition();

            auto is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool override;
            auto can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool override;
    };

    class CharDefinition : public ClassDefinition {
        public:
            CharDefinition();

            auto is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool override;
            auto can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool override;
    };

    class StringDefinition : public ClassDefinition {
        public:
            StringDefinition();

            auto is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool override;
            auto can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool override;
    };

    class ArrayDefinition : public ClassDefinition {
        public:
            ArrayDefinition();

            auto is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool override;
            auto can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool override;
    };

    class TemplateDefinition : public ClassDefinition {
        public:
            TemplateDefinition(std::string template_title);

            std::shared_ptr<ClassDefinition> template_used = nullptr;

            auto is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool override;
            auto can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool override;
    };
}