#pragma once

#include "parser.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace cblang::definitions {
    class Object;
    class ClassDefinition;
    class TemplateDefinition;
    class Scope;
    
    enum class LiteralType : uint8_t {
        BOOL,
        INT,
        STRING,
        CHAR,
        ARRAY,
        INVALID,
    };

    struct TemplatedType {
        TemplatedType(
            std::shared_ptr<ClassDefinition> p_cls,
            std::vector<std::shared_ptr<TemplatedType>> p_templates
        ) : cls(std::move(p_cls)), templates(std::move(p_templates)) {}

        std::shared_ptr<ClassDefinition> cls;
        std::vector<std::shared_ptr<TemplatedType>> templates;
    };

    class MemberDefinition {
        public:
            MemberDefinition(
                std::shared_ptr<TemplatedType> type,
                std::string name,
                std::optional<std::shared_ptr<parser::Expr>> initializer
            );
            virtual ~MemberDefinition();

            std::string name;
            std::shared_ptr<TemplatedType> type;
            std::optional<std::shared_ptr<parser::Expr>> initializer;

            bool is_static = false;
            bool is_private = false;
            bool is_const = false;
    };

    class FunctionMember : public MemberDefinition {
        public:
            FunctionMember(
                std::string name, 
                std::vector<std::shared_ptr<MemberDefinition>> p_parameters, 
                std::vector<std::shared_ptr<TemplateDefinition>> p_templates, 
                std::optional<std::shared_ptr<ClassDefinition>> p_returns
            );

            std::vector<std::shared_ptr<MemberDefinition>> parameters;
            std::unordered_map<std::string, std::shared_ptr<TemplateDefinition>> templates_by_name;
            std::vector<std::shared_ptr<TemplateDefinition>> templates;
            std::optional<std::shared_ptr<ClassDefinition>> returns;

            auto validate_call(std::vector<std::shared_ptr<TemplateDefinition>> templates, std::vector<std::shared_ptr<ClassDefinition>> args) -> bool;
    };

    class ClassDefinition : public MemberDefinition {
        public:
            ClassDefinition(
                std::string p_templated_type_name,
                std::vector<std::shared_ptr<MemberDefinition>> p_params,
                std::vector<std::shared_ptr<MemberDefinition>> p_members,
                std::vector<std::shared_ptr<TemplateDefinition>> p_templates
            );

            bool invalid = false;

            std::string type_name;
            std::string templated_type_name;
            std::vector<std::shared_ptr<MemberDefinition>> params;
            std::unordered_map<std::string, std::shared_ptr<MemberDefinition>> members_by_name;
            std::vector<std::shared_ptr<MemberDefinition>> members;
            std::unordered_map<std::string, std::shared_ptr<TemplateDefinition>> templates_by_name;
            std::vector<std::shared_ptr<TemplateDefinition>> templates;

            virtual auto is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool;
            virtual auto can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool;
    };

    class UserDefinition : public ClassDefinition {
        public:
            UserDefinition(
                const std::string& name, 
                const std::vector<std::shared_ptr<MemberDefinition>>& p_params,
                const std::vector<std::shared_ptr<MemberDefinition>>& p_members, 
                const std::vector<std::shared_ptr<TemplateDefinition>>& p_templates
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

    class FloatDefinition : public ClassDefinition {
        public:
            FloatDefinition();

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

    class FunctionDefinition : public ClassDefinition {
        public: 
            FunctionDefinition();

            auto is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool override;
            auto can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool override;
    };

    class TemplateDefinition  {
        public:
            TemplateDefinition(std::string p_template_name);

            std::string template_name;
            std::optional<std::shared_ptr<ClassDefinition>> template_used;
    };
}