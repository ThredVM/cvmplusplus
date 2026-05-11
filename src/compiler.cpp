#include "cvm/compiler.hpp"
#include "cvm/error.hpp"

Compiler::Compiler() = default;

Chunk Compiler::compile(const Program& program) {
    for (const auto& stmt : program.stmts) {
        compileStmt(*stmt);
    }
    chunk_.emitOp(OpCode::HALT, program.stmts.empty() ? 1 : program.stmts.back()->line);
    return chunk_;
}

void Compiler::compileStmt(const Stmt& stmt) {
    if (auto s = dynamic_cast<const LetStmt*>(&stmt)) compileLetStmt(*s);
    else if (auto s = dynamic_cast<const PrintStmt*>(&stmt)) compilePrintStmt(*s);
    else if (auto s = dynamic_cast<const InputStmt*>(&stmt)) compileInputStmt(*s);
    else if (auto s = dynamic_cast<const ExprStmt*>(&stmt)) compileExprStmt(*s);
    else if (auto s = dynamic_cast<const BlockStmt*>(&stmt)) compileBlockStmt(*s);
    else if (auto s = dynamic_cast<const IfStmt*>(&stmt)) compileIfStmt(*s);
    else if (auto s = dynamic_cast<const WhileStmt*>(&stmt)) compileWhileStmt(*s);
}

uint16_t Compiler::addConstant(Value v) {
    if (chunk_.constants.size() >= 0xFFFF) {
        throw CompileError("Too many constants in one chunk.");
    }
    return chunk_.addConstant(v);
}

uint16_t Compiler::addName(const std::string& name) {
    if (chunk_.names.size() >= 0xFFFF) {
        throw CompileError("Too many variable names in one chunk.");
    }
    return chunk_.addName(name);
}

void Compiler::compileLetStmt(const LetStmt& stmt) {
    compileExpr(*stmt.initializer);
    uint16_t nameIdx = addName(stmt.name);
    chunk_.emitOp(OpCode::DEFINE_GLOBAL, stmt.line);
    chunk_.emitU16(nameIdx, stmt.line);
}

void Compiler::compilePrintStmt(const PrintStmt& stmt) {
    compileExpr(*stmt.expr);
    chunk_.emitOp(OpCode::PRINT, stmt.line);
}

void Compiler::compileInputStmt(const InputStmt& stmt) {
    chunk_.emitOp(OpCode::INPUT, stmt.line);
    uint16_t nameIdx = addName(stmt.varName);
    chunk_.emitOp(OpCode::SET_GLOBAL, stmt.line);
    chunk_.emitU16(nameIdx, stmt.line);
    chunk_.emitOp(OpCode::POP, stmt.line);
}

void Compiler::compileExprStmt(const ExprStmt& stmt) {
    compileExpr(*stmt.expr);
    chunk_.emitOp(OpCode::POP, stmt.line);
}

void Compiler::compileBlockStmt(const BlockStmt& stmt) {
    for (const auto& s : stmt.stmts) {
        compileStmt(*s);
    }
}

void Compiler::compileIfStmt(const IfStmt& stmt) {
    compileExpr(*stmt.condition);

    chunk_.emitOp(OpCode::JUMP_IF_FALSE, stmt.line);
    size_t jumpFalsePatch = chunk_.code.size();
    chunk_.emitU16(0xFFFF, stmt.line);

    compileStmt(*stmt.thenBranch);

    if (stmt.elseBranch) {
        chunk_.emitOp(OpCode::JUMP, stmt.line);
        size_t jumpEndPatch = chunk_.code.size();
        chunk_.emitU16(0xFFFF, stmt.line);

        size_t elseStart = chunk_.code.size();
        uint16_t offset = static_cast<uint16_t>(elseStart - (jumpFalsePatch + 2));
        chunk_.code[jumpFalsePatch] = (offset >> 8) & 0xFF;
        chunk_.code[jumpFalsePatch + 1] = offset & 0xFF;

        compileStmt(*stmt.elseBranch);

        size_t endPos = chunk_.code.size();
        uint16_t endOffset = static_cast<uint16_t>(endPos - (jumpEndPatch + 2));
        chunk_.code[jumpEndPatch] = (endOffset >> 8) & 0xFF;
        chunk_.code[jumpEndPatch + 1] = endOffset & 0xFF;
    } else {
        size_t endPos = chunk_.code.size();
        uint16_t offset = static_cast<uint16_t>(endPos - (jumpFalsePatch + 2));
        chunk_.code[jumpFalsePatch] = (offset >> 8) & 0xFF;
        chunk_.code[jumpFalsePatch + 1] = offset & 0xFF;
    }
}

