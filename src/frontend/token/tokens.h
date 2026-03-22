#pragma once

#include <string>
#include <optional>

namespace alder::token {

// Declared before Token so Token's std::optional<OperatorKind> field compiles.
enum class OperatorKind {
    // Word operators
    Not,                    // not
    And,                    // and
    Or,                     // or

    // Symbol operators (greedy – longer patterns first)
    LogicRightBitShift,     // >>>
    Equals,                 // ==
    NotEquals,              // !=
    LessEqCompare,          // <=
    GreaterEqCompare,       // >=
    LeftBitShift,           // <<
    RightBitShift,          // >>

    Plus,                   // +
    Minus,                  // -
    Star,                   // *
    Slash,                  // /
    LessCompare,            // <
    GreaterCompare,         // >
};

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

    // keywords
    KwFn,
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
    std::string lexeme;
    int line = 0;
    int col  = 0;
    std::optional<OperatorKind> op;
};

} // namespace alder::token
