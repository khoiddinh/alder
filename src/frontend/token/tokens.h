#pragma once

#include <string>

namespace alder::token {

enum class TokenType {
    // special
    Eof,
    Invalid,
    Newline,
    Indent,
    Dedent,

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
    Comma, Colon, Arrow,

    // operators
    Operator, 
    
    // Special operator
    Assign, // =


};

struct Token {
    TokenType type;
    std::string lexeme; // chars from input
    int line; // line number it appears on
    std::optional<OperatorKind> op;
};

enum class OperatorKind {
    // Word operators
    Not,        // not
    And,        // and
    Or,         // or

    // symbol operators
    LogicRightBitShift, // >>> (preserves MSB)
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
    LessCompare,        // <
    GreaterCompare,     // >
};

}