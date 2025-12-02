#include "definitions.hpp"
#include "util.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#define CLASS_TYPE std::make_shared<TemplatedType>(std::make_shared<ClassDefinition>("class", std::vector<std::shared_ptr<MemberDefinition>>(), std::vector<std::shared_ptr<MemberDefinition>>(), std::vector<std::shared_ptr<TemplateDefinition>>()), std::vector<std::shared_ptr<TemplatedType>>()) // bruh

using namespace cblang::definitions;

cblang::definitions::MemberDefinition::MemberDefinition(
    std::shared_ptr<TemplatedType> type,
    std::string name,
    std::optional<std::shared_ptr<parser::Expr>> initializer
) : type(std::move(type)), name(std::move(name)), initializer(std::move(initializer)) {}

cblang::definitions::MemberDefinition::~MemberDefinition() = default;

cblang::definitions::FunctionMember::FunctionMember(
    std::string name, 
    std::vector<std::shared_ptr<MemberDefinition>> p_parameters, 
    std::vector<std::shared_ptr<TemplateDefinition>> p_templates, 
    std::optional<std::shared_ptr<ClassDefinition>> p_returns
) : MemberDefinition(std::make_shared<TemplatedType>(std::make_shared<FunctionDefinition>(), std::vector<std::shared_ptr<TemplatedType>>()), std::move(name), {}), // Maybe this has an initializer as the scope {}? 
    parameters(std::move(p_parameters)), 
    templates(std::move(p_templates)), 
    returns(std::move(p_returns))
{
    for (const auto& template_param : templates) {
        templates_by_name[template_param->template_name] = template_param;
    }
}

cblang::definitions::ClassDefinition::ClassDefinition(
    std::string p_templated_type_name,
    std::vector<std::shared_ptr<MemberDefinition>> p_params,
    std::vector<std::shared_ptr<MemberDefinition>> p_members,
    std::vector<std::shared_ptr<TemplateDefinition>> p_templates
) : MemberDefinition(CLASS_TYPE, split(p_templated_type_name, "<")[0], std::optional<std::shared_ptr<parser::Expr>>()), // Cooked
    templated_type_name(std::move(p_templated_type_name)), 
    params(std::move(p_params)), 
    members(std::move(p_members)), 
    templates(std::move(p_templates)) 
{
    type_name = split(p_templated_type_name, "<")[0];
    for (const auto& member : p_members) {
        members_by_name[member->name] = member;
    }
    for (const auto& templ : p_templates) {
        templates_by_name[templ->template_name] = templ;
    }
};

cblang::definitions::UserDefinition::UserDefinition(
    const std::string& p_name, 
    const std::vector<std::shared_ptr<MemberDefinition>>& p_params,
    const std::vector<std::shared_ptr<MemberDefinition>>& p_members, 
    const std::vector<std::shared_ptr<TemplateDefinition>>& p_templates
) : ClassDefinition(p_name, p_params, p_members, p_templates) {};

auto cblang::definitions::UserDefinition::is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool {
    return false;
}

auto cblang::definitions::UserDefinition::can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool {
    return false;
}

cblang::definitions::BoolDefinition::BoolDefinition() : ClassDefinition("bool", {}, {}, {}) {}

auto cblang::definitions::BoolDefinition::is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool {
    return def->type_name == "int";
}

auto cblang::definitions::BoolDefinition::can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool {
    return def->type_name == "int";
}

cblang::definitions::IntDefinition::IntDefinition() : ClassDefinition("int", {}, {}, {}) {};

auto cblang::definitions::IntDefinition::is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool {
    return def->type_name == "char" || def->type_name == "bool" || def->type_name == "string";
}

auto cblang::definitions::IntDefinition::can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool {
    return def->type_name == "char" || def->type_name == "bool";
}

cblang::definitions::FloatDefinition::FloatDefinition() : ClassDefinition("float", {}, {}, {}) {}

auto cblang::definitions::FloatDefinition::is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool {
    return false;
}

auto cblang::definitions::FloatDefinition::can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool {
    return def->type_name == "string";
}

cblang::definitions::CharDefinition::CharDefinition() : ClassDefinition("char", {}, {}, {}) {}

auto cblang::definitions::CharDefinition::is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool {
    return def->type_name == "int";
}

auto cblang::definitions::CharDefinition::can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool {
    return def->type_name == "int";
}

cblang::definitions::StringDefinition::StringDefinition() : ClassDefinition("string", {}, {}, {}) {}

auto cblang::definitions::StringDefinition::is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool {
    return def->templated_type_name == "array<char>" || def->type_name == "int";
}

auto cblang::definitions::StringDefinition::can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool {
    return def->templated_type_name == "array<char>";
}

cblang::definitions::ArrayDefinition::ArrayDefinition() : ClassDefinition("array<value_type>", {}, {}, {std::make_shared<TemplateDefinition>("value_type")}) {}

auto cblang::definitions::ArrayDefinition::is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool {
    return false;
}

auto cblang::definitions::ArrayDefinition::can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool {
    return false;
}

cblang::definitions::FunctionDefinition::FunctionDefinition() : ClassDefinition("scope", {}, {}, {}) {}

auto cblang::definitions::FunctionDefinition::is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool {
    return false;
}

auto cblang::definitions::FunctionDefinition::can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool {
    return false;
}

cblang::definitions::TemplateDefinition::TemplateDefinition(std::string p_template_name) : template_name(std::move(p_template_name)) {}