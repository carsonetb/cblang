#include "scanner.hpp"
#include "parser.hpp"

#include <memory>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <string>
#include <vector>

using namespace cblang::scanner;

static bool initialized = false;
static std::shared_ptr<spdlog::logger> logger = spdlog::stderr_color_mt("cblang::scanner");

static auto is_digit(const char& character) -> bool {
    return character >= '0' && character <= '9';
}

static auto is_alpha(const char& character) -> bool {
    return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') || character == '_';
}

static auto is_alpha_numeric(const char& character) -> bool {
    return is_alpha(character) || is_digit(character);
}

cblang::scanner::Literal::Literal() = default;
cblang::scanner::Literal::~Literal() = default;

const std::unordered_map<std::string, TokenType> cblang::scanner::Scanner::keywords = {
    {"class", CLASS_KW},
    {"true", TRUE_KW},
    {"false", FALSE_KW},
    {"private", PRIVATE_KW},
    {"static", STATIC_KW},
    {"const", CONST_KW},
    {"operator", OPERATOR_KW},
    {"cast", CAST_KW},
    {"scope", SCOPE_KW},
    {"super", SUPER_KW},
    {"return", RETURN_KW}
};

cblang::scanner::Scanner::Scanner(std::string p_source) : source(std::move(p_source)) {}

auto cblang::scanner::Scanner::scan_tokens() -> std::vector<Token> {
    while (true) {
        start = current;
        scan_token();
        if (is_at_end()) {
            break;
        }
    }

    tokens.emplace_back(TokenType::END_OF_FILE, "", line);
    return tokens;
}

auto cblang::scanner::Scanner::scan_token() -> void {
    char character = advance();
    switch (character) {
        case ' ': 
        case '\r':
        case '\t': 
            break;
        case '\n':
            line++;
            break;
        case '(': add_token(LEFT_PAREN); break;
        case ')': add_token(RIGHT_PAREN); break;
        case '[': add_token(LEFT_BRACKET); break;
        case ']': add_token(RIGHT_BRACKET); break;
        case '{': add_token(LEFT_CURLY); break;
        case '}': add_token(RIGHT_CURLY); break;
        case ',': add_token(COMMA); break;
        case '.': add_token(DOT); break;
        case '+': add_token(PLUS); break;
        case ';': add_token(SEMICOLON); break;
        case ':': add_token(COLON); break;
        case '^': add_token(CARET); break;
        case '%': add_token(MODULO); break;
        case '|': 
            if (match('|')) { add_token(PIPE_PIPE); }
            else if (match('=')) { add_token(PIPE_EQUAL); }
            else { add_token(PIPE); }
        case '-': 
            add_token(match('>') ? RETURN : MINUS); 
            break;
        case '*': 
            add_token(match('*') ? STAR_STAR : STAR); 
            break;
        case '!': 
            add_token(match('=') ? BANG_EQUAL : BANG); 
            break;
        case '=': 
            add_token(match('=') ? EQUAL_EQUAL : EQUAL); 
            break;
        case '<': 
            add_token(match('=') ? LESS_EQUAL : LEFT_ANGLE); 
            break;
        case '>': 
            add_token(match('=') ? GREATER_EQUAL : RIGHT_ANGLE); 
            break;
        case '/':
            if (match('/')) {
                while (peek() != '\n' && !is_at_end()) {
                    advance();
                }
                line++;
            }
            else {
                add_token(SLASH);
            }
            break;
        case '"': string(); break;
        case '\'':
            if (peek() == '\'') {
                logger->error("Char identifier must contain a character at line " + std::to_string(line));
            }
            else {
                // Consume the character and the closing "'"
                advance();
                if (peek() != '\'') {
                    logger->error("Char identifier must close after one character at line " + std::to_string(line));
                }
                advance();
                add_token(CHARACTER, std::make_shared<CharLiteral>(peek(-1)));
            }
            break;
        default: 
            if (is_digit(character)) {
                number();
            }
            else if (is_alpha(character)) {
                identifier();
            }
            else {
                logger->error("Unexpected character at line " + std::to_string(line));
            }
    }
}

