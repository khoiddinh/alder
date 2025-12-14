#include <unordered_map>
#include "../token/tokens.h"
#include "../ast/ast.h"

namespace alder::op_precedence {

using namespace alder::token;
using namespace alder::ast;

// ** Operator Lookup Tables
std::unordered_map<OperatorKind, UnaryOp> PREFIX_OPERATORS = {
    {OperatorKind::Minus, UnaryOp::Neg},
    {OperatorKind::Not, UnaryOp::Not}
};

std::unordered_map<OperatorKind, BinaryOp> INFIX_OPERATORS = {
        // Logical
        {OperatorKind::And, BinaryOp::And},
        {OperatorKind::Or, BinaryOp::Or},
        
        // Comparison
        {OperatorKind::Equals, BinaryOp::Eq},
        {OperatorKind::NotEquals, BinaryOp::NotEq},
        {OperatorKind::LessCompare, BinaryOp::Lt},
        {OperatorKind::LessEqCompare, BinaryOp::Lte},
        {OperatorKind::GreaterCompare, BinaryOp::Gt},
        {OperatorKind::GreaterEqCompare, BinaryOp::Gte},
        
        // Arithmetic
        {OperatorKind::Plus, BinaryOp::Add},
        {OperatorKind::Minus, BinaryOp::Sub},  // binary minus (subtraction)
        {OperatorKind::Star, BinaryOp::Mul}, 
        {OperatorKind::Slash, BinaryOp::Div},
        
        // Bitwise shifts
        {OperatorKind::LeftBitShift, BinaryOp::Shl},
        {OperatorKind::RightBitShift, BinaryOp::Shr},
        {OperatorKind::LogicRightBitShift, BinaryOp::LogicShr},
};

// ** Binding Power Lookup Table
std::unordered_map<OperatorKind, BindingPower> BINDING_POWERS = {
        // Prefix + Infix
        {OperatorKind::Minus, {.leftBP = 60, .rightBP = 60, .prefixBP = 80}},
        
        // Prefix only
        {OperatorKind::Not, {.prefixBP = 80}},
        
        // Infix - Arithmetic
        {OperatorKind::Plus, {.leftBP = 60, .rightBP = 60}},
        {OperatorKind::Star, {.leftBP = 70, .rightBP = 70}},
        {OperatorKind::Slash, {.leftBP = 70, .rightBP = 70}},
        
        // Infix - Comparison (non-associative: right < left)
        {OperatorKind::LessCompare, {.leftBP = 50, .rightBP = 49}},
        {OperatorKind::LessEqCompare, {.leftBP = 50, .rightBP = 49}},
        {OperatorKind::GreaterCompare, {.leftBP = 50, .rightBP = 49}},
        {OperatorKind::GreaterEqCompare, {.leftBP = 50, .rightBP = 49}},
        
        // Infix - Equality (non-associative)
        {OperatorKind::Equals, {.leftBP = 40, .rightBP = 39}},
        {OperatorKind::NotEquals, {.leftBP = 40, .rightBP = 39}},
        
        // Infix - Logical
        {OperatorKind::And, {.leftBP = 30, .rightBP = 30}},
        {OperatorKind::Or, {.leftBP = 20, .rightBP = 20}},
        
        // Infix - Bitwise
        {OperatorKind::LeftBitShift, {.leftBP = 55, .rightBP = 55}},
        {OperatorKind::RightBitShift, {.leftBP = 55, .rightBP = 55}},
        {OperatorKind::LogicRightBitShift, {.leftBP = 55, .rightBP = 55}},

    };


// ** Binding Power Functions
struct BindingPower {
        int leftBP = -1;
        int rightBP = -1;
        int prefixBP = -1;
};
// *Special token binding powers (not operators)
static constexpr int ASSIGN_BP = 10;
static constexpr int CALL_BP = 90;
static constexpr int INDEX_BP = 90;

int op_precedence::prefixBindingPower(const Token& token) {
    switch (token.type) {
        case TokenType::Operator: {
            OperatorKind opKind = token.op.value();
            auto it = BINDING_POWERS.find(opKind);
            // if found and has a prefix BP
            if (it != BINDING_POWERS.end() && it->second.prefixBP >= 0) {
                return it->second.prefixBP;
            }
            throw std::runtime_error("Operator has no prefix binding power");
        }
        default:
            throw std::runtime_error("There are no non-operators with prefix binding power");
    }
    
}

int op_precedence::leftBindingPower(const Token& token) {
    switch (token.type) {
        case TokenType::Assign:   
            return ASSIGN_BP;
        case TokenType::LParen:   
            return CALL_BP;
        case TokenType::LBracket: 
            return INDEX_BP;
        case TokenType::Operator: {
            OperatorKind opKind = token.op.value();
            auto it = BINDING_POWERS.find(opKind);
            // if found and has a prefix BP
            if (it != BINDING_POWERS.end() && it->second.leftBP >= 0) {
                return it->second.leftBP;
            }
            throw std::runtime_error("Operator has no left binding power");
        }
        default:
            return 0; // All terminators: Newline, Eof, Comma, RParen, etc.
    }

}

int op_precedence::rightBindingPower(const Token& token) {
    switch (token.type) {
        case TokenType::Operator: {
            OperatorKind opKind = token.op.value();
            auto it = BINDING_POWERS.find(opKind);
            // if found and has a prefix BP
            if (it != BINDING_POWERS.end() && it->second.rightBP >= 0) {
                return it->second.rightBP;
            }
            throw std::runtime_error("Operator has no right binding power");
        }
        default:
            throw std::runtime_error("Passed non-operator into RBP");
    }
    
}

int op_precedence::getAssignBindingPower() {
	return ASSIGN_BP;
}

// ** Operator Query Functions **
bool isPrefixOp(const OperatorKind opKind) {
	auto it = PREFIX_OPERATORS.find(opKind);
	return it != PREFIX_OPERATORS.end();
}

bool isInfixOp(const OperatorKind opKind) {
	auto it = INFIX_OPERATORS.find(opKind);
	return it != INFIX_OPERATORS.end();
}

ast::UnaryOp toUnaryOp(const token::OperatorKind opKind) {
	auto it = PREFIX_OPERATORS.find(opKind);
	if (it != PREFIX_OPERATORS.end()) {
		return it->second;
	} else {
		throw std::runtime_error("Unary op not found");
	}
}
ast::BinaryOp toBinaryOp(const token::OperatorKind opKind) {
	auto it = INFIX_OPERATORS.find(opKind);
	if (it != INFIX_OPERATORS.end()) {
		return it->second;
	} else {
		throw std::runtime_error("Binary op not found");
	}
}

}