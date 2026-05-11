#pragma once
#include "value.hpp"
#include <vector>
#include <cstdint>
#include <string>

// ── Instruction Set Architecture ──────────────────────────────────────────
enum class OpCode : uint8_t {
    PUSH_CONST,     // [idx:u16]
    PUSH_TRUE,
    PUSH_FALSE,

    ADD, SUB, MUL, DIV, NEG,
    EQ, NEQ, LT, LT_EQ, GT, GT_EQ,
    NOT,

    DEFINE_GLOBAL,  // [idx:u16]
    GET_GLOBAL,     // [idx:u16]
    SET_GLOBAL,     // [idx:u16]

    GET_LOCAL,      // [slot:u16]
    SET_LOCAL,      // [slot:u16]

    POP,
    JUMP,           // [offset:u16]
    JUMP_IF_FALSE,  // [offset:u16]
    LOOP,           // [offset:u16]

    PRINT,
    INPUT,
    HALT
};

struct Chunk {
    std::vector<uint8_t>     code;
    std::vector<Value>       constants;
    std::vector<std::string> names;
    std::vector<int>         lines;

    void emit(uint8_t byte, int line) {
        code.push_back(byte);
        lines.push_back(line);
    }

    void emitOp(OpCode op, int line) {
        emit(static_cast<uint8_t>(op), line);
    }

    void emitU16(uint16_t val, int line) {
        emit((val >> 8) & 0xFF, line);
        emit(val & 0xFF, line);
    }

    uint16_t addConstant(Value v) {
        constants.push_back(v);
        return static_cast<uint16_t>(constants.size() - 1);
    }

    uint16_t addName(const std::string& name) {
        for (size_t i = 0; i < names.size(); i++)
            if (names[i] == name) return static_cast<uint16_t>(i);
        names.push_back(name);
        return static_cast<uint16_t>(names.size() - 1);
    }
};