auto cblang::scanner::Scanner::advance() -> char {
    char out = source.at(current);
    current++;
    return out;
}

auto cblang::scanner::Scanner::add_token(TokenType type) -> void {
    add_token(type, nullptr);
}

auto cblang::scanner::Scanner::add_token(TokenType type, const std::shared_ptr<Literal>& literal) -> void {
    std::string text = source.substr(start, current - start);
    logger->debug("Added token from: " + text);
    tokens.emplace_back(type, text, line, literal);
}

auto cblang::scanner::Scanner::match(const char& expected) -> bool {
    if (is_at_end() || source.at(current) != expected) {
        return false;
    }

    current++;
    return true;
}

auto cblang::scanner::Scanner::string() -> void {
    while (peek() != '"' && !is_at_end()) {
        if (peek() == '\n') {
            line++;
        }
        advance();
    }

    if (is_at_end()) {
        logger->error("Unterminated string.");
        return;
    }

    advance(); // advance passed the closing ".

    std::string value = source.substr(start + 1, current - start - 2);
    add_token(STRING, std::make_shared<StringLiteral>(value));
}

auto cblang::scanner::Scanner::number() -> void {
    while (is_digit(peek())) {
        advance();
    }

    if (peek() == '.' && is_digit(peek(2))) {
        advance();

        while (is_digit(peek())) {
            advance();
        }
        add_token(FLOAT, std::make_shared<FloatLiteral>(std::stof(get_consumed())));
    }
    else {
        add_token(INT, std::make_shared<IntLiteral>(std::stoi(get_consumed())));
    }

}

auto cblang::scanner::Scanner::identifier() -> void {
    while (is_alpha_numeric(peek())) {
        advance();
    }

    std::string text = source.substr(start, current - start);
    if (keywords.contains(text)) {
        add_token(keywords.at(text));
    }
    else {
        add_token(IDENTIFIER);
    }
}

auto cblang::scanner::Scanner::peek(const int& ahead) const -> char {
    if (current + ahead - 1 > source.size()) {
        return '\0';
    }
    return source.at(current);
}

auto cblang::scanner::Scanner::is_at_end() const -> bool {
    return current >= source.size() - 1;
}

auto cblang::scanner::Scanner::get_consumed() const -> std::string {
    return source.substr(start, current - start);
}

auto cblang::scanner::init(bool verbose) -> void {
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [cblang::scanner] %^[%l] %v %$");
    logger->set_level(spdlog::level::warn);

    initialized = true;

    if (verbose) {
        enable_verbose_logs();
    }

    logger->info("Initialization complete!");
}

auto cblang::scanner::enable_verbose_logs() -> void {
    logger->set_level(spdlog::level::debug);
    logger->info("Verbose logs enabled.");
}

auto cblang::scanner::get_literal_string(const std::shared_ptr<Literal>& literal) -> std::string {
    auto as_bool = std::dynamic_pointer_cast<BoolLiteral>(literal);
    if (as_bool) { return as_bool->val ? "true" : "false"; }
    auto as_int = std::dynamic_pointer_cast<IntLiteral>(literal);
    if (as_int) { return std::to_string(as_int->val); }
    auto as_float = std::dynamic_pointer_cast<FloatLiteral>(literal);
    if (as_float) { return std::to_string(as_float->val); }
    auto as_char = std::dynamic_pointer_cast<CharLiteral>(literal);
    if (as_char) { return "'" + std::string(1, as_char->val) + "'"; }
    auto as_string = std::dynamic_pointer_cast<StringLiteral>(literal);
    if (as_string) { return "\"" + as_string->val + "\""; }
    return "Unkown literal???";
}

auto cblang::scanner::get_literal_string(const std::shared_ptr<parser::Literal>& literal) -> std::string {
    return get_literal_string(literal->literal);
}