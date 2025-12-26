#pragma once

#include "compiler.hpp"
#include "program.hpp"
#include <memory>

namespace cblang::standard {
    auto get_static_stdlib() -> compiler::StaticScope;
    auto get_stdlib_scope(const compiler::StaticScope& static_stdlib) -> std::shared_ptr<program::Scope>;
}