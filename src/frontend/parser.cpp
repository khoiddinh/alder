
#include <vector>
#include <functional>
#include <iostream> // TODO: DEBUG
#include "tokens.h"
#include "parser.h"

namespace alder {

Parser::Parser(std::vector<Token> tokens) {
    tokens_ = std::move(tokens);
    current_ = 0;
}

Module Parser::parse() {
    Module module = Module{std::vector<Item>{}};
    // rm leading newlines
    consumeOptionalNewlines();

    while (!atEnd()) {
        if (check(TokenType::KwDef)) { // function
            Func fn = parseFunc();
            module.items.emplace_back(std::move(fn));
        } else { // statement
            StmtUP stmt = parseStmt();
            module.items.emplace_back(std::move(*stmt));
        }
        // rm trailing newlines
        consumeOptionalNewlines();
    }
    return module;
}


BlockUP Parser::parseBlock() { // TODO: consider newline edge cases in inline while case; while { \nint: x = 1 \n}
    consume(TokenType::LBrace, "Expected '{' to start block");
    consumeNewline("Must have \\n (newline) character after '{' in block statement"); // require at least one leading newline
    consumeOptionalNewlines(); // rm opt leading newline
    Block block{};
    std::vector<StmtUP> blockBody{};
    while(!check(TokenType::RBrace)) { // TODO: add additional checks to break & error before EOF
        // parse statement 
        StmtUP stmt = parseStmt();
        blockBody.emplace_back(std::move(stmt));
    }
    block.body = std::move(blockBody);
    consume(TokenType::RBrace, "Expected '}' to end block");
    return std::make_unique<Block>(std::move(block));
}

StmtUP Parser::parseStmt() {
    StmtUP stmt;
    if (isTypeToken(peek().type)) { // starts with type -> must be var decl
        stmt = parseVarDecl();
    } else if (check(TokenType::KwFinal)) { // starts with final -> var decl
        stmt = parseVarDecl();
    } else if (check(TokenType::KwFor)) {
        stmt = parseForIn();
    } else if (check(TokenType::KwWhile)) {
        stmt = parseWhile();
    } else if (check(TokenType::KwIf)) { // must start with if (elif, else optional)
        stmt = parseBranch();
    } else if (check(TokenType::KwReturn)) { // should only be in func def block
        stmt = parseReturn();
    } else if (check(TokenType::Identifier)) {
        // either expression stmt or assignment
        stmt = parseAssignOrExprStmt();
    } else {
        throw std::runtime_error("Tried to parse invalid statement starting with token: " + std::to_string((int) peek().type));
    }
    if (!check(TokenType::Eof) && !check(TokenType::RBrace)) { // don't consume newline if at end of file
        consumeNewline("Statement must be end delimited by \\n (newline)"); // consume at least one newline
    }
    // remove additional trailing newlines
    consumeOptionalNewlines();
    return stmt;
}


Func Parser::parseFunc() {
    Func func = Func{};
    consume(TokenType::KwDef, "Expected function declarator");
    func.name = consume(TokenType::Identifier, "Expected function name after def").lexeme;
    consume(TokenType::LParen, "Expected '(' after function name");
    // read params
    std::vector<Param> params{};
    while (!check(TokenType::RParen)) { // TODO: add additional checks to break & error before EOF
        // parse one whole argument
        Param param = Param{};
        param.type = toTypeName(consumeGeneric(isTypeToken, "Expected argument type").type);
        consume(TokenType::Colon, "Expected colon after argument type");
        param.name = consume(TokenType::Identifier, "Expected argument name").lexeme;

        params.push_back(param);
        // if statement reads through comma if it exists no matter what
        if (consumeOptional(TokenType::Comma) && check(TokenType::RParen)) { 
            // comma case foo(int: arg1, int: arg2,) -> should error
            throw std::runtime_error("Expected additional argument but found none");
        }
        
    }
    // consume RParen
    consume(TokenType::RParen, "Missing closing ')' in function declaration");
    // assign params list
    func.params = params; 

    // parse return type
    consume(TokenType::Arrow, "Expected '->' after param declaration");
    func.retType = toTypeName(consumeGeneric(isTypeToken, "Expected return type").type);

    // optional newlines after type declaration before block (e.g. def f(..) -> [type] \n { ... })
    consumeOptionalNewlines();
    // parse block
    func.body = std::move(parseBlock());
    return func;
}


StmtUP Parser::parseVarDecl() {
    VarDecl varDecl = VarDecl{};

    // handle optionally declaring variable final (const)
    if (check(TokenType::KwFinal)) {
        varDecl.isFinal = true;
        consume(TokenType::KwFinal, "Final variable must be declared as final");
    } else {
        varDecl.isFinal = false;
    }
    varDecl.type = toTypeName(consumeGeneric(isTypeToken, "Expected type declaration").type);
    consume(TokenType::Colon, "Expected colon after type delcaration");
    varDecl.name = consume(TokenType::Identifier, "Expected variable name after type").lexeme;
    consume(TokenType::Assign, "Expected '=' after variable declaration");
    varDecl.init = parseExpr(0);
    return std::make_unique<Stmt>(std::move(varDecl));
}

StmtUP Parser::parseForIn() {
    ForIn forIn{};
    consume(TokenType::KwFor, "For loop must start with keyword 'for'");
    forIn.type = toTypeName(consumeGeneric(isTypeToken, "Expected type declaration").type);
    consume(TokenType::Colon, "Expected colon after type declaration");
    forIn.name = consume(TokenType::Identifier, "Expected variable name after type specifier").lexeme;
    consume(TokenType::KwIn, "Expected keyword in after variable name");
    forIn.iterable = parseExpr(0); // doesn't check if iterable
    consumeOptionalNewlines(); // allow newline between expr and block
    forIn.body = parseBlock(); 
    return std::make_unique<Stmt>(std::move(forIn));
}

StmtUP Parser::parseWhile() {
    While whileLoop{};
    consume(TokenType::KwWhile, "While loop must start with keyword 'while'");
    whileLoop.cond = parseExpr(0); // doesn't check validity
    consumeOptionalNewlines(); // can have newlines between expr and start of block
    whileLoop.body = parseBlock(); 
    return std::make_unique<Stmt>(std::move(whileLoop));
}

StmtUP Parser::parseAssign() {
    Assign assign{};
    assign.name = consume(TokenType::Identifier, "Expected variable name for variable assignment").lexeme;
    consume(TokenType::Assign, "Expected '=' after varable for assignment");
    assign.value = parseExpr(0);
    return std::make_unique<Stmt>(std::move(assign));
}

StmtUP Parser::parseBranch() {
    BrChain brChain{};
    // if
    consume(TokenType::KwIf, "Expected 'if' to start conditional");
    BrChain::Branch first{};
    first.cond = parseExpr(0);

    consumeOptionalNewlines(); // can have newlines between expr and start of block

    first.body = parseBlock();
    brChain.branches.emplace_back(std::move(first));
    // newlines in between branch statements
    consumeOptionalNewlines();
    // between 0 and arbitrary elif branches
    while (check(TokenType::KwElif)) {
        // newlines in between branch statements
        consumeOptionalNewlines();
        consume(TokenType::KwElif, "Expected keyword elif");
        BrChain::Branch currBranch{};
        currBranch.cond = parseExpr(0);
        consumeOptionalNewlines(); // can have newlines between expr and start of block 
        currBranch.body = parseBlock();
        brChain.branches.emplace_back(std::move(currBranch));
    }
    // newlines in between branch statements
    consumeOptionalNewlines();

    if (check(TokenType::KwElse)) {
        consume(TokenType::KwElse, "Expected 'else' but found other token");
        consumeOptionalNewlines(); // allow newline between else and block
        brChain.elseBlock = parseBlock();
    }
    return std::make_unique<Stmt>(std::move(brChain));
}

StmtUP Parser::parseReturn() {
    Return ret{};
    consume(TokenType::KwReturn, "Return statement must start with 'return'");
    ret.retExpr = parseExpr(0);
    return std::make_unique<Stmt>(std::move(ret));
}

StmtUP Parser::parseExprStmt() {
    ExprStmt exprStmt{};
    exprStmt.expr = parseExpr(0);
    return std::make_unique<Stmt>(std::move(exprStmt));
}


inline bool isValidExprEndToken(TokenType tokType) {
    // all valid expr terminations
    return (tokType == TokenType::Newline || tokType == TokenType::Eof ||  // end statement || eof
        tokType == TokenType::LBrace || tokType == TokenType::Comma);  // stmt { ...} || arg list: expr, ... rest of args
}

// TODO: test nested groupings e.g. ((1+2)-3)
ExprUP Parser::parseExpr(int minBP) {
    Token token = advance();
    ExprUP left = nud(token);

    // don't consume \n, expr parsing \n terminated
    while (!isValidExprEndToken(peek().type) && leftBindingPower(peek().type) > minBP) {
        token = advance();
        left = led(token, std::move(left));
    }
    return left;
}

StmtUP Parser::parseAssignOrExprStmt() {
    if (peekNext().type == TokenType::Assign) {
        return parseAssign();
    } else {
        return parseExprStmt();
    }
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
        
        // prefix operators
        case TokenType::Minus: {
            ExprUP right = parseExpr(prefixBindingPower(token.type)); // property of operator, not identifier token
            return std::make_unique<Expr>(Unary{UnaryOp::Neg, std::move(right)});
        }
        case TokenType::KwNot: {
            ExprUP right = parseExpr(prefixBindingPower(token.type));
            return std::make_unique<Expr>(Unary{UnaryOp::Not, std::move(right)});
        }
        // grouping
        case TokenType::LParen: {
            ExprUP groupedExpr = parseExpr(0); // zero because basically new expr
            consume(TokenType::RParen, "expected ')' to close expression");
            return groupedExpr; // parens don't create a node, only delegate order
        }

        default: 
            std::cout << (int) token.type << std::endl; // TODO: Debug
            throw std::runtime_error("Unexpected token at start of expression");
    }
}

