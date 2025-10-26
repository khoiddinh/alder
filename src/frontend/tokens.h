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
    StringLit,
    // TODO: add boolean

    // type names
    Kwi64, Kwi32, 
    KwBool,
    KwChar,
    Kwf64,
    KwVoid,

    // keywords
    BoolLit, // handled in kword parse bc alpha (true, false)

    KwDef,
    KwFinal,
    KwReturn,
    KwIf,
    KwElse,
    KwFor,
    KwNot,
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
};

struct Token {
    TokenType type;
    std::string lexeme; // chars from input
    // TODO: implement location tracking
};

}