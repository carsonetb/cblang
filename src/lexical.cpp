#include "lexical.h"
#include "util.h"

#include <algorithm>
#include <cassert>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include <unordered_map>
#include <utility>
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
    KeywordType::VAR_SCOPE_BEGIN,
    KeywordType::VAR_SCOPE_END,
    KeywordType::TEMPLATE_SCOPE_BEGIN,
    KeywordType::TEMPLATE_SCOPE_END,
    KeywordType::CODE_SCOPE_BEGIN,
    KeywordType::CODE_SCOPE_END,
};

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

auto cblang::lexical::KeywordMap::get_from_array(const std::string& item, const std::vector<KeywordType>& array) const -> KeywordType {
    for (const KeywordType& type : array) {
        if (at(type) == item) {
            return type;
        }
    }
    logger->error("Cannot get keyword " + item + " from KeywordType array.");
    return KeywordType::INVALID;
}

auto cblang::lexical::KeywordMap::get_from_array(const char& item, const std::vector<KeywordType>& array) const -> KeywordType {
    return get_from_array(std::string(1, item), array);
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

cblang::lexical::Keyword::Keyword(KeywordType p_type) : type(p_type) {}

cblang::lexical::Keyword::Keyword(KeywordType p_type, std::unordered_map<std::string, std::string> p_infos) : type(p_type), infos(std::move(p_infos)) {}

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

auto cblang::lexical::parse(std::string data, KeywordMap kw_map) -> std::vector<Keyword> {
    logger->info("Lexical parser started.");
    
    assert(kw_map.verify());

    logger->debug("Lexical parser verified KeywordMap argument.");
    
    std::vector<Keyword> out;

    std::string word_constructing;

    // TRASH
    data += kw_map.at(KeywordType::PARAM_RETURN_SEPERATOR);
    replace_all(data, "->", "\1");
    kw_map[KeywordType::PARAM_RETURN_SEPERATOR] = "\1";
    data.pop_back(); // idk why it adds one to the end.

    logger->debug(data);

    bool expecting_class_name = false;
    bool expecting_var_name = false;
    bool expecting_scope_name = false;
    std::vector<State> state_stack = {State::BEGIN};

    uint line = 0;
    uint character_index = 0;
    for (const char& character : data) {
        std::string character_string = std::string(1, character);

        character_index++;
        if (character == '\n') {
            line++;
        }

        if (kw_map.in_array(character, SEPERATORS)) {
            // -- HANDLE WORD CONSTRUCTING --
            if (!word_constructing.empty()) {
                if (word_constructing == kw_map.at(KeywordType::CLASS_KEYWORD)) {
                    logger->debug("Found class keyword.");
                    out.emplace_back(KeywordType::CLASS_KEYWORD);
                    expecting_class_name = true;
                }
                else if (word_constructing == kw_map.at(KeywordType::SCOPE_KEYWORD)) {
                    logger->debug("Found scope keyword.");
                    out.emplace_back(KeywordType::SCOPE_KEYWORD);
                    expecting_scope_name = true;
                }
                else if (expecting_scope_name) {
                    logger->debug("-- Scope name " + word_constructing);
                    out.push_back(Keyword(
                        KeywordType::VAR_NAME,
                        {{"name", word_constructing}}
                    ));
                    expecting_scope_name = false;
                }
                else if (expecting_class_name) {
                    logger->debug("-- Class name " + word_constructing);
                    out.push_back(Keyword(
                        KeywordType::CLASS_NAME,
                        {{"name", word_constructing}}
                    ));

                    if (state_stack.back() == State::ARGUMENT_LIST) {
                        expecting_class_name = false;
                        expecting_var_name = true;
                    }
                }
                else if (expecting_var_name) {
                    logger->debug("-- Var name " + word_constructing);
                    out.push_back(Keyword(
                        KeywordType::VAR_NAME,
                        {{"name", word_constructing}}
                    ));
                    expecting_var_name = false;
                }
            }

            if (character_string == kw_map.at(KeywordType::SPACE_SEPERATOR)) {
                word_constructing = "";
                continue;
            }

            // -- HANDLE CHARACTER --

            if (state_stack.back() == State::INHERITOR_LIST && character_string != kw_map.at(KeywordType::MULTIVAR_SEPERATOR)) {
                logger->debug("End inheritor list.");
                state_stack.pop_back();
                expecting_class_name = false;
            }
            else if (expecting_class_name) {
                if (character_string == kw_map.at(KeywordType::INHERITANCE_SEPERATOR)) {
                    logger->debug("Begin inheritor list.");
                    state_stack.push_back(State::INHERITOR_LIST);
                }
                else {
                    expecting_class_name = false;
                }
            }

            if (character_string == kw_map.at(KeywordType::VAR_SCOPE_BEGIN)) {
                logger->debug("Begin argument list.");
                out.emplace_back(KeywordType::VAR_SCOPE_BEGIN);
                state_stack.push_back(State::ARGUMENT_LIST);
                expecting_class_name = true;
            }
            if (character_string == kw_map.at(KeywordType::VAR_SCOPE_END)) {
                logger->debug("End argument list.");
                out.emplace_back(KeywordType::VAR_SCOPE_END);
                state_stack.pop_back();
                expecting_class_name = false;
            }

            if (character_string == kw_map.at(KeywordType::TEMPLATE_SCOPE_BEGIN)) {
                logger->debug("Begin template list.");
                out.emplace_back(KeywordType::TEMPLATE_SCOPE_BEGIN);
                state_stack.push_back(State::TEMPLATE_LIST);
                expecting_class_name = true;
            }
            if (character_string == kw_map.at(KeywordType::TEMPLATE_SCOPE_END)) {
                logger->debug("End template list.");
                out.emplace_back(KeywordType::TEMPLATE_SCOPE_END);
                state_stack.pop_back();
                expecting_class_name = false;
            }

            if (character_string == kw_map.at(KeywordType::CODE_SCOPE_BEGIN)) {
                logger->debug("Begin code scope.");
                out.emplace_back(KeywordType::CODE_SCOPE_BEGIN);
                state_stack.push_back(State::CODE);
            }
            if (character_string == kw_map.at(KeywordType::CODE_SCOPE_END)) {
                logger->debug("End code scope.");
                out.emplace_back(KeywordType::CODE_SCOPE_END);
                state_stack.pop_back();
            }

            if (character_string == kw_map.at(KeywordType::PARAM_RETURN_SEPERATOR)) {
                logger->debug("Detected function return type.");
                out.emplace_back(KeywordType::PARAM_RETURN_SEPERATOR);
                expecting_class_name = true;
            }

            if (character_string == kw_map.at(KeywordType::NAMEVAL_SEPERATOR)) {
                logger->debug("Detected nameval seperator.");
                out.emplace_back(KeywordType::NAMEVAL_SEPERATOR);
            }
            
            // Argument list, after a comma go back to the type of the argument.
            if (state_stack.back() == State::ARGUMENT_LIST && character_string == kw_map.at(KeywordType::MULTIVAR_SEPERATOR)) {
                out.emplace_back(KeywordType::MULTIVAR_SEPERATOR);
                expecting_class_name = true;
            }
            if (state_stack.back() == State::TEMPLATE_LIST && character_string == kw_map.at(KeywordType::MULTIVAR_SEPERATOR)) {
                out.emplace_back(KeywordType::MULTIVAR_SEPERATOR);
            }

            word_constructing = "";
        }
        else {
            word_constructing += character;
            continue;
        }
    }

    logger->info("Lexical parser finished.");
}