void Compiler::compileWhileStmt(const WhileStmt& stmt) {
    size_t loopStart = chunk_.code.size();

    compileExpr(*stmt.condition);

    chunk_.emitOp(OpCode::JUMP_IF_FALSE, stmt.line);
    size_t exitPatch = chunk_.code.size();
    chunk_.emitU16(0xFFFF, stmt.line);

    compileStmt(*stmt.body);

    chunk_.emitOp(OpCode::LOOP, stmt.line);
    size_t loopEnd = chunk_.code.size() + 2;
    uint16_t backOffset = static_cast<uint16_t>(loopEnd - loopStart);
    chunk_.emitU16(backOffset, stmt.line);

    size_t endPos = chunk_.code.size();
    uint16_t fwdOffset = static_cast<uint16_t>(endPos - (exitPatch + 2));
    chunk_.code[exitPatch] = (fwdOffset >> 8) & 0xFF;
    chunk_.code[exitPatch + 1] = fwdOffset & 0xFF;
}

void Compiler::compileExpr(const Expr& expr) {
    if (auto e = dynamic_cast<const NumberLitExpr*>(&expr)) compileLiteralExpr(*e);
    else if (auto e = dynamic_cast<const BoolLitExpr*>(&expr)) compileBoolLiteralExpr(*e);
    else if (auto e = dynamic_cast<const IdentExpr*>(&expr)) compileIdentExpr(*e);
    else if (auto e = dynamic_cast<const AssignExpr*>(&expr)) compileAssignExpr(*e);
    else if (auto e = dynamic_cast<const BinaryExpr*>(&expr)) compileBinaryExpr(*e);
    else if (auto e = dynamic_cast<const UnaryExpr*>(&expr)) compileUnaryExpr(*e);
    else if (auto e = dynamic_cast<const GroupingExpr*>(&expr)) compileGroupingExpr(*e);
}

void Compiler::compileBinaryExpr(const BinaryExpr& expr) {
    compileExpr(*expr.left);
    compileExpr(*expr.right);

    switch (expr.op.type) {
        case TokenType::PLUS:    chunk_.emitOp(OpCode::ADD, expr.line); break;
        case TokenType::MINUS:   chunk_.emitOp(OpCode::SUB, expr.line); break;
        case TokenType::STAR:    chunk_.emitOp(OpCode::MUL, expr.line); break;
        case TokenType::SLASH:   chunk_.emitOp(OpCode::DIV, expr.line); break;
        case TokenType::EQ_EQ:   chunk_.emitOp(OpCode::EQ,  expr.line); break;
        case TokenType::BANG_EQ: chunk_.emitOp(OpCode::NEQ, expr.line); break;
        case TokenType::LT:      chunk_.emitOp(OpCode::LT,  expr.line); break;
        case TokenType::LT_EQ:   chunk_.emitOp(OpCode::LT_EQ, expr.line); break;
        case TokenType::GT:      chunk_.emitOp(OpCode::GT,  expr.line); break;
        case TokenType::GT_EQ:   chunk_.emitOp(OpCode::GT_EQ, expr.line); break;
        default: break;
    }
}

void Compiler::compileUnaryExpr(const UnaryExpr& expr) {
    compileExpr(*expr.right);
    switch (expr.op.type) {
        case TokenType::MINUS: chunk_.emitOp(OpCode::NEG, expr.line); break;
        case TokenType::BANG:  chunk_.emitOp(OpCode::NOT, expr.line); break;
        default: break;
    }
}

void Compiler::compileLiteralExpr(const NumberLitExpr& expr) {
    uint16_t idx = addConstant(Value{expr.value});
    chunk_.emitOp(OpCode::PUSH_CONST, expr.line);
    chunk_.emitU16(idx, expr.line);
}

void Compiler::compileBoolLiteralExpr(const BoolLitExpr& expr) {
    chunk_.emitOp(expr.value ? OpCode::PUSH_TRUE : OpCode::PUSH_FALSE, expr.line);
}

void Compiler::compileIdentExpr(const IdentExpr& expr) {
    uint16_t nameIdx = addName(expr.name);
    chunk_.emitOp(OpCode::GET_GLOBAL, expr.line);
    chunk_.emitU16(nameIdx, expr.line);
}

void Compiler::compileAssignExpr(const AssignExpr& expr) {
    compileExpr(*expr.value);
    uint16_t nameIdx = addName(expr.name);
    chunk_.emitOp(OpCode::SET_GLOBAL, expr.line);
    chunk_.emitU16(nameIdx, expr.line);
}

void Compiler::compileGroupingExpr(const GroupingExpr& expr) {
    compileExpr(*expr.inner);
}
