#pragma once

#include <unordered_map>
#include "../token/tokens.h"
#include "../ast/ast.h"

namespace alder::op_precedence {

namespace token = alder::token;
namespace ast = alder::ast;

struct BindingPower {
    int leftBP = -1;
    int rightBP = -1;
    int prefixBP = -1;
};

int prefixBindingPower(const token::Token& token);
int leftBindingPower(const token::Token& token);
int rightBindingPower(const token::Token& token);
int getAssignBindingPower();

bool isPrefixOp(const token::OperatorKind opKind);
bool isInfixOp(const token::OperatorKind opKind);

ast::UnaryOp toUnaryOp(const token::OperatorKind opKind);
ast::BinaryOp toBinaryOp(const token::OperatorKind opKind);

} // namespace alder::op_precedence
