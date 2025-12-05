#pragma once 

#include "definitions.hpp"
#include "scanner.hpp"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace cblang::program {
    class Scope;
}

namespace cblang::objects {
    using namespace definitions;

    class Object {
        public:
            Object(scanner::Token p_, std::shared_ptr<ClassDefinition> p_type, std::vector<std::shared_ptr<TemplateDefinition>> p_templates);
            virtual ~Object();

            scanner::Token name;
            std::shared_ptr<ClassDefinition> type;
            std::vector<std::shared_ptr<TemplateDefinition>> defined_templates;
            std::unordered_map<std::string, std::shared_ptr<TemplateDefinition>> defined_templates_by_name;
            std::vector<std::shared_ptr<Object>> members;
            std::unordered_map<std::string, std::shared_ptr<Object>> members_by_name;

            bool is_null = true;

            virtual auto cast_from(std::shared_ptr<Object> obj) -> int;
            virtual auto cast_into(std::shared_ptr<Object> obj) -> int;
    };
    
    using InternalFunction = std::function<std::optional<std::shared_ptr<Object>>(std::vector<std::shared_ptr<TemplateDefinition>>, std::vector<std::shared_ptr<Object>>)>;

    class FunctionObject : public Object, public std::enable_shared_from_this<FunctionObject> {
        public:
            FunctionObject(
                scanner::Token p_name,
                std::vector<std::shared_ptr<TemplateDefinition>> p_templates,
                std::vector<std::shared_ptr<MemberDefinition>> p_parameters,
                std::vector<std::shared_ptr<parser::Statement>> p_code
            );
            FunctionObject(
                scanner::Token p_name,
                std::vector<std::shared_ptr<TemplateDefinition>> p_templates,
                std::vector<std::shared_ptr<MemberDefinition>> p_parameters,
                InternalFunction p_internal
            );

            std::vector<std::shared_ptr<TemplateDefinition>> templates;
            std::vector<std::shared_ptr<MemberDefinition>> parameters;
            std::optional<InternalFunction> internal;
            std::optional<std::vector<std::shared_ptr<parser::Statement>>> code;

            auto call(std::vector<std::shared_ptr<TemplateDefinition>> in_templates, std::vector<std::shared_ptr<Object>> passed_params, std::vector<program::Scope> scope_stack) -> std::optional<std::shared_ptr<Object>>;
    };
}