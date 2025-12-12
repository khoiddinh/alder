#pragma once

#include "tokens.h"
#include <string>
#include <optional>
#include <deque>
#include <stack>


namespace alder {

class Lexer {
public:
    Lexer(std::string source);

    Token next();
    Token peek();
    bool isAtEnd() const;

private:
    /// @brief Lookup table for mapping keyword strings to tokens
    static inline const std::unordered_map<std::string_view, TokenType> KEYWORDS = {
        {"def", TokenType::KwDef},
        {"final", TokenType::KwFinal},
        {"return", TokenType::KwReturn},
        {"if", TokenType::KwIf},
        {"elif", TokenType::KwElif},
        {"else", TokenType::KwElse},
        {"for", TokenType::KwFor},
        {"while", TokenType::KwWhile},
        {"in", TokenType::KwIn}
    };    
    /// @brief Lookup table for mapping punctuation schars to tokens
    static inline const std::unordered_map<char, TokenType> PUNCTUATION = {
        {'(', TokenType::LParen},
        {')', TokenType::RParen},
        {'[', TokenType::LBracket},
        {']', TokenType::RBracket},
        {',', TokenType::Comma},
        {':', TokenType::Colon}
    };
    /// @brief Lookup table for mapping word operators to tokens
    static inline const std::unordered_map<std::string_view, TokenType> WORD_OPERATORS = {
        {"not", TokenType::Not},
        {"and", TokenType::And},
        {"or", TokenType::Or}
    };

    /// @brief Ordered (greedy) table for mapping symbol operator strings to tokens
    static constexpr std::pair<std::string_view, TokenType> OPERATORS[] = {
        {">>>", TokenType::LogicRightBitShift},
        {"->", TokenType::Arrow},
        {"==", TokenType::Equals},
        {"!=", TokenType::NotEquals},
        {"<=", TokenType::LessEqCompare},
        {">=", TokenType::GreaterEqCompare},
        {"<<", TokenType::LeftBitShift},
        {">>", TokenType::RightBitShift},
        
        {"+", TokenType::Plus},
        {"-", TokenType::Minus},
        {"/", TokenType::Slash},
        {"=", TokenType::Assign},
        {"<", TokenType::LessCompare},
        {">", TokenType::GreaterCompare}
    };

    

    std::string source_;
    size_t pos_ = 0;
    std::deque<Token> pending_;
    std::stack<int> indentStack_;
    bool isAtEnd() const;
    char peekChar() const;
    char peekNextChar() const;
    char advanceChar();

    void skipWhitespaceAndComments();

    Token handleNewline();
    Token scanWord();
    Token scanNumber();
    Token scanString();
    Token scanSymbol();

    Token makeEofToken() const;
};


}