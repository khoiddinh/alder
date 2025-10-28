#pragma once
#include <vector>
#include <string>
#include <optional>
#include <functional>
#include "ast.h"   
#include "tokens.h"  

namespace alder {

struct ParseError {
    std::string message;
    // TODO: add line/col if Token has it
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens); 
    // Build AST
    Module parse();

    // ======== Public Helpers: token stream ========
    bool atEnd() const;
    const Token& peek() const;        // current


private:
    // ======== High-level ========
    Module parseModule();            // module := { item } Eof
    Item   parseItem();              // item   := func | stmt
    Func   parseFunc();              // func   := KwDef Ident "(" params? ")" "->" type block
                                      // param := type ":" Identifier
                                      // handles all param arg parsing  
    BlockUP parseBlock();               // block   := "{" { stmt } "}"  
    // ======== Statements ========
    StmtUP parseStmt();              // dispatch by first token
    StmtUP parseVarDecl();                              // vardecl := type ":" Ident "=" expr [Newline]
    StmtUP parseForIn();                                // forin   := KwFor type ":" Ident KwIn expr block
    StmtUP parseWhile();                                // while   := KwWhile expr block
    StmtUP parseAssign();                               // starts with Ident → assign or exprstmt
    StmtUP parseBranch();                               // if { stmt* } elif { stmt* } else { stmt* } (elif, else optional)
    StmtUP parseReturn();                               // return  := return expr
                                                        // should only be in function block def
    StmtUP parseExprStmt();                            // e.g. foo(5)
                                                        // ======== Expressions (Pratt Parsing)
    ExprUP parseExpr(int minBP);               // expr := equality, arg = left binding power
                                              // handles function call parsing
    StmtUP parseAssignOrExprStmt();
    // helpers for parseExpr
    ExprUP nud(const Token& token);
    ExprUP led(const Token& token, ExprUP left);
    static int prefixBindingPower(const TokenType tokType);
    static int leftBindingPower(const TokenType tokType);
    static int rightBindingPower(const TokenType tokType);  
    // ======== Private Helpers: token stream ========
    const Token& peekNext() const;        // current + 1
    const Token& advance();           // returns token then incr
    bool check(TokenType t) const;    // matches without consuming
    const Token& consume(TokenType t, const char* msg); // require token or throw
    const Token& consumeGeneric(std::function<bool(TokenType)> fn, const char* msg);  // ret next token while advancing, else throws
    std::optional<Token> consumeOptional(TokenType t); // consume if next one is of type t, otherwise do nothing

    static bool isTypeToken(TokenType t);  // check if is a type token
    // ======== Newline handling ========
    // Outside blocks, optionally treat Newline as stmt terminator:
    void consumeOptionalNewlines();
    void consumeNewline(const char* msg);

    // ======== Mappers ========
    // Map tokens to semantic enums
    static UnaryOp  toUnaryOp(TokenType t);
    static BinaryOp toBinaryOp(TokenType t);
    static TypeName toTypeName(TokenType t); // maps lexer token types to parser types

private:
    std::vector<Token> tokens_;
    size_t current_ = 0;
};

}