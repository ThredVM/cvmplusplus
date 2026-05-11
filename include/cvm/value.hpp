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

inline std::string valueToString(const Value& v) {
    if (isBool(v)) return asBool(v) ? "true" : "false";
    return std::to_string(asInt(v));
}

inline bool isTruthy(const Value& v) {
    if (isBool(v)) return asBool(v);
    return asInt(v) != 0;
}
