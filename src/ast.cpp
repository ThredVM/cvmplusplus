#include "cvm/ast.hpp"

ExprPtr AssignExpr::fold() {
    auto folded = value->fold();
    if (folded) value = std::move(folded);
    return nullptr;
}

ExprPtr BinaryExpr::fold() {
    auto f_left = left->fold();
    if (f_left) left = std::move(f_left);
    
    auto f_right = right->fold();
    if (f_right) right = std::move(f_right);

    auto l_num = dynamic_cast<NumberLitExpr*>(left.get());
    auto r_num = dynamic_cast<NumberLitExpr*>(right.get());

    if (l_num && r_num) {
        switch (op.type) {
            case TokenType::PLUS:  return std::make_unique<NumberLitExpr>(l_num->value + r_num->value, line);
            case TokenType::MINUS: return std::make_unique<NumberLitExpr>(l_num->value - r_num->value, line);
            case TokenType::STAR:  return std::make_unique<NumberLitExpr>(l_num->value * r_num->value, line);
            case TokenType::SLASH: 
                if (r_num->value != 0)
                    return std::make_unique<NumberLitExpr>(l_num->value / r_num->value, line);
                break;
            default: break;
        }
    }

    auto l_str = dynamic_cast<StringLitExpr*>(left.get());
    auto r_str = dynamic_cast<StringLitExpr*>(right.get());

    if (l_str && r_str && op.type == TokenType::PLUS) {
        return std::make_unique<StringLitExpr>(l_str->value + r_str->value, line);
    }

    return nullptr;
}

ExprPtr UnaryExpr::fold() {
    auto folded = right->fold();
    if (folded) right = std::move(folded);

    auto r_num = dynamic_cast<NumberLitExpr*>(right.get());
    if (r_num) {
        if (op.type == TokenType::MINUS) {
            return std::make_unique<NumberLitExpr>(-r_num->value, line);
        }
    }

    auto r_bool = dynamic_cast<BoolLitExpr*>(right.get());
    if (r_bool) {
        if (op.type == TokenType::BANG) {
            return std::make_unique<BoolLitExpr>(!r_bool->value, line);
        }
    }

    return nullptr;
}

ExprPtr GroupingExpr::fold() {
    auto folded = inner->fold();
    if (folded) inner = std::move(folded);
    
    // We can also unwrap grouping expressions during folding if we want
    if (auto num = dynamic_cast<NumberLitExpr*>(inner.get())) 
        return std::make_unique<NumberLitExpr>(num->value, line);
    if (auto b = dynamic_cast<BoolLitExpr*>(inner.get()))
        return std::make_unique<BoolLitExpr>(b->value, line);
    if (auto s = dynamic_cast<StringLitExpr*>(inner.get()))
        return std::make_unique<StringLitExpr>(s->value, line);

    return nullptr;
}
