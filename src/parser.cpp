#include "cvm/parser.hpp"
#include "cvm/error.hpp"

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

Program Parser::parse() {
    Program program;
    while (!isAtEnd()) {
        // Skip leading newlines
        while (check(TokenType::NEWLINE)) advance();
        if (isAtEnd()) break;
        program.stmts.push_back(statement());
    }
    return program;
}

StmtPtr Parser::statement() {
    if (match({TokenType::LET})) return letStatement();
    if (match({TokenType::PRINT})) return printStatement();
    if (match({TokenType::INPUT})) return inputStatement();
    if (match({TokenType::IF})) return ifStatement();
    if (match({TokenType::WHILE})) return whileStatement();
    if (match({TokenType::LBRACE})) return blockStatement();

    return expressionStatement();
}

StmtPtr Parser::letStatement() {
    Token name = consume(TokenType::IDENTIFIER, "Expect variable name.");
    consume(TokenType::EQ, "Expect '=' after variable name.");
    ExprPtr initializer = expression();
    consume(TokenType::NEWLINE, "Expect newline after let statement.");
    return std::make_unique<LetStmt>(name.lexeme, std::move(initializer), name.line);
}

StmtPtr Parser::printStatement() {
    int line = previous().line;
    ExprPtr value = expression();
    consume(TokenType::NEWLINE, "Expect newline after print statement.");
    return std::make_unique<PrintStmt>(std::move(value), line);
}

StmtPtr Parser::inputStatement() {
    int line = previous().line;
    Token name = consume(TokenType::IDENTIFIER, "Expect variable name.");
    consume(TokenType::NEWLINE, "Expect newline after input statement.");
    return std::make_unique<InputStmt>(name.lexeme, line);
}

StmtPtr Parser::ifStatement() {
    int line = previous().line;
    ExprPtr condition = expression();
    StmtPtr thenBranch = statement(); // statement handles block
    StmtPtr elseBranch = nullptr;
    if (match({TokenType::ELSE})) {
        elseBranch = statement();
    }
    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch), line);
}

StmtPtr Parser::whileStatement() {
    int line = previous().line;
    ExprPtr condition = expression();
    StmtPtr body = statement();
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body), line);
}

StmtPtr Parser::blockStatement() {
    int line = previous().line;
    std::vector<StmtPtr> statements;
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        while (check(TokenType::NEWLINE)) advance();
        if (check(TokenType::RBRACE)) break;
        statements.push_back(statement());
    }
    consume(TokenType::RBRACE, "Expect '}' after block.");
    return std::make_unique<BlockStmt>(std::move(statements), line);
}

StmtPtr Parser::expressionStatement() {
    ExprPtr expr = expression();
    consume(TokenType::NEWLINE, "Expect newline after expression statement.");
    return std::make_unique<ExprStmt>(std::move(expr));
}

ExprPtr Parser::expression() {
    return assignment();
}

ExprPtr Parser::assignment() {
    ExprPtr expr = equality();

    if (match({TokenType::EQ})) {
        Token equals = previous();
        ExprPtr value = assignment();

        if (IdentExpr* ident = dynamic_cast<IdentExpr*>(expr.get())) {
            return std::make_unique<AssignExpr>(ident->name, std::move(value), equals.line);
        }

        throw ParseError("Invalid assignment target.", equals.line);
    }

    return expr;
}

ExprPtr Parser::equality() {
    ExprPtr expr = comparison();

    while (match({TokenType::BANG_EQ, TokenType::EQ_EQ})) {
        Token op = previous();
        ExprPtr right = comparison();
        expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
    }

    return expr;
}

ExprPtr Parser::comparison() {
    ExprPtr expr = term();

    while (match({TokenType::GT, TokenType::GT_EQ, TokenType::LT, TokenType::LT_EQ})) {
        Token op = previous();
        ExprPtr right = term();
        expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
    }

    return expr;
}

ExprPtr Parser::term() {
    ExprPtr expr = factor();

    while (match({TokenType::MINUS, TokenType::PLUS})) {
        Token op = previous();
        ExprPtr right = factor();
        expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
    }

    return expr;
}

ExprPtr Parser::factor() {
    ExprPtr expr = unary();

    while (match({TokenType::SLASH, TokenType::STAR})) {
        Token op = previous();
        ExprPtr right = unary();
        expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
    }

    return expr;
}

ExprPtr Parser::unary() {
    if (match({TokenType::BANG, TokenType::MINUS})) {
        Token op = previous();
        ExprPtr right = unary();
        return std::make_unique<UnaryExpr>(op, std::move(right));
    }

    return primary();
}

ExprPtr Parser::primary() {
    if (match({TokenType::FALSE})) return std::make_unique<BoolLitExpr>(false, previous().line);
    if (match({TokenType::TRUE})) return std::make_unique<BoolLitExpr>(true, previous().line);

    if (match({TokenType::NUMBER})) {
        return std::make_unique<NumberLitExpr>(std::stoll(previous().lexeme), previous().line);
    }

    if (match({TokenType::IDENTIFIER})) {
        return std::make_unique<IdentExpr>(previous().lexeme, previous().line);
    }

    if (match({TokenType::LPAREN})) {
        int line = previous().line;
        ExprPtr expr = expression();
        consume(TokenType::RPAREN, "Expect ')' after expression.");
        return std::make_unique<GroupingExpr>(std::move(expr), line);
    }

    throw ParseError("Expect expression.", peek().line);
}

bool Parser::match(const std::vector<TokenType>& types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

Token Parser::advance() {
    if (!isAtEnd()) current_++;
    return previous();
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::EOF_TOKEN;
}

Token Parser::peek() const {
    return tokens_[current_];
}

Token Parser::previous() const {
    return tokens_[current_ - 1];
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    throw ParseError(message, peek().line);
}
