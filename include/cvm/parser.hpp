#pragma once
#include "token.hpp"
#include "ast.hpp"
#include <vector>

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    Program parse();

private:
    StmtPtr statement();
    StmtPtr fnStatement();
    StmtPtr returnStatement();
    StmtPtr letStatement();
    StmtPtr printStatement();
    StmtPtr inputStatement();
    StmtPtr ifStatement();
    StmtPtr whileStatement();
    StmtPtr blockStatement();
    StmtPtr expressionStatement();

    ExprPtr expression();
    ExprPtr assignment();
    ExprPtr equality();
    ExprPtr comparison();
    ExprPtr term();
    ExprPtr factor();
    ExprPtr unary();
    ExprPtr call();
    ExprPtr finishCall(ExprPtr callee);
    ExprPtr primary();

    void synchronize();

    bool match(const std::vector<TokenType>& types);
    bool check(TokenType type) const;
    Token advance();
    bool isAtEnd() const;
    Token peek() const;
    Token previous() const;
    Token consume(TokenType type, const std::string& message);

    std::vector<Token> tokens_;
    int current_ = 0;
};
