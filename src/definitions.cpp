#include "definitions.hpp"
#include "compiler.hpp"
#include "parser.hpp"
#include "scanner.hpp"
#include "util.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace cblang::definitions;

static const auto CLASS_TYPE = std::make_shared<TemplatedType>(std::make_shared<ClassDefinition>(TEMPLATED_EMPTY("class"), std::vector<std::shared_ptr<MemberDefinition>>(), std::vector<std::shared_ptr<MemberDefinition>>()), std::vector<std::shared_ptr<TemplatedType>>());

cblang::definitions::MemberDefinition::MemberDefinition(
    std::shared_ptr<TemplatedType> type,
    scanner::Token name,
    std::optional<std::shared_ptr<parser::Expr>> initializer
) : type(std::move(type)), name(std::move(name)), initializer(std::move(initializer)) {}

cblang::definitions::MemberDefinition::~MemberDefinition() = default;

cblang::definitions::FunctionMember::FunctionMember(
    const std::shared_ptr<parser::Templated>& p_name, 
    std::vector<std::shared_ptr<MemberDefinition>> p_parameters, 
    std::optional<std::shared_ptr<ClassDefinition>> p_returns,
    std::vector<std::shared_ptr<parser::Statement>> p_code
) : MemberDefinition(std::make_shared<TemplatedType>(std::make_shared<FunctionDefinition>(), std::vector<std::shared_ptr<TemplatedType>>()), p_name->name, {}), // Maybe this has an initializer as the scope {}? 
    function_name(p_name->name),
    templated_name(p_name),
    parameters(std::move(p_parameters)), 
    returns(std::move(p_returns)),
    code(std::move(p_code))
{
    for (const auto& template_param : templated_name->templates) {
        auto definition = std::make_shared<TemplateDefinition>(template_param->name);
        if (templates_by_name.contains(definition->template_name.raw)) {
            throw compiler::handle_error(definition->template_name, "A template type named " + definition->template_name.raw + " already exists!");
        }
        templates_by_name[definition->template_name.raw] = definition;
    }
}

cblang::definitions::ClassDefinition::ClassDefinition(
    std::shared_ptr<parser::Templated> p_name,
    std::vector<std::shared_ptr<MemberDefinition>> p_params,
    std::vector<std::shared_ptr<MemberDefinition>> p_members
) : MemberDefinition(CLASS_TYPE, p_name->name, std::optional<std::shared_ptr<parser::Expr>>()), // Cooked
    type_name(std::move(p_name)),
    params(std::move(p_params)), 
    members(std::move(p_members))
{
    for (const auto& member : members) {
        if (members_by_name.contains(member->name.raw)) {
            throw compiler::handle_error(member->name, "A member of name " + member->name.raw + " already exists!");
        }
        members_by_name[member->name.raw] = member;
    }
    for (const auto& template_name : type_name->templates) {
        if (!template_name->templates.empty()) {
            throw compiler::handle_error(template_name->templates[0]->name, "Class templates definitions cannot have sub templates!");
        }
        templates.push_back(std::make_shared<TemplateDefinition>(template_name->name));
    }
    for (const auto& templ : templates) {
        if (templates_by_name.contains(templ->template_name.raw)) {
            throw compiler::handle_error(templ->template_name, "A template type named " + templ->template_name.raw + " already exists!");
        }
        templates_by_name[templ->template_name.raw] = templ;
    }
    pretty_name = parser::debug_templates(type_name);
};

auto cblang::definitions::ClassDefinition::is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool {
    return false;
}

auto cblang::definitions::ClassDefinition::can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool {
    return false;
}

cblang::definitions::UserDefinition::UserDefinition(
    const std::shared_ptr<parser::Templated>& p_name, 
    const std::vector<std::shared_ptr<MemberDefinition>>& p_params,
    const std::vector<std::shared_ptr<MemberDefinition>>& p_members
) : ClassDefinition(p_name, p_params, p_members) {};

auto cblang::definitions::UserDefinition::is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool {
    return false;
}

auto cblang::definitions::UserDefinition::can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool {
    return false;
}

cblang::definitions::BoolDefinition::BoolDefinition() : ClassDefinition(TEMPLATED_EMPTY("bool"), {}, {}) {}

auto cblang::definitions::BoolDefinition::is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool {
    return def->pretty_name == "int";
}

auto cblang::definitions::BoolDefinition::can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool {
    return def->pretty_name == "int";
}

cblang::definitions::IntDefinition::IntDefinition() : ClassDefinition(TEMPLATED_EMPTY("int"), {}, {}) {};

auto cblang::definitions::IntDefinition::is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool {
    return def->pretty_name == "char" || def->pretty_name == "bool" || def->pretty_name == "string";
}

auto cblang::definitions::IntDefinition::can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool {
    return def->pretty_name == "char" || def->pretty_name == "bool";
}

cblang::definitions::FloatDefinition::FloatDefinition() : ClassDefinition(TEMPLATED_EMPTY("float"), {}, {}) {}

auto cblang::definitions::FloatDefinition::is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool {
    return false;
}

auto cblang::definitions::FloatDefinition::can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool {
    return def->pretty_name == "string";
}

cblang::definitions::CharDefinition::CharDefinition() : ClassDefinition(TEMPLATED_EMPTY("char"), {}, {}) {}

auto cblang::definitions::CharDefinition::is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool {
    return def->pretty_name == "int";
}

auto cblang::definitions::CharDefinition::can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool {
    return def->pretty_name == "int";
}

cblang::definitions::StringDefinition::StringDefinition() : ClassDefinition(TEMPLATED_EMPTY("string"), {}, {}) {}

auto cblang::definitions::StringDefinition::is_constructor_valid(std::shared_ptr<ClassDefinition> def) -> bool {
    return def->pretty_name == "array<char>";
}

auto cblang::definitions::StringDefinition::can_convert_to(std::shared_ptr<ClassDefinition> def) -> bool {
    return def->pretty_name == "array<char>";
}

cblang::definitions::ArrayDefinition::ArrayDefinition() : ClassDefinition(
    std::make_shared<parser::Templated>(
        scanner::Token(scanner::IDENTIFIER, "array", -1), 
        std::vector<std::shared_ptr<parser::Templated>>{
            TEMPLATED_EMPTY("value_type")
        }
    ),
    {}, {}
) {} //ClassDefinition("array<value_type>", {}, {}, {std::make_shared<TemplateDefinition>("value_type")}) {}

cblang::definitions::FunctionDefinition::FunctionDefinition() : ClassDefinition(TEMPLATED_EMPTY("scope"), {}, {}) {}

cblang::definitions::TemplateDefinition::TemplateDefinition(scanner::Token p_template_name) : template_name(std::move(p_template_name)) {}