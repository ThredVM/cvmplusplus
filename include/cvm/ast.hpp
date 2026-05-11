#pragma once
#include "token.hpp"
#include "value.hpp"
#include <memory>
#include <vector>
#include <string>

// ── Forward declarations ───────────────────────────────────────────────────
struct Expr;
struct Stmt;
using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

// ── Expression base ────────────────────────────────────────────────────────
struct Expr {
    int line = 0;   // source line for error reporting
    virtual ~Expr() = default;
    virtual ExprPtr fold() { return nullptr; }
};

struct NumberLitExpr : Expr {
    int64_t value;
    explicit NumberLitExpr(int64_t v, int ln) : value(v) { line = ln; }
    ExprPtr fold() override { return nullptr; }
};

struct StringLitExpr : Expr {
    std::string value;
    explicit StringLitExpr(std::string v, int ln) : value(std::move(v)) { line = ln; }
    ExprPtr fold() override { return nullptr; }
};

struct BoolLitExpr : Expr {
    bool value;
    explicit BoolLitExpr(bool v, int ln) : value(v) { line = ln; }
    ExprPtr fold() override { return nullptr; }
};

struct IdentExpr : Expr {
    std::string name;
    explicit IdentExpr(std::string n, int ln) : name(std::move(n)) { line = ln; }
    ExprPtr fold() override { return nullptr; }
};

struct AssignExpr : Expr {
    std::string name;
    ExprPtr     value;
    AssignExpr(std::string n, ExprPtr v, int ln)
        : name(std::move(n)), value(std::move(v)) { line = ln; }
    ExprPtr fold() override;
};

struct BinaryExpr : Expr {
    Token    op;      // holds the operator token (for error messages)
    ExprPtr  left;
    ExprPtr  right;
    BinaryExpr(Token o, ExprPtr l, ExprPtr r)
        : op(std::move(o)), left(std::move(l)), right(std::move(r)) { line = op.line; }
    ExprPtr fold() override;
};

struct UnaryExpr : Expr {
    Token   op;
    ExprPtr right;
    UnaryExpr(Token o, ExprPtr r)
        : op(std::move(o)), right(std::move(r)) { line = op.line; }
    ExprPtr fold() override;
};

struct GroupingExpr : Expr {
    ExprPtr inner;
    explicit GroupingExpr(ExprPtr e, int ln) : inner(std::move(e)) { line = ln; }
    ExprPtr fold() override;
};

// ── Statement base ─────────────────────────────────────────────────────────
struct Stmt {
    int line = 0;
    virtual ~Stmt() = default;
    virtual void fold() {}
};

struct ExprStmt : Stmt {
    ExprPtr expr;
    explicit ExprStmt(ExprPtr e) : expr(std::move(e)) { line = expr->line; }
    void fold() override {
        auto folded = expr->fold();
        if (folded) expr = std::move(folded);
    }
};

struct PrintStmt : Stmt {
    ExprPtr expr;
    explicit PrintStmt(ExprPtr e, int ln) : expr(std::move(e)) { line = ln; }
    void fold() override {
        auto folded = expr->fold();
        if (folded) expr = std::move(folded);
    }
};

struct InputStmt : Stmt {
    std::string varName;
    explicit InputStmt(std::string n, int ln) : varName(std::move(n)) { line = ln; }
    void fold() override {}
};

struct LetStmt : Stmt {
    std::string name;
    ExprPtr     initializer;
    LetStmt(std::string n, ExprPtr init, int ln)
        : name(std::move(n)), initializer(std::move(init)) { line = ln; }
    void fold() override {
        auto folded = initializer->fold();
        if (folded) initializer = std::move(folded);
    }
};

struct BlockStmt : Stmt {
    std::vector<StmtPtr> stmts;
    explicit BlockStmt(std::vector<StmtPtr> s, int ln)
        : stmts(std::move(s)) { line = ln; }
    void fold() override {
        for (auto& s : stmts) s->fold();
    }
};

struct IfStmt : Stmt {
    ExprPtr  condition;
    StmtPtr  thenBranch;
    StmtPtr  elseBranch;   // may be nullptr
    IfStmt(ExprPtr c, StmtPtr t, StmtPtr e, int ln)
        : condition(std::move(c)), thenBranch(std::move(t)), elseBranch(std::move(e))
        { line = ln; }
    void fold() override {
        auto folded = condition->fold();
        if (folded) condition = std::move(folded);
        thenBranch->fold();
        if (elseBranch) elseBranch->fold();
    }
};

struct WhileStmt : Stmt {
    ExprPtr condition;
    StmtPtr body;
    WhileStmt(ExprPtr c, StmtPtr b, int ln)
        : condition(std::move(c)), body(std::move(b)) { line = ln; }
    void fold() override {
        auto folded = condition->fold();
        if (folded) condition = std::move(folded);
        body->fold();
    }
};

// ── Program ────────────────────────────────────────────────────────────────
struct Program {
    std::vector<StmtPtr> stmts;
};
