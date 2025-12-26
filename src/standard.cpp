#include "standard.hpp"

#include "compiler.hpp"
#include "definitions.hpp"
#include "objects.hpp"
#include "parser.hpp"
#include "program.hpp"
#include "scanner.hpp"
#include <iostream>
#include <memory>

auto cblang::standard::get_static_stdlib() -> compiler::StaticScope {
    compiler::StaticScope out;

    out["print"] = definitions::FunctionMember::generate(
        parser::Templated::generate(scanner::Token::create_external("print"), {}),
        {
            definitions::MemberDefinition::generate(
                definitions::TemplatedType::generate(definitions::StringDefinition::generate(), {}),
                scanner::Token::create_external("to_print"),
                {}, false, false, true
            )
        },
        {},
        true, false, true, false, false
    );

    return out;
}

auto cblang::standard::get_stdlib_scope(const compiler::StaticScope& static_stdlib) -> std::shared_ptr<program::Scope> {

    auto print = objects::Variable::generate(
        scanner::Token::create_external("print"),
        objects::FunctionObject::generate(
            std::dynamic_pointer_cast<definitions::FunctionMember>(static_stdlib.at("print")),
            STATIC_INTERNAL_FUNCTION_PARAMS {
                std::cout << std::dynamic_pointer_cast<objects::StringObject>(params[0])->value << "\n";
                return {};
            },
            false, false, true, true
        ),
        false, true, true
    );

    auto standard_object = objects::Object::generate(
        definitions::ClassDefinition::generate(
            parser::Templated::generate(scanner::Token::create_external("StandardLib"), {}),
            {}, {}
        ),
        {}, {}
    );
    standard_object->members_by_name["print"] = print;

    auto out = std::make_shared<program::Scope>(standard_object);
    out->defined_variables["print"] = print;

    return out;
}