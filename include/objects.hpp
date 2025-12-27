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

#define INTERNAL_FUNCTION_PARAMS [val = value](const std::vector<std::shared_ptr<definitions::TemplateDefinition>>& in_templates, const std::vector<std::shared_ptr<objects::Object>>& params) -> std::optional<std::shared_ptr<cblang::objects::Object>>
#define INTERNAL_FUNCTION_PARAMS_CAPTURE_SELF [self = shared_from_this()](const std::vector<std::shared_ptr<definitions::TemplateDefinition>>& in_templates, const std::vector<std::shared_ptr<objects::Object>>& params) -> std::optional<std::shared_ptr<cblang::objects::Object>>
#define STATIC_INTERNAL_FUNCTION_PARAMS [](const std::vector<std::shared_ptr<definitions::TemplateDefinition>>& in_templates, const std::vector<std::shared_ptr<objects::Object>>& params) -> std::optional<std::shared_ptr<cblang::objects::Object>>

#define GET_PARAM(Type, ind) std::dynamic_pointer_cast<Type>(params[ind])

namespace cblang::program {
    class Scope;
}

using namespace cblang::definitions;

namespace cblang::objects {
    class Variable;

    using InternalFunction = std::function<std::optional<std::shared_ptr<Object>>(const std::vector<std::shared_ptr<TemplateDefinition>>&, const std::vector<std::shared_ptr<Object>>&)>;

    auto create_function(const std::shared_ptr<ClassDefinition>& type, const std::string& name, const InternalFunction& func, bool is_const = false, bool is_static = false, bool is_cast = false, bool is_operator = false) -> std::shared_ptr<Variable>;

    class Variable {
        public:
            static auto generate(const scanner::Token& p_name, const std::shared_ptr<objects::Object>& p_object, bool p_is_private, bool p_is_static, bool p_is_const) -> std::shared_ptr<Variable> {
                return std::make_shared<Variable>(p_name, p_object, p_is_private, p_is_static, p_is_const);
            }

            Variable(scanner::Token p_name, std::shared_ptr<objects::Object> p_object, bool p_is_private, bool p_is_static, bool p_is_const);

            scanner::Token name;
            std::shared_ptr<Object> object;

            bool is_private;
            bool is_static;
            bool is_const;
    };

    class Object : public std::enable_shared_from_this<Object> {
        public:
            static auto generate(const std::shared_ptr<ClassDefinition>& p_type, const std::vector<std::shared_ptr<TemplateDefinition>>& p_templates, const std::vector<std::shared_ptr<Variable>>& p_params) -> std::shared_ptr<Object> {
                return std::make_shared<Object>(p_type, p_templates, p_params);
            }

            Object(std::shared_ptr<ClassDefinition> p_type, const std::vector<std::shared_ptr<TemplateDefinition>>& p_templates, const std::vector<std::shared_ptr<Variable>>& p_params);
            virtual ~Object();

            std::shared_ptr<ClassDefinition> type;
            
            // These templates are defined -- an actual ClassDefinition is associated with them.
            std::unordered_map<std::string, std::shared_ptr<TemplateDefinition>> defined_templates;
            std::unordered_map<std::string, std::shared_ptr<Variable>> members_by_name;

            auto initialize() -> void;
            auto call(const std::string& function_name, const scanner::Token& call_point, const std::vector<std::shared_ptr<TemplateDefinition>>& in_templates, const std::vector<std::shared_ptr<Object>>& passed_params, const std::optional<std::shared_ptr<program::Scope>>& global_scope = {}) -> std::optional<std::shared_ptr<Object>>;      
            auto get_var(const scanner::Token& variable_name) const -> std::shared_ptr<Object>;
            auto get_scope() -> std::shared_ptr<program::Scope>;
            virtual auto cast_to(const std::shared_ptr<TemplatedType>& type) -> std::optional<std::shared_ptr<Object>>;
            [[nodiscard]] auto get_templated() const -> std::shared_ptr<TemplatedType>;
            [[nodiscard]] auto get_member_array() const -> std::vector<std::shared_ptr<Variable>>;
            [[nodiscard]] auto get_template_array() const -> std::vector<std::shared_ptr<TemplateDefinition>>;
    };

    class Callable : public Object {
        public:
            Callable(
                std::optional<std::shared_ptr<TemplatedType>> p_returns,
                std::vector<std::shared_ptr<TemplateDefinition>> p_templates,
                std::vector<std::shared_ptr<MemberDefinition>> p_parameters
            );

            std::optional<std::shared_ptr<TemplatedType>> returns;
            std::vector<std::shared_ptr<MemberDefinition>> parameters;
            std::unordered_map<std::string, std::shared_ptr<TemplateDefinition>> templates_by_name;
            std::vector<std::shared_ptr<TemplateDefinition>> templates;

            auto validate_call(const scanner::Token& call_point, std::vector<std::shared_ptr<TemplateDefinition>> in_templates, std::vector<std::shared_ptr<Object>> passed_params) const -> void;
            [[nodiscard]] virtual auto call_this(const scanner::Token& call_point, const std::vector<std::shared_ptr<TemplateDefinition>>& in_templates, const std::vector<std::shared_ptr<Object>>& passed_params, std::vector<std::shared_ptr<program::Scope>> owner_scope) const -> std::optional<std::shared_ptr<Object>> = 0;
    };

