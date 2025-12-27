// Replaces lexical.h
// Heavily inspired by https://craftinginterpreters.com/scanning.html

#pragma once 

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace cblang::parser {
    class Literal;
}

namespace cblang::scanner {
    enum TokenType : uint8_t {
        // Single character
        LEFT_PAREN, RIGHT_PAREN, LEFT_BRACKET, RIGHT_BRACKET, LEFT_CURLY, RIGHT_CURLY, LEFT_ANGLE, RIGHT_ANGLE,
        COMMA, DOT, MINUS, PLUS, SLASH, STAR, SEMICOLON, COLON,
        BANG, EQUAL, CARET, MODULO, PIPE,

        // Two character
        BANG_EQUAL, EQUAL_EQUAL, GREATER_EQUAL, LESS_EQUAL, STAR_STAR, RETURN,
        PLUS_EQUAL, MINUS_EQUAL, STAR_EQUAL, SLASH_EQUAL, CARET_EQUAL, STAR_STAR_EQUAL, MODULO_EQUAL, PIPE_EQUAL,
        PIPE_PIPE, AND_AND, 

        // Literals
        IDENTIFIER, STRING, FLOAT, INT, CHARACTER,

        // Keywords
        CLASS_KW, TRUE_KW, FALSE_KW, PRIVATE_KW, STATIC_KW, CONST_KW, OPERATOR_KW, CAST_KW, SCOPE_KW, SUPER_KW, 
        RETURN_KW, IF_KW, ELIF_KW, ELSE_KW, FOR_KW, WHILE_KW, IN_KW, CONTINUE_KW, BREAK_KW,
        
        END_OF_FILE
    };

    struct ScanError {
        int line;
        std::string message;
    };

    struct Literal {
        Literal();
        virtual ~Literal();
    };

    struct BoolLiteral : Literal {
        BoolLiteral(bool p_val) : val(p_val) {};
        bool val;
    };

    struct IntLiteral : Literal {
        IntLiteral(int p_val) : val(p_val) {};
        int val;
    };

    struct FloatLiteral : Literal {
        FloatLiteral(float p_val) : val(p_val) {};
        float val;
    };

    struct CharLiteral : Literal {
        CharLiteral(char p_val) : val(p_val) {};
        char val;
    };

    struct StringLiteral : Literal {
        StringLiteral(std::string p_val) : val(std::move(p_val)) {};
        std::string val;
    };

    inline auto create_literal(bool val) -> std::shared_ptr<Literal> {
        return std::make_shared<BoolLiteral>(val);
    }

    inline auto create_literal(int val) -> std::shared_ptr<Literal> {
        return std::make_shared<IntLiteral>(val);
    }

    inline auto create_literal(float val) -> std::shared_ptr<Literal> {
        return std::make_shared<FloatLiteral>(val);
    }

    inline auto create_literal(char val) -> std::shared_ptr<Literal> {
        return std::make_shared<CharLiteral>(val);
    }

    inline auto create_literal(const std::string& val) -> std::shared_ptr<Literal> {
        return std::make_shared<StringLiteral>(val);
    }

    struct Token {
        static auto create_external(const std::string& raw) -> scanner::Token {
            return {IDENTIFIER, raw, -1};
        }

        Token(TokenType p_type, std::string p_raw, int p_line) : type(p_type), raw(std::move(p_raw)), line(p_line) {}
        Token(TokenType p_type, std::string p_raw, int p_line, std::shared_ptr<Literal> p_literal) : type(p_type), raw(std::move(p_raw)), line(p_line), literal(std::move(p_literal)) {}

        TokenType type;
        std::string raw;
        std::optional<std::shared_ptr<Literal>> literal;
        int line;
        int column = 0;

        [[nodiscard]] auto to_string() const -> std::string {
            return raw + " at " + std::to_string(line);
        }
    };

    class Scanner {
        public:
            Scanner(std::string p_source);

            auto scan_tokens() -> std::vector<Token>;

        private:
            std::string source;
            std::vector<Token> tokens;

            int start = 0;
            int current = 0;
            int line = 1;

            static const std::unordered_map<std::string, TokenType> keywords;

            auto scan_token() -> void;
            auto advance() -> char;
            auto add_token(TokenType type) -> void;
            auto add_token(TokenType type, const std::shared_ptr<Literal>& literal) -> void;
            auto match(const char& expected) -> bool;
            auto string() -> void;
            auto number() -> void;
            auto identifier() -> void;
            [[nodiscard]] auto peek(const int& ahead = 1) const -> char;
            [[nodiscard]] auto is_at_end() const -> bool;
            [[nodiscard]] auto get_consumed() const -> std::string;
    };

    auto init(bool verbose = false) -> void;
    auto enable_verbose_logs() -> void;
    auto get_literal_string(const std::shared_ptr<Literal>& literal) -> std::string;
    auto get_literal_string(const std::shared_ptr<parser::Literal>& literal) -> std::string;
}