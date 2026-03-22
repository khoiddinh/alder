#pragma once

#include "tokens.h"
#include <string>
#include <optional>
#include <deque>
#include <stack>


namespace alder::lexer {

namespace tok = alder::token;

class Lexer {
public:
    Lexer(std::string source);

    tok::Token next();
    tok::Token peek();
    bool isAtEnd() const;

private:

    /// @brief Lookup table for mapping keyword strings to tokens
    static inline const std::unordered_map<std::string_view, tok::TokenType> KEYWORDS = {
        {"fn", tok::TokenType::KwFn},
        {"final", tok::TokenType::KwFinal},
        {"return", tok::TokenType::KwReturn},
        {"if", tok::TokenType::KwIf},
        {"elif", tok::TokenType::KwElif},
        {"else", tok::TokenType::KwElse},
        {"for", tok::TokenType::KwFor},
        {"while", tok::TokenType::KwWhile},
        {"in", tok::TokenType::KwIn}
    };    
    /// @brief Ordered (greedy) table for mapping punctuation to tokens
    static inline const std::unordered_map<std::string_view, tok::TokenType> PUNCTUATION = {
        {"->", tok::TokenType::Arrow},
        {"(", tok::TokenType::LParen},
        {")", tok::TokenType::RParen},
        {"[", tok::TokenType::LBracket},
        {"]", tok::TokenType::RBracket},
        {",", tok::TokenType::Comma},
        {":", tok::TokenType::Colon}
    };
    /// @brief Lookup table for mapping word operators to tokens
    static inline const std::unordered_map<std::string_view, tok::OperatorKind> WORD_OPERATORS = {
        {"not", tok::OperatorKind::Not},
        {"and", tok::OperatorKind::And},
        {"or", tok::OperatorKind::Or}
    };

    /// @brief Ordered (greedy) table for mapping symbol operator strings to tokens
    static constexpr std::pair<std::string_view, tok::OperatorKind> OPERATORS[] = {
        {">>>", tok::OperatorKind::LogicRightBitShift},
        {"==", tok::OperatorKind::Equals},
        {"!=", tok::OperatorKind::NotEquals},
        {"<=", tok::OperatorKind::LessEqCompare},
        {">=", tok::OperatorKind::GreaterEqCompare},
        {"<<", tok::OperatorKind::LeftBitShift},
        {">>", tok::OperatorKind::RightBitShift},
    
        {"+", tok::OperatorKind::Plus},
        {"-", tok::OperatorKind::Minus},
        {"*", tok::OperatorKind::Star},
        {"/", tok::OperatorKind::Slash},
        {"<", tok::OperatorKind::LessCompare},
        {">", tok::OperatorKind::GreaterCompare}
    };

    

    std::string source_;
    size_t pos_ = 0;
    int line_ = 1;
    int col_  = 1;          // current column (1-based, advances with each char)
    int tokenStartCol_ = 1; // column at the start of the token being scanned
    std::deque<tok::Token> pending_;
    std::stack<int> indentStack_;
    char peekChar() const;
    char peekNextChar() const;
    char advanceChar();

    void skipWhitespaceAndComments();

    tok::Token handleNewline();
    tok::Token scanWord();
    tok::Token scanNumber();
    tok::Token scanString();
    tok::Token scanSymbol();

    tok::Token makeEofToken() const;
};


}