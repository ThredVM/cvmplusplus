#pragma once
#include <string>

enum class TokenType {
    // Literals
    NUMBER, TRUE, FALSE, STRING,

    // Identifiers & keywords
    IDENTIFIER,
    LET, PRINT, INPUT,
    IF, ELSE, WHILE,
    FN, RETURN,

    // Operators
    PLUS, MINUS, STAR, SLASH,
    EQ, EQ_EQ,          // = and ==
    BANG, BANG_EQ,      // ! and !=
    LT, LT_EQ,
    GT, GT_EQ,

    // Delimiters
    LPAREN, RPAREN,
    LBRACE, RBRACE,
    COMMA, SEMICOLON, NEWLINE,

    // Control
    EOF_TOKEN
};

struct Token {
    TokenType   type;
    std::string lexeme;   // raw text from source
    int         line;     // 1-based line number (for error messages)

    Token(TokenType t, std::string lex, int ln)
        : type(t), lexeme(std::move(lex)), line(ln) {}
};
