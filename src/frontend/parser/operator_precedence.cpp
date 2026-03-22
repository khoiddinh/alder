#include "operator_precedence.h"
#include <stdexcept>

namespace alder::op_precedence {

using namespace alder::token;
using namespace alder::ast;

// ** Special binding powers
static constexpr int ASSIGN_BP = 10;
static constexpr int CALL_BP   = 90;
static constexpr int INDEX_BP  = 90;

// ** Operator lookup tables
static const std::unordered_map<OperatorKind, UnaryOp> PREFIX_OPERATORS = {
    {OperatorKind::Minus, UnaryOp::Neg},
    {OperatorKind::Not,   UnaryOp::Not},
};

static const std::unordered_map<OperatorKind, BinaryOp> INFIX_OPERATORS = {
    // Logical
    {OperatorKind::And,                 BinaryOp::And},
    {OperatorKind::Or,                  BinaryOp::Or},
    // Comparison
    {OperatorKind::Equals,              BinaryOp::Eq},
    {OperatorKind::NotEquals,           BinaryOp::NotEq},
    {OperatorKind::LessCompare,         BinaryOp::Lt},
    {OperatorKind::LessEqCompare,       BinaryOp::Lte},
    {OperatorKind::GreaterCompare,      BinaryOp::Gt},
    {OperatorKind::GreaterEqCompare,    BinaryOp::Gte},
    // Arithmetic
    {OperatorKind::Plus,                BinaryOp::Add},
    {OperatorKind::Minus,               BinaryOp::Sub},
    {OperatorKind::Star,                BinaryOp::Mul},
    {OperatorKind::Slash,               BinaryOp::Div},
    // Bitwise shifts
    {OperatorKind::LeftBitShift,        BinaryOp::Shl},
    {OperatorKind::RightBitShift,       BinaryOp::Shr},
    {OperatorKind::LogicRightBitShift,  BinaryOp::LogicShr},
};

static const std::unordered_map<OperatorKind, BindingPower> BINDING_POWERS = {
    // Prefix + infix
    {OperatorKind::Minus,               {.leftBP = 60, .rightBP = 60, .prefixBP = 80}},
    // Prefix only
    {OperatorKind::Not,                 {.prefixBP = 80}},
    // Arithmetic
    {OperatorKind::Plus,                {.leftBP = 60, .rightBP = 60}},
    {OperatorKind::Star,                {.leftBP = 70, .rightBP = 70}},
    {OperatorKind::Slash,               {.leftBP = 70, .rightBP = 70}},
    // Comparison (non-associative: rightBP < leftBP)
    {OperatorKind::LessCompare,         {.leftBP = 50, .rightBP = 49}},
    {OperatorKind::LessEqCompare,       {.leftBP = 50, .rightBP = 49}},
    {OperatorKind::GreaterCompare,      {.leftBP = 50, .rightBP = 49}},
    {OperatorKind::GreaterEqCompare,    {.leftBP = 50, .rightBP = 49}},
    // Equality (non-associative)
    {OperatorKind::Equals,              {.leftBP = 40, .rightBP = 39}},
    {OperatorKind::NotEquals,           {.leftBP = 40, .rightBP = 39}},
    // Logical
    {OperatorKind::And,                 {.leftBP = 30, .rightBP = 30}},
    {OperatorKind::Or,                  {.leftBP = 20, .rightBP = 20}},
    // Bitwise
    {OperatorKind::LeftBitShift,        {.leftBP = 55, .rightBP = 55}},
    {OperatorKind::RightBitShift,       {.leftBP = 55, .rightBP = 55}},
    {OperatorKind::LogicRightBitShift,  {.leftBP = 55, .rightBP = 55}},
};

// ** Binding power functions

int prefixBindingPower(const Token& token) {
    if (token.type != TokenType::Operator)
        throw std::runtime_error("Non-operator has no prefix binding power");
    OperatorKind opKind = token.op.value();
    auto it = BINDING_POWERS.find(opKind);
    if (it != BINDING_POWERS.end() && it->second.prefixBP >= 0)
        return it->second.prefixBP;
    throw std::runtime_error("Operator has no prefix binding power");
}

int leftBindingPower(const Token& token) {
    switch (token.type) {
        case TokenType::Assign:   return ASSIGN_BP;
        case TokenType::LParen:   return CALL_BP;
        case TokenType::LBracket: return INDEX_BP;
        case TokenType::Operator: {
            OperatorKind opKind = token.op.value();
            auto it = BINDING_POWERS.find(opKind);
            if (it != BINDING_POWERS.end() && it->second.leftBP >= 0)
                return it->second.leftBP;
            return 0; // prefix-only operators have no left binding power
        }
        default:
            return 0; // terminators: Newline, Eof, Comma, RParen, Colon, etc.
    }
}

int rightBindingPower(const Token& token) {
    if (token.type != TokenType::Operator)
        throw std::runtime_error("Non-operator has no right binding power");
    OperatorKind opKind = token.op.value();
    auto it = BINDING_POWERS.find(opKind);
    if (it != BINDING_POWERS.end() && it->second.rightBP >= 0)
        return it->second.rightBP;
    throw std::runtime_error("Operator has no right binding power");
}

int getAssignBindingPower() {
    return ASSIGN_BP;
}

// ** Operator query functions

bool isPrefixOp(const OperatorKind opKind) {
    return PREFIX_OPERATORS.count(opKind) > 0;
}

bool isInfixOp(const OperatorKind opKind) {
    return INFIX_OPERATORS.count(opKind) > 0;
}

UnaryOp toUnaryOp(const OperatorKind opKind) {
    auto it = PREFIX_OPERATORS.find(opKind);
    if (it != PREFIX_OPERATORS.end()) return it->second;
    throw std::runtime_error("Not a unary operator");
}

BinaryOp toBinaryOp(const OperatorKind opKind) {
    auto it = INFIX_OPERATORS.find(opKind);
    if (it != INFIX_OPERATORS.end()) return it->second;
    throw std::runtime_error("Not a binary operator");
}

} // namespace alder::op_precedence
