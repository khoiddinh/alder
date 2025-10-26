#include "lexer.h"
#include <cctype> 
#include <optional>
#include <set>
#include <iostream>

namespace alder {

Lexer::Lexer(std::string source) {
    lookahead_.reset();
    source_ = source;
}

// PUBLIC API
Token Lexer::next() {
    // if lookahead_ exists, return it and clear it
    if (lookahead_.has_value()) {
        Token nextToken = *lookahead_;
        lookahead_.reset(); // clear
        return nextToken;
    }
    // else find token skipWhitespaceAndComments, check EOF,
    skipWhitespaceAndComments(); 
    if (isAtEnd()) {
        return makeEofToken();
    } else {
        // TODO: handle types, newline
        // then dispatch: identifier, number, string, operator
        char firstChar = peekChar();
        Token nextToken;
        if (std::isalpha(firstChar) || firstChar == '_') { // only identifier can start with leading '_'
            nextToken = scanIdentifierOrKeywordOrType();
        }
        else if (std::isdigit(firstChar)) { // number
            nextToken = scanNumber();
        }
        else if (firstChar == '"') { // string
            nextToken = scanString();
        }
        else {
            nextToken = scanOperatorOrPunctOrNewline();
        }
        return nextToken;
    }
    
}

Token Lexer::peek() {
    // TODO:
    // if lookahead_ is empty, fill it by calling next()
    if (lookahead_.has_value()) {
        return *lookahead_;
    } else {
        lookahead_ = next();
        return *lookahead_;
    }
}

bool Lexer::isAtEnd() const {
    return pos_ >= source_.size();
}

// PRIVATE HELPERS (stubs for now)
// returns char at curr pos
char Lexer::peekChar() const {
    return isAtEnd() ? '\0' : source_[pos_];
}

char Lexer::peekNextChar() const {
    return (pos_ + 1 < source_.size()) ? source_[pos_ + 1] : '\0';
}

// get curr char then incr pos
char Lexer::advanceChar() {
    // TODO: update line/column

    // catch error
    if (isAtEnd()) {
        std::runtime_error("Tried to advance char past end");
    }
    return source_[pos_++];
}

// ends with pos at first non whitespace/comment char
void Lexer::skipWhitespaceAndComments() {
    while (peekChar() == ' ' || peekChar() == '#') { 
        if (peekChar() == '#') { 
            // read until newline or eof
            while (peekChar() != '\n' && peekChar() != '\0') {
                advanceChar();
            }
        } else { // don't consume \n or \0
            advanceChar();
        }
    }
}

// ends with pos at first non identifier/kword char
Token Lexer::scanIdentifierOrKeywordOrType() {
    // get the string
    std::string str = "";
    while (std::isalnum(peekChar()) || peekChar() == '_') { // types can be numeric, also read through '_'
        str += advanceChar();
    }
    TokenType tokType;
    // check if bool
    if (str == "true" || str == "false") {
        tokType = TokenType::BoolLit;
    } 
    
    // check if a keyword
    else if (str == "def") {
        tokType = TokenType::KwDef;
    } else if (str == "final") {
        tokType = TokenType::KwFinal;
    } else if (str == "return") {
        tokType = TokenType::KwReturn;
    } else if (str == "if") {
        tokType = TokenType::KwIf;
    } else if (str == "else") {
        tokType = TokenType::KwElse;
    } else if (str == "for") {
        tokType = TokenType::KwFor;
    } else if (str == "not") {
        tokType = TokenType::KwNot;
    } else if (str == "while") {
        tokType = TokenType::KwWhile;
    } else if (str == "in") {
        tokType = TokenType::KwIn;
    } 
    // check if type
    else if (str == "i64") {
        tokType = TokenType::Kwi64;
    } else if (str == "i32") {
        tokType = TokenType::Kwi32;
    } else if (str == "bool") {
        tokType = TokenType::KwBool;
    } else if (str == "char") {
        tokType = TokenType::KwChar;
    } else if (str == "f64") {
        tokType = TokenType::Kwf64;
    } else if (str == "void") {
        tokType = TokenType::KwVoid;
    }
    else { // if not keyword or type then identifier
        tokType = TokenType::Identifier;
    }
    return Token(tokType, str);
}

Token Lexer::scanNumber() {
    std::string str = "";
    while (std::isdigit(peekChar())) {
        str += advanceChar();
    }
    return Token{TokenType::IntLit, str};
}

Token Lexer::scanString() {
    if (advanceChar() != '"') { // also skips " so next pos is char
        throw std::runtime_error("Called scanString on nonstring token");
    }
    char curr = advanceChar();
    std::string str = "";
    while (curr != '"' && curr != '\0') {
        str += curr;
        curr = advanceChar();
    }
    if (curr == '\0') { // string not terminated
        throw std::runtime_error("string not terminated EOF reached");
    }
    return Token{TokenType::StringLit, str};
}

Token Lexer::scanOperatorOrPunctOrNewline() {
    char curr = advanceChar();
    std::string str = "";
    str += curr; // add the read curr to str
    TokenType tokType = TokenType::Eof;
    switch (curr) {
        // handle 1 char ops/punct first

        // parens
        case '(':
            tokType = TokenType::LParen;
            break;
        case ')':
            tokType = TokenType::RParen;
            break;
        case '{':
            tokType = TokenType::LBrace;
            break;
        case '}':
            tokType = TokenType::RBrace;
            break;
        case '[':
            tokType = TokenType::LBracket;
            break;
        case ']':
            tokType = TokenType::RBracket;
            break;

        // other punct
        case ',':
            tokType = TokenType::Comma;
            break;
        case ':':
            tokType = TokenType::Colon;
            break;

        // arith ops
        case '+':
            tokType = TokenType::Plus;
            break;
        case '-': // handle - and -> case
            if (peekChar() == '>') {
                tokType = TokenType::Arrow;
                // increment to consume '>'
                str += advanceChar(); // add to str
            } else {
                tokType = TokenType::Minus;
            }
            break;
        case '*':
            tokType = TokenType::Star;
            break;
        case '/':
            tokType = TokenType::Slash;
            break;
        case '=': // handle = and == case
            if (peekChar() == '=') {
                tokType = TokenType::Equals;
                // increment to consume second '='
                str += advanceChar(); // add to str
                
            } else {
                tokType = TokenType::Assign;
            }
            break;
        case '\n': 
            tokType = TokenType::Newline;
            break;
        default:
            tokType = TokenType::Invalid;
            break;
    }
    return Token{tokType, str};
}

Token Lexer::makeEofToken() const {
    // Must return a valid EOF token
    return Token{TokenType::Eof, ""};
}

} 