    class FunctionObject : public Callable {
        public:
            static auto generate(
                const std::shared_ptr<FunctionMember>& definition,
                const InternalFunction& p_internal,
                bool p_is_cast = false,
                bool p_is_operator = false,
                bool p_is_const = false,
                bool p_is_static = false
            ) -> std::shared_ptr<FunctionObject> {
                return std::make_shared<FunctionObject>(definition, p_internal, p_is_cast, p_is_operator, p_is_const, p_is_static);
            }

            FunctionObject(
                const std::shared_ptr<FunctionMember>& p_definition,
                std::vector<std::shared_ptr<parser::Statement>> p_code,
                bool p_is_cast = false,
                bool p_is_operator = false,
                bool p_is_const = false,
                bool p_is_static = false
            );
            FunctionObject(
                const std::shared_ptr<FunctionMember>& definition,
                InternalFunction p_internal,
                bool p_is_cast = false,
                bool p_is_operator = false,
                bool p_is_const = false,
                bool p_is_static = false
            );
            FunctionObject(
                scanner::Token declare_point,
                std::vector<std::shared_ptr<MemberDefinition>> p_parameters,
                std::vector<std::shared_ptr<TemplateDefinition>> p_templates,
                std::optional<std::shared_ptr<TemplatedType>> p_returns,
                std::optional<std::vector<std::shared_ptr<parser::Statement>>> code,
                bool p_is_cast = false,
                bool p_is_operator = false,
                bool p_is_const = false,
                bool p_is_static = false
            );

            scanner::Token declare_point;
            std::optional<scanner::Token> function_name;
            std::optional<InternalFunction> internal;
            std::optional<std::vector<std::shared_ptr<parser::Statement>>> code;

            bool is_cast;
            bool is_operator;
            bool is_const;
            bool is_static;

            [[nodiscard]] auto call_this(const scanner::Token& call_point, const std::vector<std::shared_ptr<TemplateDefinition>>& in_templates, const std::vector<std::shared_ptr<Object>>& passed_params, std::vector<std::shared_ptr<program::Scope>> owner_scope) const -> std::optional<std::shared_ptr<Object>> override;
    };

    // Represents multiple function objects with the same name,
    // which can be called with different parameters but must
    // all return the same type.
    class MultipleFunctionObject : public Object {
        public:
            MultipleFunctionObject(std::vector<std::shared_ptr<FunctionObject>> p_objects);
            MultipleFunctionObject(std::shared_ptr<FunctionObject> first_override);

            std::vector<std::shared_ptr<FunctionObject>> objects;

            [[nodiscard]] auto call_this(const scanner::Token& call_point, const std::vector<std::shared_ptr<TemplateDefinition>>& in_templates, const std::vector<std::shared_ptr<Object>>& passed_params, std::vector<std::shared_ptr<program::Scope>> owner_scope) const -> std::optional<std::shared_ptr<Object>>;
    };

    class BoolObject : public Object {
        public:
            static auto create(bool val) -> std::shared_ptr<BoolObject> {
                return std::make_shared<BoolObject>(val);
            }

            BoolObject(bool p_value = false);

            bool value;

            auto cast_to(const std::shared_ptr<TemplatedType>& type) -> std::optional<std::shared_ptr<Object>> override;
    };

    class IntObject : public Object {
        public: 
            static auto create(int val) -> std::shared_ptr<IntObject> {
                return std::make_shared<IntObject>(val);
            }

            IntObject(int p_value = 0);

            int value;

            auto cast_to(const std::shared_ptr<TemplatedType>& type) -> std::optional<std::shared_ptr<Object>> override;
    };

    class FloatObject : public Object {
        public:
            static auto create(float val) -> std::shared_ptr<FloatObject> {
                return std::make_shared<FloatObject>(val);
            }

            FloatObject(float p_value = 0.0);

            float value;

            auto cast_to(const std::shared_ptr<TemplatedType>& type) -> std::optional<std::shared_ptr<Object>> override;
    };

    class CharObject : public Object {
        public:
            static auto create(char val) -> std::shared_ptr<CharObject> {
                return std::make_shared<CharObject>(val);
            }

            CharObject(char p_value);

            char value;

            auto cast_to(const std::shared_ptr<TemplatedType>& type) -> std::optional<std::shared_ptr<Object>> override;
    };

    class StringObject : public Object {
        public:
            static auto create(const std::string& val) -> std::shared_ptr<StringObject> {
                return std::make_shared<StringObject>(val);
            }

            StringObject(std::string p_value);

            std::string value;

            auto cast_to(const std::shared_ptr<TemplatedType>& type) -> std::optional<std::shared_ptr<Object>> override;
    };

    class ArrayObject : public Object {
        public:
            static auto create(const std::vector<std::shared_ptr<Object>>& p_value, const std::shared_ptr<definitions::TemplateDefinition>& value_type) -> std::shared_ptr<ArrayObject> {
                return std::make_shared<ArrayObject>(p_value, value_type);
            }

            ArrayObject(std::vector<std::shared_ptr<Object>> p_value, std::shared_ptr<definitions::TemplateDefinition> value_type);

            std::vector<std::shared_ptr<Object>> value;
    };
}