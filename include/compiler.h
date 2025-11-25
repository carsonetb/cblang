#pragma once 

#include "cblang.h"
#include "definitions.h"
#include "lexical.h"

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace cblang::compiler {
    enum class Scope : uint8_t {
        MAIN_SCOPE,
        INIT_TEMPLATE_SCOPE,
        TEMPLATE_SCOPE,
        INIT_PARAM_SCOPE,
        PARAM_SCOPE,
        MEMBER_SCOPE,
        CODE_SCOPE,
        SCOPE_DEFINITION,
        MEMBER_DEFINITION,
        CLASS_DEFINITION,
        VALUE_RESOLUTION,
    };

    enum class MainScopeState : uint8_t {
        CLASS_KEYWORD,
        CLASS_NAME,
        BEFORE_PARAMS,
        PARAMS,
        MEMBERS,
    };

    enum class ParamScopeState : uint8_t {
        PARAM_TYPE,
        PARAM_NAME,
        END_OR_REPEAT,
    };

    enum class MemberScopeState : uint8_t {
        NEXT_VAR,
        VAR_DEFINITION,
        SCOPE_DEFINITION,
        CLASS_DEFINITION,
    };

    enum class TemplateScopeState : uint8_t {
        TYPE,
        END_OR_REPEAT,
    };

    enum class MemberDefinitionState : uint8_t {
        VAR_CLASS,
        VAR_NAME,
        END_OR_RESOLVE,
        LITERAL,
    };

    enum class ScopeDefinitionState : uint8_t {
        BEFORE_SCOPE_KEYWORD,
        SCOPE_KEYWORD,
        SCOPE_NAME,
        TEMPLATES,
        PARAMS,
        RETURN_TYPE,
        CODE,
    };

    enum class ClassDefinitionState : uint8_t {
        CLASS_KEYWORD,
        CLASS_NAME,
        INHERITS,
        TEMPLATES,
        PARAMS,
        MEMBERS,
    };

    struct ScopeState {
        ScopeState(Scope p_scope) : scope(p_scope) {}
        ScopeState(Scope p_scope, std::shared_ptr<MemberDefinition> p_generated_member) : scope(p_scope), generated_member(std::move(p_generated_member)) {}
        ScopeState(Scope p_scope, std::shared_ptr<UserDefinition> p_generated_class) : scope(p_scope), generated_class(std::move(p_generated_class)) {}

        Scope scope;
        MainScopeState main_scope_state = MainScopeState::CLASS_KEYWORD;
        ParamScopeState param_scope_state = ParamScopeState::PARAM_TYPE;
        MemberScopeState member_scope_state = MemberScopeState::NEXT_VAR;
        TemplateScopeState template_scope_state = TemplateScopeState::TYPE;
        MemberDefinitionState member_definition_state = MemberDefinitionState::VAR_CLASS;
        ScopeDefinitionState scope_definition_state = ScopeDefinitionState::BEFORE_SCOPE_KEYWORD;
        ClassDefinitionState class_definition_state = ClassDefinitionState::CLASS_KEYWORD;

        std::shared_ptr<MemberDefinition> generated_member = nullptr;
        std::shared_ptr<FunctionDefinition> generated_function = nullptr;
        std::shared_ptr<UserDefinition> generated_class = nullptr;
    };

    class State {
        public:
            std::vector<ScopeState> scope_stack = {ScopeState(Scope::MAIN_SCOPE)};
            std::vector<lexical::KeywordType> expected_keywords = {lexical::KeywordType::CLASS_KEYWORD};

            Program program;
    };

    auto init(bool verbose = false) -> void;
    auto enable_verbose_logs() -> void;
    auto compile(const std::vector<lexical::Keyword>& keywords, const lexical::KeywordMap& kw_map) -> Program;
}