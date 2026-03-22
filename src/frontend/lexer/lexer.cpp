#include "lexer.h"
#include <cctype>
#include <stdexcept>

namespace alder::lexer {

using namespace alder::token;

Lexer::Lexer(std::string source)
    : source_(std::move(source)) {
    indentStack_.push(0);
}

Token Lexer::peek() {
    if (!pending_.empty()) return pending_.front();
    Token t = next();
    pending_.push_front(t);
    return t;
}

Token Lexer::next() {
    if (!pending_.empty()) {
        Token t = pending_.front();
        pending_.pop_front();
        return t;
    }

    skipWhitespaceAndComments();

    if (isAtEnd()) {
        while (indentStack_.size() > 1) {
            indentStack_.pop();
            pending_.push_back(Token{.type = TokenType::Dedent, .lexeme = "", .line = line_, .col = col_});
        }
        if (!pending_.empty()) return next();
        return makeEofToken();
    }

    // Save column at the start of this token (after whitespace is skipped)
    tokenStartCol_ = col_;

    char c = peekChar();

    if (std::isalpha(c) || c == '_') return scanWord();
    if (std::isdigit(c))             return scanNumber();
    if (c == '"')                    return scanString();
    return scanSymbol();
}

// ** Helpers **

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
    if (isAtEnd()) throw std::runtime_error("advance past EOF");
    char c = source_[pos_++];
    col_++;
    return c;
}

void Lexer::skipWhitespaceAndComments() {
    while (true) {
        char c = peekChar();
        if (c == ' ' || c == '\t') {
            advanceChar();
        } else if (c == '#') {
            while (peekChar() != '\n' && !isAtEnd())
                advanceChar();
        } else {
            break;
        }
    }
}

// ** Scanners **

Token Lexer::handleNewline() {
    int nlCol = tokenStartCol_;

    advanceChar(); // consume '\n'
    col_ = 1;      // reset column for the new line

    pending_.push_back(Token{.type = TokenType::Newline, .lexeme = "\n", .line = line_, .col = nlCol});
    line_++;

    // Count indentation (tabs = 4 spaces)
    int spaces = 0;
    while (peekChar() == ' ' || peekChar() == '\t') {
        if (advanceChar() == '\t')
            spaces += 4;
        else
            spaces++;
    }

    if (indentStack_.empty())
        throw std::runtime_error("invalid indentation");

    if (spaces > indentStack_.top()) {
        indentStack_.push(spaces);
        pending_.push_back(Token{.type = TokenType::Indent, .lexeme = "", .line = line_, .col = 1});
    } else {
        while (!indentStack_.empty() && spaces < indentStack_.top()) {
            indentStack_.pop();
            pending_.push_back(Token{.type = TokenType::Dedent, .lexeme = "", .line = line_, .col = 1});
        }
        if (indentStack_.empty() || spaces != indentStack_.top())
            throw std::runtime_error("indentation does not match any outer level");
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
        return Token{.type = TokenType::BoolLit, .lexeme = std::string(text), .line = line_, .col = tokenStartCol_};

    if (auto it = WORD_OPERATORS.find(text); it != WORD_OPERATORS.end())
        return Token{.type = TokenType::Operator, .lexeme = std::string(text), .line = line_, .col = tokenStartCol_, .op = it->second};

    if (auto it = KEYWORDS.find(text); it != KEYWORDS.end())
        return Token{.type = it->second, .lexeme = std::string(text), .line = line_, .col = tokenStartCol_};

    return Token{.type = TokenType::Identifier, .lexeme = std::string(text), .line = line_, .col = tokenStartCol_};
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
    TokenType type = isFloat ? TokenType::FloatLit : TokenType::IntLit;
    return Token{.type = type, .lexeme = std::string(text), .line = line_, .col = tokenStartCol_};
}

Token Lexer::scanString() {
    advanceChar(); // consume opening "

    size_t start = pos_;
    while (peekChar() != '"' && !isAtEnd())
        advanceChar();

    if (isAtEnd())
        throw std::runtime_error("unterminated string");

    std::string_view text(&source_[start], pos_ - start);
    advanceChar(); // consume closing "

    return Token{.type = TokenType::StringLit, .lexeme = std::string(text), .line = line_, .col = tokenStartCol_};
}

Token Lexer::scanSymbol() {
    char c = peekChar();

    if (c == '\n')
        return handleNewline();

    for (auto [text, kind] : PUNCTUATION) {
        if (source_.compare(pos_, text.size(), text) == 0) {
            pos_ += text.size();
            col_ += static_cast<int>(text.size());
            return Token{.type = kind, .lexeme = std::string(text), .line = line_, .col = tokenStartCol_};
        }
    }

    if (source_.compare(pos_, 1, "=") == 0 && source_.compare(pos_, 2, "==") != 0) {
        pos_ += 1;
        col_ += 1;
        return Token{.type = TokenType::Assign, .lexeme = "=", .line = line_, .col = tokenStartCol_};
    }

    for (auto [text, kind] : OPERATORS) {
        if (source_.compare(pos_, text.size(), text) == 0) {
            pos_ += text.size();
            col_ += static_cast<int>(text.size());
            return Token{.type = TokenType::Operator, .lexeme = std::string(text), .line = line_, .col = tokenStartCol_, .op = kind};
        }
    }

    advanceChar();
    return Token{.type = TokenType::Invalid, .lexeme = std::string(1, c), .line = line_, .col = tokenStartCol_};
}

Token Lexer::makeEofToken() const {
    return Token{.type = TokenType::Eof, .lexeme = "", .line = line_, .col = col_};
}

} // namespace alder::lexer
