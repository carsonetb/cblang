#pragma once

#include "compiler.hpp"
#include "definitions.hpp"
#include "program.hpp"
#include <memory>

namespace cblang::standard {
    class DoubleDefinition : public ClassDefinition {
        public:
            static auto generate() -> std::shared_ptr<DoubleDefinition>;

            DoubleDefinition();

            bool generated = false;

            auto can_convert_to(const std::shared_ptr<ClassDefinition>& def) -> bool override;
            auto can_convert_from(const std::shared_ptr<TemplatedType>& type) -> bool override;
            auto cast_from(const std::shared_ptr<objects::Object>& obj) -> std::optional<std::shared_ptr<objects::Object>> override;
        private:
            static auto unsafe_singleton() -> std::shared_ptr<DoubleDefinition> {
                static auto out = std::make_shared<DoubleDefinition>();
                return out;
            }
    };

    class DoubleObject : public objects::Object {
        public:
            static auto create(double val) -> std::shared_ptr<DoubleObject> {
                return std::make_shared<DoubleObject>(val);
            }

            DoubleObject(double p_value);

            double value;

            auto cast_to(const std::shared_ptr<TemplatedType>& type) -> std::optional<std::shared_ptr<objects::Object>> override;
    };

    auto get_static_stdlib() -> compiler::StaticScope;
    auto get_static_stdlib_classes() -> compiler::StaticClassScope;
    auto get_stdlib_scope(const compiler::StaticScope& static_stdlib) -> std::shared_ptr<program::Scope>;
}