#include "standard.hpp"

#include "compiler.hpp"
#include "definitions.hpp"
#include "objects.hpp"
#include "parser.hpp"
#include "program.hpp"
#include "scanner.hpp"
#include "util.hpp"
#include <iostream>
#include <memory>
#include <optional>

using namespace cblang::standard;
using namespace cblang::definitions;

#define CREATE_OPERATOR(name, code) members_by_name[name] = cblang::objects::create_function(type, name, INTERNAL_FUNCTION_PARAMS code, true, false, false, true)

cblang::standard::DoubleDefinition::DoubleDefinition() : ClassDefinition(TEMPLATED_EMPTY("Double"), {}, {}) {}

auto cblang::standard::DoubleDefinition::generate() -> std::shared_ptr<DoubleDefinition> {
    auto double_def = unsafe_singleton();
    auto bool_def = BoolDefinition::generate();

    if (double_def->generated) {
        return double_def;
    }

    DEFINE_AND_ADD_OPERATOR(DoubleDefinition, double_def, "==", bool_def);
    DEFINE_AND_ADD_OPERATOR(DoubleDefinition, double_def, "!=", bool_def);
    DEFINE_AND_ADD_OPERATOR(DoubleDefinition, double_def, "+", double_def);
    DEFINE_AND_ADD_OPERATOR(DoubleDefinition, double_def, "-", double_def);
    DEFINE_AND_ADD_OPERATOR(DoubleDefinition, double_def, "*", double_def);
    DEFINE_AND_ADD_OPERATOR(DoubleDefinition, double_def, "/", double_def);
    DEFINE_AND_ADD_OPERATOR(DoubleDefinition, double_def, "<", bool_def);
    DEFINE_AND_ADD_OPERATOR(DoubleDefinition, double_def, ">", bool_def);
    DEFINE_AND_ADD_OPERATOR(DoubleDefinition, double_def, "<=", bool_def);
    DEFINE_AND_ADD_OPERATOR(DoubleDefinition, double_def, ">=", bool_def);

    double_def->generated = true;

    return double_def;
}

auto cblang::standard::DoubleDefinition::can_convert_to(const std::shared_ptr<ClassDefinition>& def) -> bool {
    return def->pretty_name == "string" || def->pretty_name == "bool";
}

auto cblang::standard::DoubleDefinition::can_convert_from(const std::shared_ptr<TemplatedType>& type) -> bool {
    return type->cls->pretty_name == "int" || type->cls->pretty_name == "float";
}

auto cblang::standard::DoubleDefinition::cast_from(const std::shared_ptr<objects::Object>& obj) -> std::optional<std::shared_ptr<objects::Object>> {
    if (auto as_int = std::dynamic_pointer_cast<objects::IntObject>(obj)) {
        return DoubleObject::create(static_cast<double>(as_int->value));
    }
    if (auto as_float = std::dynamic_pointer_cast<objects::FloatObject>(obj)) {
        return DoubleObject::create(static_cast<double>(as_float->value));
    }
    return {};
}

cblang::standard::DoubleObject::DoubleObject(double p_value) : objects::Object(DoubleDefinition::generate(), {}, {}), value(p_value) {
    #define SIMPLE_DOUBLE_OPERATOR(RetType, oper) CREATE_OPERATOR(#oper, {return std::make_shared<RetType>(val oper GET_PARAM(DoubleObject, 0)->value);});

    SIMPLE_DOUBLE_OPERATOR(objects::BoolObject, ==);
    SIMPLE_DOUBLE_OPERATOR(objects::BoolObject, !=);
    SIMPLE_DOUBLE_OPERATOR(DoubleObject, +);
    SIMPLE_DOUBLE_OPERATOR(DoubleObject, -);
    SIMPLE_DOUBLE_OPERATOR(DoubleObject, *);
    SIMPLE_DOUBLE_OPERATOR(DoubleObject, /);
    SIMPLE_DOUBLE_OPERATOR(objects::BoolObject, >);
    SIMPLE_DOUBLE_OPERATOR(objects::BoolObject, <);
    SIMPLE_DOUBLE_OPERATOR(objects::BoolObject, <=);
    SIMPLE_DOUBLE_OPERATOR(objects::BoolObject, >=);

    #undef SIMPLE_DOUBLE_OPERATOR
}

auto cblang::standard::DoubleObject::cast_to(const std::shared_ptr<TemplatedType>& type) -> std::optional<std::shared_ptr<objects::Object>> {
    if (type->cls->pretty_name == "double") {
        return std::make_shared<DoubleObject>(*this);
    }
    if (type->cls->pretty_name == "bool") {
        return objects::BoolObject::create(value != 0.0);
    }
    if (type->cls->pretty_name == "string") {
        return objects::StringObject::create(std::to_string(value));
    }
    return {};
}

#undef CREATE_OPERATOR

auto cblang::standard::get_static_stdlib() -> compiler::StaticScope {
    compiler::StaticScope out;

    out["print"] = FunctionMember::generate(
        parser::Templated::generate(scanner::Token::create_external("print"), {}),
        {
            MemberDefinition::generate(
                TemplatedType::generate(StringDefinition::generate(), {}),
                scanner::Token::create_external("to_print"),
                {}, false, false, true
            )
        },
        {},
        true, false, true, false, false
    );

    return out;
}

auto cblang::standard::get_static_stdlib_classes() -> compiler::StaticClassScope {
    compiler::StaticClassScope out;

    out["Double"] = DoubleDefinition::generate();

    return out;
}

auto cblang::standard::get_stdlib_scope(const compiler::StaticScope& static_stdlib) -> std::shared_ptr<program::Scope> {
    auto print = objects::Variable::generate(
        scanner::Token::create_external("print"),
        objects::FunctionObject::generate(
            std::dynamic_pointer_cast<FunctionMember>(static_stdlib.at("print")),
            STATIC_INTERNAL_FUNCTION_PARAMS {
                std::cout << std::dynamic_pointer_cast<objects::StringObject>(params[0]->cast_to(TemplatedType::generate(StringDefinition::generate(), {})).value())->value << "\n"; // TODO: What the heck man
                return {};
            },
            false, false, true, true
        ),
        false, true, true
    );

    auto standard_object = objects::Object::generate(
        ClassDefinition::generate(
            parser::Templated::generate(scanner::Token::create_external("StandardLib"), {}),
            {}, {}
        ),
        {}, {}
    );
    standard_object->members_by_name["print"] = print;

    auto out = std::make_shared<program::Scope>(standard_object);
    out->defined_variables["print"] = print;
    out->defined_classes["Double"] = DoubleDefinition::generate();

    return out;
}