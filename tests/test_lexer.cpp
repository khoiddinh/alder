#include <catch2/catch_test_macros.hpp>
#include <vector>
#include <string>
#include <string_view>
#include <iostream>

#include ".././src/frontend/lexer.h"
#include ".././src/frontend/tokens.h"

using alder::Lexer;
using alder::Token;
using alder::TokenType;

static std::vector<Token> lex_all(std::string src) {
    Lexer lx(std::move(src));
    std::vector<Token> out;
    for (;;) {
        Token t = lx.next();
        out.push_back(t);
        if (t.type == TokenType::Eof) break;
    }
    return out;
}

static Token tok(TokenType tt, std::string sv) {
    return Token{tt, sv};
}

// Compare only type/lexeme (location ignored)
static void require_token_eq(const Token& a, const Token& b) {
    if (a.lexeme != b.lexeme) {
        std::cout << "lex: " << a.lexeme << ", " << b.lexeme << std::endl;
    }
    if (a.type != b.type) {
        std::cout << "type: " << a.lexeme << ", " << b.lexeme << std::endl;
    }
    REQUIRE(a.type == b.type);
    REQUIRE(std::string(a.lexeme) == std::string(b.lexeme));
}


// TESTS

TEST_CASE("Empty input -> Eof") {
    auto ts = lex_all("");
    REQUIRE(ts.size() == 1);
    REQUIRE(ts[0].type == TokenType::Eof);
}

TEST_CASE("Identifiers vs keywords vs types vs boollit") {
    auto ts = lex_all("def foo i64 true false final return if else for while not");
    // def foo i64 true false final return if else for while not
    require_token_eq(ts[0], tok(TokenType::KwDef,   "def"));
    require_token_eq(ts[1], tok(TokenType::Identifier, "foo"));
    require_token_eq(ts[2], tok(TokenType::Kwi64,  "i64"));
    require_token_eq(ts[3], tok(TokenType::BoolLit,"true"));
    require_token_eq(ts[4], tok(TokenType::BoolLit,"false"));
    require_token_eq(ts[5], tok(TokenType::KwFinal,"final"));
    require_token_eq(ts[6], tok(TokenType::KwReturn,"return"));
    require_token_eq(ts[7], tok(TokenType::KwIf,   "if"));
    require_token_eq(ts[8], tok(TokenType::KwElse, "else"));
    require_token_eq(ts[9], tok(TokenType::KwFor,  "for"));
    require_token_eq(ts[10],tok(TokenType::KwWhile,"while"));
    require_token_eq(ts[11],tok(TokenType::KwNot,  "not"));
    REQUIRE(ts.back().type == TokenType::Eof);
}

TEST_CASE("Numbers and strings") {
    auto ts = lex_all("123 \"hello\"");
    require_token_eq(ts[0], tok(TokenType::IntLit, "123"));
    require_token_eq(ts[1], tok(TokenType::StringLit, "hello"));
    REQUIRE(ts[2].type == TokenType::Eof);
}

TEST_CASE("Operators and punctuation") {
    auto ts = lex_all("( ) { } [ ] , : + - * / = ==");
    require_token_eq(ts[0],  tok(TokenType::LParen, "("));
    require_token_eq(ts[1],  tok(TokenType::RParen, ")"));
    require_token_eq(ts[2],  tok(TokenType::LBrace, "{"));
    require_token_eq(ts[3],  tok(TokenType::RBrace, "}"));
    require_token_eq(ts[4],  tok(TokenType::LBracket, "["));
    require_token_eq(ts[5],  tok(TokenType::RBracket, "]"));
    require_token_eq(ts[6],  tok(TokenType::Comma, ","));
    require_token_eq(ts[7],  tok(TokenType::Colon, ":"));
    require_token_eq(ts[8],  tok(TokenType::Plus, "+"));
    require_token_eq(ts[9],  tok(TokenType::Minus, "-"));
    require_token_eq(ts[10], tok(TokenType::Star, "*"));
    require_token_eq(ts[11], tok(TokenType::Slash, "/"));
    require_token_eq(ts[12], tok(TokenType::Assign, "="));
    require_token_eq(ts[13], tok(TokenType::Equals, "=="));
    REQUIRE(ts[14].type == TokenType::Eof);
}

