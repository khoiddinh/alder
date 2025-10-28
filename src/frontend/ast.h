#pragma once

#include <variant>
#include <memory>
#include <string>
#include <vector>

enum class TypeKind { Float, Int, Bool, Char, Void };

struct TypeName {
    TypeKind kind;
};

enum class UnaryOp { Neg, Not }; // -x and not x
enum class BinaryOp { Mul, Div, Add, Sub, Eq , NotEq, And, Or, LEqComp, GEqComp, LComp, GComp}; // * / + - == != <= >= < >

struct Expr;
struct Stmt;
struct Block;

using ExprUP = std::unique_ptr<Expr>;
using StmtUP = std::unique_ptr<Stmt>;
using BlockUP = std::unique_ptr<Block>;

struct Identifier      { std::string name; };
struct IntLit    { std::string lexeme; };  
struct FloatLit  { std::string lexeme; };
struct StringLit { std::string value; };
struct BoolLit   { std::string value; };

struct Unary {
  UnaryOp op;
  ExprUP rhs;
};

struct Binary {
  BinaryOp op;
  ExprUP lhs;
  ExprUP rhs;
};

struct Call {
  ExprUP callee;
  std::vector<ExprUP> args;
};

struct Expr : std::variant<Identifier, IntLit, FloatLit, StringLit, BoolLit, Unary, Binary, Call> {
  using variant::variant;
};

// Statements
struct VarDecl {
  bool isFinal; // immutable
  TypeName type; 
  std::string name;
  ExprUP init;
};

struct ForIn {
  TypeName type;           // for type : name in expr { ... }
  std::string name;
  ExprUP iterable;
  BlockUP body;
};

struct While {
  ExprUP cond;
  BlockUP body;
};

struct Assign {
  std::string name;        // Identifier = expr
  ExprUP value;
};

struct BrChain {
  struct Branch { ExprUP cond; BlockUP body; };
  std::vector<Branch> branches; // first is the "if", rest are "elif"
  BlockUP elseBlock;            // optional, if empty then nullptr by default
};

struct Block {
  std::vector<StmtUP> body; // { stmt* }
};

struct Return { // should only be in blocks owned by function definitions
  ExprUP retExpr;
};

struct ExprStmt {
    ExprUP expr;
};

// Sum type for statements
struct Stmt : std::variant<Block, VarDecl, ForIn, While, Assign, BrChain, Return, ExprStmt> {
  using variant::variant;
};

// Declarations

struct Param {
  TypeName type; 
  std::string name;
};

struct Func {
  std::string name;
  std::vector<Param> params;
  TypeName retType;
  BlockUP body;
};

// Top-level item can be a function or a statement
struct Item : std::variant<Func, Stmt> {
  using variant::variant;
};

// Module
struct Module {
  std::vector<Item> items; // module := { item } Eof
};


