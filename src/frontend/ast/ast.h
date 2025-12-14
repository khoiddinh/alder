#pragma once

#include <variant>
#include <memory>
#include <string>
#include <vector>

namespace alder::ast {

// ** Module **
// * Top Level File Object

struct Module {
    std::vector<Decl> declarations;
};

// * Top-level declaration
// Can be a function or a statement
struct Decl : std::variant<GlobalStmt, Func> {
    using variant::variant;
};

// ** Global (Top-level) Statements
struct GlobalStmt {
    StmtUP stmt;
};

// ** Function Declaration **
struct Func {
    Identifier name;
    std::vector<Param> params;
    Identifier retType;
    BlockUP body; // function body
};

struct Param {
    Identifier name;
    Identifier type; 
};

// * Blocks (list of statements); Can't exist in global scope
struct Block;
using BlockUP = std::unique_ptr<Block>;
struct Block {
    std::vector<StmtUP> body;
};


// ** Statements **
struct Stmt;
using StmtUP = std::unique_ptr<Stmt>;
// Sum type for statements
struct Stmt : std::variant<Block, VarDecl, ForIn, While, BrChain, Return, ExprStmt> {
    using variant::variant;
};

// * Identifiers (types AND variables)
struct Identifier { std::string name; };


// * Statement types
struct VarDecl {
    bool isFinal; // immutable
    Identifier type; 
    Identifier name;
    ExprUP init;
};

// * Bindings (for loop)
struct VarBinding {
    Identifier name;
    Identifier type;
};

struct Binding {
    std::vector<VarBinding> varBindings;
};

struct ForIn {
    Binding binding;    // for binding in expr:
    ExprUP iterable;
    BlockUP body;
};


struct While {
    ExprUP cond;
    BlockUP body;
};

struct Branch { 
    ExprUP cond; 
    BlockUP body; 
};

struct BrChain {
    Branch ifBlock;// first is the "if"
    std::vector<Branch> elifBranches; // optional, could be empty
    BlockUP elseBlock;            // optional, if empty then nullptr by default
};

struct Return { // should only be in blocks owned by function definitions
    ExprUP retExpr;
};


// ** Expressions **
struct Expr;
using ExprUP = std::unique_ptr<Expr>;
struct Expr : std::variant<
    Identifier,
    IntLit,
    FloatLit,
    StringLit,
    BoolLit,
    ListLit,
    Unary,
    Binary,
    Call,
    Assignment,
    Index
> {
    using variant::variant;
};
// * Assignment (x = [expr])
struct Assignment {
    Identifier target;
    ExprUP value;
};
// list index a[0]; target[index]
struct Index {
    ExprUP target;
    ExprUP index;
};

// * expression types
struct Unary {
    UnaryOp op;
    ExprUP rhs;
};

struct Binary {
    BinaryOp op;
    ExprUP lhs;
    ExprUP rhs;
};

// * function call expression
struct Call {
    ExprUP callee;
    std::vector<ExprUP> args;
};

// ** Expression statements **
// (i.e. foo(5) (on its own line) vs. y = foo(5) which is stmt)
struct ExprStmt {
    ExprUP expr;
};

// ** Literals **
struct IntLit { std::string lexeme; };  
struct FloatLit { std::string lexeme; };
struct StringLit { std::string value; };
struct BoolLit { std::string value; };
struct ListLit {
    std::vector<ExprUP> elements;
};

// ** Operator enums **
enum class UnaryOp {
    Neg,        // -x
    Not,        // not x
};

enum class BinaryOp {
    // arithmetic
    Add,        // +
    Sub,        // -
    Mul,        // *
    Div,        // /

    // comparison
    Eq,         // ==
    NotEq,      // !=
    Lt,         // <
    Lte,        // <=
    Gt,         // >
    Gte,        // >=

    // logical
    And,        // and
    Or,         // or

    // bitwise / shifts (only if your language supports them)
    Shl,        // <<
    Shr,        // >>
    LogicShr,   // >>> (if you want it semantically distinct)
};

};