TEST_CASE("Newlines are tokens; spaces are skipped") {
    auto ts = lex_all("a\nb");
    require_token_eq(ts[0], tok(TokenType::Identifier, "a"));
    require_token_eq(ts[1], tok(TokenType::Newline, "\n"));
    require_token_eq(ts[2], tok(TokenType::Identifier, "b"));
    REQUIRE(ts[3].type == TokenType::Eof);
}

TEST_CASE("peek() returns next without consuming; next() consumes") {
    Lexer lx("i32");
    Token p = lx.peek();
    REQUIRE(p.type == TokenType::Kwi32);
    Token n = lx.next();
    REQUIRE(n.type == TokenType::Kwi32);
    REQUIRE(lx.isAtEnd() == true); // assuming no hidden tokens left
}

TEST_CASE("Comments and whitespace") {
    auto ts = lex_all("foo  # comment\n  i64");
    require_token_eq(ts[0], tok(TokenType::Identifier, "foo"));
    // if your design *emits* Newline, expect it:
    require_token_eq(ts[1], tok(TokenType::Newline, "\n"));
    require_token_eq(ts[2], tok(TokenType::Kwi64, "i64"));
    REQUIRE(ts[3].type == TokenType::Eof);
}

TEST_CASE("Comments at end") {
    auto ts = lex_all("foo  \n# comment");
    require_token_eq(ts[0], tok(TokenType::Identifier, "foo"));
    // if your design *emits* Newline, expect it:
    require_token_eq(ts[1], tok(TokenType::Newline, "\n"));
    REQUIRE(ts[2].type == TokenType::Eof);
}

TEST_CASE("Invalid tokens surface as Invalid") {
    auto ts = lex_all("@");
    REQUIRE(ts[0].type == TokenType::Invalid);
    REQUIRE(ts[1].type == TokenType::Eof);
}

TEST_CASE("Consecutive newlines produce consecutive Newline tokens") {
    auto ts = lex_all("a\n\nb");
    require_token_eq(ts[0], tok(TokenType::Identifier, "a"));
    require_token_eq(ts[1], tok(TokenType::Newline, "\n"));
    require_token_eq(ts[2], tok(TokenType::Newline, "\n"));
    require_token_eq(ts[3], tok(TokenType::Identifier, "b"));
    REQUIRE(ts[4].type == TokenType::Eof);
}

TEST_CASE("Comments strip until newline, newline still emitted") {
    auto ts = lex_all("x#y z\nw");
    // comment “#y z” removed, but newline remains
    require_token_eq(ts[0], tok(TokenType::Identifier, "x"));
    require_token_eq(ts[1], tok(TokenType::Newline, "\n"));
    require_token_eq(ts[2], tok(TokenType::Identifier, "w"));
    REQUIRE(ts[3].type == TokenType::Eof);
}

TEST_CASE("Comment at end of input consumes to EOF (no trailing Newline)") {
    auto ts = lex_all("x # end");
    require_token_eq(ts[0], tok(TokenType::Identifier, "x"));
    REQUIRE(ts[1].type == TokenType::Eof);
}

TEST_CASE("Spaces are skipped but newlines are tokens; tabs currently Invalid") {
    // current skipWhitespace only skips ' ' (space), not '\t'
    auto ts = lex_all("a \t b");
    // 'a'
    require_token_eq(ts[0], tok(TokenType::Identifier, "a"));
    // '\t' goes to operator/punct scanner -> default -> Invalid
    REQUIRE(ts[1].type == TokenType::Invalid);
    // 'b'
    require_token_eq(ts[2], tok(TokenType::Identifier, "b"));
    REQUIRE(ts[3].type == TokenType::Eof);
}

TEST_CASE("Numbers then letters split into IntLit then Identifier") {
    auto ts = lex_all("123abc");
    require_token_eq(ts[0], tok(TokenType::IntLit, "123"));
    // next() sees 'a' and scans identifier
    require_token_eq(ts[1], tok(TokenType::Identifier, "abc"));
    REQUIRE(ts[2].type == TokenType::Eof);
}

