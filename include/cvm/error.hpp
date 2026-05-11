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
