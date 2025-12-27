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

#define DEFINE_AND_ADD_OPERATOR(Type, type, name, ret) \
if (!(type)->members_by_name.contains(name)) {    \
    auto oper = cblang::definitions::define_operator<Type>(name, ret); \
    (type)->members.push_back(oper);              \
    (type)->members_by_name[name] = oper;         \
} 

namespace cblang::objects {
    class Object;
}

namespace cblang::definitions {
    class ClassDefinition;
    class TemplateDefinition;
    class FunctionDefinition;
    class FunctionMember;

    template <typename OtherT> auto define_operator(const std::string& name, const std::shared_ptr<ClassDefinition>& ret_type) -> std::shared_ptr<FunctionMember>;
    
    enum class LiteralType : uint8_t {
        BOOL,
        INT,
        STRING,
        CHAR,
        ARRAY,
        INVALID,
    };

    struct TemplatedType {
        static auto generate(
            const std::shared_ptr<ClassDefinition>& p_cls,
            const std::vector<std::shared_ptr<TemplatedType>>& p_templates
        ) -> std::shared_ptr<TemplatedType> {
            return std::make_shared<TemplatedType>(p_cls, p_templates);
        }

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
            static auto generate(
                const std::shared_ptr<TemplatedType>& type,
                const scanner::Token& name,
                const std::optional<std::shared_ptr<parser::Expr>>& initializer,
                bool p_is_static,
                bool p_is_private,
                bool p_is_const
            ) -> std::shared_ptr<MemberDefinition> {
                return std::make_shared<MemberDefinition>(type, name, initializer, p_is_static, p_is_private, p_is_const);
            }

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
            static auto generate(
                const std::shared_ptr<parser::Templated>& p_name,
                const std::vector<std::shared_ptr<MemberDefinition>>& p_parameters,
                const std::optional<std::shared_ptr<TemplatedType>>& p_returns,
                bool p_is_static,
                bool p_is_private,
                bool p_is_const,
                bool p_is_operator,
                bool p_is_cast
            ) -> std::shared_ptr<FunctionMember> {
                return std::make_shared<FunctionMember>(p_name, p_parameters, p_returns, std::optional<std::vector<std::shared_ptr<parser::Statement>>>(), p_is_static, p_is_private, p_is_const, p_is_operator, p_is_cast);
            }

            FunctionMember(
                const std::shared_ptr<parser::Templated>& p_name,
                std::vector<std::shared_ptr<MemberDefinition>> p_parameters,
                std::optional<std::shared_ptr<TemplatedType>> p_returns,
                std::optional<std::vector<std::shared_ptr<parser::Statement>>> p_code,
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
            static auto generate(const std::shared_ptr<parser::Templated>& p_name, const std::vector<std::shared_ptr<MemberDefinition>>& p_params, const std::vector<std::shared_ptr<MemberDefinition>>& p_members) -> std::shared_ptr<ClassDefinition> {
                return std::make_shared<ClassDefinition>(p_name, p_params, p_members);
            }

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
            virtual auto can_convert_to(const std::shared_ptr<ClassDefinition>& def) -> bool;
            virtual auto can_convert_from(const std::shared_ptr<TemplatedType>& type) -> bool;
            virtual auto cast_from(const std::shared_ptr<objects::Object>& obj) -> std::optional<std::shared_ptr<objects::Object>>;
            [[nodiscard]] auto get_functions() const -> std::vector<std::shared_ptr<FunctionMember>>;
            [[nodiscard]] auto get_function(const std::string& name) const -> std::shared_ptr<FunctionMember>;
    };

    class UserDefinition : public ClassDefinition, public std::enable_shared_from_this<UserDefinition> {
        public:
            UserDefinition(
                const std::shared_ptr<parser::Templated>& name, 
                const std::vector<std::shared_ptr<MemberDefinition>>& p_params,
                const std::vector<std::shared_ptr<MemberDefinition>>& p_members
            );

            auto can_convert_to(const std::shared_ptr<ClassDefinition>& def) -> bool override;
            auto create_object(const scanner::Token& creation_point, const std::vector<std::shared_ptr<TemplateDefinition>>& templates, const std::vector<std::shared_ptr<objects::Object>>& params) -> std::shared_ptr<objects::Object> override;
    };

