#pragma once

#include <variant>
#include <memory>
#include <string>
#include <vector>

namespace alder::ast {

// ** Forward declarations **

struct Decl;
struct GlobalStmt;
struct Func;
struct Param;
struct Block;
struct Stmt;
struct Identifier;
struct VarDecl;
struct VarBinding;
struct Binding;
struct ForIn;
struct While;
struct Branch;
struct BrChain;
struct Return;
struct ExprStmt;
struct Expr;
struct IntLit;
struct FloatLit;
struct StringLit;
struct BoolLit;
struct ListLit;
struct Unary;
struct Binary;
struct Call;
struct Assignment;
struct Indices;
struct Slice;
enum class UnaryOp;
enum class BinaryOp;

using BlockUP = std::unique_ptr<Block>;
using StmtUP  = std::unique_ptr<Stmt>;
using ExprUP  = std::unique_ptr<Expr>;

// ** Operator enums **

enum class UnaryOp {
    Neg,        // -x
    Not,        // not x
};

enum class BinaryOp {
    // arithmetic
    Add, Sub, Mul, Div,
    // comparison
    Eq, NotEq, Lt, Lte, Gt, Gte,
    // logical
    And, Or,
    // bitwise / shifts
    Shl, Shr, LogicShr,
};

// ** Identifiers **

struct Identifier { std::string name; };

// ** Expressions **

struct IntLit    { std::string lexeme; };
struct FloatLit  { std::string lexeme; };
struct StringLit { std::string value;  };
struct BoolLit   { std::string value;  };

struct ListLit {
    std::vector<ExprUP> elements;
};

struct Unary {
    UnaryOp op;
    ExprUP  rhs;
};

struct Binary {
    BinaryOp op;
    ExprUP   lhs;
    ExprUP   rhs;
};

struct Call {
    ExprUP              callee;
    std::vector<ExprUP> args;
};

// x = expr  OR  a[i] = expr
struct Assignment {
    ExprUP target;
    ExprUP value;
};

// a[0]  or  a[0, 1]
struct Indices {
    ExprUP              target;
    std::vector<ExprUP> indices;
};

// a[start:stop]  — start and/or stop may be omitted (nullptr)
struct Slice {
    ExprUP target;
    ExprUP start;
    ExprUP stop;
};

struct Expr : std::variant<
    Identifier,
    IntLit, FloatLit, StringLit, BoolLit, ListLit,
    Unary, Binary, Call, Assignment, Indices, Slice
> {
    using variant::variant;
};

// ** Statements **

struct VarDecl {
    bool      isFinal = false;
    ExprUP    typeExpr;
    Identifier name;
    ExprUP    init;
};

struct VarBinding {
    Identifier name;
    ExprUP     typeExpr;
};

struct Binding {
    std::vector<VarBinding> varBindings;
};

struct ForIn {
    Binding  binding;
    ExprUP   iterable;
    BlockUP  body;
};

struct While {
    ExprUP  cond;
    BlockUP body;
};

struct Branch {
    ExprUP  cond;
    BlockUP body;
};

struct BrChain {
    Branch              ifBlock;
    std::vector<Branch> elifBranches;
    BlockUP             elseBlock;
};

struct Return {
    ExprUP retExpr;
};

struct ExprStmt {
    ExprUP expr;
};

struct Stmt : std::variant<VarDecl, ForIn, While, BrChain, Return, ExprStmt> {
    using variant::variant;
};

// ** Blocks **

struct Block {
    std::vector<StmtUP> body;
};

// ** Top-level declarations **

struct GlobalStmt {
    StmtUP stmt;
};

struct Param {
    Identifier name;
    ExprUP     type;
};

struct Func {
    Identifier          name;
    std::vector<Param>  params;
    ExprUP              retTypeExpr;
    BlockUP             body;
};

struct Decl : std::variant<GlobalStmt, Func> {
    using variant::variant;
};

// ** Module **

struct Module {
    std::vector<Decl> declarations;
};

} // namespace alder::ast
