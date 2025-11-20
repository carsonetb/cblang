#include "lexical.h"
#include "util.h"

#include <cassert>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

#define INCREMENT_ENUM(name, type) name = static_cast<type>(static_cast<uint8_t>(name) + 1)

using namespace cblang::lexical;

bool initialized = false;
static std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_mt("cblang::lexical");

static const std::vector<KeywordType> SEPERATORS {
    KeywordType::INHERITANCE_SEPERATOR,
    KeywordType::MULTIVAR_SEPERATOR,
    KeywordType::NAMEVAL_SEPERATOR,
    KeywordType::PARAM_RETURN_SEPERATOR,
    KeywordType::CODE_LINE_SEPERATOR,
    KeywordType::SPACE_SEPERATOR,
    KeywordType::EOF_SEPERATOR,
};

cblang::lexical::State::State() : high_level_state(HighLevelState::MAIN_FILE) {}

auto parse_class_seperator(
    const char& character, 
    const std::string& word_constructing, 
    State& state, 
    std::vector<KeywordType>& expected,
    std::vector<KeywordType>& expected_separators,
    std::vector<KeywordType>& ignored_separators
) -> int {
    if (state.high_level_state == State::HighLevelState::PARSING_CLASS) {
        logger->critical("Unexpected high level state for parse class function call!");
        return 1;
    }

    if (state.parse_class_state == State::ParseClassState::NAME) {
        if (!state.parsing_class_definition) {
            logger->critical("Parsing class definition is null.");
            return 1;
        }
        state.parsing_class_definition->type_name = word_constructing;
        expected = {};
        ignored_separators = {KeywordType::SPACE_SEPERATOR};
        expected_separators = {KeywordType::INHERITANCE_SEPERATOR, cblang::lexical::KeywordType::INIT_PARAM_SCOPE_BEGIN};
    }

    return 0;
}

auto cblang::lexical::State::increment_state() -> bool {
    if (high_level_state == HighLevelState::MAIN_FILE) {
        return true;
    }
    if (high_level_state == HighLevelState::PARSING_CLASS) {
        if (parse_class_state == ParseClassState::MEMBERS) {
            return true;
        }
        INCREMENT_ENUM(high_level_state, HighLevelState);
    }
    return false;
}

auto cblang::lexical::KeywordMap::verify() const -> bool {
    for (const KeywordType& type : KeywordMap::required_types()) {
        if (!contains(type)) {
            return false;
        }
    }
    return true;
}

auto cblang::lexical::KeywordMap::in_array(const std::string& item, const std::vector<KeywordType>& array) const -> bool {
    for (const KeywordType& type : array) {
        if (at(type) == item) {
            return true;
        }
    }
    return false;
}

auto cblang::lexical::KeywordMap::in_array(const char& item, const std::vector<KeywordType>& array) const -> bool {
    return in_array(std::string(1, item), array);
}

auto cblang::lexical::KeywordMap::required_types() -> std::vector<KeywordType> {
    return {
        KeywordType::CLASS_KEYWORD,
        KeywordType::SCOPE_KEYWORD,
        KeywordType::SUPER_KEYWORD,
        KeywordType::PRIVATE_KEYWORD,
        KeywordType::STATIC_KEYWORD,
        KeywordType::CONST_KEYWORD,
        KeywordType::OPERATOR_KEYWORD,
        KeywordType::CAST_KEYWORD,
        KeywordType::TRUE_CONSTRUCTOR,
        KeywordType::FALSE_CONSTRUCTOR,
        KeywordType::INHERITANCE_SEPERATOR,
        KeywordType::MULTIVAR_SEPERATOR,
        KeywordType::NAMEVAL_SEPERATOR,
        KeywordType::MEMBER_ACCESS,
        KeywordType::PARAM_RETURN_SEPERATOR,
        KeywordType::CODE_LINE_SEPERATOR,
        KeywordType::SPACE_SEPERATOR,
        KeywordType::OPERATOR_PLUS,
        KeywordType::OPERATOR_MINUS,
        KeywordType::OPERATOR_MULTIPLY,
        KeywordType::OPERATOR_DIVIDE,
        KeywordType::OPERATOR_CARET,
        KeywordType::OPERATOR_DOUBLE_ASTRIX,
        KeywordType::OPERATOR_MODULO,
        KeywordType::OPERATOR_EQUALITY,
        KeywordType::OPERATOR_NOT_EQUAL,
        KeywordType::OPERATOR_LESS_THAN,
        KeywordType::OPERATOR_GREATER_THAN,
        KeywordType::OPERATOR_LESS_EQUAL,
        KeywordType::OPERATOR_GREATER_EQUAL,
        KeywordType::OPERATOR_PIPE,
        KeywordType::VAR_SCOPE_BEGIN,
        KeywordType::VAR_SCOPE_END,
        KeywordType::TEMPLATE_SCOPE_BEGIN,
        KeywordType::TEMPLATE_SCOPE_END,
        KeywordType::CODE_SCOPE_BEGIN,
        KeywordType::CODE_SCOPE_END,
        KeywordType::ARRAY_SCOPE_BEGIN,
        KeywordType::ARRAY_SCOPE_END,
        KeywordType::STRING_BEGIN,
        KeywordType::STRING_END,
        KeywordType::CHAR_BEGIN,
        KeywordType::CHAR_END,
    };
}

