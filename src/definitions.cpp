#include "definitions.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace cblang::definitions;

cblang::definitions::MemberDefinition::MemberDefinition(std::string name) : name(std::move(name)) {
    
}

cblang::definitions::MemberDefinition::~MemberDefinition() = default;

cblang::definitions::ParameterDefinition::ParameterDefinition(std::string name) : MemberDefinition(std::move(name)) {

};

cblang::definitions::FunctionDefinition::FunctionDefinition(
    std::string name, 
    std::vector<std::shared_ptr<ParameterDefinition>> p_parameters, 
    std::vector<std::shared_ptr<TemplateDefinition>> p_templates, 
    std::shared_ptr<ClassDefinition> p_returns
) : MemberDefinition(std::move(name)), 
    parameters(std::move(p_parameters)), 
    templates(std::move(p_templates)), 
    returns(std::move(p_returns))
{
    for (const auto& template_param : templates) {
        templates_by_name[template_param->type_name] = template_param;
    }
}

cblang::definitions::ClassDefinition::ClassDefinition() = default;

cblang::definitions::ClassDefinition::~ClassDefinition() = default;

cblang::definitions::UserDefinition::UserDefinition() = default;

cblang::definitions::UserDefinition::UserDefinition(
    std::string name, 
    const std::vector<std::shared_ptr<MemberDefinition>>& p_members, 
    std::vector<std::shared_ptr<TemplateDefinition>> p_templates
) {
    type_name = std::move(name);
    for (const auto& member : p_members) {
        members_by_name[member->name] = member;
    }
    templates = std::move(p_templates);
    for (const auto& template_param : templates) {
        templates_by_name[template_param->type_name] = template_param;
    }
};

auto cblang::definitions::UserDefinition::is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool {
    return false;
}

auto cblang::definitions::UserDefinition::can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool {
    return false;
}

cblang::definitions::BoolDefinition::BoolDefinition() {
    type_name = "bool";
};

auto cblang::definitions::BoolDefinition::is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool {
    return def->type_name == "int";
}

auto cblang::definitions::BoolDefinition::can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool {
    return def->type_name == "int";
}

cblang::definitions::IntDefinition::IntDefinition() {
    type_name = "int";
};

auto cblang::definitions::IntDefinition::is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool {
    return def->type_name == "char" || def->type_name == "bool";
}

auto cblang::definitions::IntDefinition::can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool {
    return def->type_name == "char" || def->type_name == "bool";
}

cblang::definitions::CharDefinition::CharDefinition() {
    type_name = "char";
}

auto cblang::definitions::CharDefinition::is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool {
    return def->type_name == "int";
}

auto cblang::definitions::CharDefinition::can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool {
    return def->type_name == "int";
}

cblang::definitions::StringDefinition::StringDefinition() {
    type_name = "char";
}

auto cblang::definitions::StringDefinition::is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool {
    return def->templated_type_name == "array<char>";
}

auto cblang::definitions::StringDefinition::can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool {
    return def->templated_type_name == "array<char>";
}

cblang::definitions::ArrayDefinition::ArrayDefinition() {
    type_name = "array";

    auto value_type = std::make_shared<TemplateDefinition>("value_type");
    templates.push_back(value_type);
    templates_by_name["value_type"] = value_type;
}

auto cblang::definitions::ArrayDefinition::is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool {
    return false;
}

auto cblang::definitions::ArrayDefinition::can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool {
    return false;
}

cblang::definitions::TemplateDefinition::TemplateDefinition(std::string template_title) {
    type_name = std::move(template_title);
}

auto cblang::definitions::TemplateDefinition::is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool {
    return false;
}

auto cblang::definitions::TemplateDefinition::can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool {
    return false;
}