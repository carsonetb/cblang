#include "objects.hpp"

#include "program.hpp"
#include "definitions.hpp"

#include <cassert>
#include <memory>
#include <utility>
#include <vector>

using namespace std::placeholders;

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

auto cblang::objects::FunctionObject::call(std::vector<std::shared_ptr<TemplateDefinition>> in_templates, std::vector<std::shared_ptr<Object>> passed_params, std::vector<program::Scope> scope_stack) -> std::optional<std::shared_ptr<Object>> {
    program::Scope this_scope{};
    
}

cblang::objects::MultipleFunctionObject::MultipleFunctionObject(
    std::vector<std::shared_ptr<FunctionObject>> p_objects
) : Object(std::make_shared<definitions::FunctionDefinition>(), {}),
    objects(std::move(p_objects)) {

}