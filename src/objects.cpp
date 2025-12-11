#include "objects.hpp"

#include "parser.hpp"
#include "program.hpp"
#include "definitions.hpp"
#include "scanner.hpp"

#include <cassert>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace cblang;
using namespace cblang::objects;

cblang::objects::Object::Object(
    std::shared_ptr<ClassDefinition> p_type,
    std::vector<std::shared_ptr<TemplateDefinition>> p_templates
) : type(std::move(p_type)), 
    defined_templates(std::move(p_templates)) 
{
    for (const auto& template_param : defined_templates) {
        defined_templates_by_name[template_param->template_name.raw] = template_param;
    }
}

auto cblang::objects::Object::cast_from(std::shared_ptr<Object> obj) -> int {
    return 1;
}

auto cblang::objects::Object::cast_into(std::shared_ptr<Object> obj) -> int {
    return 1;
}

auto cblang::objects::Object::get_templated() const -> std::shared_ptr<definitions::TemplatedType> {
    std::vector<std::shared_ptr<definitions::TemplatedType>> templates;
    templates.reserve(defined_templates.size());
    for (const auto& template_defintion : defined_templates) {
        templates.push_back(template_defintion->template_used.value());
    }
    return std::make_shared<definitions::TemplatedType>(type, templates);
}

cblang::objects::FunctionObject::FunctionObject(
    std::shared_ptr<definitions::FunctionMember> p_definition,
    std::vector<std::shared_ptr<parser::Statement>> p_code,
    bool p_is_operator
) : Object(std::make_shared<definitions::FunctionDefinition>(), {}),
    definition(std::move(p_definition)),
    code(std::move(p_code)),
    is_operator(p_is_operator)
{

}

cblang::objects::FunctionObject::FunctionObject(
    std::shared_ptr<definitions::FunctionMember> p_definition,
    InternalFunction p_internal,
    bool p_is_operator
) : Object(std::make_shared<definitions::FunctionDefinition>(), {}),
    definition(std::move(p_definition)),
    internal(std::move(p_internal)),
    is_operator(p_is_operator)
{

}

auto cblang::objects::FunctionObject::validate_call(const scanner::Token& call_point, std::vector<std::shared_ptr<definitions::TemplateDefinition>> in_templates, std::vector<std::shared_ptr<Object>> passed_params) -> void {
    if (in_templates.size() < definition->templates.size()) {
        throw program::handle_error(call_point, "Function requires " + std::to_string(definition->templates.size()) + " templates but only " + std::to_string(in_templates.size()) + " were provided.");
    }

    if (passed_params.size() < definition->parameters.size()) {
        throw program::handle_error(call_point, "Function requires " + std::to_string(definition->parameters.size()) + " parameters but only " + std::to_string(passed_params.size()) + " were provided.");
    }

    for (int i = 0; i < in_templates.size(); i++) {
        const auto& in_template = in_templates[i];
        if (i >= definition->templates.size()) {
            throw program::handle_error(in_template->template_name, "Function requires only " + std::to_string(definition->templates.size()) + " templates.");
        }
        auto this_template = definition->templates[i];
        in_template->template_name = this_template->template_name;
    }

    for (int i = 0; i < passed_params.size(); i++) {
        const auto& param_obj = passed_params[i];
        if (i >= definition->parameters.size()) {
            throw program::handle_error(call_point, "Function requires only " + std::to_string(definition->parameters.size()) + " parameters.");
        }
        auto param_def = definition->parameters[i];
        if (param_obj->get_templated() != param_def->type) {
            throw program::handle_error(call_point, "Passed variable of type " + param_obj->get_templated()->stringify() + " but expected type " + param_def->type->stringify() + " (param name " + param_def->name.raw + ").");
        }
        auto this_variable = std::make_shared<Variable>(param_def->name, param_obj);
    }
}

auto cblang::objects::FunctionObject::call(const scanner::Token& call_point, const std::vector<std::shared_ptr<TemplateDefinition>>& in_templates, const std::vector<std::shared_ptr<Object>>& passed_params, std::vector<std::shared_ptr<program::Scope>> owner_scope) -> std::optional<std::shared_ptr<Object>> {
    auto call_scope = std::make_shared<program::Scope>();
    owner_scope.push_back(call_scope);

    validate_call(call_point, in_templates, passed_params);

    for (const auto& in_template : in_templates) {
        call_scope->defined_templates[in_template->template_name.raw] = in_template;
    }

    if (internal) {
        return internal.value()(in_templates, passed_params);
    }
    if (code) {
        program::ScopeParser parser = program::ScopeParser(code.value(), owner_scope, definition->returns.has_value());
        return parser.process();
    }
    throw program::handle_error(call_point, "Attempt to call a null function '" + call_point.raw + "'.");
}

cblang::objects::MultipleFunctionObject::MultipleFunctionObject(
    std::vector<std::shared_ptr<FunctionObject>> p_objects
) : Object(std::make_shared<definitions::FunctionDefinition>(), {}),
    objects(std::move(p_objects)) {

}

cblang::objects::BoolObject::BoolObject(bool p_value) : Object(std::make_shared<definitions::BoolDefinition>(), {}), value(p_value) {

}

cblang::objects::IntObject::IntObject(int p_value) : Object(std::make_shared<definitions::IntDefinition>(), {}), value(p_value) {

}

cblang::objects::FloatObject::FloatObject(float p_value) : Object(std::make_shared<definitions::FloatDefinition>(), {}), value(p_value) {

}

cblang::objects::CharObject::CharObject(char p_value) : Object(std::make_shared<definitions::CharDefinition>(), {}), value(p_value) {

}

cblang::objects::StringObject::StringObject(std::string p_value) : Object(std::make_shared<definitions::StringDefinition>(), {}), value(std::move(p_value)) {

}

cblang::objects::ArrayObject::ArrayObject(std::vector<std::shared_ptr<Object>> p_value, std::shared_ptr<definitions::TemplateDefinition> value_type) 
    : Object(
        std::make_shared<definitions::ArrayDefinition>(), 
        {
            std::move(value_type)
        }
    ), value(std::move(p_value)) 
{

}