#pragma once

#include "tokens.h"
#include <string>
#include <optional>
#include <set>

namespace alder {

class Lexer {
public:
    Lexer(std::string source);

    Token next();
    Token peek();
    bool isAtEnd() const;

private:
    std::string source_;
    size_t pos_ = 0;
    std::optional<Token> lookahead_;
    char peekChar() const;
    char peekNextChar() const;
    char advanceChar();
    void skipWhitespaceAndComments();
    Token scanIdentifierOrKeywordOrType();
    Token scanNumber();
    Token scanString();
    Token scanOperatorOrPunctOrNewline();
    Token makeEofToken() const;
};


}