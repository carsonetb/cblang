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

#define INTERNAL_FUNCTION_PARAMS [this](const std::vector<std::shared_ptr<TemplateDefinition>>& in_templates, const std::vector<std::shared_ptr<Object>>& params) -> std::optional<std::shared_ptr<cblang::objects::Object>>

// This is horid but I can't think of a better way.
#define LITERAL_EQUALITY_OPERATORS(ObjectType) InternalFunction eq_eq_oper = [this](const std::vector<std::shared_ptr<TemplateDefinition>>& in_templates, const std::vector<std::shared_ptr<Object>>& params) { \
        auto other = std::dynamic_pointer_cast<ObjectType>(params[0]); \
        return std::make_shared<BoolObject>(value == other->value); \
    }; \
\
    InternalFunction not_eq_oper = [this](const std::vector<std::shared_ptr<TemplateDefinition>>& in_templates, const std::vector<std::shared_ptr<Object>>& params) { \
        auto other = std::dynamic_pointer_cast<ObjectType>(params[0]); \
        return std::make_shared<BoolObject>(value != other->value); \
    }; \
    \
    members_by_name["=="] = Variable::generate( \
        scanner::Token::create_external("=="), \
        FunctionObject::generate( \
            std::dynamic_pointer_cast<FunctionMember>(type->members_by_name["=="]), \
            eq_eq_oper, \
            false, true, true, false \
        ), false, false, true \
    ); \
    \
    members_by_name["!="] = Variable::generate( \
        scanner::Token::create_external("!="), \
        FunctionObject::generate( \
            std::dynamic_pointer_cast<FunctionMember>(type->members_by_name["!="]), \
            not_eq_oper, \
            false, true, true, false \
        ), false, false, true \
    );

using namespace cblang;
using namespace cblang::objects;

cblang::objects::Variable::Variable(
    scanner::Token p_name, 
    std::shared_ptr<objects::Object> p_object, 
    bool p_is_private, 
    bool p_is_static,
    bool p_is_const
) : name(std::move(p_name)), 
    object(std::move(p_object)),
    is_private(p_is_private),
    is_static(p_is_static),
    is_const(p_is_const)
{
    object->initialize();
}

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
}

cblang::objects::Object::~Object() = default;

// TODO: This isn't called everywhere.
auto cblang::objects::Object::initialize() -> void {
    for (const auto& member : type->members_by_name) {
        auto member_def = member.second;
        if (std::dynamic_pointer_cast<ClassDefinition>(member_def)) {
            continue;
        }
        auto as_function = std::dynamic_pointer_cast<FunctionMember>(member_def);
        if (as_function) {
            if (!as_function->code.has_value()) {
                continue; // This will probably have been created already or will be created by whatever Object defined it.
            }
            members_by_name[as_function->function_name.raw] = std::make_shared<Variable>(
                as_function->function_name, 
                std::make_shared<FunctionObject>(
                    as_function, 
                    as_function->code.value(),
                    as_function->is_cast,
                    as_function->is_operator,
                    as_function->is_const,
                    as_function->is_static
                ),
                as_function->is_private,
                as_function->is_static,
                true // functions can't be modified ... 
            );
            continue;
        }
        if (member_def->is_static) { // TODO: Static variables should be initialized correctly
            continue;
        }
        if (!member_def->initializer) {
            throw program::handle_error(member_def->name, "Member requires an initializer.");
        }
        program::ScopeParser parser = program::ScopeParser(member_def->initializer.value(), {get_scope()}); // TODO: Slow
        members_by_name[member_def->name.raw] = std::make_shared<Variable>(
            member_def->name, 
            parser.process_expr(), 
            member_def->is_private, 
            member_def->is_static, 
            member_def->is_const
        );
    }
}

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
    auto out = std::make_shared<program::Scope>(shared_from_this());
    out->defined_variables = members_by_name;
    out->defined_templates = defined_templates;
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
    std::optional<std::shared_ptr<TemplatedType>> p_returns,
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
        auto this_variable = std::make_shared<Variable>(param_def->name, param_obj, false, false, false);
    }
}

