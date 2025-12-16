#include "definitions.hpp"
#include "compiler.hpp"
#include "objects.hpp"
#include "parser.hpp"
#include "program.hpp"
#include "scanner.hpp"
#include "util.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace cblang::definitions;

static const auto CLASS_TYPE = std::make_shared<TemplatedType>(std::make_shared<ClassDefinition>(TEMPLATED_EMPTY("class"), std::vector<std::shared_ptr<MemberDefinition>>(), std::vector<std::shared_ptr<MemberDefinition>>()), std::vector<std::shared_ptr<TemplatedType>>());

auto cblang::definitions::TemplatedType::operator==(const TemplatedType& rhs) const -> bool {
    if (cls->name.raw != rhs.cls->name.raw) {
        return false;
    }
    if (templates.size() != rhs.templates.size()) {
        return false;
    }
    for (int i = 0; i < templates.size(); i++) {
        const auto& this_template = templates[i];
        auto other_template = rhs.templates[i];
        if (*this_template != *other_template) {
            return false;
        }
    }
    return true;
}

auto cblang::definitions::TemplatedType::operator!=(const TemplatedType& rhs) const -> bool {
    return !operator==(rhs);
}

auto cblang::definitions::TemplatedType::stringify() const -> std::string {
    std::string out;
    if (templates.empty()) {
        return cls->name.raw;
    }
    out += cls->name.raw + "<";
    for (const auto& templated : templates) {
        out += templated->stringify() + ", ";
    }
    out += ">";
    return out;
}

cblang::definitions::MemberDefinition::MemberDefinition(
    std::shared_ptr<TemplatedType> type,
    scanner::Token name,
    std::optional<std::shared_ptr<parser::Expr>> initializer,
                bool p_is_static,
                bool p_is_private,
                bool p_is_const
) : type(std::move(type)), name(std::move(name)), initializer(std::move(initializer)),
    is_static(p_is_static), is_private(p_is_private), is_const(p_is_const) 
{

}

cblang::definitions::MemberDefinition::~MemberDefinition() = default;

cblang::definitions::FunctionMember::FunctionMember(
    const std::shared_ptr<parser::Templated>& p_name, 
    std::vector<std::shared_ptr<MemberDefinition>> p_parameters, 
    std::optional<std::shared_ptr<TemplatedType>> p_returns,
    std::vector<std::shared_ptr<parser::Statement>> p_code,
    bool p_is_static,
    bool p_is_private,
    bool p_is_const,
    bool p_is_operator,
    bool p_is_cast
) : MemberDefinition(
        std::make_shared<TemplatedType>(std::make_shared<FunctionDefinition>(), 
        std::vector<std::shared_ptr<TemplatedType>>()), 
        p_name->name, 
        {}, 
        p_is_static, 
        p_is_private, 
        p_is_const
    ),
    function_name(p_name->name),
    templated_name(p_name),
    parameters(std::move(p_parameters)), 
    returns(std::move(p_returns)),
    code(std::move(p_code)),
    is_operator(p_is_operator),
    is_cast(p_is_cast)
{
    for (const auto& template_param : templated_name->templates) {
        auto definition = std::make_shared<TemplateDefinition>(template_param->name);
        if (templates_by_name.contains(definition->template_name.raw)) {
            throw compiler::handle_error(definition->template_name, "A template type named " + definition->template_name.raw + " already exists!");
        }
        templates_by_name[definition->template_name.raw] = definition;
    }
}

auto cblang::definitions::FunctionMember::validate_call(const scanner::Token& call_point, const std::vector<std::shared_ptr<TemplateDefinition>>& test_templates, const std::vector<std::shared_ptr<TemplatedType>>& args) -> void {
    if (test_templates.size() != templates.size()) {
        throw compiler::handle_error(call_point, "Incorrect number of templates supplied to function call.");
    }
    if (args.size() != parameters.size()) {
        throw compiler::handle_error(call_point, "Incorrect number of parameters to function call.");
    }
    for (int i = 0; i < parameters.size(); i++) {
        const auto& expected = parameters[i];
        const auto& supplied = args[i];
        if (*expected->type != *supplied) {
            throw compiler::handle_error(call_point, "(for argument " + std::to_string(i) + ") Expected type '" + expected->type->stringify() + "' but type '" + supplied->stringify() + "' was supplied.");
        }
    }
}

