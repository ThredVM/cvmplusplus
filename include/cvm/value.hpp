#pragma once
#include <variant>
#include <string>
#include <stdexcept>

using Value = std::variant<int64_t, bool, std::string>;

// Helpers
inline bool isInt(const Value& v)  { return std::holds_alternative<int64_t>(v); }
inline bool isBool(const Value& v) { return std::holds_alternative<bool>(v); }
inline bool isString(const Value& v) { return std::holds_alternative<std::string>(v); }

inline int64_t asInt(const Value& v)  { return std::get<int64_t>(v); }
inline bool    asBool(const Value& v) { return std::get<bool>(v); }
inline const std::string& asString(const Value& v) { return std::get<std::string>(v); }

inline std::string valueToString(const Value& v) {
    if (isBool(v)) return asBool(v) ? "true" : "false";
    if (isString(v)) return asString(v);
    return std::to_string(asInt(v));
}

inline bool isTruthy(const Value& v) {
    if (isBool(v)) return asBool(v);
    if (isString(v)) return !asString(v).empty();
    return asInt(v) != 0;
}