TEST_CASE("Long identifiers and keywords boundary") {
    auto ts = lex_all("returnX return");
    require_token_eq(ts[0], tok(TokenType::Identifier, "returnX"));
    require_token_eq(ts[1], tok(TokenType::KwReturn, "return"));
    REQUIRE(ts[2].type == TokenType::Eof);
}

TEST_CASE("Types vs identifiers that start with type text") {
    auto ts = lex_all("i64 i64bit i32 i32x bool boolean char char_ f64 f64y");
    require_token_eq(ts[0], tok(TokenType::Kwi64, "i64"));
    require_token_eq(ts[1], tok(TokenType::Identifier, "i64bit"));
    require_token_eq(ts[2], tok(TokenType::Kwi32, "i32"));
    require_token_eq(ts[3], tok(TokenType::Identifier, "i32x"));
    require_token_eq(ts[4], tok(TokenType::KwBool, "bool"));
    require_token_eq(ts[5], tok(TokenType::Identifier, "boolean"));
    require_token_eq(ts[6], tok(TokenType::KwChar, "char"));
    require_token_eq(ts[7], tok(TokenType::Identifier, "char_"));
    require_token_eq(ts[8], tok(TokenType::Kwf64, "f64"));
    require_token_eq(ts[9], tok(TokenType::Identifier, "f64y"));

    auto ts2 = lex_all("test_var def #comments weofweif \n _var");
    require_token_eq(ts2[0], tok(TokenType::Identifier, "test_var"));
    require_token_eq(ts2[1], tok(TokenType::KwDef, "def"));
    require_token_eq(ts2[2], tok(TokenType::Newline, "\n"));
    require_token_eq(ts2[3], tok(TokenType::Identifier, "_var"));
    
    REQUIRE(ts.back().type == TokenType::Eof);
}

TEST_CASE("Operators: '=' vs '==' lexeme and types") {
    auto ts = lex_all("= == ==");
    require_token_eq(ts[0], tok(TokenType::Assign, "="));
    require_token_eq(ts[1], tok(TokenType::Equals, "=="));
    require_token_eq(ts[2], tok(TokenType::Equals, "=="));
    REQUIRE(ts[3].type == TokenType::Eof);
}

TEST_CASE("Mixed punctuation sequence") {
    auto ts = lex_all("([{}]),:");
    require_token_eq(ts[0], tok(TokenType::LParen, "("));
    require_token_eq(ts[1], tok(TokenType::LBracket, "["));
    require_token_eq(ts[2], tok(TokenType::LBrace, "{"));
    require_token_eq(ts[3], tok(TokenType::RBrace, "}"));
    require_token_eq(ts[4], tok(TokenType::RBracket, "]"));
    require_token_eq(ts[5], tok(TokenType::RParen, ")"));
    require_token_eq(ts[6], tok(TokenType::Comma, ","));
    require_token_eq(ts[7], tok(TokenType::Colon, ":"));
    REQUIRE(ts[8].type == TokenType::Eof);
}

TEST_CASE("Strings: empty, spaces inside, and followed by identifier") {
    auto ts = lex_all("\"\" \"a b c\"foo");
    require_token_eq(ts[0], tok(TokenType::StringLit, ""));
    require_token_eq(ts[1], tok(TokenType::StringLit, "a b c"));
    require_token_eq(ts[2], tok(TokenType::Identifier, "foo"));
    REQUIRE(ts[3].type == TokenType::Eof);
}

TEST_CASE("Peek does not consume across multiple calls") {
    Lexer lx("def");
    Token p1 = lx.peek();
    Token p2 = lx.peek();
    REQUIRE(p1.type == TokenType::KwDef);
    REQUIRE(p2.type == TokenType::KwDef);
    Token n1 = lx.next();
    REQUIRE(n1.type == TokenType::KwDef);
    REQUIRE(lx.isAtEnd() == true);
}

TEST_CASE("Identifier boundary stops at non-alnum") {
    auto ts = lex_all("abc,123");
    require_token_eq(ts[0], tok(TokenType::Identifier, "abc"));
    require_token_eq(ts[1], tok(TokenType::Comma, ","));
    require_token_eq(ts[2], tok(TokenType::IntLit, "123"));
    REQUIRE(ts[3].type == TokenType::Eof);
}