cblang::definitions::ClassDefinition::ClassDefinition(
    std::shared_ptr<parser::Templated> p_name,
    std::vector<std::shared_ptr<MemberDefinition>> p_params,
    std::vector<std::shared_ptr<MemberDefinition>> p_members
) : MemberDefinition(CLASS_TYPE, p_name->name, std::optional<std::shared_ptr<parser::Expr>>(), true, false, false), // Cooked
    type_name(std::move(p_name)),
    params(std::move(p_params)), 
    members(std::move(p_members))
{
    for (const auto& member : members) {
        if (members_by_name.contains(member->name.raw)) {
            throw compiler::handle_error(member->name, "A member of name " + member->name.raw + " already exists! (defined on line " + std::to_string(members_by_name[member->name.raw]->name.line) + ")");
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

auto cblang::definitions::ClassDefinition::is_constructor_valid(const std::shared_ptr<ClassDefinition>& def) -> bool {
    return false;
}

auto cblang::definitions::ClassDefinition::can_convert_to(const std::shared_ptr<ClassDefinition>& def) -> bool {
    return false;
}

auto cblang::definitions::ClassDefinition::get_functions() const -> std::vector<std::shared_ptr<FunctionMember>> {
    std::vector<std::shared_ptr<FunctionMember>> out;
    for (const auto& member : members) {
        auto as_function = std::dynamic_pointer_cast<FunctionMember>(member);
        if (as_function) {
            out.push_back(as_function);
        }
    }
    return out;
}

auto cblang::definitions::ClassDefinition::create_object(const scanner::Token& creation_point, const std::vector<std::shared_ptr<TemplateDefinition>>& templates, const std::vector<std::shared_ptr<objects::Object>>& params) -> std::shared_ptr<objects::Object> {
    throw program::handle_error(creation_point, "Class cannot be created directly (via calling a constructor function).");
}

cblang::definitions::UserDefinition::UserDefinition(
    const std::shared_ptr<parser::Templated>& p_name, 
    const std::vector<std::shared_ptr<MemberDefinition>>& p_params,
    const std::vector<std::shared_ptr<MemberDefinition>>& p_members
) : ClassDefinition(p_name, p_params, p_members) {};

auto cblang::definitions::UserDefinition::is_constructor_valid(const std::shared_ptr<ClassDefinition>& def) -> bool {
    return false;
}

auto cblang::definitions::UserDefinition::can_convert_to(const std::shared_ptr<ClassDefinition>& def) -> bool {
    return false;
}

auto cblang::definitions::UserDefinition::create_object(const scanner::Token& creation_point, const std::vector<std::shared_ptr<TemplateDefinition>>& templates, const std::vector<std::shared_ptr<objects::Object>>& passed_params) -> std::shared_ptr<objects::Object> {
    if (passed_params.size() != params.size()) {
        throw program::handle_error(creation_point, "Passed " + std::to_string(passed_params.size()) + " params but expected " + std::to_string(params.size()) + ".");
    }
    std::vector<std::shared_ptr<objects::Variable>> var_params;
    for (int i = 0; i < passed_params.size(); i++) {
        auto passed_param = passed_params[i];
        auto param_def = params[i];
        if (*passed_param->get_templated() != *param_def->type) {
            throw program::handle_error(creation_point, "For argument " + std::to_string(i) + ": Passed param of type " + passed_param->get_templated()->stringify() + " but expected " + param_def->type->stringify() + ".");
        }
        var_params.push_back(std::make_shared<objects::Variable>(param_def->name, passed_param, false, false, false));
    }
    auto out = std::make_shared<objects::Object>(shared_from_this(), templates, var_params);
    out->initialize();
    return out;
}

cblang::definitions::BoolDefinition::BoolDefinition() : ClassDefinition(TEMPLATED_EMPTY("bool"), {}, {}) {}

auto cblang::definitions::BoolDefinition::is_constructor_valid(const std::shared_ptr<ClassDefinition>& def) -> bool {
    return def->pretty_name == "int";
}

auto cblang::definitions::BoolDefinition::can_convert_to(const std::shared_ptr<ClassDefinition>& def) -> bool {
    return def->pretty_name == "int";
}

cblang::definitions::IntDefinition::IntDefinition() : ClassDefinition(TEMPLATED_EMPTY("int"), {}, {}) {};

auto cblang::definitions::IntDefinition::is_constructor_valid(const std::shared_ptr<ClassDefinition>& def) -> bool {
    return def->pretty_name == "char" || def->pretty_name == "bool" || def->pretty_name == "string";
}

auto cblang::definitions::IntDefinition::can_convert_to(const std::shared_ptr<ClassDefinition>& def) -> bool {
    return def->pretty_name == "char" || def->pretty_name == "bool";
}

cblang::definitions::FloatDefinition::FloatDefinition() : ClassDefinition(TEMPLATED_EMPTY("float"), {}, {}) {}

auto cblang::definitions::FloatDefinition::is_constructor_valid(const std::shared_ptr<ClassDefinition>& def) -> bool {
    return false;
}

auto cblang::definitions::FloatDefinition::can_convert_to(const std::shared_ptr<ClassDefinition>& def) -> bool {
    return def->pretty_name == "string";
}

cblang::definitions::CharDefinition::CharDefinition() : ClassDefinition(TEMPLATED_EMPTY("char"), {}, {}) {}

auto cblang::definitions::CharDefinition::is_constructor_valid(const std::shared_ptr<ClassDefinition>& def) -> bool {
    return def->pretty_name == "int";
}

auto cblang::definitions::CharDefinition::can_convert_to(const std::shared_ptr<ClassDefinition>& def) -> bool {
    return def->pretty_name == "int";
}

cblang::definitions::StringDefinition::StringDefinition() : ClassDefinition(TEMPLATED_EMPTY("string"), {}, {}) {}

auto cblang::definitions::StringDefinition::is_constructor_valid(const std::shared_ptr<ClassDefinition>& def) -> bool {
    return def->pretty_name == "array<char>";
}

auto cblang::definitions::StringDefinition::can_convert_to(const std::shared_ptr<ClassDefinition>& def) -> bool {
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

cblang::definitions::TemplateDefinition::TemplateDefinition(std::shared_ptr<TemplatedType> p_template_used) : template_used(std::move(p_template_used)) {}