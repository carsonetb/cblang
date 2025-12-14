#include "objects.hpp"

#include "parser.hpp"
#include "program.hpp"
#include "definitions.hpp"
#include "scanner.hpp"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace cblang;
using namespace cblang::objects;

cblang::objects::Variable::Variable(scanner::Token p_name, std::shared_ptr<objects::Object> p_object) : name(std::move(p_name)), object(std::move(p_object)) {}

cblang::objects::Object::Object(
    std::shared_ptr<ClassDefinition> p_type,
    const std::vector<std::shared_ptr<TemplateDefinition>>& p_templates,
    const std::vector<std::shared_ptr<Variable>>& p_params
) : type(std::move(p_type))
{
    for (const auto& template_param : p_templates) {
        defined_templates[template_param->template_name.raw] = template_param;
    }
    for (const auto& param : p_params) {
        members_by_name[param->name.raw] = param;
    }
    for (const auto& member : type->members_by_name) {
        auto as_function = std::dynamic_pointer_cast<FunctionMember>(member.second);
        if (!as_function) {
            continue;
        }
        members_by_name[as_function->function_name.raw] = std::make_shared<Variable>(
            as_function->function_name, 
            std::make_shared<FunctionObject>(
                as_function, 
                as_function->code, 
                as_function->is_operator
            )
        );
    }
}

cblang::objects::Object::~Object() = default;

auto cblang::objects::Object::call(const std::string& function_name, const scanner::Token& call_point, const std::vector<std::shared_ptr<TemplateDefinition>>& in_templates, const std::vector<std::shared_ptr<Object>>& passed_params) -> std::optional<std::shared_ptr<Object>> {
    if (!members_by_name.contains(function_name)) {
        throw program::handle_error(call_point, "Object of type " + get_templated()->stringify() + " has no function '" + function_name + "'.");
    }
    auto as_callable = std::dynamic_pointer_cast<Callable>(members_by_name[function_name]->object);
    if (!as_callable) {
        throw program::handle_error(call_point, "Attempt to call '" + get_templated()->stringify() + "." + function_name + ", but it's a variable.");
    }
    auto out = as_callable->call_this(call_point, in_templates, passed_params, {get_scope()});
    return out;
}

auto cblang::objects::Object::get_var(const scanner::Token& variable_name) const -> std::shared_ptr<Object> {
    if (!members_by_name.contains(variable_name.raw)) {
        throw program::handle_error(variable_name, "Object of type " + type->name.to_string() + " has no member " + variable_name.raw + ".");
    }
    return members_by_name.at(variable_name.raw)->object;
}

auto cblang::objects::Object::get_scope() -> std::shared_ptr<program::Scope> {
    auto out = std::make_shared<program::Scope>();
    out->defined_variables = members_by_name;
    out->defined_templates = defined_templates;
    out->scope_object = shared_from_this();
    return out;
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
        templates.push_back(template_defintion.second->template_used.value());
    }
    return std::make_shared<definitions::TemplatedType>(type, templates);
}

auto cblang::objects::Object::get_member_array() const -> std::vector<std::shared_ptr<Variable>> {
    std::vector<std::shared_ptr<Variable>> out;
    out.reserve(members_by_name.size());
    for (const auto& variable : members_by_name) {
        out.push_back(variable.second);
    }
    return out;
}

auto cblang::objects::Object::get_template_array() const -> std::vector<std::shared_ptr<TemplateDefinition>> {
    std::vector<std::shared_ptr<TemplateDefinition>> out;
    out.reserve(defined_templates.size());
    for (const auto& templ : defined_templates) {
        out.push_back(templ.second);
    }
    return out;
}

cblang::objects::Callable::Callable(
    std::optional<std::shared_ptr<ClassDefinition>> p_returns,
    std::vector<std::shared_ptr<TemplateDefinition>> p_templates,
    std::vector<std::shared_ptr<MemberDefinition>> p_parameters
):  Object(std::make_shared<FunctionDefinition>(), {}, {}),
    templates(std::move(p_templates)),
    returns(std::move(p_returns)),
    parameters(std::move(p_parameters)) 
{
    for (const auto& in_template : p_templates) {
        templates_by_name[in_template->template_name.raw] = in_template;
    }
}