TEST_CASE("Newline tokenization with trailing spaces") {
    auto ts = lex_all("a  \n  b");
    require_token_eq(ts[0], tok(TokenType::Identifier, "a"));
    require_token_eq(ts[1], tok(TokenType::Newline, "\n"));
    require_token_eq(ts[2], tok(TokenType::Identifier, "b"));
    REQUIRE(ts[3].type == TokenType::Eof);
}

TEST_CASE("Invalid single characters surface as Invalid") {
    // current implementation: anything not handled in scanOperatorOrPunctOrNewline() -> Invalid
    auto ts = lex_all("@ $ ` \\");
    REQUIRE(ts[0].type == TokenType::Invalid);
    REQUIRE(ts[1].type == TokenType::Invalid);
    REQUIRE(ts[2].type == TokenType::Invalid);
    REQUIRE(ts[3].type == TokenType::Invalid);
    REQUIRE(ts[4].type == TokenType::Eof);
}

TEST_CASE("Numbers: leading zeros and long sequences") {
    auto ts = lex_all("00012 9876543210");
    require_token_eq(ts[0], tok(TokenType::IntLit, "00012")); // lexer preserves lexeme
    require_token_eq(ts[1], tok(TokenType::IntLit, "9876543210"));
    REQUIRE(ts[2].type == TokenType::Eof);
}

TEST_CASE("Equals lookahead does not over-consume when single '='") {
    auto ts = lex_all("= =x");
    require_token_eq(ts[0], tok(TokenType::Assign, "="));
    require_token_eq(ts[1], tok(TokenType::Assign, "="));
    require_token_eq(ts[2], tok(TokenType::Identifier, "x"));
    REQUIRE(ts[3].type == TokenType::Eof);
}

TEST_CASE("Newline after comment still recognized") {
    auto ts = lex_all("# cmt\n\nx");
    require_token_eq(ts[0], tok(TokenType::Newline, "\n"));
    require_token_eq(ts[1], tok(TokenType::Newline, "\n"));
    require_token_eq(ts[2], tok(TokenType::Identifier, "x"));
    REQUIRE(ts[3].type == TokenType::Eof);
}

TEST_CASE("Unterminated string throws (enable after implementing throws)") {
    // REQUIRE_THROWS_AS(lex_all("\"unterminated"), std::runtime_error);
    SUCCEED("Enable this test after scanString/advanceChar throw on unterminated strings.");
}


TEST_CASE("Keyword in") {
    auto ts = lex_all("for x in range { x = 1 }");
    require_token_eq(ts[0], tok(TokenType::KwFor,   "for"));
    require_token_eq(ts[1], tok(TokenType::Identifier, "x"));
    require_token_eq(ts[2], tok(TokenType::KwIn,    "in"));
    require_token_eq(ts[3], tok(TokenType::Identifier, "range"));
}

TEST_CASE("Arrow operator") {
    auto ts = lex_all("x -> y");
    require_token_eq(ts[0], tok(TokenType::Identifier, "x"));
    require_token_eq(ts[1], tok(TokenType::Arrow, "->"));
    require_token_eq(ts[2], tok(TokenType::Identifier, "y"));
    REQUIRE(ts[3].type == TokenType::Eof);
}

TEST_CASE("Arrow does not trigger on '-' without '>'") {
    auto ts = lex_all("x - y");
    require_token_eq(ts[0], tok(TokenType::Identifier, "x"));
    require_token_eq(ts[1], tok(TokenType::Minus, "-"));
    require_token_eq(ts[2], tok(TokenType::Identifier, "y"));
    REQUIRE(ts[3].type == TokenType::Eof);
}

TEST_CASE("Arrow at start of line") {
    auto ts = lex_all("->x");
    require_token_eq(ts[0], tok(TokenType::Arrow, "->"));
    require_token_eq(ts[1], tok(TokenType::Identifier, "x"));
    REQUIRE(ts[2].type == TokenType::Eof);
}