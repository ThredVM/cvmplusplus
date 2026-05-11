#include "cvm/debug.hpp"
#include <iostream>
#include <iomanip>

static const char* tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::NUMBER: return "NUMBER";
        case TokenType::TRUE: return "TRUE";
        case TokenType::FALSE: return "FALSE";
        case TokenType::STRING: return "STRING";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::LET: return "LET";
        case TokenType::PRINT: return "PRINT";
        case TokenType::INPUT: return "INPUT";
        case TokenType::IF: return "IF";
        case TokenType::ELSE: return "ELSE";
        case TokenType::WHILE: return "WHILE";
        case TokenType::FN: return "FN";
        case TokenType::RETURN: return "RETURN";
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::STAR: return "STAR";
        case TokenType::SLASH: return "SLASH";
        case TokenType::EQ: return "EQ";
        case TokenType::EQ_EQ: return "EQ_EQ";
        case TokenType::BANG: return "BANG";
        case TokenType::BANG_EQ: return "BANG_EQ";
        case TokenType::LT: return "LT";
        case TokenType::LT_EQ: return "LT_EQ";
        case TokenType::GT: return "GT";
        case TokenType::GT_EQ: return "GT_EQ";
        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::LBRACE: return "LBRACE";
        case TokenType::RBRACE: return "RBRACE";
        case TokenType::COMMA: return "COMMA";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::NEWLINE: return "NEWLINE";
        case TokenType::EOF_TOKEN: return "EOF";
        default: return "UNKNOWN";
    }
}

void dumpTokens(const std::vector<Token>& tokens) {
    std::cout << "--- Tokens ---" << std::endl;
    for (const auto& token : tokens) {
        std::cout << std::setw(4) << token.line << " | " 
                  << std::setw(12) << tokenTypeToString(token.type) << " '" 
                  << token.lexeme << "'" << std::endl;
    }
}

static void printIndent(int indent) {
    for (int i = 0; i < indent; i++) std::cout << "  ";
}

static void dumpExpr(const Expr& expr, int indent) {
    printIndent(indent);
    if (auto e = dynamic_cast<const NumberLitExpr*>(&expr)) {
        std::cout << "NumberLiteral: " << e->value << std::endl;
    } else if (auto e = dynamic_cast<const StringLitExpr*>(&expr)) {
        std::cout << "StringLiteral: \"" << e->value << "\"" << std::endl;
    } else if (auto e = dynamic_cast<const BoolLitExpr*>(&expr)) {
        std::cout << "BoolLiteral: " << (e->value ? "true" : "false") << std::endl;
    } else if (auto e = dynamic_cast<const IdentExpr*>(&expr)) {
        std::cout << "Identifier: " << e->name << std::endl;
    } else if (auto e = dynamic_cast<const BinaryExpr*>(&expr)) {
        std::cout << "BinaryExpr: " << e->op.lexeme << std::endl;
        dumpExpr(*e->left, indent + 1);
        dumpExpr(*e->right, indent + 1);
    } else if (auto e = dynamic_cast<const UnaryExpr*>(&expr)) {
        std::cout << "UnaryExpr: " << e->op.lexeme << std::endl;
        dumpExpr(*e->right, indent + 1);
    } else if (auto e = dynamic_cast<const GroupingExpr*>(&expr)) {
        std::cout << "Grouping" << std::endl;
        dumpExpr(*e->inner, indent + 1);
    } else if (auto e = dynamic_cast<const AssignExpr*>(&expr)) {
        std::cout << "Assignment: " << e->name << std::endl;
        dumpExpr(*e->value, indent + 1);
    } else if (auto e = dynamic_cast<const CallExpr*>(&expr)) {
        std::cout << "CallExpr" << std::endl;
        printIndent(indent + 1); std::cout << "Callee:" << std::endl;
        dumpExpr(*e->callee, indent + 2);
        printIndent(indent + 1); std::cout << "Arguments:" << std::endl;
        for (const auto& arg : e->args) dumpExpr(*arg, indent + 2);
    }
}

static void dumpStmt(const Stmt& stmt, int indent) {
    printIndent(indent);
    if (auto s = dynamic_cast<const LetStmt*>(&stmt)) {
        std::cout << "LetStmt: " << s->name << std::endl;
        dumpExpr(*s->initializer, indent + 1);
    } else if (auto s = dynamic_cast<const PrintStmt*>(&stmt)) {
        std::cout << "PrintStmt" << std::endl;
        dumpExpr(*s->expr, indent + 1);
    } else if (auto s = dynamic_cast<const InputStmt*>(&stmt)) {
        std::cout << "InputStmt: " << s->varName << std::endl;
    } else if (auto s = dynamic_cast<const ExprStmt*>(&stmt)) {
        std::cout << "ExprStmt" << std::endl;
        dumpExpr(*s->expr, indent + 1);
    } else if (auto s = dynamic_cast<const BlockStmt*>(&stmt)) {
        std::cout << "BlockStmt" << std::endl;
        for (const auto& substmt : s->stmts) dumpStmt(*substmt, indent + 1);
    } else if (auto s = dynamic_cast<const IfStmt*>(&stmt)) {
        std::cout << "IfStmt" << std::endl;
        printIndent(indent + 1); std::cout << "Condition:" << std::endl;
        dumpExpr(*s->condition, indent + 2);
        printIndent(indent + 1); std::cout << "Then:" << std::endl;
        dumpStmt(*s->thenBranch, indent + 2);
        if (s->elseBranch) {
            printIndent(indent + 1); std::cout << "Else:" << std::endl;
            dumpStmt(*s->elseBranch, indent + 2);
        }
    } else if (auto s = dynamic_cast<const WhileStmt*>(&stmt)) {
        std::cout << "WhileStmt" << std::endl;
        printIndent(indent + 1); std::cout << "Condition:" << std::endl;
        dumpExpr(*s->condition, indent + 2);
        printIndent(indent + 1); std::cout << "Body:" << std::endl;
        dumpStmt(*s->body, indent + 2);
    } else if (auto s = dynamic_cast<const FnStmt*>(&stmt)) {
        std::cout << "FnStmt: " << s->name << " (";
        for (size_t i = 0; i < s->params.size(); i++) {
            std::cout << s->params[i] << (i == s->params.size() - 1 ? "" : ", ");
        }
        std::cout << ")" << std::endl;
        dumpStmt(*s->body, indent + 1);
    } else if (auto s = dynamic_cast<const ReturnStmt*>(&stmt)) {
        std::cout << "ReturnStmt" << std::endl;
        if (s->value) dumpExpr(*s->value, indent + 1);
    }
}

