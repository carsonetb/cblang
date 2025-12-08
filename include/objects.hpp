#pragma once 

#include "definitions.hpp"
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
            Variable(scanner::Token p_name, std::optional<std::shared_ptr<Object>> p_object);

            std::string name;
            std::shared_ptr<Object> object;
    };

    class Object {
        public:
            Object(std::shared_ptr<ClassDefinition> p_type, std::vector<std::shared_ptr<TemplateDefinition>> p_templates);
            virtual ~Object();

            std::shared_ptr<ClassDefinition> type;
            
            // These templates are defined -- an actual ClassDefinition is associated with them.
            std::vector<std::shared_ptr<TemplateDefinition>> defined_templates;
            std::unordered_map<std::string, std::shared_ptr<TemplateDefinition>> defined_templates_by_name;
            std::vector<std::shared_ptr<Object>> members;
            std::unordered_map<std::string, std::shared_ptr<Object>> members_by_name;

            bool is_null = true;

            virtual auto cast_from(std::shared_ptr<Object> obj) -> int;
            virtual auto cast_into(std::shared_ptr<Object> obj) -> int;
    };
    
    using InternalFunction = std::function<std::optional<std::shared_ptr<Object>>(std::vector<std::shared_ptr<TemplateDefinition>>, std::vector<std::shared_ptr<Object>>)>;

    class ScopeCallable : public Object {
        public:
            virtual auto call(std::vector<std::shared_ptr<TemplateDefinition>> in_templates, std::vector<std::shared_ptr<Object>> passed_params, std::vector<program::Scope> scope_stack) -> std::optional<std::shared_ptr<Object>> = 0;
    };

    class FunctionObject : public Object {
        public:
            FunctionObject(
                std::shared_ptr<FunctionMember> p_definition,
                std::vector<std::shared_ptr<parser::Statement>> p_code,
                bool p_is_operator = false
            );
            FunctionObject(
                std::shared_ptr<FunctionMember> definition,
                InternalFunction p_internal,
                bool p_is_operator = false
            );

            std::shared_ptr<FunctionMember> definition;
            std::optional<InternalFunction> internal;
            std::optional<std::vector<std::shared_ptr<parser::Statement>>> code;
            bool is_operator;

            auto call(std::vector<std::shared_ptr<TemplateDefinition>> in_templates, std::vector<std::shared_ptr<Object>> passed_params, std::vector<program::Scope> scope_stack) -> std::optional<std::shared_ptr<Object>>;
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
}