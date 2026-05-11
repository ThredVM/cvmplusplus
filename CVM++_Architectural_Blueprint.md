# CVM++ — Complete Architectural Blueprint
### Stack-Based Virtual Machine & Custom Compiler in C++
*Principal Architect's Deep-Dive Plan — Vibe Coding Edition*

---

## Pre-Code Architectural Reasoning

Before touching an editor, every structural decision must be locked in. Below is the full chain-of-thought analysis.

### 1. Architectural Bottlenecks Identified & Resolved

| Concern | Risk | Decision |
|---|---|---|
| **VM exec loop** | `switch` dispatch has branch-misprediction overhead | Use `switch` for MVP clarity; mark extension point for computed-goto in Phase 4 |
| **AST ownership** | Raw pointers → dangling references, leaks | All AST nodes owned by `std::unique_ptr`; parent holds children |
| **Bytecode operand width** | 1-byte index limits constant pool to 255 entries | Use **2-byte (uint16_t) operand** for all indexed instructions from day one — avoids a painful refactor |
| **Stack overflow** | Unbounded recursion in compiler or VM | Compiler: iterative AST traversal where possible; VM: cap stack at configurable `MAX_STACK` (default 256) |
| **Variable scoping** | Flat global map breaks nested blocks | Phase 1/2: single global `unordered_map`; Phase 3+: scope chain via vector of frames |
| **Error propagation** | Each stage fails differently | Single `CVMError` exception hierarchy thrown upward; `main.cpp` catches and pretty-prints |
| **Type mismatches** | `int + bool` silently broken | Explicit runtime type check in every binary-op handler; throw `RuntimeError` with a message |
| **Division by zero** | Undefined behavior | Explicit guard before `DIV` execution in VM |
| **Jump patching** | Forward jumps unknown at emit time | **Backpatching**: emit a placeholder `0xFFFF`, compile body, then write real offset |

### 2. Tech Stack Decisions (Justified)

- **C++17** minimum — required for `std::variant`, `std::optional`, structured bindings, and `if constexpr`.
- **CMake 3.16+** — portable build; `FetchContent` available if test framework added later.
- **Standard library only** — no Boost, no LLVM. The goal is understanding, not production use.
- **AST representation: Class hierarchy + `std::unique_ptr`** — more didactic than `std::variant`; visitors are a natural teaching moment. A `Visitor` pattern is wired in but made optional for MVP.
- **Value type: `std::variant<int64_t, bool>`** — type-safe, no heap allocation for primitives, trivially extensible later (add `double`, `std::string`).
- **Bytecode container: `Chunk` struct** — bundles `std::vector<uint8_t> code`, `std::vector<Value> constants`, `std::vector<std::string> names`, and `std::vector<int> lines` together.
- **Stack-based ISA** — operands consumed from stack, results pushed back. Zero-operand arithmetic opcodes keep the instruction stream dense and the VM loop simple.

---

## Pipeline Diagram

```
 ┌──────────────────────────────────────────────────────────────────────────┐
 │                         CVM++ PIPELINE                                    │
 └──────────────────────────────────────────────────────────────────────────┘

  Source file / REPL input
  "let x = 10 + 2\nprint x"
          │
          ▼
  ┌───────────────┐    std::vector<Token>
  │     LEXER     │ ─────────────────────►  [LET][IDENT:"x"][EQ][NUMBER:10]
  │   lexer.cpp   │                         [PLUS][NUMBER:2][NEWLINE][PRINT]
  └───────────────┘                         [IDENT:"x"][EOF]
          │
          ▼
  ┌───────────────┐    AST (unique_ptr tree)
  │    PARSER     │ ─────────────────────►  Program
  │   parser.cpp  │                           └─ LetStmt("x")
  └───────────────┘                                └─ BinaryExpr(+)
                                                        ├─ NumberLit(10)
                                                        └─ NumberLit(2)
                                                   PrintStmt
                                                     └─ IdentExpr("x")
          │
          ▼
  ┌───────────────┐    Chunk (bytecode + pools)
  │   COMPILER   │ ─────────────────────►  PUSH_CONST  0   ; 10
  │  compiler.cpp │                         PUSH_CONST  1   ; 2
  └───────────────┘                         ADD
                                            DEFINE_GLOBAL 0 ; "x"
                                            GET_GLOBAL    0 ; "x"
                                            PRINT
                                            HALT
          │
          ▼
  ┌───────────────┐    stdout
  │      VM       │ ─────────────────────►  12
  │    vm.cpp     │
  └───────────────┘

  Constants pool: [10, 2]
  Names pool:     ["x"]
```

---

## Recommended Folder Structure

