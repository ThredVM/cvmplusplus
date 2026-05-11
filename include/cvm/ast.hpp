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
};

struct NumberLitExpr : Expr {
    int64_t value;
    explicit NumberLitExpr(int64_t v, int ln) : value(v) { line = ln; }
};

struct BoolLitExpr : Expr {
    bool value;
    explicit BoolLitExpr(bool v, int ln) : value(v) { line = ln; }
};

struct IdentExpr : Expr {
    std::string name;
    explicit IdentExpr(std::string n, int ln) : name(std::move(n)) { line = ln; }
};

struct AssignExpr : Expr {
    std::string name;
    ExprPtr     value;
    AssignExpr(std::string n, ExprPtr v, int ln)
        : name(std::move(n)), value(std::move(v)) { line = ln; }
};

struct BinaryExpr : Expr {
    Token    op;      // holds the operator token (for error messages)
    ExprPtr  left;
    ExprPtr  right;
    BinaryExpr(Token o, ExprPtr l, ExprPtr r)
        : op(std::move(o)), left(std::move(l)), right(std::move(r)) { line = op.line; }
};

struct UnaryExpr : Expr {
    Token   op;
    ExprPtr right;
    UnaryExpr(Token o, ExprPtr r)
        : op(std::move(o)), right(std::move(r)) { line = op.line; }
};

struct GroupingExpr : Expr {
    ExprPtr inner;
    explicit GroupingExpr(ExprPtr e, int ln) : inner(std::move(e)) { line = ln; }
};

// ── Statement base ─────────────────────────────────────────────────────────
struct Stmt {
    int line = 0;
    virtual ~Stmt() = default;
};

struct ExprStmt : Stmt {
    ExprPtr expr;
    explicit ExprStmt(ExprPtr e) : expr(std::move(e)) { line = expr->line; }
};

struct PrintStmt : Stmt {
    ExprPtr expr;
    explicit PrintStmt(ExprPtr e, int ln) : expr(std::move(e)) { line = ln; }
};

struct InputStmt : Stmt {
    std::string varName;
    explicit InputStmt(std::string n, int ln) : varName(std::move(n)) { line = ln; }
};

struct LetStmt : Stmt {
    std::string name;
    ExprPtr     initializer;
    LetStmt(std::string n, ExprPtr init, int ln)
        : name(std::move(n)), initializer(std::move(init)) { line = ln; }
};

struct BlockStmt : Stmt {
    std::vector<StmtPtr> stmts;
    explicit BlockStmt(std::vector<StmtPtr> s, int ln)
        : stmts(std::move(s)) { line = ln; }
};

struct IfStmt : Stmt {
    ExprPtr  condition;
    StmtPtr  thenBranch;
    StmtPtr  elseBranch;   // may be nullptr
    IfStmt(ExprPtr c, StmtPtr t, StmtPtr e, int ln)
        : condition(std::move(c)), thenBranch(std::move(t)), elseBranch(std::move(e))
        { line = ln; }
};

struct WhileStmt : Stmt {
    ExprPtr condition;
    StmtPtr body;
    WhileStmt(ExprPtr c, StmtPtr b, int ln)
        : condition(std::move(c)), body(std::move(b)) { line = ln; }
};

// ── Program ────────────────────────────────────────────────────────────────
struct Program {
    std::vector<StmtPtr> stmts;
};
