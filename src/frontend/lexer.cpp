#include "lexer.h"
#include <cctype>
#include <stdexcept>

namespace alder {

Lexer::Lexer(std::string source)
    : source_(std::move(source)) {}

Token Lexer::peek() {
    if (!pending_.empty()) {
        return pending_.front();
    }
    return;
}

Token Lexer::next() {
    if (!pending_.empty()) {
        Token t = pending_.front();
        pending_.pop_front();
        return t;
    }

    skipWhitespaceAndComments();

    // if EOF, flush detents (close out any statements)
    if (isAtEnd()) {
        while (!indentStack_.empty()) {
            indentStack_.pop();
            pending_.push_back({TokenType::Detent, ""});
        }
        if (!pending_.empty()) {
            return next();
        }
        return makeEofToken();
    }

    char c = peekChar();

    if (std::isalpha(c) || c == '_')
        return scanWord();

    if (std::isdigit(c))
        return scanNumber();

    if (c == '"')
        return scanString();

    return scanSymbol();
}

// ===== helpers =====

bool Lexer::isAtEnd() const {
    return pos_ >= source_.size();
}

char Lexer::peekChar() const {
    return isAtEnd() ? '\0' : source_[pos_];
}

char Lexer::peekNextChar() const {
    return (pos_ + 1 < source_.size()) ? source_[pos_ + 1] : '\0';
}

char Lexer::advanceChar() {
    if (isAtEnd()) {
        throw std::runtime_error("advance past EOF");
    }
    return source_[pos_++];
}

void Lexer::skipWhitespaceAndComments() {
    while (true) {
        char c = peekChar();

        if (c == ' ' || c == '\t') {
            advanceChar();
        }
        else if (c == '#') {
            while (peekChar() != '\n' && !isAtEnd())
                advanceChar();
        }
        else {
            break;
        }
    }
}

// ===== scanners =====
Token Lexer::handleNewline() {
    // consume \n
    advanceChar();
    pending_.push_back({TokenType::Newline, "\n"});
    
    // count spaces
    int spaces = 0;
    while (peekChar() == ' ') {
        advanceChar();
        spaces++;
    }

    if (indentStack_.empty()) {
        throw std::runtime_error("Invalid starting intendation");
    }

    // if additional indent
    if (spaces > indentStack_.top()) {
        indentStack_.push(spaces);
    } else { 
        // create detents
        while (!indentStack_.empty() && spaces > indentStack_.top()) {
            indentStack_.pop();
            pending_.push_back({TokenType::Detent, ""});
        }
        if (spaces != indentStack_.top()) {
            throw std::runtime_error("Indents do not match");
        }
    }

    Token t = pending_.front();
    pending_.pop_front();
    return t;
}

Token Lexer::scanWord() {
    size_t start = pos_;
    while (std::isalnum(peekChar()) || peekChar() == '_')
        advanceChar();

    std::string_view text(&source_[start], pos_ - start);

    if (text == "true" || text == "false")
        return {TokenType::BoolLit, std::string(text)};

    if (auto it = WORD_OPERATORS.find(text); it != WORD_OPERATORS.end())
        return {it->second, std::string(text)};

    if (auto it = KEYWORDS.find(text); it != KEYWORDS.end())
        return {it->second, std::string(text)};

    return {TokenType::Identifier, std::string(text)};
}

Token Lexer::scanNumber() {
    size_t start = pos_;
    bool isFloat = false;

    while (std::isdigit(peekChar()))
        advanceChar();

    if (peekChar() == '.' && std::isdigit(peekNextChar())) {
        isFloat = true;
        advanceChar();
        while (std::isdigit(peekChar()))
            advanceChar();
    }

    std::string_view text(&source_[start], pos_ - start);
    return {isFloat ? TokenType::FloatLit : TokenType::IntLit, std::string(text)};
}

Token Lexer::scanString() {
    advanceChar(); // consume opening "

    size_t start = pos_;
    while (peekChar() != '"' && !isAtEnd())
        advanceChar();

    if (isAtEnd()) {
        throw std::runtime_error("unterminated string");
    }

    std::string_view text(&source_[start], pos_ - start);
    advanceChar(); // closing "

    return {TokenType::StringLit, std::string(text)};
}

Token Lexer::scanSymbol() {
    char c = peekChar();

    if (c == '\n') {
        return handleNewline();
    }

    if (auto it = PUNCTUATION.find(c); it != PUNCTUATION.end()) {
        advanceChar();
        return {it->second, std::string(1, c)};
    }

    for (auto [text, kind] : OPERATORS) {
        if (source_.compare(pos_, text.size(), text) == 0) {
            pos_ += text.size();
            return {kind, std::string(text)};
        }
    }

    advanceChar();
    return {TokenType::Invalid, std::string(1, c)};
}

Token Lexer::makeEofToken() const {
    return {TokenType::Eof, ""};
}

}