```
cvm++/
├── CMakeLists.txt              # Root build definition
├── README.md
│
├── include/                    # All public headers (no .cpp needed in small project)
│   ├── token.hpp               # TokenType enum + Token struct
│   ├── lexer.hpp               # Lexer class declaration
│   ├── ast.hpp                 # All AST node classes (Expr + Stmt hierarchies)
│   ├── parser.hpp              # Parser class declaration
│   ├── value.hpp               # Value variant + helpers
│   ├── chunk.hpp               # Chunk struct + OpCode enum
│   ├── compiler.hpp            # Compiler class declaration
│   ├── vm.hpp                  # VM class declaration
│   └── error.hpp               # CVMError hierarchy
│
├── src/
│   ├── main.cpp                # Entry point: REPL + file runner
│   ├── lexer.cpp
│   ├── parser.cpp
│   ├── compiler.cpp
│   └── vm.cpp
│
├── tests/
│   ├── test_lexer.cpp          # Unit tests for tokenization
│   ├── test_parser.cpp         # Unit tests for AST shape
│   └── test_vm.cpp             # End-to-end execution tests
│
└── scripts/                    # .cvm sample programs
    ├── 01_hello.cvm
    ├── 02_arithmetic.cvm
    ├── 03_variables.cvm
    ├── 04_booleans.cvm
    ├── 05_if_else.cvm
    ├── 06_while_loop.cvm
    ├── 07_input.cvm
    └── 08_fibonacci.cvm
```

**Rule:** Headers in `include/` are the contracts. `.cpp` files in `src/` are the implementations. Never include a `.cpp`.

---

## CMakeLists.txt Skeleton

```cmake
cmake_minimum_required(VERSION 3.16)
project(cvmpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # for clangd / IDE support

# Compiler warnings
add_compile_options(-Wall -Wextra -Wpedantic)

include_directories(include)

add_executable(cvm
    src/main.cpp
    src/lexer.cpp
    src/parser.cpp
    src/compiler.cpp
    src/vm.cpp
)

# Optional: enable tests
# enable_testing()
# add_subdirectory(tests)
```

Build commands:
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
./cvm ../scripts/01_hello.cvm
```

---

## Complete Data Structure Designs

### A. `token.hpp` — Tokens

```cpp
#pragma once
#include <string>

enum class TokenType {
    // Literals
    NUMBER, TRUE, FALSE,

    // Identifiers & keywords
    IDENTIFIER,
    LET, PRINT, INPUT,
    IF, ELSE, WHILE,

    // Operators
    PLUS, MINUS, STAR, SLASH,
    EQ, EQ_EQ,          // = and ==
    BANG, BANG_EQ,      // ! and !=
    LT, LT_EQ,
    GT, GT_EQ,

    // Delimiters
    LPAREN, RPAREN,
    LBRACE, RBRACE,
    SEMICOLON, NEWLINE,

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
```

### B. `value.hpp` — Runtime Values

```cpp
#pragma once
#include <variant>
#include <string>
#include <stdexcept>

using Value = std::variant<int64_t, bool>;

// Helpers
inline bool isInt(const Value& v)  { return std::holds_alternative<int64_t>(v); }
inline bool isBool(const Value& v) { return std::holds_alternative<bool>(v); }

inline int64_t asInt(const Value& v)  { return std::get<int64_t>(v); }
inline bool    asBool(const Value& v) { return std::get<bool>(v); }

std::string valueToString(const Value& v);
// Implementation: returns "true"/"false" for bool, decimal string for int

bool isTruthy(const Value& v);
// Implementation: bool → its value; int → nonzero
```

### C. `chunk.hpp` — Opcodes & Bytecode Container

```cpp
#pragma once
#include "value.hpp"
#include <vector>
#include <cstdint>
#include <string>

// ── Instruction Set Architecture ──────────────────────────────────────────
//
//  Format:  [1-byte opcode] [optional 2-byte operand (uint16_t, big-endian)]
//
//  Stack effect documented as: (inputs -- outputs)

enum class OpCode : uint8_t {
    // Constants & Literals          operand        stack effect
    PUSH_CONST,     // [idx:u16]    push constants[idx]       ( -- v )
    PUSH_TRUE,      //              push true                 ( -- v )
    PUSH_FALSE,     //              push false                ( -- v )

    // Arithmetic (integers only)
    ADD,            //                                        (a b -- a+b)
    SUB,            //                                        (a b -- a-b)
    MUL,            //                                        (a b -- a*b)
    DIV,            //              checks div-by-zero        (a b -- a/b)
    NEG,            //                                        (a -- -a)

    // Comparison (produce bool)
    EQ,             //                                        (a b -- bool)
    NEQ,            //                                        (a b -- bool)
    LT,             //                                        (a b -- bool)
    LT_EQ,          //                                        (a b -- bool)
    GT,             //                                        (a b -- bool)
    GT_EQ,          //                                        (a b -- bool)

    // Boolean
    NOT,            //                                        (bool -- bool)

    // Variables                     operand
    DEFINE_GLOBAL,  // [idx:u16]    globals[names[idx]] = pop()  (v -- )
    GET_GLOBAL,     // [idx:u16]    push globals[names[idx]]     ( -- v )
    SET_GLOBAL,     // [idx:u16]    globals[names[idx]] = top()  (v -- v) NB: leaves value

    // Stack
    POP,            //              discard top               (v -- )

