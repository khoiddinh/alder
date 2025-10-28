#pragma once

#include <string>

namespace alder {

enum class TokenType {
    // special
    Eof,
    Invalid,
    Newline,

    // identifiers & literals
    Identifier,
    IntLit,
    FloatLit,
    StringLit,
    // TODO: add char lit    

    // type names
    KwInt, // default 64 bit
    KwBool,
    KwChar,
    KwFloat, // default 64 bit // idx: 10
    KwVoid,

    // keywords
    BoolLit, // handled in kword parse bc alpha (true, false)

    KwDef,
    KwFinal,
    KwReturn,
    KwIf,
    KwElif,
    KwElse,
    KwFor,
    KwNot, // idx: 20
    KwAnd,
    KwOr,
    KwWhile,
    KwIn,

    // punctuation
    LParen, RParen,
    LBrace, RBrace,
    LBracket, RBracket,
    Comma, Colon, 
    Arrow, 

    // operators
    Plus,       // +
    Minus,      // -
    Star,       // *
    Slash,       // /
    Assign,     // =
    Equals,     // ==
    LEqCompare, // <=
    GEqCompare, // >=
    LCompare,   // <
    GCompare,   // >
    NotEq,      // !=
    
};

struct Token {
    TokenType type;
    std::string lexeme; // chars from input
};

}