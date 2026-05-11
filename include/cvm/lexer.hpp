#pragma once
#include "token.hpp"
#include <vector>
#include <string>

class Lexer {
public:
    explicit Lexer(std::string source);
    std::vector<Token> tokenize();

private:
    char advance();
    char peek() const;
    char peekNext() const;
    bool isAtEnd() const;
    bool match(char expected);
    void skipWhitespace();
    
    void addToken(TokenType type);
    void addToken(TokenType type, std::string lexeme);
    
    void identifier();
    void number();
    void string();

    std::string source_;
    std::vector<Token> tokens_;
    int start_ = 0;
    int current_ = 0;
    int line_ = 1;
};