    // Control Flow                  operand
    JUMP,           // [offset:u16] ip += offset (unconditional forward)
    JUMP_IF_FALSE,  // [offset:u16] if !truthy(top): ip += offset; pop()
    LOOP,           // [offset:u16] ip -= offset (unconditional backward)

    // I/O
    PRINT,          //              print & pop top           (v -- )
    INPUT,          //              read line, push int       ( -- v )

    HALT            //              stop execution
};

// ── Chunk ──────────────────────────────────────────────────────────────────
struct Chunk {
    std::vector<uint8_t>     code;       // raw bytecode stream
    std::vector<Value>       constants;  // constant pool
    std::vector<std::string> names;      // global variable name pool
    std::vector<int>         lines;      // parallel to code[] for debug

    // Emit a single byte; track source line
    void emit(uint8_t byte, int line) {
        code.push_back(byte);
        lines.push_back(line);
    }

    // Emit opcode (convenience cast)
    void emitOp(OpCode op, int line) {
        emit(static_cast<uint8_t>(op), line);
    }

    // Emit 2-byte operand (big-endian)
    void emitU16(uint16_t val, int line) {
        emit((val >> 8) & 0xFF, line);
        emit(val & 0xFF, line);
    }

    // Add a constant; return its index
    uint16_t addConstant(Value v) {
        constants.push_back(v);
        return static_cast<uint16_t>(constants.size() - 1);
    }

    // Add a name; return its index (deduplicates)
    uint16_t addName(const std::string& name) {
        for (size_t i = 0; i < names.size(); i++)
            if (names[i] == name) return static_cast<uint16_t>(i);
        names.push_back(name);
        return static_cast<uint16_t>(names.size() - 1);
    }
};
```

### D. `ast.hpp` — AST Node Hierarchy

```cpp
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
```

### E. `error.hpp` — Error Hierarchy

```cpp
#pragma once
#include <stdexcept>
#include <string>

struct CVMError : std::runtime_error {
    int line;
    CVMError(const std::string& msg, int ln = -1)
        : std::runtime_error(msg), line(ln) {}
};

struct LexError     : CVMError { using CVMError::CVMError; };
struct ParseError   : CVMError { using CVMError::CVMError; };
struct CompileError : CVMError { using CVMError::CVMError; };
struct RuntimeError : CVMError { using CVMError::CVMError; };
```

### F. `vm.hpp` — Virtual Machine

```cpp
#pragma once
#include "chunk.hpp"
#include <unordered_map>
#include <vector>

constexpr size_t MAX_STACK = 256;

class VM {
public:
    explicit VM(bool debugMode = false);
    void execute(const Chunk& chunk);

private:
    std::vector<Value>                    stack_;
    std::unordered_map<std::string,Value> globals_;
    bool                                  debug_;

    // Stack helpers
    void        push(Value v);
    Value       pop();
    const Value& peek(int distance = 0) const;

    // Read 2-byte operand from ip
    uint16_t readU16(const uint8_t* ip) const;

