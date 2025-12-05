#include "objects.hpp"

#include "program.hpp"
#include "definitions.hpp"

#include <cassert>
#include <memory>
#include <utility>
#include <vector>

using namespace std::placeholders;

cblang::objects::Object::Object(
    scanner::Token p_name,
    std::shared_ptr<ClassDefinition> p_type,
    std::vector<std::shared_ptr<TemplateDefinition>> p_templates
) : name(std::move(p_name)),
    type(std::move(p_type)), 
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
    scanner::Token p_name,
    std::vector<std::shared_ptr<TemplateDefinition>> p_templates,
    std::vector<std::shared_ptr<MemberDefinition>> p_parameters,
    std::vector<std::shared_ptr<parser::Statement>> p_code
) : Object(std::move(p_name), std::make_shared<definitions::FunctionDefinition>(), {}),
    templates(std::move(p_templates)),
    parameters(std::move(p_parameters)),
    code(std::move(p_code)) 
{

}

cblang::objects::FunctionObject::FunctionObject(
    scanner::Token p_name,
    std::vector<std::shared_ptr<TemplateDefinition>> p_templates,
    std::vector<std::shared_ptr<MemberDefinition>> p_parameters,
    InternalFunction p_internal
) : Object(std::move(p_name), std::make_shared<definitions::FunctionDefinition>(), {}),
    templates(std::move(p_templates)),
    parameters(std::move(p_parameters)),
    internal(std::move(p_internal))
{

}

auto cblang::objects::FunctionObject::call(std::vector<std::shared_ptr<TemplateDefinition>> in_templates, std::vector<std::shared_ptr<Object>> passed_params, std::vector<program::Scope> scope_stack) -> std::optional<std::shared_ptr<Object>> {
    program::Scope this_scope{};
    
}

