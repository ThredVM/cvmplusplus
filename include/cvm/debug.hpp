#pragma once
#include "token.hpp"
#include "ast.hpp"
#include "chunk.hpp"
#include <vector>

void dumpTokens(const std::vector<Token>& tokens);
void dumpAST(const Program& program);
void disassemble(const Chunk& chunk, const std::string& name);
int disassembleInstruction(const Chunk& chunk, int offset);