    // Pretty-print stack state (debug only)
    void dumpStack() const;
};
```

---

## Bytecode Encoding Scheme (Reference Table)

```
Opcode          | Encoding                      | Bytes | Notes
─────────────────────────────────────────────────────────────────────────────
PUSH_CONST      | [0x00][hi][lo]                | 3     | index into constants[]
PUSH_TRUE       | [0x01]                        | 1     |
PUSH_FALSE      | [0x02]                        | 1     |
ADD             | [0x03]                        | 1     |
SUB             | [0x04]                        | 1     |
MUL             | [0x05]                        | 1     |
DIV             | [0x06]                        | 1     | runtime div-by-zero guard
NEG             | [0x07]                        | 1     |
EQ              | [0x08]                        | 1     |
NEQ             | [0x09]                        | 1     |
LT              | [0x0A]                        | 1     |
LT_EQ           | [0x0B]                        | 1     |
GT              | [0x0C]                        | 1     |
GT_EQ           | [0x0D]                        | 1     |
NOT             | [0x0E]                        | 1     |
DEFINE_GLOBAL   | [0x0F][hi][lo]                | 3     | index into names[]
GET_GLOBAL      | [0x10][hi][lo]                | 3     | index into names[]
SET_GLOBAL      | [0x11][hi][lo]                | 3     | index into names[]
POP             | [0x12]                        | 1     |
JUMP            | [0x13][hi][lo]                | 3     | ip += operand
JUMP_IF_FALSE   | [0x14][hi][lo]                | 3     | if !truthy: ip += operand; pop
LOOP            | [0x15][hi][lo]                | 3     | ip -= operand
PRINT           | [0x16]                        | 1     |
INPUT           | [0x17]                        | 1     |
HALT            | [0x18]                        | 1     |
```

**Jump offset arithmetic:**
- `JUMP` / `JUMP_IF_FALSE` offsets are relative to the byte *after* the full 3-byte instruction. So `ip` advances past the instruction first, then adds the offset.
- `LOOP` offset is similarly relative; the VM subtracts it to go backward.
- Backpatching: emit `JUMP_IF_FALSE 0xFFFF`, save the offset of those two bytes, compile body, then write `(current_offset - patch_offset - 2)` into those two bytes.

---

## VM Execution Loop Design

```cpp
void VM::execute(const Chunk& chunk) {
    const uint8_t* ip = chunk.code.data();
    const uint8_t* end = ip + chunk.code.size();

    auto READ_U16 = [&]() -> uint16_t {
        uint16_t hi = *ip++;
        uint16_t lo = *ip++;
        return (hi << 8) | lo;
    };

    while (ip < end) {
        if (debug_) dumpStack();

        OpCode op = static_cast<OpCode>(*ip++);

        switch (op) {
            case OpCode::PUSH_CONST: {
                uint16_t idx = READ_U16();
                push(chunk.constants[idx]);
                break;
            }
            case OpCode::PUSH_TRUE:  push(Value{true});  break;
            case OpCode::PUSH_FALSE: push(Value{false}); break;

            case OpCode::ADD: {
                Value b = pop(), a = pop();
                if (!isInt(a) || !isInt(b))
                    throw RuntimeError("'+' requires integers", -1);
                push(Value{asInt(a) + asInt(b)});
                break;
            }
            // ... SUB, MUL, NEG similarly ...

            case OpCode::DIV: {
                Value b = pop(), a = pop();
                if (!isInt(a) || !isInt(b))
                    throw RuntimeError("'/' requires integers", -1);
                if (asInt(b) == 0)
                    throw RuntimeError("Division by zero", -1);
                push(Value{asInt(a) / asInt(b)});
                break;
            }

            case OpCode::DEFINE_GLOBAL: {
                uint16_t idx = READ_U16();
                globals_[chunk.names[idx]] = pop();
                break;
            }
            case OpCode::GET_GLOBAL: {
                uint16_t idx = READ_U16();
                auto it = globals_.find(chunk.names[idx]);
                if (it == globals_.end())
                    throw RuntimeError("Undefined variable: " + chunk.names[idx], -1);
                push(it->second);
                break;
            }

            case OpCode::JUMP: {
                uint16_t offset = READ_U16();
                ip += offset;
                break;
            }
            case OpCode::JUMP_IF_FALSE: {
                uint16_t offset = READ_U16();
                Value cond = pop();
                if (!isTruthy(cond)) ip += offset;
                break;
            }
            case OpCode::LOOP: {
                uint16_t offset = READ_U16();
                ip -= offset;
                break;
            }

            case OpCode::PRINT: {
                std::cout << valueToString(pop()) << "\n";
                break;
            }
            case OpCode::INPUT: {
                std::string line;
                std::getline(std::cin, line);
                try { push(Value{std::stoll(line)}); }
                catch (...) { throw RuntimeError("input must be an integer"); }
                break;
            }
            case OpCode::HALT:
                return;

            default:
                throw RuntimeError("Unknown opcode");
        }
    }
}
```

---

## Parser Design — Recursive Descent Grammar

```
program        → statement* EOF
statement      → letStmt | printStmt | inputStmt | ifStmt | whileStmt
               | block | exprStmt
letStmt        → "let" IDENTIFIER "=" expression NEWLINE
printStmt      → "print" expression NEWLINE
inputStmt      → "input" IDENTIFIER NEWLINE
ifStmt         → "if" expression block ( "else" block )?
whileStmt      → "while" expression block
block          → "{" statement* "}"
exprStmt       → expression NEWLINE

expression     → assignment
assignment     → IDENTIFIER "=" assignment | equality
equality       → comparison ( ("==" | "!=") comparison )*
comparison     → term ( ("<" | "<=" | ">" | ">=") term )*
term           → factor ( ("+" | "-") factor )*
factor         → unary ( ("*" | "/") unary )*
unary          → ("!" | "-") unary | primary
primary        → NUMBER | "true" | "false" | IDENTIFIER | "(" expression ")"
```

**Precedence climbs upward** — expression is lowest, primary is highest. Each grammar rule maps 1:1 to a C++ method in `Parser`.

---

## Compiler — Key Patterns

### Emitting a Binary Expression

```cpp
void Compiler::compileBinaryExpr(const BinaryExpr& node) {
    compileExpr(*node.left);          // push left
    compileExpr(*node.right);         // push right
    switch (node.op.type) {
        case TokenType::PLUS:    chunk_.emitOp(OpCode::ADD, node.line); break;
        case TokenType::MINUS:   chunk_.emitOp(OpCode::SUB, node.line); break;
        case TokenType::STAR:    chunk_.emitOp(OpCode::MUL, node.line); break;
        case TokenType::SLASH:   chunk_.emitOp(OpCode::DIV, node.line); break;
        case TokenType::EQ_EQ:   chunk_.emitOp(OpCode::EQ,  node.line); break;
        case TokenType::LT:      chunk_.emitOp(OpCode::LT,  node.line); break;
        // ...
    }
}
```

### Emitting If/Else with Backpatching

```cpp
void Compiler::compileIfStmt(const IfStmt& node) {
    compileExpr(*node.condition);           // condition on stack

    // Emit JUMP_IF_FALSE with placeholder
    chunk_.emitOp(OpCode::JUMP_IF_FALSE, node.line);
    size_t jumpFalsePatch = chunk_.code.size();
    chunk_.emitU16(0xFFFF, node.line);      // placeholder

    compileStmt(*node.thenBranch);          // compile then-block

    if (node.elseBranch) {
        // Jump over else-block at end of then-block
        chunk_.emitOp(OpCode::JUMP, node.line);
        size_t jumpEndPatch = chunk_.code.size();
        chunk_.emitU16(0xFFFF, node.line);

        // Patch JUMP_IF_FALSE
        size_t elseStart = chunk_.code.size();
        uint16_t offset = static_cast<uint16_t>(elseStart - (jumpFalsePatch + 2));
        chunk_.code[jumpFalsePatch]     = (offset >> 8) & 0xFF;
        chunk_.code[jumpFalsePatch + 1] = offset & 0xFF;

        compileStmt(*node.elseBranch);

        // Patch JUMP
        size_t endPos = chunk_.code.size();
        uint16_t endOffset = static_cast<uint16_t>(endPos - (jumpEndPatch + 2));
        chunk_.code[jumpEndPatch]     = (endOffset >> 8) & 0xFF;
        chunk_.code[jumpEndPatch + 1] = endOffset & 0xFF;
    } else {
        // Patch JUMP_IF_FALSE to here
        size_t endPos = chunk_.code.size();
        uint16_t offset = static_cast<uint16_t>(endPos - (jumpFalsePatch + 2));
        chunk_.code[jumpFalsePatch]     = (offset >> 8) & 0xFF;
        chunk_.code[jumpFalsePatch + 1] = offset & 0xFF;
    }
}
```

### Emitting While with LOOP

```cpp
void Compiler::compileWhileStmt(const WhileStmt& node) {
    size_t loopStart = chunk_.code.size();   // ip target to loop back to

    compileExpr(*node.condition);

    chunk_.emitOp(OpCode::JUMP_IF_FALSE, node.line);
    size_t exitPatch = chunk_.code.size();
    chunk_.emitU16(0xFFFF, node.line);

    compileStmt(*node.body);

    // Emit LOOP — offset = distance from end of LOOP instr back to loopStart
    chunk_.emitOp(OpCode::LOOP, node.line);
    size_t loopEnd = chunk_.code.size() + 2;   // +2 for the operand we're about to emit
    uint16_t backOffset = static_cast<uint16_t>(loopEnd - loopStart);
    chunk_.emitU16(backOffset, node.line);

    // Patch exit jump
    size_t endPos = chunk_.code.size();
    uint16_t fwdOffset = static_cast<uint16_t>(endPos - (exitPatch + 2));
    chunk_.code[exitPatch]     = (fwdOffset >> 8) & 0xFF;
    chunk_.code[exitPatch + 1] = fwdOffset & 0xFF;
}
```

---

## Error Handling Strategy

### Lexer Errors
- On unexpected character: throw `LexError("Unexpected character '" + c + "'", line_)`.
- Never silently skip; failing loudly prevents cascading garbage tokens.

### Parser Errors
- On unexpected token: throw `ParseError("Expected X but got Y", token.line)`.
- **Synchronization** (optional Phase 3): catch `ParseError` inside `parseStatement()`, discard tokens until a statement boundary (`NEWLINE` or `EOF`), and resume — enabling multiple errors per run.

### Compiler Errors
- On unknown AST node type (should never happen if parser is correct): throw `CompileError(...)`.
- On constant pool overflow (> 65535 entries): throw `CompileError("Too many constants")`.

### Runtime Errors
- On undefined variable, division by zero, type mismatch: throw `RuntimeError(msg)`.
- `main.cpp` catch block prints: `[Runtime Error] line N: message`.

### `main.cpp` Error Display Pattern
```cpp
try {
    auto tokens = Lexer(source).tokenize();
    auto ast    = Parser(tokens).parse();
    Chunk chunk = Compiler().compile(ast);
    VM(debugMode).execute(chunk);
} catch (const LexError& e) {
    std::cerr << "[Lex Error]     line " << e.line << ": " << e.what() << "\n";
} catch (const ParseError& e) {
    std::cerr << "[Parse Error]   line " << e.line << ": " << e.what() << "\n";
} catch (const CompileError& e) {
    std::cerr << "[Compile Error] " << e.what() << "\n";
} catch (const RuntimeError& e) {
    std::cerr << "[Runtime Error] " << e.what() << "\n";
}
```

---

## Phase-by-Phase Task List

Each task has: **Goal**, **Files Touched**, **Acceptance Criterion**.

---

### PHASE 1 — MVP: End-to-End Arithmetic + Print
*Goal: `print 1 + 2 * 3` → outputs `7`*

---

**Task 1.1 — Project Scaffold**
- Goal: Create all files with empty stubs; confirm CMake builds a clean binary.
- Files: `CMakeLists.txt`, all headers, all `.cpp` with `// TODO` bodies, `main.cpp` with `int main() { return 0; }`.
- ✅ Acceptance: `cmake .. && make` succeeds with zero errors.

---

**Task 1.2 — Implement `token.hpp` + `error.hpp`**
- Goal: Define all token types and the full error hierarchy as shown above.
- Files: `include/token.hpp`, `include/error.hpp`.
- ✅ Acceptance: Header-only, compiles in isolation via `g++ -std=c++17 -c include/token.hpp`.

---

**Task 1.3 — Implement `value.hpp` + `chunk.hpp`**
- Goal: Define `Value` variant, `OpCode` enum, and `Chunk` struct with `emit`, `emitOp`, `emitU16`, `addConstant`, `addName`.
- Files: `include/value.hpp`, `include/chunk.hpp`.
- ✅ Acceptance: Can create a `Chunk`, call `addConstant(Value{42})`, and verify `constants[0]` holds `42`.

---

**Task 1.4 — Implement Lexer (arithmetic + print)**
- Goal: Tokenize numbers, `+`, `-`, `*`, `/`, `(`, `)`, `print`, and newlines.
- Files: `include/lexer.hpp`, `src/lexer.cpp`.
- Key methods: `Lexer(std::string src)`, `std::vector<Token> tokenize()`.
- Internal state: `source_`, `start_`, `current_`, `line_`.
- ✅ Acceptance: `Lexer("print 1 + 2 * 3").tokenize()` produces:
  `[PRINT][NUMBER:1][PLUS][NUMBER:2][STAR][NUMBER:3][NEWLINE][EOF_TOKEN]`

---

**Task 1.5 — Implement AST nodes (expressions only)**
- Goal: Add `NumberLitExpr`, `BinaryExpr`, `UnaryExpr`, `GroupingExpr`, `PrintStmt`, `ExprStmt` to `ast.hpp`.
- Files: `include/ast.hpp`.
- ✅ Acceptance: Header compiles; can manually construct `BinaryExpr(PLUS, NumberLitExpr(1), NumberLitExpr(2))`.

---

**Task 1.6 — Implement Parser (arithmetic + print)**
- Goal: Parse `print expr` and arithmetic expressions with correct precedence.
- Files: `include/parser.hpp`, `src/parser.cpp`.
- Methods: `parseProgram()`, `parseStatement()`, `parsePrintStmt()`, `parseExpression()` → `parseEquality()` → … → `parsePrimary()`.
- ✅ Acceptance: Parsing `"print 1 + 2 * 3\n"` produces a `Program` with one `PrintStmt` wrapping a `BinaryExpr(+, 1, BinaryExpr(*, 2, 3))`.

---

**Task 1.7 — Implement Compiler (expressions + print)**
- Goal: Walk AST, emit `PUSH_CONST`, `ADD/SUB/MUL/DIV/NEG`, `PRINT`, `HALT`.
- Files: `include/compiler.hpp`, `src/compiler.cpp`.
- ✅ Acceptance: Compiling `print 1 + 2 * 3` produces bytecode:
  `PUSH_CONST 0, PUSH_CONST 1, PUSH_CONST 2, MUL, ADD, PRINT, HALT`
  with `constants = [1, 2, 3]`.

---

**Task 1.8 — Implement VM (arithmetic + print)**
- Goal: Fetch-decode-execute loop handling all arithmetic opcodes, `PRINT`, `HALT`.
- Files: `include/vm.hpp`, `src/vm.cpp`.
- ✅ Acceptance: Running the bytecode from Task 1.7 prints `7` to stdout.

---

**Task 1.9 — Wire up `main.cpp` (file runner)**
- Goal: `./cvm scripts/01_hello.cvm` reads the file, runs the full pipeline, prints result.
- Files: `src/main.cpp`, `scripts/01_hello.cvm`.
- `01_hello.cvm` contents: `print 1 + 2 * 3`
- ✅ Acceptance: Running `./cvm scripts/01_hello.cvm` outputs `7`. **🎉 First working script.**

---

### PHASE 2 — Beta: Variables, Booleans, Control Flow, REPL

---

**Task 2.1 — Lexer Extension: keywords + operators**
- Goal: Add `let`, `true`, `false`, `if`, `else`, `while`, `input`, `=`, `==`, `!=`, `<`, `<=`, `>`, `>=`, `!`, `{`, `}`, `;`.
- ✅ Acceptance: `Lexer("let x = true\n").tokenize()` produces `[LET][IDENT:x][EQ][TRUE][NEWLINE][EOF]`.

---

**Task 2.2 — AST Extension: all node types**
- Goal: Add `BoolLitExpr`, `IdentExpr`, `AssignExpr`, `LetStmt`, `InputStmt`, `BlockStmt`, `IfStmt`, `WhileStmt` to `ast.hpp`.
- ✅ Acceptance: All nodes constructible without compile errors.

---

**Task 2.3 — Parser Extension: let, assignment, booleans**
- Goal: Parse `let x = expr`, `x = expr`, `true`, `false`, identifiers.
- ✅ Acceptance: `let x = 10 + 2` produces `LetStmt("x", BinaryExpr(+, 10, 2))`.

---

**Task 2.4 — Parser Extension: comparison + equality**
- Goal: Parse `==`, `!=`, `<`, `<=`, `>`, `>=` at correct precedence level.
- ✅ Acceptance: `1 + 2 == 3` parses as `EQ(ADD(1,2), 3)` not `ADD(1, EQ(2,3))`.

---

**Task 2.5 — Parser Extension: if/else, while, blocks**
- Goal: Parse `if cond { ... }`, `if cond { ... } else { ... }`, `while cond { ... }`.
- ✅ Acceptance: Parsing `if x < 10 { print x }` produces an `IfStmt` with correct child nodes.

---

**Task 2.6 — Parser Extension: input**
- Goal: Parse `input varname` → `InputStmt("varname")`.
- ✅ Acceptance: `input n` parses to `InputStmt("n")`.

---

**Task 2.7 — Compiler Extension: variables + booleans**
- Goal: Emit `PUSH_TRUE/FALSE`, `DEFINE_GLOBAL`, `GET_GLOBAL`, `SET_GLOBAL`, `NOT`, `EQ/NEQ/LT/etc`, `POP`.
- ✅ Acceptance: `let x = 5` compiles to `PUSH_CONST 0 [5], DEFINE_GLOBAL 0 ["x"], HALT`.

---

**Task 2.8 — Compiler Extension: if/else with backpatching**
- Goal: Implement backpatching as shown in the architecture section.
- ✅ Acceptance: `if true { print 1 } else { print 2 }` compiles to valid bytecode that the VM executes printing `1`.

---

**Task 2.9 — Compiler Extension: while with LOOP**
- Goal: Emit `LOOP` opcode with correct backward offset.
- ✅ Acceptance: `while x < 3 { print x\n x = x + 1 }` compiles and VM prints `0 1 2`.

---

**Task 2.10 — Compiler Extension: input**
- Goal: Emit `INPUT` opcode + `SET_GLOBAL`.
- ✅ Acceptance: `input n\nprint n` reads a line from stdin and prints the integer.

---

**Task 2.11 — VM Extension: all new opcodes**
- Goal: Add handlers for `PUSH_TRUE/FALSE`, `NOT`, `EQ/NEQ/LT/LT_EQ/GT/GT_EQ`, `DEFINE/GET/SET_GLOBAL`, `JUMP/JUMP_IF_FALSE/LOOP`, `INPUT`.
- ✅ Acceptance: All scripts in `scripts/` execute correctly.

---

**Task 2.12 — Add REPL**
- Goal: `./cvm` with no arguments enters a REPL: prints `cvm> `, reads a line, executes it, prints result, loops.
- ✅ Acceptance: `cvm> print 2 + 2` prints `4`. `cvm> exit` quits cleanly.
- Note: REPL should preserve globals between lines — pass a persistent `VM` instance.

---

**Task 2.13 — Sample Scripts**
- Write `scripts/02_arithmetic.cvm` through `scripts/07_input.cvm`.
- ✅ Acceptance: Each script runs and produces the expected output as documented in a comment at the top of each file.

---

### PHASE 3 — Polish: Errors, Debug Mode, Hardening

---

**Task 3.1 — Source Locations in Errors**
- Goal: `RuntimeError` in VM carries the source line. Requires threading line info through the `Chunk::lines[]` array.
- In VM: `int currentLine = chunk.lines[ip - chunk.code.data() - 1]` after each fetch.
- ✅ Acceptance: `print 10 / 0` prints `[Runtime Error] line 1: Division by zero`.

---

**Task 3.2 — `--dump-tokens` flag**
- Goal: `./cvm --dump-tokens script.cvm` prints the token stream and exits.
- ✅ Acceptance: Correct token list with types and line numbers is printed.

---

**Task 3.3 — `--dump-ast` flag**
- Goal: `./cvm --dump-ast script.cvm` prints a tree representation of the AST.
- Implementation: Add a `void printAST(const Program&, int indent=0)` utility in `src/debug.cpp`.
- ✅ Acceptance: Output clearly shows tree structure with node types and values.

---

**Task 3.4 — `--dump-bytecode` flag**
- Goal: `./cvm --dump-bytecode script.cvm` prints a human-readable disassembly.
- Format: `0000  PUSH_CONST   0    ; 42    (line 1)`
- Implementation: `disassemble(const Chunk&)` utility.
- ✅ Acceptance: Output matches expected bytecode from compiler tests.

---

**Task 3.5 — Edge Case Hardening**
- Goal: Explicitly handle and test:
  - `print true + 1` → `[Runtime Error] line 1: '+' requires integers`
  - `let x = 10 / 0` → `[Runtime Error] line 1: Division by zero`
  - `print y` (y undefined) → `[Runtime Error] line 1: Undefined variable: y`
  - `input x` followed by non-integer → `[Runtime Error] line 2: input must be an integer`
  - Stack overflow (255+ items) → `[Runtime Error]: Stack overflow`
- ✅ Acceptance: Each case tested with a `.cvm` file; error message is clear and exits with code 1.

---

**Task 3.6 — Fibonacci Script**
- Goal: `scripts/08_fibonacci.cvm` computes and prints the first N Fibonacci numbers using `while` and `let`.
- ✅ Acceptance: `./cvm scripts/08_fibonacci.cvm` correctly prints the sequence.

```
let a = 0
let b = 1
let i = 0
while i < 10 {
    print a
    let temp = b
    b = a + b
    a = temp
    i = i + 1
}
```

---

### PHASE 4 — Scale (Stretch Goals)

| Task | Description | Key Challenge |
|---|---|---|
| 4.1 Functions | `fn add(a, b) { ... }` + `call` opcode | Call frames, local variable slots |
| 4.2 String type | `"hello"` literals | Extend `Value` to `std::variant<int64_t, bool, std::string>` |
| 4.3 Computed Goto | `goto *dispatch_table[opcode]` | GCC/Clang extension; significant speedup |
| 4.4 Constant Folding | Fold `1 + 2` at compile time | Walk AST before emitting, replace with literal |
| 4.5 Local Variables | Stack-slot locals (no hash map) | Change `DEFINE_GLOBAL` → `DEFINE_LOCAL` with frame offset |

---

## Assumptions & Potential Pitfalls

| # | Pitfall | Mitigation in This Plan |
|---|---|---|
| 1 | **NEWLINE sensitivity**: The grammar uses newlines as statement terminators (like Python), making the lexer context-sensitive. | The lexer emits `NEWLINE` tokens but the parser accepts multiple consecutive newlines (treats them as blank lines). Blocks `{}` do not require newlines. |
| 2 | **`let` vs assignment**: `let x = 5` is declaration; `x = 5` is reassignment. Assigning to an undeclared variable should error. | VM's `SET_GLOBAL` checks if the key already exists; if not, throws `RuntimeError("Undefined variable")`. |
| 3 | **`if` condition leaves value on stack**: `JUMP_IF_FALSE` must pop the condition, or the stack leaks. | `JUMP_IF_FALSE` always pops the top of stack before checking — baked into the opcode semantics. |
| 4 | **LOOP offset off-by-one**: Getting the backward jump offset wrong causes infinite loops or skipped instructions. | Use the formula `loopEnd - loopStart` where `loopEnd` is computed *after* emitting the operand bytes. Test with a counter-controlled while loop. |
| 5 | **REPL state**: Each REPL line is a fresh compile, but globals must persist. | The `VM` instance lives outside the loop; only the `Chunk` is reconstructed per line. The `Compiler` gets a fresh `Chunk` each time. |
| 6 | **Integer overflow**: `int64_t` is used for all integers, giving massive range; no overflow concern for typical scripts. | If needed in Phase 4, add range checks. |
| 7 | **Empty `else`**: Parser must not try to parse an `else` block if the next token is not `else`. | `if (peek().type == TokenType::ELSE)` guard before parsing else branch. |
| 8 | **`input` pushed onto wrong variable**: `INPUT` pushes to stack, then `SET_GLOBAL` pops it — but the variable must already exist. | `InputStmt` compiles to: emit `INPUT`, then emit `SET_GLOBAL idx`. Requires that the variable was already declared with `let` prior. |

---

## Implementation Order Summary (Quick Reference)

```
Week 1 (MVP):
  1.1 Scaffold → 1.2 Tokens → 1.3 Values+Chunk → 1.4 Lexer
  → 1.5 AST → 1.6 Parser → 1.7 Compiler → 1.8 VM → 1.9 main.cpp
  MILESTONE: ./cvm scripts/01_hello.cvm prints 7

Week 2 (Beta):
  2.1-2.6 Lexer+AST+Parser extensions
  → 2.7-2.10 Compiler extensions
  → 2.11 VM extensions
  → 2.12 REPL
  → 2.13 Sample scripts
  MILESTONE: Fibonacci script runs; REPL works

Week 3 (Polish):
  3.1 Line-accurate errors → 3.2-3.4 Debug flags
  → 3.5 Edge cases → 3.6 Fibonacci
  MILESTONE: All edge cases handled; full debug output available
```

---

## Quick-Reference: Building & Running

```bash
# Build
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
cd ..

# Run a script
./build/cvm scripts/01_hello.cvm

# Start the REPL
./build/cvm

# Debug flags (Phase 3)
./build/cvm --dump-tokens     scripts/05_if_else.cvm
./build/cvm --dump-ast        scripts/05_if_else.cvm
./build/cvm --dump-bytecode   scripts/05_if_else.cvm
```

---

## Sample Script Reference

**`scripts/01_hello.cvm`**
```
print 1 + 2 * 3
```
Expected output: `7`

**`scripts/05_if_else.cvm`**
```
let x = 15
if x < 10 {
    print 0
} else {
    print 1
}
```
Expected output: `1`

**`scripts/06_while_loop.cvm`**
```
let i = 0
while i < 5 {
    print i
    i = i + 1
}
```
Expected output: `0 1 2 3 4` (each on its own line)

**`scripts/07_input.cvm`**
```
let n = 0
input n
print n
```
Expected: reads integer from stdin, prints it back.

---

*End of CVM++ Architectural Blueprint. Every design decision is locked. Open your editor and start with Task 1.1.*