// inline helpers
inline ExprUP makeBinary(BinaryOp op, ExprUP lhs, ExprUP rhs) {
    return std::make_unique<Expr>(Binary{op, std::move(lhs), std::move(rhs)});
}
inline ExprUP makeCall(ExprUP callee, std::vector<ExprUP> args) {
    return std::make_unique<Expr>(Call{ std::move(callee), std::move(args) });
}

ExprUP Parser::led(const Token& token, ExprUP left) {
    switch (token.type) {
        case TokenType::Plus: {
            ExprUP right = parseExpr(rightBindingPower(token.type));
            return makeBinary(BinaryOp::Add, std::move(left), std::move(right));
        }
        case TokenType::Minus: {
            ExprUP right = parseExpr(rightBindingPower(token.type));
            return makeBinary(BinaryOp::Sub, std::move(left), std::move(right));
        }
        case TokenType::Star: {
            ExprUP right = parseExpr(rightBindingPower(token.type));
            return makeBinary(BinaryOp::Mul, std::move(left), std::move(right));
        }
        case TokenType::Slash: {
            ExprUP right = parseExpr(rightBindingPower(token.type));
            return makeBinary(BinaryOp::Div, std::move(left), std::move(right));
        }
        case TokenType::Equals: {
            ExprUP right = parseExpr(rightBindingPower(token.type));
            return makeBinary(BinaryOp::Eq, std::move(left), std::move(right));
        }
        case TokenType::NotEq: {
            ExprUP right = parseExpr(rightBindingPower(token.type));
            return makeBinary(BinaryOp::NotEq, std::move(left), std::move(right));
        }
        case TokenType::KwAnd: {
            ExprUP right = parseExpr(rightBindingPower(token.type));
            return makeBinary(BinaryOp::And, std::move(left), std::move(right));
        }
        case TokenType::KwOr: {
            ExprUP right = parseExpr(rightBindingPower(token.type));
            return makeBinary(BinaryOp::Or, std::move(left), std::move(right));
        }
        case TokenType::LEqCompare: {
            ExprUP right = parseExpr(rightBindingPower(token.type));
            return makeBinary(BinaryOp::LEqComp, std::move(left), std::move(right));
        }
        case TokenType::GEqCompare: {
            ExprUP right = parseExpr(rightBindingPower(token.type));
            return makeBinary(BinaryOp::GEqComp, std::move(left), std::move(right));
        }
        case TokenType::LCompare: {
            ExprUP right = parseExpr(rightBindingPower(token.type));
            return makeBinary(BinaryOp::LComp, std::move(left), std::move(right));
        }
        case TokenType::GCompare: {
            ExprUP right = parseExpr(rightBindingPower(token.type));
            return makeBinary(BinaryOp::GComp, std::move(left), std::move(right));
        }
        // handle function calls in expressions
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
            return makeCall(std::move(left), std::move(args));
        }
        default: {
            throw std::runtime_error("Unexpected token in infix/postfix position");
        }
    }
}
int Parser::prefixBindingPower(const TokenType tokType) {
    switch(tokType) {
        case TokenType::Minus:
        case TokenType::KwNot:
            return 80;
        default:
            throw std::runtime_error("Token: " + std::to_string((int) tokType) + " cannot be prefix bound");
    }
}
int Parser::leftBindingPower(const TokenType tokType) {
    switch (tokType) {
        case TokenType::LParen:
            return 90;
        case TokenType::Star:
        case TokenType::Slash:
            return 70;
        case TokenType::Plus:
        case TokenType::Minus:
            return 60;
        case TokenType::LCompare:
        case TokenType::LEqCompare:
        case TokenType::GCompare:
        case TokenType::GEqCompare:
            return 50;
        case TokenType::Equals:   // ==
        case TokenType::NotEq:    // !=
            return 40;
        case TokenType::KwAnd:    // and
            return 30;
        case TokenType::KwOr:     // or
            return 20;
        case TokenType::RParen:   // )
            return 0; // no binding power
        default:
            throw std::runtime_error(
                "Invalid token found in expression; left binding power does not exist for token: " 
                + std::to_string((int) tokType));
    }
}

