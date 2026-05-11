#include "cvm/vm.hpp"
#include "cvm/error.hpp"
#include <iostream>

VM::VM(bool debugMode) : debug_(debugMode) {
    stack_.reserve(MAX_STACK);
}

void VM::execute(const Chunk& chunk) {
    auto mainFunc = std::make_shared<Function>();
    mainFunc->name = "main";
    mainFunc->arity = 0;
    mainFunc->startAddress = 0;
    
    frames_.push_back({mainFunc, chunk.code.data(), 0});

    while (!frames_.empty()) {
        CallFrame& frame = frames_.back();
        const uint8_t*& ip = frame.ip;

        if (debug_) dumpStack();

        int line = chunk.lines[ip - chunk.code.data()];
        OpCode op = static_cast<OpCode>(*ip++);

        switch (op) {
            case OpCode::PUSH_CONST: {
                uint16_t idx = readU16(ip);
                push(chunk.constants[idx]);
                break;
            }
            case OpCode::PUSH_TRUE:  push(Value{true});  break;
            case OpCode::PUSH_FALSE: push(Value{false}); break;

            case OpCode::ADD: {
                Value b = pop(), a = pop();
                if (isInt(a) && isInt(b)) {
                    push(Value{asInt(a) + asInt(b)});
                } else if (isString(a) && isString(b)) {
                    push(Value{asString(a) + asString(b)});
                } else if (isString(a) && isInt(b)) {
                    push(Value{asString(a) + std::to_string(asInt(b))});
                } else if (isInt(a) && isString(b)) {
                    push(Value{std::to_string(asInt(a)) + asString(b)});
                } else {
                    throw RuntimeError("'+' requires integers or strings", line);
                }
                break;
            }
            case OpCode::SUB: {
                Value b = pop(), a = pop();
                if (!isInt(a) || !isInt(b)) throw RuntimeError("'-' requires integers", line);
                push(Value{asInt(a) - asInt(b)});
                break;
            }
            case OpCode::MUL: {
                Value b = pop(), a = pop();
                if (!isInt(a) || !isInt(b)) throw RuntimeError("'*' requires integers", line);
                push(Value{asInt(a) * asInt(b)});
                break;
            }
            case OpCode::DIV: {
                Value b = pop(), a = pop();
                if (!isInt(a) || !isInt(b)) throw RuntimeError("'/' requires integers", line);
                if (asInt(b) == 0) throw RuntimeError("Division by zero", line);
                push(Value{asInt(a) / asInt(b)});
                break;
            }
            case OpCode::NEG: {
                Value a = pop();
                if (!isInt(a)) throw RuntimeError("'-' unary requires integer", line);
                push(Value{-asInt(a)});
                break;
            }

            case OpCode::EQ: {
                Value b = pop(), a = pop();
                push(Value{a == b});
                break;
            }
            case OpCode::NEQ: {
                Value b = pop(), a = pop();
                push(Value{a != b});
                break;
            }
            case OpCode::LT: {
                Value b = pop(), a = pop();
                if (!isInt(a) || !isInt(b)) throw RuntimeError("'<' requires integers", line);
                push(Value{asInt(a) < asInt(b)});
                break;
            }
            case OpCode::LT_EQ: {
                Value b = pop(), a = pop();
                if (!isInt(a) || !isInt(b)) throw RuntimeError("'<=' requires integers", line);
                push(Value{asInt(a) <= asInt(b)});
                break;
            }
            case OpCode::GT: {
                Value b = pop(), a = pop();
                if (!isInt(a) || !isInt(b)) throw RuntimeError("'>' requires integers", line);
                push(Value{asInt(a) > asInt(b)});
                break;
            }
            case OpCode::GT_EQ: {
                Value b = pop(), a = pop();
                if (!isInt(a) || !isInt(b)) throw RuntimeError("'>=' requires integers", line);
                push(Value{asInt(a) >= asInt(b)});
                break;
            }
            case OpCode::NOT: {
                push(Value{!isTruthy(pop())});
                break;
            }

            case OpCode::DEFINE_GLOBAL: {
                uint16_t idx = readU16(ip);
                globals_[chunk.names[idx]] = pop();
                break;
            }
            case OpCode::GET_GLOBAL: {
                uint16_t idx = readU16(ip);
                auto it = globals_.find(chunk.names[idx]);
                if (it == globals_.end()) throw RuntimeError("Undefined variable: " + chunk.names[idx], line);
                push(it->second);
                break;
            }
            case OpCode::SET_GLOBAL: {
                uint16_t idx = readU16(ip);
                const std::string& name = chunk.names[idx];
                if (globals_.find(name) == globals_.end()) throw RuntimeError("Undefined variable: " + name, line);
                globals_[name] = peek();
                break;
            }

            case OpCode::GET_LOCAL: {
                uint16_t slot = readU16(ip);
                if (frame.slots + slot >= stack_.size()) throw RuntimeError("Invalid local slot access", line);
                push(stack_[frame.slots + slot]);
                break;
            }
            case OpCode::SET_LOCAL: {
                uint16_t slot = readU16(ip);
                if (frame.slots + slot >= stack_.size()) throw RuntimeError("Invalid local slot access", line);
                stack_[frame.slots + slot] = peek();
                break;
            }

            case OpCode::CALL: {
                uint8_t argc = *ip++;
                Value calleeValue = peek(argc);
                if (!isFunction(calleeValue)) {
                    throw RuntimeError("Can only call functions", line);
                }
                auto function = asFunction(calleeValue);
                if (argc != function->arity) {
                    throw RuntimeError("Expected " + std::to_string(function->arity) + " arguments but got " + std::to_string(argc), line);
                }
                if (frames_.size() >= MAX_FRAMES) {
                    throw RuntimeError("Stack overflow (too many call frames)", line);
                }
                frames_.push_back({function, chunk.code.data() + function->startAddress, stack_.size() - argc});
                break;
            }
            case OpCode::RET: {
                Value result = pop();
                size_t slots = frames_.back().slots;
                frames_.pop_back();
                
                if (frames_.empty()) return;

                while (stack_.size() > slots - 1) {
                    stack_.pop_back();
                }
                
                push(result);
                break;
            }

            case OpCode::POP: pop(); break;

            case OpCode::JUMP: {
                uint16_t offset = readU16(ip);
                ip += offset;
                break;
            }
            case OpCode::JUMP_IF_FALSE: {
                uint16_t offset = readU16(ip);
                if (!isTruthy(pop())) ip += offset;
                break;
            }
            case OpCode::LOOP: {
                uint16_t offset = readU16(ip);
                ip -= offset;
                break;
            }

            case OpCode::PRINT: {
                std::cout << valueToString(pop()) << "\n";
                break;
            }
            case OpCode::INPUT: {
                std::string line_in;
                if (!std::getline(std::cin, line_in)) return;
                try { push(Value{std::stoll(line_in)}); }
                catch (...) { throw RuntimeError("input must be an integer", line); }
                break;
            }
            case OpCode::HALT: return;

            default: throw RuntimeError("Unknown opcode", line);
        }
    }
}

void VM::push(Value v) {
    if (stack_.size() >= MAX_STACK) throw RuntimeError("Stack overflow");
    stack_.push_back(std::move(v));
}

Value VM::pop() {
    if (stack_.empty()) throw RuntimeError("Stack underflow");
    Value v = std::move(stack_.back());
    stack_.pop_back();
    return v;
}

const Value& VM::peek(int distance) const {
    if ((int)stack_.size() <= distance) throw RuntimeError("Stack peek out of bounds");
    return stack_[stack_.size() - 1 - distance];
}

uint16_t VM::readU16(const uint8_t*& ip) const {
    uint16_t hi = *ip++;
    uint16_t lo = *ip++;
    return (hi << 8) | lo;
}

void VM::dumpStack() const {
    std::cout << "          ";
    for (const auto& val : stack_) {
        std::cout << "[ " << valueToString(val) << " ]";
    }
    std::cout << "\n";
}
