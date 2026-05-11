#include "cvm/lexer.hpp"
#include "cvm/error.hpp"
#include <unordered_map>

static const std::unordered_map<std::string, TokenType> keywords = {
    {"let",   TokenType::LET},
    {"print", TokenType::PRINT},
    {"input", TokenType::INPUT},
    {"if",    TokenType::IF},
    {"else",  TokenType::ELSE},
    {"while", TokenType::WHILE},
    {"true",  TokenType::TRUE},
    {"false", TokenType::FALSE},
};

Lexer::Lexer(std::string source) : source_(std::move(source)) {}

std::vector<Token> Lexer::tokenize() {
    while (!isAtEnd()) {
        start_ = current_;
        char c = advance();
        switch (c) {
            case '(': addToken(TokenType::LPAREN); break;
            case ')': addToken(TokenType::RPAREN); break;
            case '{': addToken(TokenType::LBRACE); break;
            case '}': addToken(TokenType::RBRACE); break;
            case ';': addToken(TokenType::SEMICOLON); break;
            case '+': addToken(TokenType::PLUS); break;
            case '-': addToken(TokenType::MINUS); break;
            case '*': addToken(TokenType::STAR); break;
            case '/': addToken(TokenType::SLASH); break;
            case '!':
                addToken(match('=') ? TokenType::BANG_EQ : TokenType::BANG);
                break;
            case '=':
                addToken(match('=') ? TokenType::EQ_EQ : TokenType::EQ);
                break;
            case '<':
                addToken(match('=') ? TokenType::LT_EQ : TokenType::LT);
                break;
            case '>':
                addToken(match('=') ? TokenType::GT_EQ : TokenType::GT);
                break;
            case ' ':
            case '\r':
            case '\t':
                break;
            case '\n':
                addToken(TokenType::NEWLINE);
                line_++;
                break;
            default:
                if (isdigit(c)) {
                    number();
                } else if (isalpha(c) || c == '_') {
                    identifier();
                } else {
                    throw LexError("Unexpected character: " + std::string(1, c), line_);
                }
                break;
        }
    }
    tokens_.emplace_back(TokenType::EOF_TOKEN, "", line_);
    return tokens_;
}

char Lexer::advance() {
    return source_[current_++];
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source_[current_];
}

char Lexer::peekNext() const {
    if (current_ + 1 >= (int)source_.size()) return '\0';
    return source_[current_ + 1];
}

bool Lexer::isAtEnd() const {
    return current_ >= (int)source_.size();
}

bool Lexer::match(char expected) {
    if (isAtEnd()) return false;
    if (source_[current_] != expected) return false;
    current_++;
    return true;
}

void Lexer::addToken(TokenType type) {
    addToken(type, source_.substr(start_, current_ - start_));
}

void Lexer::addToken(TokenType type, std::string lexeme) {
    tokens_.emplace_back(type, std::move(lexeme), line_);
}

void Lexer::number() {
    while (isdigit(peek())) advance();
    addToken(TokenType::NUMBER);
}

void Lexer::identifier() {
    while (isalnum(peek()) || peek() == '_') advance();
    std::string text = source_.substr(start_, current_ - start_);
    auto it = keywords.find(text);
    if (it != keywords.end()) {
        addToken(it->second);
    } else {
        addToken(TokenType::IDENTIFIER);
    }
}
