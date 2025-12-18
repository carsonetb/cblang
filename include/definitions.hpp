#pragma once

#include "scanner.hpp"
#include "parser.hpp"
#include <cstdint>
#include <optional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace cblang::objects {
    class Object;
}

namespace cblang::definitions {
    class ClassDefinition;
    class TemplateDefinition;
    class FunctionDefinition;
    
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

        TemplatedType(
            std::shared_ptr<ClassDefinition> p_cls
        ) : cls(std::move(p_cls)) {}

        std::shared_ptr<ClassDefinition> cls;
        std::vector<std::shared_ptr<TemplatedType>> templates;

        auto operator==(const TemplatedType& rhs) const -> bool;
        auto operator!=(const TemplatedType& rhs) const -> bool;
        [[nodiscard]] auto stringify() const -> std::string;
    };

    class MemberDefinition {
        public:
            MemberDefinition(
                std::shared_ptr<TemplatedType> type,
                scanner::Token name,
                std::optional<std::shared_ptr<parser::Expr>> initializer,
                bool p_is_static,
                bool p_is_private,
                bool p_is_const
            );
            virtual ~MemberDefinition();

            scanner::Token name;
            std::shared_ptr<TemplatedType> type;
            std::optional<std::shared_ptr<parser::Expr>> initializer;

            bool is_static;
            bool is_private;
            bool is_const;
    };

    class FunctionMember : public MemberDefinition {
        public:
            FunctionMember(
                const std::shared_ptr<parser::Templated>& p_name,
                std::vector<std::shared_ptr<MemberDefinition>> p_parameters,
                std::optional<std::shared_ptr<TemplatedType>> p_returns,
                std::vector<std::shared_ptr<parser::Statement>> p_code,
                bool p_is_static,
                bool p_is_private,
                bool p_is_const,
                bool p_is_operator,
                bool p_is_cast
            );

            FunctionMember(
                const std::shared_ptr<parser::Templated>& p_name,
                std::vector<std::shared_ptr<MemberDefinition>> p_parameters,
                std::optional<std::shared_ptr<TemplatedType>> p_returns,
                bool p_is_static,
                bool p_is_private,
                bool p_is_const,
                bool p_is_operator,
                bool p_is_cast
            );

            scanner::Token function_name;
            std::shared_ptr<parser::Templated> templated_name;
            std::vector<std::shared_ptr<MemberDefinition>> parameters;
            std::unordered_map<std::string, std::shared_ptr<TemplateDefinition>> templates_by_name;
            std::vector<std::shared_ptr<TemplateDefinition>> templates;
            std::optional<std::shared_ptr<TemplatedType>> returns;
            std::optional<std::vector<std::shared_ptr<parser::Statement>>> code;

            bool is_operator;
            bool is_cast;

            auto validate_call(const scanner::Token& call_point, const std::vector<std::shared_ptr<TemplateDefinition>>& test_templates, const std::vector<std::shared_ptr<TemplatedType>>& args) -> void;
    };

    class ClassDefinition : public MemberDefinition {
        public:
            ClassDefinition(
                std::shared_ptr<parser::Templated> p_name,
                std::vector<std::shared_ptr<MemberDefinition>> p_params,
                std::vector<std::shared_ptr<MemberDefinition>> p_members
            );

            bool invalid = false;

            std::string pretty_name;
            std::shared_ptr<parser::Templated> type_name;
            std::vector<std::shared_ptr<MemberDefinition>> params;
            std::unordered_map<std::string, std::shared_ptr<MemberDefinition>> members_by_name;
            std::vector<std::shared_ptr<MemberDefinition>> members;
            std::unordered_map<std::string, std::shared_ptr<TemplateDefinition>> templates_by_name;
            std::vector<std::shared_ptr<TemplateDefinition>> templates;

            virtual auto create_object(const scanner::Token& creation_point, const std::vector<std::shared_ptr<TemplateDefinition>>& templates, const std::vector<std::shared_ptr<objects::Object>>& params) -> std::shared_ptr<objects::Object>;
            virtual auto is_constructor_valid(const std::shared_ptr<ClassDefinition>& def) -> bool;
            virtual auto can_convert_to(const std::shared_ptr<ClassDefinition>& def) -> bool;
            [[nodiscard]] auto get_functions() const -> std::vector<std::shared_ptr<FunctionMember>>;
    };

    class UserDefinition : public ClassDefinition, public std::enable_shared_from_this<UserDefinition> {
        public:
            UserDefinition(
                const std::shared_ptr<parser::Templated>& name, 
                const std::vector<std::shared_ptr<MemberDefinition>>& p_params,
                const std::vector<std::shared_ptr<MemberDefinition>>& p_members
            );

            auto is_constructor_valid(const std::shared_ptr<ClassDefinition>& def) -> bool override;
            auto can_convert_to(const std::shared_ptr<ClassDefinition>& def) -> bool override;
            auto create_object(const scanner::Token& creation_point, const std::vector<std::shared_ptr<TemplateDefinition>>& templates, const std::vector<std::shared_ptr<objects::Object>>& params) -> std::shared_ptr<objects::Object> override;
    };

    class BoolDefinition : public ClassDefinition {
        public:
            BoolDefinition();

            auto is_constructor_valid(const std::shared_ptr<ClassDefinition>& def) -> bool override;
            auto can_convert_to(const std::shared_ptr<ClassDefinition>& def) -> bool override;
    };

    class IntDefinition : public ClassDefinition {
        public:
            IntDefinition();

            auto is_constructor_valid(const std::shared_ptr<ClassDefinition>& def) -> bool override;
            auto can_convert_to(const std::shared_ptr<ClassDefinition>& def) -> bool override;
    };

    class FloatDefinition : public ClassDefinition {
        public:
            FloatDefinition();

            auto is_constructor_valid(const std::shared_ptr<ClassDefinition>& def) -> bool override;
            auto can_convert_to(const std::shared_ptr<ClassDefinition>& def) -> bool override;
    };

    class CharDefinition : public ClassDefinition {
        public:
            CharDefinition();

            auto is_constructor_valid(const std::shared_ptr<ClassDefinition>& def) -> bool override;
            auto can_convert_to(const std::shared_ptr<ClassDefinition>& def) -> bool override;
    };

    class StringDefinition : public ClassDefinition {
        public:
            StringDefinition();

            auto is_constructor_valid(const std::shared_ptr<ClassDefinition>& def) -> bool override;
            auto can_convert_to(const std::shared_ptr<ClassDefinition>& def) -> bool override;
    };

    class ArrayDefinition : public ClassDefinition {
        public:
            ArrayDefinition();
    };

    class FunctionDefinition : public ClassDefinition {
        public: 
            FunctionDefinition();
    };

    class TemplateDefinition {
        public:
            TemplateDefinition(scanner::Token p_template_name);
            TemplateDefinition(std::shared_ptr<TemplatedType> p_template_used);

            scanner::Token template_name = scanner::Token(scanner::TokenType::IDENTIFIER, "${UNNAMED_TEMPLATE}", -1);
            std::optional<std::shared_ptr<TemplatedType>> template_used;
    };
}