auto cblang::objects::Callable::validate_call(const scanner::Token& call_point, std::vector<std::shared_ptr<definitions::TemplateDefinition>> in_templates, std::vector<std::shared_ptr<Object>> passed_params) const -> void {
    if (in_templates.size() < templates.size()) {
        throw program::handle_error(call_point, "Function requires " + std::to_string(templates.size()) + " templates but only " + std::to_string(in_templates.size()) + " were provided.");
    }

    if (passed_params.size() < parameters.size()) {
        throw program::handle_error(call_point, "Function requires " + std::to_string(parameters.size()) + " parameters but only " + std::to_string(passed_params.size()) + " were provided.");
    }

    for (int i = 0; i < in_templates.size(); i++) {
        const auto& in_template = in_templates[i];
        if (i >= templates.size()) {
            throw program::handle_error(in_template->template_name, "Function requires only " + std::to_string(templates.size()) + " templates.");
        }
        auto this_template = templates[i];
        in_template->template_name = this_template->template_name;
    }

    for (int i = 0; i < passed_params.size(); i++) {
        const auto& param_obj = passed_params[i];
        if (i >= parameters.size()) {
            throw program::handle_error(call_point, "Function requires only " + std::to_string(parameters.size()) + " parameters.");
        }
        auto param_def = parameters[i];
        if (param_obj->get_templated() != param_def->type) {
            throw program::handle_error(call_point, "Passed variable of type " + param_obj->get_templated()->stringify() + " but expected type " + param_def->type->stringify() + " (param name " + param_def->name.raw + ").");
        }
        auto this_variable = std::make_shared<Variable>(param_def->name, param_obj);
    }
}

cblang::objects::FunctionObject::FunctionObject(
    const std::shared_ptr<definitions::FunctionMember>& p_definition,
    std::vector<std::shared_ptr<parser::Statement>> p_code,
    bool p_is_operator
) : Callable(p_definition->returns, p_definition->templates, p_definition->parameters),
    declare_point(p_definition->function_name),
    function_name(p_definition->function_name),
    code(std::move(p_code)),
    is_operator(p_is_operator)
{

}

cblang::objects::FunctionObject::FunctionObject(
    const std::shared_ptr<definitions::FunctionMember>& p_definition,
    InternalFunction p_internal,
    bool p_is_operator
) : Callable(p_definition->returns, p_definition->templates, p_definition->parameters),
    declare_point(p_definition->function_name),
    function_name(p_definition->function_name),
    internal(std::move(p_internal)),
    is_operator(p_is_operator)
{

}

cblang::objects::FunctionObject::FunctionObject(
    scanner::Token p_declare_point,
    std::vector<std::shared_ptr<MemberDefinition>> p_parameters,
    std::vector<std::shared_ptr<TemplateDefinition>> p_templates,
    std::optional<std::shared_ptr<ClassDefinition>> p_returns,
    std::optional<std::vector<std::shared_ptr<parser::Statement>>> p_code
) : Callable(std::move(p_returns), std::move(p_templates), std::move(p_parameters)),
    declare_point(std::move(p_declare_point)),
    code(std::move(p_code)),
    is_operator(false)
{

}

auto cblang::objects::FunctionObject::call_this(const scanner::Token& call_point, const std::vector<std::shared_ptr<TemplateDefinition>>& in_templates, const std::vector<std::shared_ptr<Object>>& passed_params, std::vector<std::shared_ptr<program::Scope>> owner_scope) const -> std::optional<std::shared_ptr<Object>> {
    auto owner = owner_scope.back()->scope_object;
    auto call_scope = std::make_shared<program::Scope>();
    call_scope->scope_object = owner;
    owner_scope.push_back(call_scope);

    validate_call(call_point, in_templates, passed_params);

    for (const auto& in_template : in_templates) {
        call_scope->defined_templates[in_template->template_name.raw] = in_template;
    }

    if (internal) {
        return internal.value()(in_templates, passed_params);
    }
    if (code) {
        program::ScopeParser parser = program::ScopeParser(code.value(), owner_scope, returns.has_value());
        return parser.process();
    }
    throw program::handle_error(call_point, "Attempt to call a null function '" + call_point.raw + "'.");
}

cblang::objects::MultipleFunctionObject::MultipleFunctionObject(
    std::vector<std::shared_ptr<FunctionObject>> p_objects
) : Object(std::make_shared<definitions::FunctionDefinition>(), {}, {}),
    objects(std::move(p_objects)) {

}

cblang::objects::BoolObject::BoolObject(bool p_value) : Object(std::make_shared<definitions::BoolDefinition>(), {}, {}), value(p_value) {

}

cblang::objects::IntObject::IntObject(int p_value) : Object(std::make_shared<definitions::IntDefinition>(), {}, {}), value(p_value) {

}

cblang::objects::FloatObject::FloatObject(float p_value) : Object(std::make_shared<definitions::FloatDefinition>(), {}, {}), value(p_value) {

}

cblang::objects::CharObject::CharObject(char p_value) : Object(std::make_shared<definitions::CharDefinition>(), {}, {}), value(p_value) {

}

cblang::objects::StringObject::StringObject(std::string p_value) : Object(std::make_shared<definitions::StringDefinition>(), {}, {}), value(std::move(p_value)) {

}

cblang::objects::ArrayObject::ArrayObject(std::vector<std::shared_ptr<Object>> p_value, std::shared_ptr<definitions::TemplateDefinition> value_type) 
    : Object(
        std::make_shared<definitions::ArrayDefinition>(), 
        {
            std::move(value_type)
        }, 
        {}
    ), value(std::move(p_value)) 
{

}