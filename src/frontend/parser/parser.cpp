
#include "parser.h"

#include <vector>
#include <functional>
#include <iostream> // TODO: DEBUG
#include "../token/tokens.h"
#include "operator_precedence.h"

namespace alder::parser {

using namespace alder::ast;
using namespace alder::token;

// ** PUBLIC API **

Parser::Parser(std::vector<Token> tokens) {
    tokens_ = std::move(tokens);
    current_ = 0;
}

// * Top-level parser method
Module Parser::parse() {
    Module module = Module{std::vector<Decl>{}};
    // rm leading newlines
    consumeOptionalNewlines();

    while (!atEnd()) {
        module.declarations.push_back(parseDecl());
    }
    return module;
}

// ** PUBLIC TOKEN HANDLERS **
bool Parser::atEnd() const {
    return tokens_[current_].type == TokenType::Eof;
}

const Token& Parser::peek() const {
    return tokens_[current_];
}

// ** PRIVATE TOKEN HANDLERS **
const Token& Parser::peekNext() const {
    if (current_+1 >= tokens_.size()) {
        throw std::runtime_error("Tried to peekNext past end of token stream");
    }
    return tokens_[current_+1];
}

const Token& Parser::advance() {
    if (atEnd()) {
        throw std::runtime_error("Tried to read past EOF, check matching parens");
    }
    return tokens_[current_++];
}

bool Parser::check(TokenType t) const {
    return tokens_[current_].type == t;
}

const Token& Parser::consume(TokenType t, const char* msg) {
    if (check(t)) {
        return advance();
    } else {
        throw std::runtime_error(msg);
    }
}

// ** NEWLINE HANDLERS **
void Parser::consumeOptionalNewlines() {
    while (check(TokenType::Newline) && !check(TokenType::Eof)) {
        advance();
    }
}

void Parser::consumeNewline(const char* msg) {
    if (check(TokenType::Newline)) {
        advance();
    } else {
        std::cout << (int) peek().type  << " " << (int) peekNext().type << std::endl;    

        throw std::runtime_error(msg);
    }
}
/// @brief Requires consumption of at least one newline but will consume greedily
/// @param msg error message thrown if less than one newline to consume
void Parser::consumeAtLeastOneNewline(const char* msg) {
    consumeNewline(msg);
    consumeOptionalNewlines();
}


// ** PARSER LOGIC **

// ** Top Level Declaration Parsers **

Decl Parser::parseDecl() {
    Decl decl{};
    if (check(TokenType::KwDef)) {
        decl = parseFunc();
    } else {
        decl = parseGlobalStmt();
    }
    // rm trailing newlines
    consumeOptionalNewlines();
    return decl;
}

/// @brief Parses function declaration statement
///
/// Grammar:
///     arg = identifier ":" identifier , (only if arg to follow)
///     func ::= def "(" arg* ")" -> identifier ":"
///
/// Consumes:
///     - KwDef
///     - LParen
///     - Identifier (variable name)
///     - Colon
///     - Identifier (type name)
///     - ...
///     - RParen
///     - Arrow
///     - Identifier (return type)
///     - Colon
///     - Block
///
/// @return A Function Declaration statement AST node.
Func Parser::parseFunc() {
    Func fn{};
    consume(TokenType::KwDef, "expected function declaration");
    fn.name = Identifier{ consume(TokenType::Identifier, "expected function name").lexeme };
    consume(TokenType::LParen, "function expected '('");

    std::vector<ExprUP> params;
    while (!check(TokenType::RParen) && !check(TokenType::Eof)) {
        params.push_back(parseExpr(0));
        if (check(TokenType::RParen)) { 
            break;
        } else {
            consume(TokenType::Comma, "expected ',' or ')' in argument list");
        }
    }
    consume(TokenType::RParen, "expected ')' to close argument list");
    consume(TokenType::Arrow, "expected '->' after params");
    fn.retTypeExpr = parseTypeExpr();
    consume(TokenType::Colon, "expected ':' after type");
    fn.body = parseBlock();
    return fn;
}

// * Parse Global Statements
GlobalStmt Parser::parseGlobalStmt() {
    return GlobalStmt { parseStmt() };
}


// ** Statement Parsers **
BlockUP Parser::parseBlock() {
    Block block{};
    consumeAtLeastOneNewline("Expected \\n to start block");
    consume(TokenType::Indent, "Expected indent");

    // allow optional new lines to start block
    consumeOptionalNewlines();
    while (!check(TokenType::Dedent) && !check(TokenType::Eof)) {
        block.body.push_back(parseStmt());
        consumeOptionalNewlines(); // opt newlines between statements
    }
    // consume end token
    consume(TokenType::Dedent, "Expected dedent");
    return std::make_unique<Block>(std::move(block));
}

// * Parse Statement (Non Top-Level)
StmtUP Parser::parseStmt() {
    if (check(TokenType::KwIf)) {
        return parseBranch();
    } 
    if (check(TokenType::KwWhile)) {
        return parseWhile();
    }
    if (check(TokenType::KwFor)) {
        return parseForIn();
    }

    // lambda func helper
    bool startsVarDecl = [this]() {
        return check(TokenType::Identifier) && peekNext().type == TokenType::Colon;
    }();
    if (check(TokenType::KwFinal) || startsVarDecl) {
        return parseVarDecl();
    }

    // otherwise its an expr stmt
    StmtUP stmt = parseExprStmt();

    // consume statement end
    if (check(TokenType::Newline)) {
        consumeAtLeastOneNewline("Expected \\n");
        return;
    }
    if (check(TokenType::Dedent) || check(TokenType::Eof)) {
        return;
    }
    throw std::runtime_error("Expected end of statement");
    return stmt;
}

// * Variable Declaration
/// @brief Parses variable declaration statement
///
/// Grammar:
///     var_decl ::= ["final"] identifier ":" identifier (type_name) "=" expr
///
/// Consumes:
///     - optional `KwFinal`
///     - Identifier (variable name)
///     - Colon
///     - Identifier (type name)
///     - Operator('=')
///     - expression
///
/// @return A VarDecl statement AST node.
StmtUP Parser::parseVarDecl() {
    VarDecl varDecl = VarDecl{};

    // handle optionally declaring variable final (const)
    if (check(TokenType::KwFinal)) {
        varDecl.isFinal = true;
        consume(TokenType::KwFinal, "Final variable must be declared as final");
    } else {
        varDecl.isFinal = false;
    }
    varDecl.name = Identifier{ consume(TokenType::Identifier, "Expected variable name declaration").lexeme };
    consume(TokenType::Colon, "Expected colon after type delcaration");
    varDecl.typeExpr = parseTypeExpr();
    consume(TokenType::Assign, "Expected '=' after variable declaration");
    varDecl.init = parseExpr(0);
    return std::make_unique<Stmt>(std::move(varDecl));
}


// * For Loop
/// @brief Parses for loop statement
///
/// Grammar:
///     for [binding] in [expr]:
///         [block]
///
/// Consumes:
///     - KwFor
///     - Binding
///     - KwIn
///     - Expr
///     - Colon
///     - Newline
///     - Indent
///     - Block
///     - Dedent
///
/// @return A ForIn statement AST node.
StmtUP Parser::parseForIn() {
    ForIn forIn{};

    consume(TokenType::KwFor, "For loop must start with keyword 'for'");
    forIn.binding = parseBinding();
    consume(TokenType::KwIn, "Expected keyword 'in'");
    // TODO: add functionality for tuple iteratable (to match binding)
    forIn.iterable = parseExpr(0);
    consume(TokenType::Colon, "Expected ':' after expression");
    consumeAtLeastOneNewline("Expected \\n after for loop statement");

    return std::make_unique<Stmt>(std::move(forIn));
}

// * Binding Parsers (for the for loop)
/// @brief Parses binding (tuple)
///
/// Grammar:
///     var_binding ::= Identifier ':' Identifier
///
/// Consumes:
///     - LParen
///     - Var Binding
///     - RParen (to close) || Comma (to continue)
///     - Var Binding
///     - ...
///
/// @return A Binding AST node.
Binding Parser::parseBinding() {
    Binding binding{};
    consume(TokenType::LParen, "Expected '(' at binding start");
    // at least one var binding
    binding.varBindings.push_back(parseVarBinding()); // avoid copy via rvalue

    while (check(TokenType::Comma)) {
        advance();
        binding.varBindings.push_back(parseVarBinding());
    }

    consume(TokenType::RParen, "Expected ')' to close binding");
    return binding;
}

/// @brief Parses singular variable binding
///
/// Grammar:
///     var_binding ::= Identifier ':' Identifier
///
/// Consumes:
///     - Identifier
///     - Colon
///     - Identifier
///
/// @return A VarBinding AST node.
VarBinding Parser::parseVarBinding() {
    VarBinding vb{};
    vb.name = Identifier{ consume(TokenType::Identifier, "Expected variable name").lexeme };
    consume(TokenType::Colon, "Expected ':' after variable name");
    vb.typeExpr = parseTypeExpr();
    return vb;
}


// * While loop
/// @brief Parses while loop statement
///
/// Grammar:
///     while [expr]:
///         [block]
///
/// Consumes:
///     - KwWhile
///     - Expr
///     - Colon
///     - Newline
///     - Indent
///     - Block
///     - Dedent
///
/// @return A While statement AST node.
StmtUP Parser::parseWhile() {
    While whileLoop{};

    consume(TokenType::KwWhile, "While loop must start with keyword 'while'");
    whileLoop.cond = parseExpr(0);
    consume(TokenType::Colon, "Expected ':' after expression");
    consumeAtLeastOneNewline("Expected '\\n' after while statement");
    consume(TokenType::Indent, "Expected indent to begin while block");
    whileLoop.body = parseBlock(); 
    consume(TokenType::Dedent, "Expected dedent after while block");
    return std::make_unique<Stmt>(std::move(whileLoop));
}

// * Conditional statements (if, elif, else)
/// @brief Parses conditional statement
///
/// Grammar:
///     if [expr]:
///         [block]
///     elif [expr]: (optional)
///         [block]
///     ...
///     else: (optional)
///         [block]
/// Consumes:
///     - KwIf
///     - Expr
///     - Colon
///     - Newline
///     - Indent
///     - Block
///     - Dedent
///
/// @return A While statement AST node.
StmtUP Parser::parseBranch() {
    BrChain brChain{};
    // if
    brChain.ifBlock = parseConditionalBranch(TokenType::KwIf, "Expected 'if' keyword");

    // newlines in between branch statements
    consumeOptionalNewlines();

    // between 0 and arbitrary elif branches
    while (check(TokenType::KwElif)) {
        // newlines in between branch statements
        consumeOptionalNewlines();
        brChain.elifBranches.push_back(parseConditionalBranch(TokenType::KwElif, "Expected 'elif"));
    }
    // newlines in between branch statements
    consumeOptionalNewlines();

    if (check(TokenType::KwElse)) {
        consume(TokenType::KwElse, "Expected 'else' but found other token");
        consume(TokenType::Colon, "Expected ':' after else");
        brChain.elseBlock = parseBlock();
    }
    return std::make_unique<Stmt>(std::move(brChain));
}

// Parse branch helper
Branch Parser::parseConditionalBranch(TokenType kw, const char* context) {
    Branch b{};
 
    consume(kw, context);
    b.cond = parseExpr(0);
    consume(TokenType::Colon, "Expected ':'");
    b.body = parseBlock();

    return b;
}

// * Return parser
StmtUP Parser::parseReturn() {
    Return ret{};
    consume(TokenType::KwReturn, "Return statement must start with 'return'");
    ret.retExpr = parseExpr(0);
    return std::make_unique<Stmt>(std::move(ret));
}

// ** Expression Statement Parser
StmtUP Parser::parseExprStmt() {
    ExprStmt exprStmt{};
    exprStmt.expr = parseExpr(0);
    return std::make_unique<Stmt>(std::move(exprStmt));
}

// ** Expression Parser (Pratt Parser)
ExprUP Parser::parseTypeExpr() {
    return parseExpr(0);
}

ExprUP Parser::parseExpr(int minBP) {
    Token token = advance();
    ExprUP left = nud(token);

    while (op_precedence::leftBindingPower(peek()) > minBP) {
        token = advance();
        left = led(token, std::move(left));
    }
    return left;
}

// already consume token in parseExpr
ExprUP Parser::nud(const Token& token) {
    switch (token.type) {
        // Literals
        case TokenType::Identifier: 
            return std::make_unique<Expr>(Identifier{token.lexeme});
        case TokenType::IntLit:
            return std::make_unique<Expr>(IntLit{token.lexeme});
        case TokenType::FloatLit:
            return std::make_unique<Expr>(FloatLit{token.lexeme});
        case TokenType::StringLit:
            return std::make_unique<Expr>(StringLit{token.lexeme});
        case TokenType::BoolLit:
            return std::make_unique<Expr>(BoolLit{token.lexeme});
        
        // List literal
        case TokenType::LBracket: {
            ListLit list{};
            while (!check(TokenType::RBracket) && !check(TokenType::Eof)) {
                list.elements.push_back(parseExpr(0));
                if (check(TokenType::RBracket)) {
                    break;
                } else {
                    consume(TokenType::Comma, "Expected ','");
                }
            }
            consume(TokenType::RBracket, "Expected ']' to close list");
            return std::make_unique<Expr>(std::move(list));
        }

        // grouping
        case TokenType::LParen: {
            ExprUP groupedExpr = parseExpr(0); // zero because basically new expr
            consume(TokenType::RParen, "expected ')' to close expression");
            return groupedExpr; // parens don't create a node, only delegate order
        }
        
        // prefix operators
        case TokenType::Operator: {
            OperatorKind opKind = token.op.value(); // throws if null
            if (op_precedence::isPrefixOp(opKind)) {
                throw std::runtime_error("Invalid prefix operator");
            }
            
            ExprUP right = parseExpr(op_precedence::prefixBindingPower(token));
            return std::make_unique<Expr>(Unary{
                op_precedence::toUnaryOp(opKind), std::move(right)
            });
        }        

        default: 
            std::cout << (int) token.type << std::endl; // TODO: Debug
            throw std::runtime_error("Unexpected token at start of expression");
    }
}


ExprUP Parser::led(const Token& token, ExprUP left) {
    switch (token.type) {
        // Assignment
        case TokenType::Assign: {
            if (!std::holds_alternative<Identifier>(*left)) {
                throw std::runtime_error("Invalid assignment target");
            }
            Identifier target = std::get<Identifier>(*left);
            int assignBP = op_precedence::getAssignBindingPower();
            ExprUP value = parseExpr(assignBP - 1);  // Right-associative
            return std::make_unique<Expr>(Assignment{
                std::move(target),
                std::move(value)
            });
        }
        
        
        // Indexing: a[0]
        case TokenType::LBracket: {
            std::vector<ExprUP> indexExpr;

            // at least index
            indexExpr.push_back(parseExpr(0));
            while (check(TokenType::Comma)) {
                consume(TokenType::Comma, "Expected comma to continue index list");
                indexExpr.push_back(parseExpr(0));
            }
            consume(TokenType::RBracket, "expected ']'");
            return std::make_unique<Expr>(
                Indices{std::move(left), std::move(indexExpr)}
            );
        }
        
        // handle function calls in expressions
        case TokenType::LParen: {
            // LParen is already consumed in function 'led'
            std::vector<ExprUP> args;
            if (!check(TokenType::RParen)) {
                while (true) {
                    args.push_back(parseExpr(0));
                    if (check(TokenType::RParen)) break;
                    consume(TokenType::Comma, "expected ',' or ')' in argument list");
                }
            }
            consume(TokenType::RParen, "expected ')' to close argument list");
            return std::make_unique<Expr>(
                Call{ std::move(left), std::move(args) }
            );
        }

        // Binary operators
        case TokenType::Operator: {
            OperatorKind opKind = token.op.value();
            if (op_precedence::isInfixOp(opKind)) {
                ExprUP right = parseExpr(op_precedence::rightBindingPower(token));
                return std::make_unique<Expr>(
                    Binary{op_precedence::toBinaryOp(opKind), std::move(left), std::move(right)}
                );
            }
            throw std::runtime_error("Unknown operator");
        }
        
        default: {
            throw std::runtime_error("Unexpected token in infix/postfix position");
        }
    }
}





}