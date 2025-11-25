#include "compiler.h"
#include "cblang.h"
#include "definitions.h"
#include "lexical.h"

#include <memory>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <string>
#include <sys/types.h>
#include <unordered_map>
#include <vector>

#define COMPILE_ERROR(type, message) out.errors.emplace_back(keyword, ParseError::ErrorType::type, message)

using namespace cblang::compiler;

static bool initialized = false;
static std::shared_ptr<spdlog::logger> logger = spdlog::stderr_color_st("cblang::compiler");

auto cblang::compiler::init(bool verbose) -> void {
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [cblang::compiler] %^[%l] %v %$");
    logger->set_level(spdlog::level::warn);

    initialized = true;

    if (verbose) {
        enable_verbose_logs();
    }

    logger->info("Initialization complete!");
}

auto cblang::compiler::enable_verbose_logs() -> void {
    logger->set_level(spdlog::level::debug);
    logger->info("Verbose logs enabled.");
}

auto cblang::compiler::compile(const std::vector<lexical::Keyword> &keywords, const lexical::KeywordMap& kw_map) -> Program {
    logger->info("Compiler started.");

    Program out;
    State state;

    std::unordered_map<std::string, std::shared_ptr<definitions::ClassDefinition>> defined_classes = {
        {"bool", std::make_shared<definitions::BoolDefinition>()},
        {"int", std::make_shared<definitions::StringDefinition>()},
        {"char", std::make_shared<definitions::CharDefinition>()},
        {"string", std::make_shared<definitions::StringDefinition>()},
        {"array", std::make_shared<definitions::ArrayDefinition>()},
    };

    int index = 0;
    for (int i = 0; i < keywords.size(); i++) {
        const auto& keyword = keywords.at(i);
        if (keyword.type == lexical::KeywordType::EOF_SEPERATOR && !state.scope_stack.empty()) {
            COMPILE_ERROR(EOF_ERROR, "Unexpected end of file (scope stack is not empty)");
            return out;
        }

        //auto next_keyword = keywords[index + 1];
        auto type = keyword.type;
        auto& state_struct = state.scope_stack.back();
        auto main_state = state_struct.scope;

        if (main_state == Scope::MAIN_SCOPE) {
            auto& this_state = state_struct.main_scope_state;
            if (this_state == MainScopeState::CLASS_KEYWORD) {
                if (type != lexical::KeywordType::CLASS_KEYWORD) {
                    COMPILE_ERROR(EXPECTED_KEYWORD, "Expected 'class' keyword at the beginning of a file.");
                    return out;
                }
                logger->debug("MainScopeState class keyword.");
                state_struct.generated_class = std::make_shared<definitions::UserDefinition>();
                this_state = MainScopeState::CLASS_NAME;
            }
            else if (this_state == MainScopeState::CLASS_NAME) {
                std::string name = keyword.infos.at("name");
                if (name != "Main") {
                    COMPILE_ERROR(EXPECTED_KEYWORD, "Class must be named 'Main'.");
                    return out;
                }
                logger->debug("MainScopeState class name: " + name);
                state_struct.generated_class->type_name = name;
                this_state = MainScopeState::BEFORE_PARAMS;
            }
            else if (this_state == MainScopeState::BEFORE_PARAMS) {
                if (type == lexical::KeywordType::VAR_SCOPE_BEGIN) {
                    logger->debug("MainScopeState parameters begin.");
                    this_state = MainScopeState::PARAMS;
                    state.scope_stack.emplace_back(Scope::INIT_PARAM_SCOPE, state_struct.generated_class);
                }
                else if (type == lexical::KeywordType::NAMEVAL_SEPERATOR) {
                    logger->debug("MainScopeState members begin.");
                    this_state = MainScopeState::MEMBERS;
                    state.scope_stack.emplace_back(Scope::MEMBER_SCOPE, state_struct.generated_class);
                }
                else {
                    COMPILE_ERROR(EXPECTED_KEYWORD, "Expected a parameter opener or nameval seperator (for parameters/members).");
                    return out;
                }
            }
            else if (this_state == MainScopeState::PARAMS) {
                if (type != lexical::KeywordType::NAMEVAL_SEPERATOR) {
                    COMPILE_ERROR(EXPECTED_KEYWORD, "Expected a nameval seperator (for members).");
                    return out;
                }
                logger->debug("MainScopeState members begin.");
                state.scope_stack.emplace_back(Scope::MEMBER_SCOPE, state_struct.generated_class);
                this_state = MainScopeState::MEMBERS;
            }
            else if (this_state == MainScopeState::MEMBERS) {
                logger->debug("MainScopeState finish.");
                state.scope_stack.pop_back();
            }
        }
        
        else if (main_state == Scope::INIT_PARAM_SCOPE) {
            auto& this_state = state_struct.param_scope_state;
            if (this_state == ParamScopeState::PARAM_TYPE) {
                if (keyword.type != lexical::KeywordType::CLASS_NAME) {
                    COMPILE_ERROR(EXPECTED_KEYWORD, "Expected a class name for a parameter");
                    return out;
                }
                std::string type = keyword.infos.at("name");
                if (!defined_classes.contains(type)) {
                    COMPILE_ERROR(UNKNOWN_TYPE, "Unknown type for parameter: " + type);
                    return out;
                }
                logger->debug("-- added parameter of type " + type);
                auto param = std::make_shared<definitions::MemberDefinition>("unnamed");
                param->type = defined_classes.at(type);
                state_struct.generated_class->params.push_back(param);
                this_state = ParamScopeState::PARAM_NAME;
            }
            else if (this_state == ParamScopeState::PARAM_NAME) {
                if (keyword.type == lexical::KeywordType::TEMPLATE_SCOPE_BEGIN) {
                    logger->debug("Template scope begin");
                    state.scope_stack.emplace_back(Scope::TEMPLATE_SCOPE, state_struct.generated_class->params.back());
                }
                else if (keyword.type == lexical::KeywordType::VAR_NAME) {
                    std::string name = keyword.infos.at("name");
                    logger->debug("-- parameter name is " + name);
                    auto param = state_struct.generated_class->params.back();
                    param->name = name;
                    state_struct.generated_class->members_by_name[name] = param;
                    this_state = ParamScopeState::END_OR_REPEAT;
                }
                else {
                    COMPILE_ERROR(EXPECTED_KEYWORD, "Expected a var name of a parameter.");
                    return out;
                }
            }
            else if (this_state == ParamScopeState::END_OR_REPEAT) {
                if (keyword.type == lexical::KeywordType::VAR_SCOPE_END) {
                    logger->debug("Param scope end");
                    state.scope_stack.pop_back();
                }
                else if (keyword.type == lexical::KeywordType::MULTIVAR_SEPERATOR) {
                    logger->debug("-- multivar separator");
                    this_state = ParamScopeState::PARAM_TYPE;
                }
                else {
                    COMPILE_ERROR(EXPECTED_KEYWORD, "Expected a param scope end, or multivar separator.");
                    return out;
                }
            }
        }

        else if (main_state == Scope::MEMBER_SCOPE) {
            auto& this_state = state_struct.member_scope_state;
            if (this_state == MemberScopeState::NEXT_VAR) {
                if (keyword.type == lexical::KeywordType::CLASS_NAME) {
                    i--; // We will have to repeat this class.
                    logger->debug("Member variable definition begin");
                    auto new_member = std::make_shared<definitions::MemberDefinition>("unnamed");
                    state_struct.generated_class->members.push_back(new_member);
                    this_state = MemberScopeState::VAR_DEFINITION;
                    state.scope_stack.emplace_back(Scope::MEMBER_DEFINITION, new_member);
                }
                else if (keyword.type == lexical::KeywordType::SCOPE_KEYWORD) {

                }
                else if (keyword.type == lexical::KeywordType::CLASS_KEYWORD) {
                }
                else if (keyword.type == lexical::KeywordType::VAR_SCOPE_BEGIN) { continue; }
                else {
                    COMPILE_ERROR(EXPECTED_KEYWORD, "Expected a member variable, scope, or class.");
                    return out;
                }
            }
            else if (this_state == MemberScopeState::VAR_DEFINITION) {
                if (keyword.type != lexical::KeywordType::MULTIVAR_SEPERATOR) {
                    COMPILE_ERROR(EXPECTED_KEYWORD, "Expected a multivar seperator after variable definition ends (something went wrong)");
                    return out;
                }
                logger->debug("-- finished defining member variable");
                auto finished_adding = state_struct.generated_class->members.back();
                state_struct.generated_class->members_by_name[finished_adding->name] = finished_adding;
                this_state = MemberScopeState::NEXT_VAR;
            }
        }

        else if (main_state == Scope::TEMPLATE_SCOPE) {
            auto& this_state = state_struct.template_scope_state;
            if (this_state == TemplateScopeState::TYPE) {
                if (keyword.type != lexical::KeywordType::CLASS_NAME) {
                    COMPILE_ERROR(EXPECTED_KEYWORD, "Expected a type for a template argument.");
                    return out;
                }
                std::string type = keyword.infos.at("name");
                if (!defined_classes.contains(type)) {
                    COMPILE_ERROR(UNKNOWN_TYPE, "Unknown type for template: " + type);
                    return out;
                }
                auto member = state_struct.generated_member;
                unsigned long template_index = member->templates.size();
                if (template_index >= member->type->templates.size()) { // If the index of the template to add exceeds the specified list length, error.
                    COMPILE_ERROR(TOO_MANY_ARGUMENTS, "The number of templates passed is greater than the specified number.");
                    return out;
                }
                logger->debug("-- template type " + type);
                auto template_typing = member->type->templates[template_index];
                auto new_template = std::make_shared<definitions::TemplateDefinition>(template_typing->type_name);
                new_template->template_used = defined_classes.at(type);
                member->templates.push_back(new_template);
                this_state = TemplateScopeState::END_OR_REPEAT;
            }
            else if (this_state == TemplateScopeState::END_OR_REPEAT) {
                if (keyword.type == lexical::KeywordType::TEMPLATE_SCOPE_END) {
                    logger->debug("Template scope end");
                    state.scope_stack.pop_back();
                }
                else if (keyword.type == lexical::KeywordType::MULTIVAR_SEPERATOR) {
                    logger->debug("-- multivar separator");
                    this_state = TemplateScopeState::TYPE;
                }
                else {
                    COMPILE_ERROR(EXPECTED_KEYWORD, "Expected a template scope end, or multivar separator.");
                    return out;
                }
            }
        }

        else if (main_state == Scope::MEMBER_DEFINITION) {
            auto& this_state = state_struct.member_definition_state;
            if (this_state == MemberDefinitionState::VAR_CLASS) {
                if (!state_struct.generated_member) {
                    state_struct.generated_member = std::make_shared<definitions::MemberDefinition>("unnamed");
                }
                if (keyword.type == lexical::KeywordType::CLASS_NAME) {
                    std::string type = keyword.infos.at("name");
                    logger->debug("-- var type is " + type);
                    if (!defined_classes.contains(type)) {
                        COMPILE_ERROR(UNKNOWN_TYPE, "Unkown type for member definition.");
                        return out;
                    }
                    state_struct.generated_member->type = defined_classes.at(type);
                    this_state = MemberDefinitionState::VAR_NAME;
                }
                else if (keyword.type == lexical::KeywordType::PRIVATE_KEYWORD) {
                    logger->debug("-- var is private");
                    state_struct.generated_member->is_private = true;
                }
                else if (keyword.type == lexical::KeywordType::STATIC_KEYWORD) {
                    logger->debug("-- var is static");
                    state_struct.generated_member->is_static = true;
                }
                else if (keyword.type == lexical::KeywordType::CONST_KEYWORD) {
                    logger->debug("-- var is const");
                    state_struct.generated_member->is_const = true;
                }
                else {
                    COMPILE_ERROR(EXPECTED_KEYWORD, "Expected either a variable class, private, static, or const.");
                    return out;
                }
            }
            else if (this_state == MemberDefinitionState::VAR_NAME) {
                if (keyword.type != lexical::KeywordType::VAR_NAME) {
                    COMPILE_ERROR(EXPECTED_KEYWORD, "Expected a variable name after its type.");
                    return out;
                }
                std::string name = keyword.infos.at("name");
                logger->debug("-- var name is " + name);
                state_struct.generated_member->name = name;
                this_state = MemberDefinitionState::END_OR_RESOLVE;
            }
            else if (this_state == MemberDefinitionState::END_OR_RESOLVE) {
                if (keyword.type == lexical::KeywordType::MULTIVAR_SEPERATOR) {
                    logger->debug("Variable definition end");
                    i--;
                    state.scope_stack.pop_back();
                }
                else if (keyword.type == lexical::KeywordType::NAMEVAL_SEPERATOR) {
                    logger->debug("-- variable has literal constructor");
                    this_state = MemberDefinitionState::LITERAL;
                }
                else {
                    COMPILE_ERROR(EXPECTED_KEYWORD, "Expected multivar seperator or literal constructor");
                    return out;
                }
            }
            else if (this_state == MemberDefinitionState::LITERAL) {
                if (keyword.type != lexical::KeywordType::DEFINITION_TEXT) {
                    COMPILE_ERROR(EXPECTED_KEYWORD, "Expected definition text");
                    return out;
                }
                definitions::LiteralType literal_type = definitions::LiteralType::INVALID;
                auto type = state_struct.generated_member->type;
                if (type->type_name == "bool") { literal_type = definitions::LiteralType::BOOL; }
                if (type->type_name == "int") { literal_type = definitions::LiteralType::INT; }
                if (type->type_name == "char") { literal_type = definitions::LiteralType::CHAR; }
                if (type->type_name == "string") { literal_type = definitions::LiteralType::STRING; }
                if (type->type_name == "array") { literal_type = definitions::LiteralType::ARRAY; }
                if (literal_type == definitions::LiteralType::INVALID) {
                    COMPILE_ERROR(INVALID_TYPE, "Only literals may have constructors.");
                    return out;
                }
                std::string to_parse = keyword.infos.at("text");
                std::vector<lexical::Keyword> ret;
                int err = lexical::process_literal(literal_type, to_parse, kw_map, ret);
                if (err > 0) {
                    COMPILE_ERROR(EXPECTED_KEYWORD, "Error parsing literal definition.");
                    return out;
                }
                logger->debug("-- valid initializer parsed: " + to_parse);
                state_struct.generated_member->has_initializer = true;
                state_struct.generated_member->initializer = ret;
                state.scope_stack.pop_back();
            }
        }

        index++;
    }

    logger->info("Compiler finished successfully.");

    return {};
}