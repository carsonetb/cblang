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

auto cblang::compiler::compile(const std::vector<lexical::Keyword> &keywords) -> Program {
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
    for (const auto& keyword : keywords) {
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
        
        if (main_state == Scope::INIT_PARAM_SCOPE) {
            auto& this_state = state_struct.param_scope_state;
            if (this_state == ParamScopeState::PARAM_TYPE) {
                if (keyword.type != lexical::KeywordType::CLASS_NAME) {
                    COMPILE_ERROR(EXPECTED_KEYWORD, "Expected a class name for a parameter");
                    return out;
                }
                std::string type = keyword.infos.at("name");
                if (!defined_classes.contains(type)) {
                    COMPILE_ERROR(UNKOWN_TYPE, "Unknown type for parameter: " + type);
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
                    state_struct.generated_class->members[name] = param;
                    this_state = ParamScopeState::END_OR_REPEAT;
                }
                else {
                    COMPILE_ERROR(EXPECTED_KEYWORD, "Expected a var name of a parameter.");
                    return out;
                }
            }
            else if (this_state == ParamScopeState::END_OR_REPEAT) {
                if (keyword.type == lexical::KeywordType::VAR_SCOPE_END) {
                    logger->debug("-- param scope end");
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

        if (main_state == Scope::TEMPLATE_SCOPE) {
            auto& this_state = state_struct.template_scope_state;
            if (this_state == TemplateScopeState::TYPE) {
                if (keyword.type != lexical::KeywordType::CLASS_NAME) {
                    COMPILE_ERROR(EXPECTED_KEYWORD, "Expected a type for a template argument.");
                    return out;
                }
                std::string type = keyword.infos.at("name");
                if (!defined_classes.contains(type)) {
                    COMPILE_ERROR(UNKOWN_TYPE, "Unknown type for template: " + type);
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

        index++;
    }

    logger->info("Compiler finished successfully.");

    return {};
}