auto cblang::lexical::KeywordMap::default_map() -> KeywordMap {
    KeywordMap out;
    out[KeywordType::CLASS_KEYWORD] = "class";
    out[KeywordType::SCOPE_KEYWORD] = "scope";
    out[KeywordType::SUPER_KEYWORD] = "super";
    out[KeywordType::PRIVATE_KEYWORD] = "private";
    out[KeywordType::STATIC_KEYWORD] = "static";
    out[KeywordType::CONST_KEYWORD] = "const";
    out[KeywordType::OPERATOR_KEYWORD] = "operator";
    out[KeywordType::CAST_KEYWORD] = "cast";
    out[KeywordType::TRUE_CONSTRUCTOR] = "true";
    out[KeywordType::FALSE_CONSTRUCTOR] = "false";
    out[KeywordType::INHERITANCE_SEPERATOR] = ":";
    out[KeywordType::MULTIVAR_SEPERATOR] = ",";
    out[KeywordType::NAMEVAL_SEPERATOR] = "=";
    out[KeywordType::MEMBER_ACCESS] = ".";
    out[KeywordType::PARAM_RETURN_SEPERATOR] = "->";
    out[KeywordType::CODE_LINE_SEPERATOR] = ";";
    out[KeywordType::SPACE_SEPERATOR] = " ";
    out[KeywordType::EOF_SEPERATOR] = EOF;
    out[KeywordType::OPERATOR_PLUS] = "+";
    out[KeywordType::OPERATOR_MINUS] = "-";
    out[KeywordType::OPERATOR_MULTIPLY] = "*";
    out[KeywordType::OPERATOR_DIVIDE] = "/";
    out[KeywordType::OPERATOR_CARET] = "^";
    out[KeywordType::OPERATOR_DOUBLE_ASTRIX] = "**";
    out[KeywordType::OPERATOR_MODULO] = "%";
    out[KeywordType::OPERATOR_EQUALITY] = "==";
    out[KeywordType::OPERATOR_NOT_EQUAL] = "!=";
    out[KeywordType::OPERATOR_LESS_THAN] = "<";
    out[KeywordType::OPERATOR_GREATER_THAN] = ">";
    out[KeywordType::OPERATOR_LESS_EQUAL] = "<=";
    out[KeywordType::OPERATOR_GREATER_EQUAL] = ">=";
    out[KeywordType::OPERATOR_PIPE] = "|";
    out[KeywordType::OPERATOR_EQUALS] = "=";
    out[KeywordType::VAR_SCOPE_BEGIN] = "(";
    out[KeywordType::VAR_SCOPE_END] = ")";
    out[KeywordType::TEMPLATE_SCOPE_BEGIN] = "<";
    out[KeywordType::TEMPLATE_SCOPE_END] = ">";
    out[KeywordType::CODE_SCOPE_BEGIN] = "{";
    out[KeywordType::CODE_SCOPE_END] = "}";
    out[KeywordType::ARRAY_SCOPE_BEGIN] = "[";
    out[KeywordType::ARRAY_SCOPE_END] = "]";
    out[KeywordType::STRING_BEGIN] = "\"";
    out[KeywordType::STRING_END] = "\"";
    out[KeywordType::CHAR_BEGIN] = "'";
    out[KeywordType::CHAR_END] = "'";
    return out;
}

auto cblang::lexical::init(bool verbose) -> void {
    logger->set_pattern("[%Y-%m-%d %H:%M:%S] [cblang::lexical] %^[%l] %v %$");
    logger->set_level(spdlog::level::warn);

    initialized = true;

    if (verbose) {
        enable_verbose_logs();
    }

    logger->info("Initialization complete!");
}

auto cblang::lexical::enable_verbose_logs() -> void {
    CHECK_INITIALIZED;

    logger->set_level(spdlog::level::debug);
    logger->info("Verbose logs enabled.");
}

auto cblang::lexical::parse(std::string data, const KeywordMap& kw_map) -> std::vector<Keyword> {
    logger->info("Lexical parser started.");
    
    assert(kw_map.verify());

    logger->debug("Lexical parser verified KeywordMap argument.");
    
    std::vector<Keyword> out;

    State state;
    std::vector<KeywordType> expected = {KeywordType::CLASS_KEYWORD};
    std::vector<KeywordType> expected_seperators = {KeywordType::SPACE_SEPERATOR};
    std::vector<KeywordType> ignored_seperators = {KeywordType::SPACE_SEPERATOR};
    std::string word_constructing;

    data += kw_map.at(KeywordType::EOF_SEPERATOR);

    uint line = 0;
    uint character_index = 0;
    for (const char& character : data) {
        character_index++;
        if (character == '\n') {
            line++;
        }

        if (!kw_map.in_array(character, SEPERATORS)) {
            word_constructing += character;
            continue;
        }

        if (kw_map.in_array(character, expected_seperators)) {
            if (expected.empty() || kw_map.in_array(word_constructing, expected)) {
                // Put a function here to handle each case based on a parser state.
            }
            else {
                logger->error("Unexpected keyword " + word_constructing + "! Exiting.");
                return {};
            }
        }
        else {
            auto as_string = std::string(1, character);
            if (as_string == kw_map.at(KeywordType::EOF_SEPERATOR)) {
                logger->error("Unexpected keyword seperator EOF! Exiting.");
            }
            else {
                logger->error("Unexpected keyword seperator " + as_string + "! Exiting.");
            }
            return {};
        }
    }

    logger->info("Lexical parser finished.");
}