int Parser::rightBindingPower(const TokenType tokType) {
    switch (tokType) {
        case TokenType::Star:
        case TokenType::Slash:
            return 70; // left-assoc
        case TokenType::Plus:
        case TokenType::Minus:
            return 60; // left-assoc
        case TokenType::LCompare:   // <
        case TokenType::LEqCompare: // <=
        case TokenType::GCompare:   // >
        case TokenType::GEqCompare: // >=
            return 49; // non-assoc: slightly less than LBP(50)
        case TokenType::Equals:     // ==
        case TokenType::NotEq:      // !=
            return 39; // non-assoc: slightly less than LBP(40)
        case TokenType::KwAnd:    // and
            return 30;
        case TokenType::KwOr:     // or
            return 20;
        default:
            throw std::runtime_error("Right binding power does not exist for token");
    }
}
// Helpers
bool Parser::atEnd() const {
    return tokens_[current_].type == TokenType::Eof;
}

const Token& Parser::peek() const {
    return tokens_[current_];
}

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

const Token& Parser::consumeGeneric(std::function<bool(TokenType)> fn, const char* msg) {
    if (fn(peek().type)) {
        return advance();
    } else {
        throw std::runtime_error(msg);
    }
}


std::optional<Token> Parser::consumeOptional(TokenType t) {
    if (check(t)) {
        return advance();
    } else {
        return std::nullopt;
    }
}

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

// STMT helpers
bool Parser::isTypeToken(TokenType t) {
    switch (t) {
        case TokenType::KwInt:
        case TokenType::KwFloat:
        case TokenType::KwBool:
        case TokenType::KwChar:
        case TokenType::KwVoid:
            return true;
        default:
            return false;
    }
}

// Mappers

TypeName Parser::toTypeName(TokenType t) {
    switch (t) {
        case TokenType::KwInt: return TypeName{TypeKind::Int};
        case TokenType::KwFloat: return TypeName{TypeKind::Float};
        case TokenType::KwBool: return TypeName{TypeKind::Bool};
        case TokenType::KwChar: return TypeName{TypeKind::Char};
        case TokenType::KwVoid: return TypeName{TypeKind::Void};
        default:
            throw std::runtime_error("expected type token");
    }
}

}