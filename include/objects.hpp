#pragma once 

#include "definitions.hpp"
#include "parser.hpp"
#include "scanner.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace cblang::program {
    class Scope;
}

namespace cblang::objects {
    using namespace definitions;

    class Variable {
        public:
            Variable(scanner::Token p_name, std::shared_ptr<objects::Object> p_object);

            std::string name;
            std::shared_ptr<Object> object;
    };

    class Object {
        public:
            Object(std::shared_ptr<ClassDefinition> p_type, std::vector<std::shared_ptr<TemplateDefinition>> p_templates);
            virtual ~Object();

            std::shared_ptr<ClassDefinition> type;
            
            // TODO: Remove members array to make the Obejct type take up less memory.
            // These templates are defined -- an actual ClassDefinition is associated with them.
            std::vector<std::shared_ptr<TemplateDefinition>> defined_templates;
            std::unordered_map<std::string, std::shared_ptr<TemplateDefinition>> defined_templates_by_name;
            std::vector<std::shared_ptr<Object>> members;
            std::unordered_map<std::string, std::shared_ptr<Object>> members_by_name;
            std::shared_ptr<program::Scope> this_scope;

            bool is_null = true;

            virtual auto cast_from(std::shared_ptr<Object> obj) -> int;
            virtual auto cast_into(std::shared_ptr<Object> obj) -> int;
            [[nodiscard]] auto get_templated() const -> std::shared_ptr<TemplatedType>;
    };
    
    using InternalFunction = std::function<std::optional<std::shared_ptr<Object>>(std::vector<std::shared_ptr<TemplateDefinition>>, std::vector<std::shared_ptr<Object>>)>;

    class Callable : public Object {
        public:
            Callable(
                std::optional<std::shared_ptr<ClassDefinition>> p_returns,
                std::vector<std::shared_ptr<TemplateDefinition>> p_templates,
                std::vector<std::shared_ptr<MemberDefinition>> p_parameters
            );

            std::optional<std::shared_ptr<ClassDefinition>> returns;
            std::vector<std::shared_ptr<MemberDefinition>> parameters;
            std::unordered_map<std::string, std::shared_ptr<TemplateDefinition>> templates_by_name;
            std::vector<std::shared_ptr<TemplateDefinition>> templates;

            auto validate_call(const scanner::Token& call_point, std::vector<std::shared_ptr<TemplateDefinition>> in_templates, std::vector<std::shared_ptr<Object>> passed_params) const -> void;
            [[nodiscard]] virtual auto call(const scanner::Token& call_point, const std::vector<std::shared_ptr<TemplateDefinition>>& in_templates, const std::vector<std::shared_ptr<Object>>& passed_params, std::vector<std::shared_ptr<program::Scope>> owner_scope) const -> std::optional<std::shared_ptr<Object>> = 0;
    };

    class FunctionObject : public Callable {
        public:
            FunctionObject(
                const std::shared_ptr<FunctionMember>& p_definition,
                std::vector<std::shared_ptr<parser::Statement>> p_code,
                bool p_is_operator = false
            );
            FunctionObject(
                const std::shared_ptr<FunctionMember>& definition,
                InternalFunction p_internal,
                bool p_is_operator = false
            );
            FunctionObject(
                scanner::Token declare_point,
                std::vector<std::shared_ptr<MemberDefinition>> p_parameters,
                std::vector<std::shared_ptr<TemplateDefinition>> p_templates,
                std::optional<std::shared_ptr<ClassDefinition>> p_returns,
                std::optional<std::vector<std::shared_ptr<parser::Statement>>> code
            );

            scanner::Token declare_point;
            std::optional<scanner::Token> function_name;
            std::optional<InternalFunction> internal;
            std::optional<std::vector<std::shared_ptr<parser::Statement>>> code;
            bool is_operator;

            [[nodiscard]] auto call(const scanner::Token& call_point, const std::vector<std::shared_ptr<TemplateDefinition>>& in_templates, const std::vector<std::shared_ptr<Object>>& passed_params, std::vector<std::shared_ptr<program::Scope>> owner_scope) const -> std::optional<std::shared_ptr<Object>> override;
    };

    // Represents multiple function objects with the same name,
    // which can be called with different parameters but must
    // all return the same type.
    class MultipleFunctionObject : public Object {
        public:
            MultipleFunctionObject(std::vector<std::shared_ptr<FunctionObject>> p_objects);
            MultipleFunctionObject(std::shared_ptr<FunctionObject> first_override);

            std::vector<std::shared_ptr<FunctionObject>> objects;
    };

    class BoolObject : public Object {
        public:
            BoolObject(bool p_value = false);

            bool value;
    };

    class IntObject : public Object {
        public: 
            IntObject(int p_value = 0);

            int value;
    };

    class FloatObject : public Object {
        public:
            FloatObject(float p_value = 0.0);

            float value;
    };

    class CharObject : public Object {
        public:
            CharObject(char p_value);

            char value;
    };

    class StringObject : public Object {
        public:
            StringObject(std::string p_value);

            std::string value;
    };

    class ArrayObject : public Object {
        public:
            ArrayObject(std::vector<std::shared_ptr<Object>> p_value, std::shared_ptr<definitions::TemplateDefinition> value_type);

            std::vector<std::shared_ptr<Object>> value;
    };
}