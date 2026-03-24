#include "parser.h"

#include <vector>
#include <functional>
#include "../token/tokens.h"
#include "operator_precedence.h"

namespace alder::parser {

using namespace alder::ast;
using namespace alder::token;

// ** Public API **

Parser::Parser(std::vector<Token> tokens) {
    tokens_ = std::move(tokens);
    current_ = 0;
}

Module Parser::parse() {
    Module module = Module{std::vector<Decl>{}};
    consumeOptionalNewlines();
    while (!atEnd())
        module.declarations.push_back(parseDecl());
    return module;
}

// ** Public token handlers **

bool Parser::atEnd() const {
    return tokens_[current_].type == TokenType::Eof;
}

const Token& Parser::peek() const {
    return tokens_[current_];
}

// ** Private token handlers **

const Token& Parser::peekNext() const {
    if (current_ + 1 >= tokens_.size())
        throw error("unexpected end of input");
    return tokens_[current_ + 1];
}

const Token& Parser::advance() {
    if (atEnd())
        throw error("unexpected end of input");
    return tokens_[current_++];
}

bool Parser::check(TokenType t) const {
    return tokens_[current_].type == t;
}

const Token& Parser::consume(TokenType t, const char* msg) {
    if (check(t)) return advance();
    throw error(msg);
}

// ** Error helpers **

diag::CompileError Parser::error(const std::string& msg) const {
    return errorAt(peek(), msg);
}

diag::CompileError Parser::errorAt(const Token& tok, const std::string& msg) const {
    return diag::CompileError(msg, {tok.line, tok.col});
}

// ** Newline handlers **

void Parser::consumeOptionalNewlines() {
    while (check(TokenType::Newline) && !check(TokenType::Eof))
        advance();
}

void Parser::consumeNewline(const char* msg) {
    if (check(TokenType::Newline))
        advance();
    else
        throw error(msg);
}

void Parser::consumeAtLeastOneNewline(const char* msg) {
    consumeNewline(msg);
    consumeOptionalNewlines();
}

// ** Top-level declarations **

Decl Parser::parseDecl() {
    Decl decl{};
    if (check(TokenType::KwFn))
        decl = parseFunc();
    else if (check(TokenType::KwDef))
				decl = parseMacro();
		else
        decl = parseGlobalStmt();
    consumeOptionalNewlines();
    return decl;
}

Func Parser::parseFunc() {
    Func fn{};
    consume(TokenType::KwFn, "expected 'fn' to start function declaration");
    fn.name = Identifier{ consume(TokenType::Identifier, "expected function name").lexeme };
    consume(TokenType::LParen, "expected '(' after function name");

    while (!check(TokenType::RParen) && !check(TokenType::Eof)) {
        Param p{};
        p.name = Identifier{ consume(TokenType::Identifier, "expected parameter name").lexeme };
        consume(TokenType::Colon, "expected ':' after parameter name");
        p.type = parseTypeExpr();
        fn.params.push_back(std::move(p));
        if (check(TokenType::Comma)) advance();
    }
    consume(TokenType::RParen, "expected ')' to close parameter list");
    consume(TokenType::Arrow, "expected '->' after parameters");
    fn.retTypeExpr = parseTypeExpr();
    consume(TokenType::Colon, "expected ':' after return type");
    fn.body = parseBlock();
    return fn;
}

GlobalStmt Parser::parseGlobalStmt() {
    return GlobalStmt{ parseStmt() };
}

Macro Parser::parseMacro() {
		Macro macro{};
		consume(TokenType::KwDef, "expected 'def' to declare macro");
		macro.name = Identifier{ consume(TokenType::Identifier, "expected macro name").lexeme };
		macro.value = parseExpr(0);
		return macro;
}

// ** Statement parsers **

BlockUP Parser::parseBlock() {
    Block block{};
    consumeAtLeastOneNewline("expected newline to start block");
    consume(TokenType::Indent, "expected indent");
    consumeOptionalNewlines();
    while (!check(TokenType::Dedent) && !check(TokenType::Eof)) {
        block.body.push_back(parseStmt());
        consumeOptionalNewlines();
    }
    consume(TokenType::Dedent, "expected dedent to close block");
    return std::make_unique<Block>(std::move(block));
}

StmtUP Parser::parseStmt() {
    if (check(TokenType::KwReturn)) return parseReturn();
    if (check(TokenType::KwIf))     return parseBranch();
    if (check(TokenType::KwWhile))  return parseWhile();
    if (check(TokenType::KwFor))    return parseForIn();

    bool startsVarDecl = check(TokenType::Identifier) && peekNext().type == TokenType::Colon;
    if (check(TokenType::KwFinal) || startsVarDecl)
        return parseVarDecl();

    return parseExprStmt();
}

StmtUP Parser::parseVarDecl() {
    VarDecl varDecl{};
    if (check(TokenType::KwFinal)) {
        varDecl.isFinal = true;
        advance();
    }
    varDecl.name = Identifier{ consume(TokenType::Identifier, "expected variable name").lexeme };
    consume(TokenType::Colon, "expected ':' after variable name");
    varDecl.typeExpr = parseTypeExpr();
    consume(TokenType::Assign, "expected '=' in variable declaration");
    varDecl.init = parseExpr(0);
    return std::make_unique<Stmt>(std::move(varDecl));
}

StmtUP Parser::parseForIn() {
    ForIn forIn{};
    consume(TokenType::KwFor, "expected 'for'");
    forIn.binding = parseBinding();
    consume(TokenType::KwIn, "expected 'in' after binding");
    forIn.iterable = parseExpr(0);
    consume(TokenType::Colon, "expected ':' after iterable");
    forIn.body = parseBlock();
    return std::make_unique<Stmt>(std::move(forIn));
}

Binding Parser::parseBinding() {
    Binding binding{};
    consume(TokenType::LParen, "expected '(' to start binding");
    binding.varBindings.push_back(parseVarBinding());
    while (check(TokenType::Comma)) {
        advance();
        binding.varBindings.push_back(parseVarBinding());
    }
    consume(TokenType::RParen, "expected ')' to close binding");
    return binding;
}

VarBinding Parser::parseVarBinding() {
    VarBinding vb{};
    vb.name = Identifier{ consume(TokenType::Identifier, "expected variable name in binding").lexeme };
    consume(TokenType::Colon, "expected ':' after variable name");
    vb.typeExpr = parseTypeExpr();
    return vb;
}

StmtUP Parser::parseWhile() {
    While whileLoop{};
    consume(TokenType::KwWhile, "expected 'while'");
    whileLoop.cond = parseExpr(0);
    consume(TokenType::Colon, "expected ':' after condition");
    whileLoop.body = parseBlock();
    return std::make_unique<Stmt>(std::move(whileLoop));
}

StmtUP Parser::parseBranch() {
    BrChain brChain{};
    brChain.ifBlock = parseConditionalBranch(TokenType::KwIf, "expected 'if'");
    consumeOptionalNewlines();

    while (check(TokenType::KwElif)) {
        consumeOptionalNewlines();
        brChain.elifBranches.push_back(parseConditionalBranch(TokenType::KwElif, "expected 'elif'"));
    }
    consumeOptionalNewlines();

    if (check(TokenType::KwElse)) {
        consume(TokenType::KwElse, "expected 'else'");
        consume(TokenType::Colon, "expected ':' after 'else'");
        brChain.elseBlock = parseBlock();
    }
    return std::make_unique<Stmt>(std::move(brChain));
}

Branch Parser::parseConditionalBranch(TokenType kw, const char* context) {
    Branch b{};
    consume(kw, context);
    b.cond = parseExpr(0);
    consume(TokenType::Colon, "expected ':' after condition");
    b.body = parseBlock();
    return b;
}

StmtUP Parser::parseReturn() {
    Return ret{};
    consume(TokenType::KwReturn, "expected 'return'");
    ret.retExpr = parseExpr(0);
    return std::make_unique<Stmt>(std::move(ret));
}

StmtUP Parser::parseExprStmt() {
    ExprStmt exprStmt{};
    exprStmt.expr = parseExpr(0);
    return std::make_unique<Stmt>(std::move(exprStmt));
}

// ** Expression parser (Pratt) **

ExprUP Parser::parseTypeExpr() {
    return parseExpr(op_precedence::getAssignBindingPower());
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

ExprUP Parser::nud(const Token& token) {
    switch (token.type) {
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

        case TokenType::LBracket: {
            ListLit list{};
            while (!check(TokenType::RBracket) && !check(TokenType::Eof)) {
                list.elements.push_back(parseExpr(0));
                if (check(TokenType::RBracket)) break;
                consume(TokenType::Comma, "expected ',' between list elements");
            }
            consume(TokenType::RBracket, "expected ']' to close list");
            return std::make_unique<Expr>(std::move(list));
        }

        case TokenType::LParen: {
            ExprUP grouped = parseExpr(0);
            consume(TokenType::RParen, "expected ')' to close grouped expression");
            return grouped;
        }

        case TokenType::Operator: {
            OperatorKind opKind = token.op.value();
            if (!op_precedence::isPrefixOp(opKind))
                throw errorAt(token, "'" + token.lexeme + "' is not a valid prefix operator");
            ExprUP right = parseExpr(op_precedence::prefixBindingPower(token));
            return std::make_unique<Expr>(Unary{op_precedence::toUnaryOp(opKind), std::move(right)});
        }

        default:
            throw errorAt(token, "unexpected token '" + token.lexeme + "'");
    }
}

ExprUP Parser::led(const Token& token, ExprUP left) {
    switch (token.type) {
        case TokenType::Assign: {
            ExprUP value = parseExpr(op_precedence::getAssignBindingPower() - 1);
            return std::make_unique<Expr>(Assignment{std::move(left), std::move(value)});
        }

        case TokenType::LBracket: {
            // Parse optional start expression (omitted in e.g. nums[:5])
            ExprUP first = nullptr;
            if (!check(TokenType::Colon) && !check(TokenType::RBracket))
                first = parseExpr(0);

            if (check(TokenType::Colon)) {
                // Slice: target[start:stop]
                advance(); // consume ':'
                ExprUP stop = nullptr;
                if (!check(TokenType::RBracket))
                    stop = parseExpr(0);
                consume(TokenType::RBracket, "expected ']' to close slice");
                return std::make_unique<Expr>(Slice{std::move(left), std::move(first), std::move(stop)});
            }

            // Regular index
            std::vector<ExprUP> indices;
            if (first) indices.push_back(std::move(first));
            while (check(TokenType::Comma)) {
                advance();
                indices.push_back(parseExpr(0));
            }
            consume(TokenType::RBracket, "expected ']' to close index");
            return std::make_unique<Expr>(Indices{std::move(left), std::move(indices)});
        }

        case TokenType::LParen: {
            std::vector<ExprUP> args;
            if (!check(TokenType::RParen)) {
                while (true) {
                    args.push_back(parseExpr(0));
                    if (check(TokenType::RParen)) break;
                    consume(TokenType::Comma, "expected ',' or ')' in argument list");
                }
            }
            consume(TokenType::RParen, "expected ')' to close argument list");
            return std::make_unique<Expr>(Call{std::move(left), std::move(args)});
        }

        case TokenType::Operator: {
            OperatorKind opKind = token.op.value();
            if (!op_precedence::isInfixOp(opKind))
                throw errorAt(token, "'" + token.lexeme + "' cannot be used as a binary operator");
            ExprUP right = parseExpr(op_precedence::rightBindingPower(token));
            return std::make_unique<Expr>(Binary{op_precedence::toBinaryOp(opKind), std::move(left), std::move(right)});
        }

        default:
            throw errorAt(token, "unexpected token '" + token.lexeme + "' in expression");
    }
}

} // namespace alder::parser
