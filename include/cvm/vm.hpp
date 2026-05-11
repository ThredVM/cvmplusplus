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
    uint16_t readU16(const uint8_t*& ip) const;

    // Pretty-print stack state (debug only)
    void dumpStack() const;
};
