#pragma once 

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace cblang::lexical {
    enum class KeywordType : std::uint8_t {
        // BASIC KEYWORDS //

        CLASS_KEYWORD, // "class"
        SCOPE_KEYWORD, // "scope"
        SUPER_KEYWORD, // "super"
        PRIVATE_KEYWORD, // "private"
        STATIC_KEYWORD, // "static"
        CONST_KEYWORD, // "const"
        OPERATOR_KEYWORD, // "operator"
        CAST_KEYWORD, // "cast"

        // CORE VARIABLE CONSTRUCTORS

        TRUE_CONSTRUCTOR, // "true"
        FALSE_CONSTRUCTOR, // "false"
        INTEGER_CONSTRUCTOR, // 1, 2, 3, etc.
        CHARACTER_CONSTRUCTOR, // 'a', 'b', etc.
        STRING_CONSTRUCTOR, // "asdsadf", etc.

        // NAMES //

        CLASS_NAME, // class [Name]
        VAR_NAME, // Type [name]
        FUNCTION_NAME, // scope [name]()

        // TYPES

        VAR_TYPE, // [Type] name
        FUNCTION_RETURNS, // () -> [type] =

        // SEPERATORS //

        INHERITANCE_SEPERATOR, // ":"
        MULTIVAR_SEPERATOR, // ","
        NAMEVAL_SEPERATOR, // "="
        MEMBER_ACCESS, // "."
        PARAM_RETURN_SEPERATOR, // "->"
        CODE_LINE_SEPERATOR, // ";"
        SPACE_SEPERATOR, // " "
        EOF_SEPERATOR, // By default this is EOF in stdio.h

        // OPERATORS //

        OPERATOR_PLUS, // "+"
        OPERATOR_MINUS, // "-"
        OPERATOR_MULTIPLY, // "*"
        OPERATOR_DIVIDE, // "/"
        OPERATOR_CARET, // "^"
        OPERATOR_DOUBLE_ASTRIX, // "*"
        OPERATOR_MODULO, // "%"
        OPERATOR_EQUALITY, // "=="
        OPERATOR_NOT_EQUAL, // "!="
        OPERATOR_LESS_THAN, // "<"
        OPERATOR_GREATER_THAN, // ">"
        OPERATOR_LESS_EQUAL, // "<="
        OPERATOR_GREATER_EQUAL, // ">="
        OPERATOR_PIPE, // "|"
        OPERATOR_EQUALS, // "="
        OPERATOR_BRACKETS, // "[...]"

        // SCOPE ENCLOSERS //

        VAR_SCOPE_BEGIN, // "("
        VAR_SCOPE_END, // ")"
        INIT_PARAM_SCOPE_BEGIN, // [(]Type name, ...
        INIT_PARAM_SCOPE_END, // ..., Type name[)]
        PASS_PARAM_SCOPE_BEGIN, // function[(]var, ...
        PASS_PARAM_SCOPE_END, // ..., var[)]
        MEMBER_SCOPE_BEGIN, // = [(]scope name {...}, Type name, ...
        MEMBER_SCOPE_END, // ..., scope name {...}, Type name[)]
        TEMPLATE_SCOPE_BEGIN, // [<]T1, ...
        TEMPLATE_SCOPE_END, // ..., T2[>]
        CODE_SCOPE_BEGIN, // scope name [{] ...
        CODE_SCOPE_END, // ...; return 1[}]
        ARRAY_SCOPE_BEGIN, // [[]var, var2, ...
        ARRAY_SCOPE_END, // ..., var3, var4[]]
        STRING_BEGIN, // ["]blah ...
        STRING_END, // ... blah ["]
        CHAR_BEGIN, // [']b'
        CHAR_END, // 'b[']
    };

    class KeywordMap : public std::unordered_map<KeywordType, std::string> {
        public:
            auto verify() const -> bool;
            auto in_array(const std::string& item, const std::vector<KeywordType>& array) const -> bool;
            auto in_array(const char& item, const std::vector<KeywordType>& array) const -> bool;

            static auto required_types() -> std::vector<KeywordType>;
            static auto default_map() -> KeywordMap;
    };

    struct Keyword {
        KeywordType type;

        std::unordered_map<std::string, std::string> infos;
    };

    auto init(bool verbose = false) -> void;
    auto enable_verbose_logs() -> void;
    auto parse(std::string data, const KeywordMap& kw_map) -> std::vector<Keyword>;
}