void dumpAST(const Program& program) {
    std::cout << "--- AST ---" << std::endl;
    for (const auto& stmt : program.stmts) {
        dumpStmt(*stmt, 0);
    }
}

void disassemble(const Chunk& chunk, const std::string& name) {
    std::cout << "== " << name << " ==" << std::endl;
    for (int offset = 0; offset < (int)chunk.code.size(); ) {
        offset = disassembleInstruction(chunk, offset);
    }
}

static int simpleInstruction(const std::string& name, int offset) {
    std::cout << name << std::endl;
    return offset + 1;
}

static int u16Instruction(const std::string& name, const Chunk& chunk, int offset) {
    uint16_t val = (chunk.code[offset + 1] << 8) | chunk.code[offset + 2];
    std::cout << std::left << std::setw(16) << name << " " << std::right << std::setw(4) << val << std::endl;
    return offset + 3;
}

static int constantInstruction(const std::string& name, const Chunk& chunk, int offset) {
    uint16_t constant = (chunk.code[offset + 1] << 8) | chunk.code[offset + 2];
    std::cout << std::left << std::setw(16) << name << " " << std::right << std::setw(4) << constant << " '";
    std::cout << valueToString(chunk.constants[constant]) << "'" << std::endl;
    return offset + 3;
}

static int nameInstruction(const std::string& name, const Chunk& chunk, int offset) {
    uint16_t nameIdx = (chunk.code[offset + 1] << 8) | chunk.code[offset + 2];
    std::cout << std::left << std::setw(16) << name << " " << std::right << std::setw(4) << nameIdx << " '";
    std::cout << chunk.names[nameIdx] << "'" << std::endl;
    return offset + 3;
}

int disassembleInstruction(const Chunk& chunk, int offset) {
    std::cout << std::right << std::setfill('0') << std::setw(4) << offset << " ";
    std::cout << std::setfill(' ') << std::setw(4) << chunk.lines[offset] << " ";

    OpCode op = static_cast<OpCode>(chunk.code[offset]);
    switch (op) {
        case OpCode::PUSH_CONST: return constantInstruction("PUSH_CONST", chunk, offset);
        case OpCode::PUSH_TRUE:  return simpleInstruction("PUSH_TRUE", offset);
        case OpCode::PUSH_FALSE: return simpleInstruction("PUSH_FALSE", offset);
        case OpCode::ADD:        return simpleInstruction("ADD", offset);
        case OpCode::SUB:        return simpleInstruction("SUB", offset);
        case OpCode::MUL:        return simpleInstruction("MUL", offset);
        case OpCode::DIV:        return simpleInstruction("DIV", offset);
        case OpCode::NEG:        return simpleInstruction("NEG", offset);
        case OpCode::EQ:         return simpleInstruction("EQ", offset);
        case OpCode::NEQ:        return simpleInstruction("NEQ", offset);
        case OpCode::LT:         return simpleInstruction("LT", offset);
        case OpCode::LT_EQ:      return simpleInstruction("LT_EQ", offset);
        case OpCode::GT:         return simpleInstruction("GT", offset);
        case OpCode::GT_EQ:      return simpleInstruction("GT_EQ", offset);
        case OpCode::NOT:        return simpleInstruction("NOT", offset);
        case OpCode::DEFINE_GLOBAL: return nameInstruction("DEFINE_GLOBAL", chunk, offset);
        case OpCode::GET_GLOBAL:    return nameInstruction("GET_GLOBAL", chunk, offset);
        case OpCode::SET_GLOBAL:    return nameInstruction("SET_GLOBAL", chunk, offset);
        case OpCode::GET_LOCAL:     return u16Instruction("GET_LOCAL", chunk, offset);
        case OpCode::SET_LOCAL:     return u16Instruction("SET_LOCAL", chunk, offset);
        case OpCode::CALL: {
            uint8_t argc = chunk.code[offset + 1];
            std::cout << std::left << std::setw(16) << "CALL" << " " << std::right << std::setw(4) << (int)argc << std::endl;
            return offset + 2;
        }
        case OpCode::RET:           return simpleInstruction("RET", offset);
        case OpCode::POP:           return simpleInstruction("POP", offset);
        case OpCode::JUMP:          return u16Instruction("JUMP", chunk, offset);
        case OpCode::JUMP_IF_FALSE: return u16Instruction("JUMP_IF_FALSE", chunk, offset);
        case OpCode::LOOP:          return u16Instruction("LOOP", chunk, offset);
        case OpCode::PRINT:         return simpleInstruction("PRINT", offset);
        case OpCode::INPUT:         return simpleInstruction("INPUT", offset);
        case OpCode::HALT:          return simpleInstruction("HALT", offset);
        default:
            std::cout << "Unknown opcode " << (int)op << std::endl;
            return offset + 1;
    }
}
