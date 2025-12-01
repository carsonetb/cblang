#pragma once 

#include "definitions.hpp"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace cblang::objects {
    using namespace definitions;
    
    class Scope {
        public:
            std::string code;  
    };

    class Object {
        public:
            Object(std::shared_ptr<ClassDefinition> p_type, const std::vector<std::shared_ptr<TemplateDefinition>>& p_templates);
            virtual ~Object();

            std::string name;
            std::shared_ptr<ClassDefinition> type;
            std::vector<std::shared_ptr<TemplateDefinition>> template_array;
            std::unordered_map<std::string, std::shared_ptr<TemplateDefinition>> templates;
            std::unordered_map<std::string, std::shared_ptr<Object>> members;

            bool is_null = true;
            bool is_const = false;

            virtual auto init_internal() -> void;
            virtual auto cast(std::shared_ptr<Object> obj) -> int = 0;
            virtual auto cast_into(std::shared_ptr<Object> obj) -> int = 0;
    };

    class FunctionObject : public Object, public std::enable_shared_from_this<FunctionObject> {
        public:
            using InternalFunction = std::function<std::shared_ptr<Object>(std::vector<std::shared_ptr<TemplateDefinition>>, std::vector<std::shared_ptr<Object>>)>;

            FunctionObject(
                const std::vector<std::shared_ptr<TemplateDefinition>>& p_templates,
                std::shared_ptr<Scope> p_scope
            );
            FunctionObject(
                const std::vector<std::shared_ptr<TemplateDefinition>>& p_templates,
                InternalFunction p_internal
            );

            enum : uint8_t {
                USER,
                INTERNAL
            };
            uint8_t type;

            InternalFunction internal;
            std::shared_ptr<Scope> scope; // Only used for user functions.

            auto cast(std::shared_ptr<Object> obj) -> int override;
            auto cast_into(std::shared_ptr<Object> obj) -> int override;
    };


    class BoolObject : public Object, public std::enable_shared_from_this<BoolObject> {
        public:
            BoolObject(const std::vector<std::shared_ptr<TemplateDefinition>>& p_templates);
            BoolObject(const std::vector<std::shared_ptr<TemplateDefinition>>& p_templates, bool p_value);

            bool value = false;

            auto cast(std::shared_ptr<Object> obj) -> int override;
            auto cast_into(std::shared_ptr<Object> obj) -> int override;
    };

    class IntObject : public Object, public std::enable_shared_from_this<IntObject> {
        public:
            IntObject(const std::vector<std::shared_ptr<TemplateDefinition>>& p_templates);
            IntObject(const std::vector<std::shared_ptr<TemplateDefinition>>& p_templates, int p_value);

            int value = 0;

            auto cast(std::shared_ptr<Object> obj) -> int override;
            auto cast_into(std::shared_ptr<Object> obj) -> int override;
    };

    class CharObject : public Object, public std::enable_shared_from_this<CharObject> {
        public:
            CharObject();
            CharObject(char p_value);
            CharObject(const std::vector<std::shared_ptr<TemplateDefinition>>& p_templates);
            CharObject(const std::vector<std::shared_ptr<TemplateDefinition>>& p_templates, char p_value);

            char value = 0;

            auto cast(std::shared_ptr<Object> obj) -> int override;
            auto cast_into(std::shared_ptr<Object> obj) -> int override;
    };

    class StringObject : public Object, public std::enable_shared_from_this<StringObject> {
        public:
            StringObject(const std::vector<std::shared_ptr<TemplateDefinition>>& p_templates);
            StringObject(const std::vector<std::shared_ptr<TemplateDefinition>>& p_templates, std::string p_value);

            std::string value;

            auto cast(std::shared_ptr<Object> obj) -> int override;
            auto cast_into(std::shared_ptr<Object> obj) -> int override;
    };

    class ArrayObject : public Object, public std::enable_shared_from_this<ArrayObject> {
        public:
            ArrayObject(std::shared_ptr<ClassDefinition> p_type, const std::vector<std::shared_ptr<TemplateDefinition>>& p_templates);
            ArrayObject(std::shared_ptr<ClassDefinition> p_type, const std::vector<std::shared_ptr<TemplateDefinition>>& p_templates, std::vector<std::shared_ptr<Object>> p_value);

            std::vector<std::shared_ptr<Object>> value;

            auto init_internal() -> void override;
            auto cast(std::shared_ptr<Object> obj) -> int override;
            auto cast_into(std::shared_ptr<Object> obj) -> int override;

            auto append(std::vector<std::shared_ptr<TemplateDefinition>> templates, std::vector<std::shared_ptr<Object>> args) -> std::shared_ptr<Object>;
    };
}