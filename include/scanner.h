// Replaces lexical.h
// Heavily inspired by https://craftinginterpreters.com/scanning.html

#pragma once 

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace cblang::scanner {
    enum TokenType : uint8_t {
        // Single character
        LEFT_PAREN, RIHGT_PAREN, LEFT_BRACKET, RIGHT_BRACKET, LEFT_CURLY_BRACE, RIGHT_CURLY_BRACE, LEFT_ANGLE_BRACE, RIGHT_ANGLE_BRACE,
        COMMA, DOT, MINUS, PLUS, SLASH, STAR, SEMICOLON,
        BANG, EQUAL, GREATER, LESS, CARET, MODULO, PIPE,

        // Two character
        BANG_EQUAL, EQUAL_EQUAL, GREATER_EQUAL, LESS_EQUAL, DOUBLE_ASTRIX,

        // Literals
        IDENTIFIER, STRING, NUMBER,

        // Keywords
        CLASS_KW, TRUE_KW, FALSE_KW, PRIVATE_KW, STATIC_KW, CONST_KW, OPERATOR_KW, CAST_KW, SCOPE_KW, SUPER_KW, 
        
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
        BoolLiteral(bool p_val);
        bool val;
    };

    struct IntLiteral : Literal {
        IntLiteral(int p_val);
        int val;
    };

    struct CharLiteral : Literal {
        CharLiteral(char p_val);
        char val;
    };

    struct StringLiteral : Literal {
        StringLiteral(std::string p_val);
        std::string val;
    };

    struct ArrayLiteral : Literal {
        ArrayLiteral(std::vector<std::shared_ptr<Literal>> p_val);
        std::vector<std::shared_ptr<Literal>> val;
    };

    struct Token {
        Token(TokenType p_type, std::string p_raw, int p_line) : type(p_type), raw(std::move(p_raw)), line(p_line) {}
        Token(TokenType p_type, std::string p_raw, int p_line, std::shared_ptr<Literal> p_literal) : type(p_type), raw(std::move(p_raw)), line(p_line), literal(std::move(p_literal)) {}

        TokenType type;
        std::string raw;
        std::shared_ptr<Literal> literal;
        int line;

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
            int line = 0;

            auto scan_token() -> void;
            auto advance() -> char;
            auto add_token(TokenType type) -> void;
            auto add_token(TokenType type, const std::shared_ptr<Literal>& literal) -> void;
            [[nodiscard]] auto is_at_end() const -> bool;
    };
}