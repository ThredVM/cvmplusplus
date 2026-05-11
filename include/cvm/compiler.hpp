#pragma once
#include "ast.hpp"
#include "chunk.hpp"

class Compiler {
public:
    Compiler();
    Chunk compile(const Program& program);

private:
    struct Local {
        std::string name;
        int depth;
    };

    void compileStmt(const Stmt& stmt);
    void compileLetStmt(const LetStmt& stmt);
    void compilePrintStmt(const PrintStmt& stmt);
    void compileInputStmt(const InputStmt& stmt);
    void compileExprStmt(const ExprStmt& stmt);
    void compileBlockStmt(const BlockStmt& stmt);
    void compileIfStmt(const IfStmt& stmt);
    void compileWhileStmt(const WhileStmt& stmt);

    void compileExpr(const Expr& expr);
    void compileBinaryExpr(const BinaryExpr& expr);
    void compileUnaryExpr(const UnaryExpr& expr);
    void compileLiteralExpr(const NumberLitExpr& expr);
    void compileStringLiteralExpr(const StringLitExpr& expr);
    void compileBoolLiteralExpr(const BoolLitExpr& expr);
    void compileIdentExpr(const IdentExpr& expr);
    void compileAssignExpr(const AssignExpr& expr);
    void compileGroupingExpr(const GroupingExpr& expr);

    uint16_t addConstant(Value v);
    uint16_t addName(const std::string& name);

    // Local variable support
    void beginScope();
    void endScope(int line);
    void addLocal(const std::string& name, int line);
    int resolveLocal(const std::string& name);

    Chunk chunk_;
    std::vector<Local> locals_;
    int scopeDepth_ = 0;
};
