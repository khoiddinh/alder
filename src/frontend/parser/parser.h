#pragma once
#include <vector>
#include <string>
#include <optional>
#include <functional>
#include "ast.h"
#include "tokens.h"
#include "operator_precedence.h"
#include "diagnostic.h"


namespace alder::parser {

namespace ast = alder::ast;
namespace token = alder::token;

class Parser {
public:
    explicit Parser(std::vector<token::Token> tokens); 
    // * Build AST
    ast::Module parse();

    bool atEnd() const;
    const token::Token& peek() const;      // current

private:
    // * Instance variables
    std::vector<token::Token> tokens_;
    size_t current_ = 0;

private:
    // * Token handlers
    const token::Token& peekNext() const;        // current + 1
    const token::Token& advance();           // returns token then incr
    bool check(token::TokenType t) const;    // matches without consuming
    const token::Token& consume(token::TokenType t, const char* msg); // require token or throw

    // Build a CompileError using the current token's location
    [[nodiscard]] diag::CompileError error(const std::string& msg) const;
    [[nodiscard]] diag::CompileError errorAt(const token::Token& tok, const std::string& msg) const;
    
    // * Newline handlers
    void consumeOptionalNewlines();
    void consumeNewline(const char* msg);
    void consumeAtLeastOneNewline(const char* msg); 

private:
    // * Top Level Declaration Parsers
    ast::Decl   parseDecl();              // item   := func | stmt
    ast::Func   parseFunc();              // func   := KwFn Ident "(" params? ")" "->" type block
                                      // param := type ":" Identifier
                                      // handles all param arg parsing  
    ast::GlobalStmt parseGlobalStmt();              // dispatch by first token
		ast::Macro		parseMacro();

    // * Statement Parsers
    ast::BlockUP parseBlock();               // block   := "{" { stmt } "}"  
    ast::StmtUP parseStmt();
    ast::StmtUP parseVarDecl();                              // vardecl := type ":" Ident "=" expr [Newline]
    
    // For loop parser functions
    ast::StmtUP parseForIn();                                // forin   := KwFor type ":" Ident KwIn expr block
    ast::Binding parseBinding();
    ast::VarBinding parseVarBinding();

    ast::StmtUP parseWhile();                                // while   := KwWhile expr block
    
    // Conditional parser functions
    ast::StmtUP parseBranch();                               // if { stmt* } elif { stmt* } else { stmt* } (elif, else optional)
    ast::Branch parseConditionalBranch(token::TokenType kw, const char* context);

    ast::StmtUP parseReturn();
    
    // * Expression Statement Parser
    ast::StmtUP parseExprStmt();                            // e.g. foo(5)

    // * Expressions Parser (Pratt Parser)
    ast::ExprUP parseTypeExpr(); // parses type expression
    ast::ExprUP parseExpr(int minBP); // handles variable assignment (i.e. x=5)

    // helpers for parseExpr
    ast::ExprUP nud(const token::Token& token);
    ast::ExprUP led(const token::Token& token, ast::ExprUP left);
};

}