cblang::objects::FunctionObject::FunctionObject(
    const std::shared_ptr<definitions::FunctionMember>& p_definition,
    std::vector<std::shared_ptr<parser::Statement>> p_code,
    bool p_is_cast,
    bool p_is_operator,
    bool p_is_const,
    bool p_is_static
) : Callable(p_definition->returns, p_definition->templates, p_definition->parameters),
    declare_point(p_definition->function_name),
    function_name(p_definition->function_name),
    code(std::move(p_code)),
    is_cast(p_is_cast),
    is_operator(p_is_operator),
    is_const(p_is_const),
    is_static(p_is_static)
{

}

cblang::objects::FunctionObject::FunctionObject(
    const std::shared_ptr<definitions::FunctionMember>& p_definition,
    InternalFunction p_internal,
    bool p_is_cast,
    bool p_is_operator,
    bool p_is_const,
    bool p_is_static
) : Callable(p_definition->returns, p_definition->templates, p_definition->parameters),
    declare_point(p_definition->function_name),
    function_name(p_definition->function_name),
    internal(std::move(p_internal)),
    is_cast(p_is_cast),
    is_operator(p_is_operator),
    is_const(p_is_const),
    is_static(p_is_static)
{

}

cblang::objects::FunctionObject::FunctionObject(
    scanner::Token p_declare_point,
    std::vector<std::shared_ptr<MemberDefinition>> p_parameters,
    std::vector<std::shared_ptr<TemplateDefinition>> p_templates,
    std::optional<std::shared_ptr<TemplatedType>> p_returns,
    std::optional<std::vector<std::shared_ptr<parser::Statement>>> p_code,
    bool p_is_cast,
    bool p_is_operator,
    bool p_is_const,
    bool p_is_static
) : Callable(std::move(p_returns), std::move(p_templates), std::move(p_parameters)),
    declare_point(std::move(p_declare_point)),
    code(std::move(p_code)),
    is_cast(p_is_cast),
    is_operator(p_is_operator),
    is_const(p_is_const),
    is_static(p_is_static)
{

}

auto cblang::objects::FunctionObject::call_this(const scanner::Token& call_point, const std::vector<std::shared_ptr<TemplateDefinition>>& in_templates, const std::vector<std::shared_ptr<Object>>& passed_params, std::vector<std::shared_ptr<program::Scope>> owner_scope) const -> std::optional<std::shared_ptr<Object>> {
    auto owner = owner_scope.back()->scope_object;
    auto call_scope = std::make_shared<program::Scope>(owner);
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
    LITERAL_EQUALITY_OPERATORS(BoolObject);
}

cblang::objects::IntObject::IntObject(int p_value) : Object(std::make_shared<definitions::IntDefinition>(), {}, {}), value(p_value) {
    LITERAL_EQUALITY_OPERATORS(IntObject);
}

cblang::objects::FloatObject::FloatObject(float p_value) : Object(std::make_shared<definitions::FloatDefinition>(), {}, {}), value(p_value) {
    LITERAL_EQUALITY_OPERATORS(FloatObject);
}

cblang::objects::CharObject::CharObject(char p_value) : Object(std::make_shared<definitions::CharDefinition>(), {}, {}), value(p_value) {
    LITERAL_EQUALITY_OPERATORS(CharObject);
}

cblang::objects::StringObject::StringObject(std::string p_value) : Object(std::make_shared<definitions::StringDefinition>(), {}, {}), value(std::move(p_value)) {
    LITERAL_EQUALITY_OPERATORS(StringObject);
}

cblang::objects::ArrayObject::ArrayObject(std::vector<std::shared_ptr<Object>> p_value, std::shared_ptr<definitions::TemplateDefinition> value_type) 
    : Object(
        std::make_shared<definitions::ArrayDefinition>(value_type), 
        {}, 
        {}
    ), value(std::move(p_value)) 
{
    defined_templates["value_type"] = value_type;

    InternalFunction append = INTERNAL_FUNCTION_PARAMS {
        value.push_back(params[0]);

        return {};
    };
    members_by_name["append"] = Variable::generate(
        scanner::Token::create_external("append"),
        FunctionObject::generate(
            type->get_function("append"),
            append,
            false, false, false, false
        ),
        false, false, true
    );
}