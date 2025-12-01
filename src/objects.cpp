#include "objects.hpp"

#include "definitions.hpp"

#include <functional>
#include <memory>
#include <utility>
#include <vector>

#define __NAME_TYPE_TEMPLATE_PARAMS std::shared_ptr<ClassDefinition> p_type, const std::vector<std::shared_ptr<TemplateDefinition>>& p_templates
#define __NAME_TEMPLATE_PARAMS const std::vector<std::shared_ptr<TemplateDefinition>>& p_templates
#define __INIT_OBJECT(type) Object(std::make_shared<type>(), p_templates)
#define __INIT_OBJECT_PTYPE Object(std::move(p_type), p_templates)

using namespace std::placeholders;

cblang::objects::Object::Object(
    std::shared_ptr<ClassDefinition> p_type,
    const std::vector<std::shared_ptr<TemplateDefinition>>& p_templates
) : type(std::move(p_type)), template_array(p_templates) 
{
    for (const auto& template_param : p_templates) {
        templates[template_param->type_name] = template_param;
    }
}

auto cblang::objects::Object::init_internal() -> void {}

cblang::objects::BoolObject::BoolObject(__NAME_TEMPLATE_PARAMS) : __INIT_OBJECT(BoolDefinition) {}

cblang::objects::BoolObject::BoolObject(__NAME_TEMPLATE_PARAMS, bool p_value) : __INIT_OBJECT(BoolDefinition), value(p_value) {}

auto cblang::objects::BoolObject::cast(std::shared_ptr<Object> obj) -> int {
    auto as_bool = std::dynamic_pointer_cast<BoolObject>(obj);
    if (as_bool) {
        value = as_bool->value;
        return 0;
    }
    auto as_int = std::dynamic_pointer_cast<IntObject>(obj);
    if (as_int) {
        value = as_int->value != 0;
        return 0;
    }
    return 1;
}

auto cblang::objects::BoolObject::cast_into(std::shared_ptr<Object> obj) -> int {
    auto as_bool = std::dynamic_pointer_cast<BoolObject>(obj);
    if (as_bool) {
        as_bool->value = value;
        return 0;
    }
    auto as_int = std::dynamic_pointer_cast<IntObject>(obj);
    if (as_int) {
        as_int->value = static_cast<int>(value);
        return 0;
    }
    return 1;
}

cblang::objects::IntObject::IntObject(__NAME_TEMPLATE_PARAMS) : __INIT_OBJECT(IntDefinition) {}

cblang::objects::IntObject::IntObject(__NAME_TEMPLATE_PARAMS, int p_value) : __INIT_OBJECT(IntDefinition), value(p_value) {}

auto cblang::objects::IntObject::cast(std::shared_ptr<Object> obj) -> int {
    auto as_bool = std::dynamic_pointer_cast<BoolObject>(obj);
    if (as_bool) {
        value = static_cast<int>(as_bool->value);
        return 0;
    }
    auto as_int = std::dynamic_pointer_cast<IntObject>(obj);
    if (as_int) {
        value = as_int->value;
        return 0;
    }
    auto as_char = std::dynamic_pointer_cast<CharObject>(obj);
    if (as_char) {
        value = static_cast<unsigned char>(as_char->value);
        return 0;
    }
    return 1;
}

auto cblang::objects::IntObject::cast_into(std::shared_ptr<Object> obj) -> int {
    auto as_bool = std::dynamic_pointer_cast<BoolObject>(obj);
    if (as_bool) {
        as_bool->value = value != 0;
        return 0;
    }
    auto as_int = std::dynamic_pointer_cast<IntObject>(obj);
    if (as_int) {
        as_int->value = value;
        return 0;
    }
    auto as_char = std::dynamic_pointer_cast<CharObject>(obj);
    if (as_char) {
        as_char->value = static_cast<char>(value);
        return 0;
    }
    return 1;
}

cblang::objects::CharObject::CharObject() : Object(std::make_shared<CharDefinition>(), std::vector<std::shared_ptr<TemplateDefinition>>()) {}

