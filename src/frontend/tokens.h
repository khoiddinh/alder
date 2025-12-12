#pragma once

#include <string>

namespace alder {

enum class TokenType {
    // special
    Eof,
    Invalid,
    Newline,
    Indent,
    Detent,

    // identifiers & literals
    Identifier,
    IntLit,
    FloatLit,
    StringLit,
    BoolLit,
    // TODO: add char lit 

    // keywords
    
    KwDef,
    KwFinal,
    KwReturn,
    KwIf,
    KwElif,
    KwElse,
    KwFor,
    KwWhile,
    KwIn,

    // punctuation
    LParen, RParen,
    LBracket, RBracket,
    Comma, Colon, 

    // operators
    Operator

};

struct Token {
    TokenType type;
    std::string lexeme; // chars from input
    std::optional<OperatorKind> op;
};

enum class OperatorKind {
    // Word operators
    Not,        // not
    And,        // and
    Or,         // or

    // symbol operators
    LogicRightBitShift, // >>> (preserves MSB)
    Arrow,              // ->
    Equals,             // ==
    NotEquals,          // !=
    LessEqCompare,      // <=
    GreaterEqCompare,   // >=
    LeftBitShift,       // <<
    RightBitShift,      // >>
    
    
    Plus,               // +
    Minus,              // -
    Star,               // *
    Slash,              // /
    Assign,             // =
    LessCompare,        // <
    GreaterCompare,     // >
};

}