    class BoolDefinition : public ClassDefinition {
        public:
            static auto generate() -> std::shared_ptr<BoolDefinition>;

            BoolDefinition();

            bool generated = false;

            auto can_convert_to(const std::shared_ptr<ClassDefinition>& def) -> bool override;
        private:
            static auto unsafe_singleton() -> std::shared_ptr<BoolDefinition> {
                static auto out = std::make_shared<BoolDefinition>();
                return out;
            }
    };

    class IntDefinition : public ClassDefinition {
        public:
            static auto generate() -> std::shared_ptr<IntDefinition>;

            IntDefinition();

            bool generated = false;

            auto can_convert_to(const std::shared_ptr<ClassDefinition>& def) -> bool override;
        
        private:
            static auto unsafe_singleton() -> std::shared_ptr<IntDefinition> {
                static auto out = std::make_shared<IntDefinition>();
                return out;
            }
    };

    class FloatDefinition : public ClassDefinition {
        public:
            static auto generate() -> std::shared_ptr<FloatDefinition>;

            FloatDefinition();

            bool generated = false;

            auto can_convert_to(const std::shared_ptr<ClassDefinition>& def) -> bool override;
        
        private:
            static auto unsafe_singleton() -> std::shared_ptr<FloatDefinition> {
                static auto out = std::make_shared<FloatDefinition>();
                return out;
            }
        
    };

    class CharDefinition : public ClassDefinition {
        public:
            static auto generate() -> std::shared_ptr<CharDefinition>;

            CharDefinition();

            bool generated = false;

            auto can_convert_to(const std::shared_ptr<ClassDefinition>& def) -> bool override;
        
        private:
            static auto unsafe_singleton() -> std::shared_ptr<CharDefinition> {
                static auto out = std::make_shared<CharDefinition>();
                return out;
            }
    };

    class StringDefinition : public ClassDefinition {
        public:
            static auto generate() -> std::shared_ptr<StringDefinition>;

            StringDefinition();

            bool generated = false;

            auto can_convert_to(const std::shared_ptr<ClassDefinition>& def) -> bool override;
        
        private:
            static auto unsafe_singleton() -> std::shared_ptr<StringDefinition> {
                static auto out = std::make_shared<StringDefinition>();
                return out;
            }
    };

    class ArrayDefinition : public ClassDefinition {
        public:
            ArrayDefinition(std::shared_ptr<TemplateDefinition> p_value_type);
        
        private:
            std::shared_ptr<TemplateDefinition> value_type;
    };

    class FunctionDefinition : public ClassDefinition {
        public: 
            FunctionDefinition();
    };

    class TemplateDefinition {
        public:
            static auto generate(const std::shared_ptr<TemplatedType>& p_template_used) -> std::shared_ptr<TemplateDefinition> {
                return std::make_shared<TemplateDefinition>(p_template_used);
            }

            static auto generate(const std::string& p_name) -> std::shared_ptr<TemplateDefinition> {
                return std::make_shared<TemplateDefinition>(scanner::Token::create_external(p_name));
            }

            TemplateDefinition(scanner::Token p_template_name);
            TemplateDefinition(std::shared_ptr<TemplatedType> p_template_used);

            scanner::Token template_name = scanner::Token(scanner::TokenType::IDENTIFIER, "${UNNAMED_TEMPLATE}", -1);
            std::optional<std::shared_ptr<TemplatedType>> template_used;
    };

    template <typename OtherT>
    auto define_operator(const std::string& name, const std::shared_ptr<ClassDefinition>& ret_type) -> std::shared_ptr<FunctionMember> {
        return FunctionMember::generate(
            parser::Templated::generate(scanner::Token::create_external(name), {}),
            {
                MemberDefinition::generate(
                    TemplatedType::generate(std::make_shared<OtherT>(), {}),
                    scanner::Token::create_external("other"),
                    {}, false, false, true
                )
            },
            TemplatedType::generate(ret_type, {}),
            false, false, true, true, false
        );
    }
}