cblang::objects::CharObject::CharObject(char p_value) : Object(std::make_shared<CharDefinition>(), std::vector<std::shared_ptr<TemplateDefinition>>()), value(p_value) {}

cblang::objects::CharObject::CharObject(__NAME_TEMPLATE_PARAMS) : __INIT_OBJECT(CharDefinition) {}

cblang::objects::CharObject::CharObject(__NAME_TEMPLATE_PARAMS, char p_value) : __INIT_OBJECT(CharDefinition), value(p_value) {}

auto cblang::objects::CharObject::cast(std::shared_ptr<Object> obj) -> int {
    auto as_char = std::dynamic_pointer_cast<CharObject>(obj);
    if (as_char) {
        value = as_char->value;
        return 0;
    }
    auto as_int = std::dynamic_pointer_cast<IntObject>(obj);
    if (as_int) {
        value = static_cast<char>(as_int->value);
        return 0;
    }
    return 1;
}

auto cblang::objects::CharObject::cast_into(std::shared_ptr<Object> obj) -> int {
    auto as_char = std::dynamic_pointer_cast<CharObject>(obj);
    if (as_char) {
        as_char->value = value;
        return 0;
    }
    auto as_int = std::dynamic_pointer_cast<IntObject>(obj);
    if (as_int) {
        as_int->value = static_cast<unsigned char>(value);
        return 0;
    }
    return 1;
}

cblang::objects::StringObject::StringObject(__NAME_TEMPLATE_PARAMS) : __INIT_OBJECT(StringDefinition) {}

cblang::objects::StringObject::StringObject(__NAME_TEMPLATE_PARAMS, std::string p_value) : __INIT_OBJECT(StringDefinition), value(std::move(p_value)) {}

auto cblang::objects::StringObject::cast(std::shared_ptr<Object> obj) -> int {
    if (obj->type->type_name == "string") {
        value = std::static_pointer_cast<StringObject>(obj)->value;
        return 0;
    }
    if (obj->type->templated_type_name == "vector<char>") {
        std::string out;
        std::vector<std::shared_ptr<Object>> string_vector = std::static_pointer_cast<ArrayObject>(obj)->value;
        for (const auto& string_object : string_vector) {
            out += std::static_pointer_cast<StringObject>(string_object)->value;
        }
        value = out;
        return 0;
    }
    return 1;
}

auto cblang::objects::StringObject::cast_into(std::shared_ptr<Object> obj) -> int {
    if (obj->type->type_name == "string") {
        std::static_pointer_cast<StringObject>(obj)->value = value;
        return 0;
    }
    if (obj->type->type_name == "vector<char>") {
        auto as_array = std::static_pointer_cast<ArrayObject>(obj);
        for (char character : value) {
            auto char_obj = std::make_shared<CharObject>(std::vector<std::shared_ptr<TemplateDefinition>>(), character);
            std::vector<std::shared_ptr<Object>> args;
            args.push_back(char_obj);
            as_array->append({}, args);
        }
        return 0;
    }
    return 1;
}

cblang::objects::ArrayObject::ArrayObject(__NAME_TYPE_TEMPLATE_PARAMS) : __INIT_OBJECT_PTYPE {
    members["append"] = std::make_shared<FunctionObject>(template_array, FunctionObject::InternalFunction(std::bind_front(&ArrayObject::append, this)));
    members["append"]->name = "append";
}

cblang::objects::ArrayObject::ArrayObject(__NAME_TYPE_TEMPLATE_PARAMS, std::vector<std::shared_ptr<Object>> p_value) : __INIT_OBJECT_PTYPE, value(std::move(p_value)) {
    members["append"] = std::make_shared<FunctionObject>(template_array, FunctionObject::InternalFunction(std::bind_front(&ArrayObject::append, this)));
    members["append"]->name = "append";
}

auto cblang::objects::ArrayObject::init_internal() -> void {}

auto cblang::objects::ArrayObject::cast(std::shared_ptr<Object> obj) -> int {
    if (obj->type->templated_type_name == type->templated_type_name) {
        value = std::static_pointer_cast<ArrayObject>(obj)->value;
        return 0;
    }
    return 1;
}