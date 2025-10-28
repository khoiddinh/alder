// tests/test_parser.cpp
// Catch2 v3.x
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <vector>
#include <string>
#include <optional>
#include <variant>
#include <memory>

#include ".././src/frontend/tokens.h"
#include ".././src/frontend/ast.h"
#include ".././src/frontend/parser.h"

using namespace alder;
using Catch::Matchers::ContainsSubstring;

// ---------- Tiny token factory ----------
static Token T(TokenType tt, std::string lx = "") { return Token{tt, std::move(lx)}; }
// Append EOF
static std::vector<Token> eof(std::vector<Token> v) { v.push_back(T(TokenType::Eof)); return v; }

// ---------- Minimal AST helpers ----------
namespace astx {

static bool isBinary(const Expr& e, BinaryOp op) {
    if (auto p = std::get_if<Binary>(&e)) return p->op == op;
    return false;
}

static const Binary*     asBinary(const Expr& e)   { return std::get_if<Binary>(&e); }
static const Unary*      asUnary (const Expr& e)   { return std::get_if<Unary>(&e);  }
static const Identifier* asIdent (const Expr& e)   { return std::get_if<Identifier>(&e); }
static const IntLit*     asInt   (const Expr& e)   { return std::get_if<IntLit>(&e); }
static const FloatLit*   asFloat (const Expr& e)   { return std::get_if<FloatLit>(&e); }
static const StringLit*  asStr   (const Expr& e)   { return std::get_if<StringLit>(&e); }
static const BoolLit*    asBool  (const Expr& e)   { return std::get_if<BoolLit>(&e); }
static const Call*       asCall  (const Expr& e)   { return std::get_if<Call>(&e);   }

static const VarDecl*  asVarDecl(const Stmt& s) { return std::get_if<VarDecl>(&s); }
static const Assign*   asAssign (const Stmt& s) { return std::get_if<Assign>(&s); }
static const ForIn*    asForIn  (const Stmt& s) { return std::get_if<ForIn>(&s);  }
static const While*    asWhile  (const Stmt& s) { return std::get_if<While>(&s);  }
static const BrChain*  asBr     (const Stmt& s) { return std::get_if<BrChain>(&s);}
static const Return*   asReturn (const Stmt& s) { return std::get_if<Return>(&s); }
static const Block*    asBlock  (const Stmt& s) { return std::get_if<Block>(&s);  }

static const Func*     asFunc(const Item& it) { return std::get_if<Func>(&it); }
static const Stmt*     asStmt(const Item& it) { return std::get_if<Stmt>(&it); }

// Helper for ExprStmt (declared in ast.h)
static const ExprStmt* asExprStmt(const Stmt& s) { return std::get_if<ExprStmt>(&s); }

} // namespace astx


// ========= Smoke: empty module =========
TEST_CASE("Parser: empty module (just EOF)", "[parser][module]") {
    Parser p(eof({}));
    Module m = p.parse();
    REQUIRE(m.items.empty());
}

// ========= simple var decl (requires '= expr \\n') =========
TEST_CASE("Parser: simple var decl with initializer and newline", "[parser][stmt][vardecl]") {
    // int: x = 1 + 2 \n
    auto toks = eof({
        T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "x"),
        T(TokenType::Assign),
        T(TokenType::IntLit, "1"), T(TokenType::Plus), T(TokenType::IntLit, "2"),
        T(TokenType::Newline),
    });

    Parser p(toks);
    Module m = p.parse();

    REQUIRE(m.items.size() == 1);
    const Stmt* s = astx::asStmt(m.items[0]); REQUIRE(s);
    const VarDecl* v = astx::asVarDecl(*s);   REQUIRE(v);
    REQUIRE(v->isFinal == false);
    REQUIRE(v->type.kind == TypeKind::Int);
    REQUIRE(v->name == "x");
    REQUIRE(v->init != nullptr);

    REQUIRE(astx::isBinary(*v->init, BinaryOp::Add));
    const Binary* add = astx::asBinary(*v->init); REQUIRE(add != nullptr);
    REQUIRE(astx::asInt(*add->lhs.get()) != nullptr);
    REQUIRE(astx::asInt(*add->rhs.get()) != nullptr);
}

// ========= final var decl =========
TEST_CASE("Parser: final var decl (requires '= expr')", "[parser][stmt][vardecl]") {
    // final int: n = 1 \n
    auto toks = eof({
        T(TokenType::KwFinal),
        T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "n"),
        T(TokenType::Assign),
        T(TokenType::IntLit, "1"),
        T(TokenType::Newline),
    });

    Parser p(toks);
    Module m = p.parse();
    REQUIRE(m.items.size() == 1);
    const Stmt* s = astx::asStmt(m.items[0]); REQUIRE(s);
    const VarDecl* v = astx::asVarDecl(*s);   REQUIRE(v);
    REQUIRE(v->isFinal == true);
    REQUIRE(v->type.kind == TypeKind::Int);
    REQUIRE(v->name == "n");
}

// ========= precedence & associativity =========
TEST_CASE("Parser: Pratt precedence (* over +)", "[parser][expr]") {
    // int: x = 1 + 2 * 3 \n
    auto toks = eof({
        T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "x"),
        T(TokenType::Assign),
        T(TokenType::IntLit, "1"),
        T(TokenType::Plus),
        T(TokenType::IntLit, "2"),
        T(TokenType::Star),
        T(TokenType::IntLit, "3"),
        T(TokenType::Newline),
    });

    Parser p(toks);
    Module m = p.parse();

    const VarDecl* v = astx::asVarDecl(*astx::asStmt(m.items[0]));
    REQUIRE(v != nullptr);
    const Binary* add = astx::asBinary(*v->init);
    REQUIRE(add != nullptr);
    REQUIRE(add->op == BinaryOp::Add);

    const Binary* mul = astx::asBinary(*add->rhs);
    REQUIRE(mul != nullptr);
    REQUIRE(mul->op == BinaryOp::Mul);

    REQUIRE(astx::asInt(*add->lhs.get()) != nullptr);
    REQUIRE(astx::asInt(*mul->lhs.get()) != nullptr);
    REQUIRE(astx::asInt(*mul->rhs.get()) != nullptr);
}

TEST_CASE("Parser: unary prefix binds tight", "[parser][expr]") {
    // int: x = -1 * 2 \n   => ((-1) * 2)
    auto toks = eof({
        T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "x"),
        T(TokenType::Assign),
        T(TokenType::Minus), T(TokenType::IntLit, "1"),
        T(TokenType::Star),
        T(TokenType::IntLit, "2"),
        T(TokenType::Newline),
    });

    Parser p(toks);
    Module m = p.parse();
    const VarDecl* v = astx::asVarDecl(*astx::asStmt(m.items[0]));
    REQUIRE(v != nullptr);

    const Binary* mul = astx::asBinary(*v->init);
    REQUIRE(mul != nullptr);
    REQUIRE(mul->op == BinaryOp::Mul);

    const Unary* neg = astx::asUnary(*mul->lhs);
    REQUIRE(neg != nullptr);
    REQUIRE(neg->op == UnaryOp::Neg);
    REQUIRE(astx::asInt(*neg->rhs) != nullptr);
}

// ========= comparison precedence =========
TEST_CASE("Parser: comparison has lower precedence than +", "[parser][expr]") {
    // int: x = 1 + 2 < 4 \n => ( (1+2) < 4 )
    auto toks = eof({
        T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "x"),
        T(TokenType::Assign),
        T(TokenType::IntLit, "1"),
        T(TokenType::Plus),
        T(TokenType::IntLit, "2"),
        T(TokenType::LCompare),
        T(TokenType::IntLit, "4"),
        T(TokenType::Newline),
    });

    Parser p(toks);
    Module m = p.parse();
    const VarDecl* v = astx::asVarDecl(*astx::asStmt(m.items[0]));
    REQUIRE(v != nullptr);

    const Binary* lt = astx::asBinary(*v->init);
    REQUIRE(lt != nullptr);
    REQUIRE(lt->op == BinaryOp::LComp);

    const Binary* add = astx::asBinary(*lt->lhs);
    REQUIRE(add != nullptr);
    REQUIRE(add->op == BinaryOp::Add);
}

// ========= simple assignment statement =========
TEST_CASE("Parser: assignment statement", "[parser][stmt][assign]") {
    // int: x = 1\n
    // x = 3 + 4\n
    auto toks = eof({
        T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "x"),
        T(TokenType::Assign), T(TokenType::IntLit, "1"), T(TokenType::Newline),

        T(TokenType::Identifier, "x"),
        T(TokenType::Assign),
        T(TokenType::IntLit, "3"), T(TokenType::Plus), T(TokenType::IntLit, "4"),
        T(TokenType::Newline),
    });

    Parser p(toks);
    Module m = p.parse();

    REQUIRE(m.items.size() == 2);
    const Assign* a = astx::asAssign(*astx::asStmt(m.items[1]));
    REQUIRE(a != nullptr);
    REQUIRE(a->name == "x");
    REQUIRE(astx::isBinary(*a->value, BinaryOp::Add));
}

// ========= while loop =========
TEST_CASE("Parser: while loop with block", "[parser][stmt][while]") {
    // while 1 \n { \n int: y = 2\n }
    auto toks = eof({
        T(TokenType::KwWhile),
        T(TokenType::IntLit, "1"),
        T(TokenType::Newline),
        T(TokenType::LBrace),
        T(TokenType::Newline),
          T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "y"),
          T(TokenType::Assign), T(TokenType::IntLit, "2"),
          T(TokenType::Newline),
        T(TokenType::RBrace),
    });

    Parser p(toks);
    Module m = p.parse();

    REQUIRE(m.items.size() == 1);
    const While* w = astx::asWhile(*astx::asStmt(m.items[0]));
    REQUIRE(w != nullptr);
    REQUIRE(w->cond != nullptr);
    REQUIRE(astx::asInt(*w->cond) != nullptr);
    REQUIRE(w->body != nullptr);
    REQUIRE(w->body->body.size() == 1);
    REQUIRE(astx::asVarDecl(*w->body->body[0]) != nullptr);
}

// ========= for-in =========
TEST_CASE("Parser: for-in loop", "[parser][stmt][forin]") {
    // for int: i in 1 { \n } (note: must be newline after '{')
    auto toks = eof({
        T(TokenType::KwFor),
        T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "i"),
        T(TokenType::KwIn),
        T(TokenType::IntLit, "1"),
        T(TokenType::LBrace),
        T(TokenType::Newline),
        T(TokenType::RBrace),
    });

    Parser p(toks);
    Module m = p.parse();

    REQUIRE(m.items.size() == 1);
    const ForIn* f = astx::asForIn(*astx::asStmt(m.items[0]));
    REQUIRE(f != nullptr);
    REQUIRE(f->type.kind == TypeKind::Int);
    REQUIRE(f->name == "i");
    REQUIRE(f->iterable != nullptr);
    REQUIRE(astx::asInt(*f->iterable) != nullptr);
    REQUIRE(f->body != nullptr);
    REQUIRE(f->body->body.empty());
}

// ========= if / elif / else =========
TEST_CASE("Parser: if / elif / else chain", "[parser][stmt][branch]") {
    // if 1 { \n int: a = 1\n } elif 2 { \n int: b = 2\n } else { \n int: c = 3\n }
    auto toks = eof({
        T(TokenType::KwIf), T(TokenType::IntLit, "1"),
        T(TokenType::LBrace),
        T(TokenType::Newline),
          T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "a"),
          T(TokenType::Assign), T(TokenType::IntLit, "1"), T(TokenType::Newline),
        T(TokenType::RBrace),

        T(TokenType::KwElif), T(TokenType::IntLit, "2"),
        T(TokenType::LBrace),
        T(TokenType::Newline),
          T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "b"),
          T(TokenType::Assign), T(TokenType::IntLit, "2"), T(TokenType::Newline),
        T(TokenType::RBrace),

        T(TokenType::KwElse),
        T(TokenType::LBrace),
        T(TokenType::Newline),
          T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "c"),
          T(TokenType::Assign), T(TokenType::IntLit, "3"), T(TokenType::Newline),
        T(TokenType::RBrace),
    });

    Parser p(toks);
    Module m = p.parse();

    REQUIRE(m.items.size() == 1);
    const BrChain* br = astx::asBr(*astx::asStmt(m.items[0]));
    REQUIRE(br != nullptr);
    REQUIRE(br->branches.size() == 2); // if + one elif
    REQUIRE(br->elseBlock != nullptr);

    // spot-check first branch body has VarDecl a
    REQUIRE(br->branches[0].body->body.size() == 1);
    const VarDecl* a = astx::asVarDecl(*br->branches[0].body->body[0]); REQUIRE(a != nullptr);
    REQUIRE(a->name == "a");
}

// ========= functions =========
TEST_CASE("Parser: function with params, return, and body", "[parser][func]") {
    // def add(int: a, int: b) -> int \n { \n return a + b \n}
    auto toks = eof({
        T(TokenType::KwDef), T(TokenType::Identifier, "add"),
        T(TokenType::LParen),
          T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "a"),
          T(TokenType::Comma),
          T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "b"),
        T(TokenType::RParen),
        T(TokenType::Arrow),
        T(TokenType::KwInt),
        T(TokenType::Newline),
        T(TokenType::LBrace),
        T(TokenType::Newline),

          T(TokenType::KwReturn),
            T(TokenType::Identifier, "a"), T(TokenType::Plus), T(TokenType::Identifier, "b"), 
            T(TokenType::Newline),
        T(TokenType::RBrace),
    });

    Parser p(toks);
    Module m = p.parse();

    REQUIRE(m.items.size() == 1);
    const Func* fn = astx::asFunc(m.items[0]); REQUIRE(fn != nullptr);
    REQUIRE(fn->name == "add");
    REQUIRE(fn->retType.kind == TypeKind::Int);
    REQUIRE(fn->params.size() == 2);
    REQUIRE(fn->params[0].type.kind == TypeKind::Int);
    REQUIRE(fn->params[0].name == "a");
    REQUIRE(fn->params[1].name == "b");

    REQUIRE(fn->body != nullptr);
    REQUIRE(fn->body->body.size() == 1);
    const Return* r = astx::asReturn(*fn->body->body[0]); REQUIRE(r != nullptr);
    REQUIRE(astx::isBinary(*r->retExpr, BinaryOp::Add));
}

// ========= error: trailing comma in params =========
TEST_CASE("Parser: error on trailing comma in parameter list", "[parser][func][error]") {
    // def f(int: a, ) -> void { }
    auto toks = eof({
        T(TokenType::KwDef), T(TokenType::Identifier, "f"),
        T(TokenType::LParen),
          T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "a"),
          T(TokenType::Comma),
        T(TokenType::RParen),
        T(TokenType::Arrow), T(TokenType::KwVoid),
        T(TokenType::LBrace), T(TokenType::RBrace),
    });

    Parser p(toks);
    REQUIRE_THROWS_WITH(p.parse(), ContainsSubstring("Expected additional argument"));
}

// ========= error: missing newline after var decl =========
TEST_CASE("Parser: error when newline missing after var decl assuming no eof", "[parser][stmt][error]") {
    // int: x = 1  (no newline -> parseVarDecl() calls consumeNewline())
    // bool: y = true
    auto toks = eof({
        T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "x"),
        T(TokenType::Assign),
        T(TokenType::IntLit, "1"),
        // no Newline
        T(TokenType::KwBool), T(TokenType::Colon), T(TokenType::Identifier, "y"),
        T(TokenType::Assign),
        T(TokenType::BoolLit, "true"),
    });

    Parser p(toks);
    REQUIRE_THROWS_WITH(p.parse(), ContainsSubstring("Invalid token found in expression"));
}

// ========= error: unbalanced parens =========
TEST_CASE("Parser: error when ')' missing", "[parser][expr][error]") {
    // int: x = (1 + 2  \n
    auto toks = eof({
        T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "x"),
        T(TokenType::Assign),
        T(TokenType::LParen),
        T(TokenType::IntLit, "1"), T(TokenType::Plus), T(TokenType::IntLit, "2"),
        T(TokenType::Newline),
    });

    Parser p(toks);
    REQUIRE_THROWS_WITH(p.parse(), ContainsSubstring("expected ')'"));
}

// ========= call as expression (postfix) in initializer =========
TEST_CASE("Parser: function call expression (in var init)", "[parser][expr][call][postfix]") {
    // int: x = foo(1, 2)\n
    auto toks = eof({
        T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "x"),
        T(TokenType::Assign),
        T(TokenType::Identifier, "foo"),
        T(TokenType::LParen),
          T(TokenType::IntLit, "1"), T(TokenType::Comma), T(TokenType::IntLit, "2"),
        T(TokenType::RParen),
        T(TokenType::Newline),
    });

    Parser p(toks);
    Module m = p.parse();

    const VarDecl* v = astx::asVarDecl(*astx::asStmt(m.items[0])); REQUIRE(v != nullptr);
    const Call* call = astx::asCall(*v->init); REQUIRE(call != nullptr);
    const Identifier* callee = astx::asIdent(*call->callee); REQUIRE(callee != nullptr);
    REQUIRE(callee->name == "foo");
    REQUIRE(call->args.size() == 2);
    REQUIRE(astx::asInt(*call->args[0]) != nullptr);
    REQUIRE(astx::asInt(*call->args[1]) != nullptr);
}

// ========= blocks must consume '}' =========
TEST_CASE("Parser: block consumes '}' and parsing resumes", "[parser][block]") {
    // pre; while 1 { \n stmt \n } \n post
    auto toks = eof({
        T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "pre"),
        T(TokenType::Assign), T(TokenType::IntLit, "0"), T(TokenType::Newline),

        T(TokenType::KwWhile), T(TokenType::IntLit, "1"),
        T(TokenType::LBrace),
        T(TokenType::Newline),
          T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "a"),
          T(TokenType::Assign), T(TokenType::IntLit, "1"), T(TokenType::Newline),
        T(TokenType::RBrace), T(TokenType::Newline),

        // after '}', another top-level decl should parse fine:
        T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "post"),
        T(TokenType::Assign), T(TokenType::IntLit, "2"), T(TokenType::Newline)
    });

    Parser p(toks);
    Module m = p.parse();
    REQUIRE(m.items.size() == 3);
}

// ========= constructor should initialize current_ (peek works) =========
TEST_CASE("Parser: constructor initializes current_ to 0 (peek)", "[parser][ctor][bug]") {
    auto toks = eof({ T(TokenType::KwInt) });
    Parser p(toks);
    REQUIRE(p.peek().type == TokenType::KwInt);
}

// ========= optional newlines are consumed between items =========
TEST_CASE("Parser: consumes optional leading/trailing newlines between items", "[parser][newline]") {
    auto toks = eof({
        T(TokenType::Newline), T(TokenType::Newline),
        T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "x"),
        T(TokenType::Assign), T(TokenType::IntLit, "1"), T(TokenType::Newline),
        T(TokenType::Newline),
        T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "y"),
        T(TokenType::Assign), T(TokenType::IntLit, "2"), T(TokenType::Newline),
        T(TokenType::Newline),
    });

    Parser p(toks);
    Module m = p.parse();
    REQUIRE(m.items.size() == 2);
    REQUIRE(astx::asVarDecl(*astx::asStmt(m.items[0]))->name == "x");
    REQUIRE(astx::asVarDecl(*astx::asStmt(m.items[1]))->name == "y");
}

// ========= return in function =========
TEST_CASE("Parser: return statement in function body", "[parser][return]") {
    // def f()->int { \n return 1 + 2 \n }
    auto toks = eof({
        T(TokenType::KwDef), T(TokenType::Identifier, "f"),
        T(TokenType::LParen), T(TokenType::RParen),
        T(TokenType::Arrow), T(TokenType::KwInt),
        T(TokenType::LBrace),
        T(TokenType::Newline),
          T(TokenType::KwReturn), T(TokenType::IntLit, "1"), T(TokenType::Plus), T(TokenType::IntLit, "2"), T(TokenType::Newline),
        T(TokenType::RBrace),
    });

    Parser p(toks);
    Module m = p.parse();
    REQUIRE(m.items.size() == 1);
    const Func* f = astx::asFunc(m.items[0]); REQUIRE(f != nullptr);
    REQUIRE(f->body->body.size() == 1);
    REQUIRE(astx::asReturn(*f->body->body[0]) != nullptr);
}

// ========= boolean & not =========
TEST_CASE("Parser: bool literal and 'not' unary", "[parser][expr][bool]") {
    // bool: b = not true \n
    auto toks = eof({
        T(TokenType::KwBool), T(TokenType::Colon), T(TokenType::Identifier, "b"),
        T(TokenType::Assign),
        T(TokenType::KwNot), T(TokenType::BoolLit, "true"),
        T(TokenType::Newline),
    });

    Parser p(toks);
    Module m = p.parse();
    const VarDecl* v = astx::asVarDecl(*astx::asStmt(m.items[0])); REQUIRE(v != nullptr);

    const Unary* u = astx::asUnary(*v->init); REQUIRE(u != nullptr);
    REQUIRE(u->op == UnaryOp::Not);
    REQUIRE(astx::asBool(*u->rhs) != nullptr);
}

// ========= float & string literals =========
TEST_CASE("Parser: float & string literals", "[parser][expr][lits]") {
    // float: f = 3.14 \n
    // char:  s = "hi" \n
    auto toks = eof({
        T(TokenType::KwFloat), T(TokenType::Colon), T(TokenType::Identifier, "f"),
        T(TokenType::Assign), T(TokenType::FloatLit, "3.14"), T(TokenType::Newline),

        T(TokenType::KwChar), T(TokenType::Colon), T(TokenType::Identifier, "s"),
        T(TokenType::Assign), T(TokenType::StringLit, "hi"), T(TokenType::Newline),
    });

    Parser p(toks);
    Module m = p.parse();
    REQUIRE(m.items.size() == 2);

    const VarDecl* vf = astx::asVarDecl(*astx::asStmt(m.items[0])); REQUIRE(vf != nullptr);
    REQUIRE(vf->type.kind == TypeKind::Float);
    REQUIRE(astx::asFloat(*vf->init) != nullptr);
    const FloatLit* fl = astx::asFloat(*vf->init); REQUIRE(fl != nullptr);
    REQUIRE(fl->lexeme == "3.14");

    const VarDecl* vs = astx::asVarDecl(*astx::asStmt(m.items[1])); REQUIRE(vs != nullptr);
    REQUIRE(vs->type.kind == TypeKind::Char);
    REQUIRE(astx::asStr(*vs->init) != nullptr);
    const StringLit* sl = astx::asStr(*vs->init); REQUIRE(sl != nullptr);
    REQUIRE(sl->value == "hi");
}

// ========= expr-stmt: top-level call =========
TEST_CASE("Parser: expression statement (top-level call)", "[parser][stmt][exprstmt]") {
    // foo(1, 2)\n
    auto toks = eof({
        T(TokenType::Identifier, "foo"),
        T(TokenType::LParen),
          T(TokenType::IntLit, "1"), T(TokenType::Comma), T(TokenType::IntLit, "2"),
        T(TokenType::RParen),
        T(TokenType::Newline),
    });

    Parser p(toks);
    Module m = p.parse();

    REQUIRE(m.items.size() == 1);
    const Stmt* s = astx::asStmt(m.items[0]); REQUIRE(s != nullptr);

    const ExprStmt* es = astx::asExprStmt(*s); REQUIRE(es != nullptr);
    const Call* call = std::get_if<Call>(es->expr.get()); REQUIRE(call != nullptr);

    const Identifier* callee = astx::asIdent(*call->callee); REQUIRE(callee != nullptr);
    REQUIRE(callee->name == "foo");
    REQUIRE(call->args.size() == 2);
    REQUIRE(astx::asInt(*call->args[0]) != nullptr);
    REQUIRE(astx::asInt(*call->args[1]) != nullptr);
}

// ========= expr-stmt: inside a block =========
TEST_CASE("Parser: expression statement inside a block", "[parser][stmt][exprstmt][block]") {
    // while 1 { \n foo(42)\n }
    auto toks = eof({
        T(TokenType::KwWhile), T(TokenType::IntLit, "1"),
        T(TokenType::LBrace),
        T(TokenType::Newline),
          T(TokenType::Identifier, "foo"),
          T(TokenType::LParen), T(TokenType::IntLit, "42"), T(TokenType::RParen),
          T(TokenType::Newline),
        T(TokenType::RBrace)
    });

    Parser p(toks);
    Module m = p.parse();
    REQUIRE(m.items.size() == 1);

    const While* w = astx::asWhile(*astx::asStmt(m.items[0])); REQUIRE(w != nullptr);
    REQUIRE(w->body != nullptr);
    REQUIRE(w->body->body.size() == 1);

    const Stmt& inner = *w->body->body[0];
    const ExprStmt* es = astx::asExprStmt(inner); REQUIRE(es != nullptr);
    const Call* call = std::get_if<Call>(es->expr.get()); REQUIRE(call != nullptr);

    const Identifier* callee = astx::asIdent(*call->callee); REQUIRE(callee != nullptr);
    REQUIRE(callee->name == "foo");
    REQUIRE(call->args.size() == 1);
    REQUIRE(astx::asInt(*call->args[0]) != nullptr);
}

// ========= expr-stmt: missing newline error =========
TEST_CASE("Parser: expression statement requires newline terminator", "[parser][stmt][exprstmt][error]") {
    // foo(1)  (no newline)
    // float: f = 3.14
    auto toks = eof({
        T(TokenType::Identifier, "foo"),
        T(TokenType::LParen), T(TokenType::IntLit, "1"), T(TokenType::RParen),
        // missing Newline
        T(TokenType::Assign), T(TokenType::FloatLit, "3.14"), T(TokenType::Newline),
    });

    Parser p(toks);
    REQUIRE_THROWS(p.parse()); // e.g., "Expected newline token"
}

// ========= call chaining: foo(1)(2) =========
TEST_CASE("Parser: chained calls foo(1)(2)", "[parser][expr][call][chain]") {
    auto toks = eof({
        T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "x"),
        T(TokenType::Assign),
          T(TokenType::Identifier, "foo"),
          T(TokenType::LParen), T(TokenType::IntLit, "1"), T(TokenType::RParen),
          T(TokenType::LParen), T(TokenType::IntLit, "2"), T(TokenType::RParen),
        T(TokenType::Newline),
    });

    Parser p(toks);
    Module m = p.parse();
    const VarDecl* v = astx::asVarDecl(*astx::asStmt(m.items[0])); REQUIRE(v);

    const Call* outer = astx::asCall(*v->init); REQUIRE(outer);
    REQUIRE(outer->args.size() == 1);
    REQUIRE(astx::asInt(*outer->args[0]) != nullptr);

    const Call* inner = astx::asCall(*outer->callee); REQUIRE(inner);
    REQUIRE(inner->args.size() == 1);
    REQUIRE(astx::asInt(*inner->args[0]) != nullptr);

    const Identifier* callee = astx::asIdent(*inner->callee); REQUIRE(callee);
    REQUIRE(callee->name == "foo");
}

// ========= grouped-call: (foo)(42) =========
TEST_CASE("Parser: grouped callee (foo)(42)", "[parser][expr][call][grouped]") {
    auto toks = eof({
        T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "x"),
        T(TokenType::Assign),
          T(TokenType::LParen), T(TokenType::Identifier, "foo"), T(TokenType::RParen),
          T(TokenType::LParen), T(TokenType::IntLit, "42"), T(TokenType::RParen),
        T(TokenType::Newline),
    });

    Parser p(toks);
    Module m = p.parse();
    const VarDecl* v = astx::asVarDecl(*astx::asStmt(m.items[0])); REQUIRE(v);

    const Call* call = astx::asCall(*v->init); REQUIRE(call);
    REQUIRE(call->args.size() == 1);
    REQUIRE(astx::asInt(*call->args[0]) != nullptr);

    const Identifier* callee = astx::asIdent(*call->callee); REQUIRE(callee);
    REQUIRE(callee->name == "foo");
}

/*
// ========= comparisons are non-associative: 1 < 2 < 3 should fail =========
TEST_CASE("Parser: chained comparisons error (non-associative)", "[parser][expr][error]") {
    auto toks = eof({
        T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "x"),
        T(TokenType::Assign),
          T(TokenType::IntLit, "1"), T(TokenType::LCompare), T(TokenType::IntLit, "2"),
          T(TokenType::LCompare), T(TokenType::IntLit, "3"),
        T(TokenType::Newline),
    });

    Parser p(toks);
    REQUIRE_THROWS(p.parse());
}
*/

// ========= expression stmt vs assignment disambiguation =========
TEST_CASE("Parser: expr-stmt then assign", "[parser][stmt][exprstmt][assign]") {
    // x + 1 \n
    // x = 2  \n
    auto toks = eof({
        T(TokenType::Identifier, "x"), T(TokenType::Plus), T(TokenType::IntLit, "1"), T(TokenType::Newline),
        T(TokenType::Identifier, "x"), T(TokenType::Assign), T(TokenType::IntLit, "2"), T(TokenType::Newline),
    });

    Parser p(toks);
    Module m = p.parse();
    REQUIRE(m.items.size() == 2);

    const ExprStmt* es = astx::asExprStmt(*astx::asStmt(m.items[0])); REQUIRE(es);
    REQUIRE(astx::isBinary(*es->expr, BinaryOp::Add));

    const Assign* asg = astx::asAssign(*astx::asStmt(m.items[1])); REQUIRE(asg);
    REQUIRE(asg->name == "x");
    REQUIRE(astx::asInt(*asg->value) != nullptr);
}

// ========= call args: error on trailing comma =========
TEST_CASE("Parser: call arg list trailing comma is error", "[parser][expr][call][error]") {
    // foo(1, ) \n
    auto toks = eof({
        T(TokenType::Identifier, "foo"),
        T(TokenType::LParen), T(TokenType::IntLit, "1"), T(TokenType::Comma), T(TokenType::RParen),
        T(TokenType::Newline),
    });

    Parser p(toks);
    REQUIRE_THROWS(p.parse());
}

// ========= call args: missing comma between args =========
TEST_CASE("Parser: call arg list missing comma error", "[parser][expr][call][error]") {
    // foo(1 2) \n
    auto toks = eof({
        T(TokenType::Identifier, "foo"),
        T(TokenType::LParen), T(TokenType::IntLit, "1"), T(TokenType::IntLit, "2"), T(TokenType::RParen),
        T(TokenType::Newline),
    });

    Parser p(toks);
    REQUIRE_THROWS(p.parse());
}

// ========= empty call: foo() =========
TEST_CASE("Parser: empty call arg list", "[parser][expr][call]") {
    auto toks = eof({
        T(TokenType::Identifier, "foo"), T(TokenType::LParen), T(TokenType::RParen), T(TokenType::Newline),
    });

    Parser p(toks);
    Module m = p.parse();
    const ExprStmt* es = astx::asExprStmt(*astx::asStmt(m.items[0])); REQUIRE(es);
    const Call* call = astx::asCall(*es->expr); REQUIRE(call);
    REQUIRE(call->args.empty());
}

// ========= unary 'not' and grouping: not (true) =========
TEST_CASE("Parser: unary not with grouping", "[parser][expr][unary]") {
    auto toks = eof({
        T(TokenType::KwBool), T(TokenType::Colon), T(TokenType::Identifier, "b"),
        T(TokenType::Assign), T(TokenType::KwNot),
        T(TokenType::LParen), T(TokenType::BoolLit, "true"), T(TokenType::RParen),
        T(TokenType::Newline),
    });

    Parser p(toks);
    Module m = p.parse();
    const VarDecl* v = astx::asVarDecl(*astx::asStmt(m.items[0])); REQUIRE(v);
    const Unary* u = astx::asUnary(*v->init); REQUIRE(u);
    REQUIRE(u->op == UnaryOp::Not);
    REQUIRE(astx::asBool(*u->rhs) != nullptr);
}

// ========= empty block =========
TEST_CASE("Parser: empty block in while", "[parser][block]") {
    // while 1 { \n }
    auto toks = eof({
        T(TokenType::KwWhile), T(TokenType::IntLit, "1"),
        T(TokenType::LBrace), T(TokenType::Newline), T(TokenType::RBrace),
    });

    Parser p(toks);
    Module m = p.parse();
    const While* w = astx::asWhile(*astx::asStmt(m.items[0])); REQUIRE(w);
    REQUIRE(w->body != nullptr);
    REQUIRE(w->body->body.empty());
}

// ========= nested blocks: while { if { } } =========
TEST_CASE("Parser: nested blocks (while with inner if)", "[parser][block][nested]") {
    auto toks = eof({
        T(TokenType::KwWhile), T(TokenType::IntLit, "1"),
        T(TokenType::LBrace), T(TokenType::Newline),

            T(TokenType::KwIf), T(TokenType::IntLit, "1"),
            T(TokenType::LBrace), T(TokenType::Newline),
            T(TokenType::RBrace), // end inner if
            T(TokenType::Newline),

        T(TokenType::RBrace), // end while
    });

    Parser p(toks);
    Module m = p.parse();
    REQUIRE(m.items.size() == 1);
    const While* w = astx::asWhile(*astx::asStmt(m.items[0])); REQUIRE(w);
    REQUIRE(w->body->body.size() == 1);
    REQUIRE(astx::asBr(*w->body->body[0]) != nullptr);
}

// ========= var-decl errors: missing ':' after type =========
TEST_CASE("Parser: error - missing ':' after type in decl", "[parser][stmt][vardecl][error]") {
    auto toks = eof({
        T(TokenType::KwInt), /* missing Colon */ T(TokenType::Identifier, "x"),
        T(TokenType::Assign), T(TokenType::IntLit, "1"), T(TokenType::Newline),
    });

    Parser p(toks);
    REQUIRE_THROWS(p.parse());
}

// ========= var-decl errors: missing '=' in decl =========
TEST_CASE("Parser: error - missing '=' in decl", "[parser][stmt][vardecl][error2]") {
    auto toks = eof({
        T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "x"),
        /* missing Assign */ T(TokenType::IntLit, "1"), T(TokenType::Newline),
    });

    Parser p(toks);
    REQUIRE_THROWS(p.parse());
}

// ========= function: zero params, newline allowed before body =========
TEST_CASE("Parser: function zero params with newline before body", "[parser][func]") {
    auto toks = eof({
        T(TokenType::KwDef), T(TokenType::Identifier, "f"),
        T(TokenType::LParen), T(TokenType::RParen),
        T(TokenType::Arrow), T(TokenType::KwVoid),
        T(TokenType::Newline),                 // optional newline before block per your grammar
        T(TokenType::LBrace), T(TokenType::Newline),
        T(TokenType::RBrace),
    });

    Parser p(toks);
    Module m = p.parse();
    const Func* fn = astx::asFunc(m.items[0]); REQUIRE(fn);
    REQUIRE(fn->params.empty());
    REQUIRE(fn->retType.kind == TypeKind::Void);
    REQUIRE(fn->body->body.empty());
}

// ========= error: missing newline after expr-stmt at top level =========
TEST_CASE("Parser: error - top-level expr-stmt missing newline", "[parser][stmt][exprstmt][error2]") {
    auto toks = eof({
        T(TokenType::Identifier, "foo"),
        T(TokenType::LParen), T(TokenType::RParen),
        // missing Newline
        T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "x"),
        T(TokenType::Assign), T(TokenType::IntLit, "1"), T(TokenType::Newline),
    });

    Parser p(toks);
    REQUIRE_THROWS(p.parse());
}

// ========= RParen has no binding power: (1) + 2 parses fine =========
TEST_CASE("Parser: RParen stops expression (lbp=0)", "[parser][expr][paren]") {
    auto toks = eof({
        T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "x"),
        T(TokenType::Assign),
        T(TokenType::LParen), T(TokenType::IntLit, "1"), T(TokenType::RParen),
        T(TokenType::Plus), T(TokenType::IntLit, "2"),
        T(TokenType::Newline),
    });

    Parser p(toks);
    Module m = p.parse();
    const VarDecl* v = astx::asVarDecl(*astx::asStmt(m.items[0])); REQUIRE(v);
    REQUIRE(astx::isBinary(*v->init, BinaryOp::Add));
}

// ========= while header may have newline before '{' (your grammar allows it) =========
TEST_CASE("Parser: while header newline before block is allowed", "[parser][stmt][while]") {
    auto toks = eof({
        T(TokenType::KwWhile), T(TokenType::IntLit, "1"),
        T(TokenType::Newline),
        T(TokenType::LBrace), T(TokenType::Newline),
        T(TokenType::RBrace),
    });

    Parser p(toks);
    Module m = p.parse();
    REQUIRE(astx::asWhile(*astx::asStmt(m.items[0])) != nullptr);
}

// ========= for-in header: newline BEFORE block is allowed (matches while) =========
TEST_CASE("Parser: for-in header newline before block is allowed", "[parser][stmt][forin]") {
    auto toks = eof({
        T(TokenType::KwFor),
        T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "i"),
        T(TokenType::KwIn), T(TokenType::IntLit, "1"),
        T(TokenType::Newline),           // allowed
        T(TokenType::LBrace), T(TokenType::Newline),
        T(TokenType::RBrace),
    });

    Parser p(toks);
    Module m = p.parse();
    REQUIRE(m.items.size() == 1);
    const ForIn* f = astx::asForIn(*astx::asStmt(m.items[0])); REQUIRE(f);
    REQUIRE(f->type.kind == TypeKind::Int);
    REQUIRE(f->name == "i");
    REQUIRE(astx::asInt(*f->iterable) != nullptr);
    REQUIRE(f->body != nullptr);
    REQUIRE(f->body->body.empty());
}

// ========= if header: newline BEFORE block is allowed (matches while) =========
TEST_CASE("Parser: if header newline before block is allowed", "[parser][stmt][branch]") {
    auto toks = eof({
        T(TokenType::KwIf), T(TokenType::IntLit, "1"),
        T(TokenType::Newline),           // allowed
        T(TokenType::LBrace), T(TokenType::Newline),
          T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "a"),
          T(TokenType::Assign), T(TokenType::IntLit, "1"), T(TokenType::Newline),
        T(TokenType::RBrace),
    });

    Parser p(toks);
    Module m = p.parse();
    REQUIRE(m.items.size() == 1);
    const BrChain* br = astx::asBr(*astx::asStmt(m.items[0])); REQUIRE(br);
    REQUIRE(br->branches.size() == 1);
    REQUIRE(br->branches[0].body != nullptr);
    REQUIRE(br->branches[0].body->body.size() == 1);
    REQUIRE(astx::asVarDecl(*br->branches[0].body->body[0]) != nullptr);
}

// ========= block must have at least one newline after '{' =========
TEST_CASE("Parser: block requires newline after '{'", "[parser][block][newline]") {
    // while 1 { }  <-- missing newline after '{'
    auto toks = eof({
        T(TokenType::KwWhile), T(TokenType::IntLit, "1"),
        T(TokenType::LBrace),               // no Newline here -> error
        T(TokenType::RBrace),
    });
    Parser p(toks);
    REQUIRE_THROWS_WITH(p.parse(), ContainsSubstring("Must have \\n (newline) character after '{'"));
}

// ========= block may have multiple leading newlines after '{' =========
TEST_CASE("Parser: block allows multiple leading newlines", "[parser][block][newline]") {
    auto toks = eof({
        T(TokenType::KwWhile), T(TokenType::IntLit, "1"),
        T(TokenType::LBrace),
        T(TokenType::Newline), T(TokenType::Newline),
        T(TokenType::RBrace),
    });
    Parser p(toks);
    Module m = p.parse();
    const While* w = astx::asWhile(*astx::asStmt(m.items[0])); REQUIRE(w);
    REQUIRE(w->body != nullptr);
    REQUIRE(w->body->body.empty());
}

// ========= simple statements require newline (unless EOF) =========
TEST_CASE("Parser: missing newline after assignment before next stmt is error", "[parser][stmt][newline][error]") {
    auto toks = eof({
        T(TokenType::Identifier, "x"), T(TokenType::Assign), T(TokenType::IntLit, "1"),
        // missing Newline
        T(TokenType::Identifier, "y"), T(TokenType::Assign), T(TokenType::IntLit, "2"), T(TokenType::Newline),
    });
    Parser p(toks);
    REQUIRE_THROWS_WITH(p.parse(), ContainsSubstring("Invalid token found in expression"));
}

TEST_CASE("Parser: newline after final top-level simple stmt can be omitted via EOF", "[parser][stmt][newline]") {
    auto toks = std::vector<Token>{
        T(TokenType::Identifier, "x"), T(TokenType::Assign), T(TokenType::IntLit, "1"),
        // no Newline, immediate EOF appended by eof()
    };
    toks = eof(std::move(toks));
    Parser p(toks);
    Module m = p.parse();
    REQUIRE(m.items.size() == 1);
    REQUIRE(astx::asAssign(*astx::asStmt(m.items[0])) != nullptr);
}



// ========= function: multiple optional newlines after '-> type' before block =========
TEST_CASE("Parser: function allows multiple newlines before block", "[parser][func][newline]") {
    auto toks = eof({
        T(TokenType::KwDef), T(TokenType::Identifier, "f"),
        T(TokenType::LParen), T(TokenType::RParen),
        T(TokenType::Arrow), T(TokenType::KwVoid),
        T(TokenType::Newline), T(TokenType::Newline),
        T(TokenType::LBrace), T(TokenType::Newline),
        T(TokenType::RBrace),
    });
    Parser p(toks);
    Module m = p.parse();
    const Func* fn = astx::asFunc(m.items[0]); REQUIRE(fn);
    REQUIRE(fn->retType.kind == TypeKind::Void);
    REQUIRE(fn->body != nullptr);
    REQUIRE(fn->body->body.empty());
}

// ========= elif / else also allow optional newlines before their blocks =========
TEST_CASE("Parser: elif and else allow newline before block", "[parser][branch][newline]") {
    auto toks = eof({
        T(TokenType::KwIf), T(TokenType::IntLit, "1"),
        T(TokenType::LBrace), T(TokenType::Newline), T(TokenType::RBrace),

        T(TokenType::KwElif), T(TokenType::IntLit, "2"),
        T(TokenType::Newline),
        T(TokenType::LBrace), T(TokenType::Newline), T(TokenType::RBrace),

        T(TokenType::KwElse),
        T(TokenType::Newline),
        T(TokenType::LBrace), T(TokenType::Newline), T(TokenType::RBrace),
    });
    Parser p(toks);
    Module m = p.parse();
    const BrChain* br = astx::asBr(*astx::asStmt(m.items[0])); REQUIRE(br);
    REQUIRE(br->branches.size() == 2);
    REQUIRE(br->elseBlock != nullptr);
}

// ========= expression termination at comma inside call =========
TEST_CASE("Parser: expressions terminate at comma inside call args", "[parser][expr][comma]") {
    // foo(1+2, 3)
    auto toks = eof({
        T(TokenType::Identifier, "foo"), T(TokenType::LParen),
          T(TokenType::IntLit, "1"), T(TokenType::Plus), T(TokenType::IntLit, "2"),
          T(TokenType::Comma),
          T(TokenType::IntLit, "3"),
        T(TokenType::RParen), T(TokenType::Newline),
    });
    Parser p(toks);
    Module m = p.parse();
    const ExprStmt* es = astx::asExprStmt(*astx::asStmt(m.items[0])); REQUIRE(es);
    const Call* call = astx::asCall(*es->expr); REQUIRE(call);
    REQUIRE(call->args.size() == 2);
    REQUIRE(astx::isBinary(*call->args[0], BinaryOp::Add));
    REQUIRE(astx::asInt(*call->args[1]) != nullptr);
}

// ========= nested calls with spaces/newlines inside =========
TEST_CASE("Parser: nested calls with mixed spacing", "[parser][expr][call]") {
    // int: x = foo( bar(1), (baz)(2) ) \n
    auto toks = eof({
        T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "x"),
        T(TokenType::Assign),
        T(TokenType::Identifier, "foo"), T(TokenType::LParen),
          T(TokenType::Identifier, "bar"), T(TokenType::LParen), T(TokenType::IntLit, "1"), T(TokenType::RParen),
          T(TokenType::Comma),
          T(TokenType::LParen), T(TokenType::Identifier, "baz"), T(TokenType::RParen),
            T(TokenType::LParen), T(TokenType::IntLit, "2"), T(TokenType::RParen),
        T(TokenType::RParen), T(TokenType::Newline),
    });
    Parser p(toks);
    Module m = p.parse();
    const VarDecl* v = astx::asVarDecl(*astx::asStmt(m.items[0])); REQUIRE(v);
    const Call* outer = astx::asCall(*v->init); REQUIRE(outer);
    REQUIRE(outer->args.size() == 2);
    REQUIRE(astx::asCall(*outer->args[0]) != nullptr);  // bar(1)
    REQUIRE(astx::asCall(*outer->args[1]) != nullptr);  // (baz)(2)
}

/*
// ========= comparison non-associativity is enforced (1 < 2 < 3 is error) =========
TEST_CASE("Parser: chained comparisons are not allowed", "[parser][expr][error]") {
    auto toks = eof({
        T(TokenType::Identifier, "x"), T(TokenType::Assign),
        T(TokenType::IntLit, "1"), T(TokenType::LCompare), T(TokenType::IntLit, "2"),
        T(TokenType::LCompare), T(TokenType::IntLit, "3"),
        T(TokenType::Newline),
    });
    Parser p(toks);
    REQUIRE_THROWS(p.parse());
}
*/

// ========= unary before grouping vs binary minus disambiguation =========
TEST_CASE("Parser: unary minus vs binary minus", "[parser][expr][unary]") {
    // int: a = -1 + 2 \n
    // int: b = (1) - 2 \n
    auto toks = eof({
        T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "a"),
        T(TokenType::Assign), T(TokenType::Minus), T(TokenType::IntLit, "1"),
        T(TokenType::Plus), T(TokenType::IntLit, "2"), T(TokenType::Newline),

        T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "b"),
        T(TokenType::Assign), T(TokenType::LParen), T(TokenType::IntLit, "1"), T(TokenType::RParen),
        T(TokenType::Minus), T(TokenType::IntLit, "2"), T(TokenType::Newline),
    });
    Parser p(toks);
    Module m = p.parse();
    const VarDecl* va = astx::asVarDecl(*astx::asStmt(m.items[0])); REQUIRE(va);
    REQUIRE(astx::isBinary(*va->init, BinaryOp::Add));
    const Unary* ua = astx::asUnary(*astx::asBinary(*va->init)->lhs); REQUIRE(ua);
    REQUIRE(ua->op == UnaryOp::Neg);

    const VarDecl* vb = astx::asVarDecl(*astx::asStmt(m.items[1])); REQUIRE(vb);
    REQUIRE(astx::isBinary(*vb->init, BinaryOp::Sub));
}

// ========= nested parens: ((1 + (2 * (3 + 4)))) =========
TEST_CASE("Parser: nested parentheses shape", "[parser][expr][paren]") {
    // int: x = ((1 + (2 * (3 + 4)))) \n
    auto toks = eof({
        T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "x"),
        T(TokenType::Assign),

        T(TokenType::LParen),
          T(TokenType::LParen),
            T(TokenType::IntLit, "1"),
            T(TokenType::Plus),
            T(TokenType::LParen),
              T(TokenType::IntLit, "2"),
              T(TokenType::Star),
              T(TokenType::LParen),
                T(TokenType::IntLit, "3"), T(TokenType::Plus), T(TokenType::IntLit, "4"),
              T(TokenType::RParen),
            T(TokenType::RParen),
          T(TokenType::RParen),
        T(TokenType::RParen),

        T(TokenType::Newline),
    });

    Parser p(toks);
    Module m = p.parse();

    const VarDecl* v = astx::asVarDecl(*astx::asStmt(m.items[0])); REQUIRE(v);
    const Binary* add_outer = astx::asBinary(*v->init); REQUIRE(add_outer);
    REQUIRE(add_outer->op == BinaryOp::Add);

    const Binary* mul = astx::asBinary(*add_outer->rhs); REQUIRE(mul);
    REQUIRE(mul->op == BinaryOp::Mul);

    const Binary* add_inner = astx::asBinary(*mul->rhs); REQUIRE(add_inner);
    REQUIRE(add_inner->op == BinaryOp::Add);

    REQUIRE(astx::asInt(*add_outer->lhs) != nullptr); // 1
    REQUIRE(astx::asInt(*mul->lhs) != nullptr);       // 2
    REQUIRE(astx::asInt(*add_inner->lhs) != nullptr); // 3
    REQUIRE(astx::asInt(*add_inner->rhs) != nullptr); // 4
}

// ========= parens override precedence: (1+2) * (3+4) =========
TEST_CASE("Parser: parentheses control precedence", "[parser][expr][paren]") {
    // int: x = (1 + 2) * (3 + 4) \n
    auto toks = eof({
        T(TokenType::KwInt), T(TokenType::Colon), T(TokenType::Identifier, "x"),
        T(TokenType::Assign),

        T(TokenType::LParen), T(TokenType::IntLit, "1"), T(TokenType::Plus), T(TokenType::IntLit, "2"), T(TokenType::RParen),
        T(TokenType::Star),
        T(TokenType::LParen), T(TokenType::IntLit, "3"), T(TokenType::Plus), T(TokenType::IntLit, "4"), T(TokenType::RParen),

        T(TokenType::Newline),
    });

    Parser p(toks);
    Module m = p.parse();

    const VarDecl* v = astx::asVarDecl(*astx::asStmt(m.items[0])); REQUIRE(v);
    const Binary* mul = astx::asBinary(*v->init); REQUIRE(mul);
    REQUIRE(mul->op == BinaryOp::Mul);

    const Binary* left_add  = astx::asBinary(*mul->lhs); REQUIRE(left_add);
    const Binary* right_add = astx::asBinary(*mul->rhs); REQUIRE(right_add);
    REQUIRE(left_add->op == BinaryOp::Add);
    REQUIRE(right_add->op == BinaryOp::Add);
}

// ========= logical and/or with comparisons: (1<2) and (3<4) =========
TEST_CASE("Parser: and/or precedence with comparisons", "[parser][expr][logic]") {
    // bool: b = 1 < 2 and 3 < 4 \n
    auto toks = eof({
        T(TokenType::KwBool), T(TokenType::Colon), T(TokenType::Identifier, "b"),
        T(TokenType::Assign),

        T(TokenType::IntLit, "1"), T(TokenType::LCompare), T(TokenType::IntLit, "2"),
        T(TokenType::KwAnd),
        T(TokenType::IntLit, "3"), T(TokenType::LCompare), T(TokenType::IntLit, "4"),

        T(TokenType::Newline),
    });

    Parser p(toks);
    Module m = p.parse();

    const VarDecl* v = astx::asVarDecl(*astx::asStmt(m.items[0])); REQUIRE(v);
    const Binary* andNode = astx::asBinary(*v->init); REQUIRE(andNode);
    REQUIRE(andNode->op == BinaryOp::And);

    const Binary* cmpL = astx::asBinary(*andNode->lhs); REQUIRE(cmpL);
    const Binary* cmpR = astx::asBinary(*andNode->rhs); REQUIRE(cmpR);
    REQUIRE(cmpL->op == BinaryOp::LComp);
    REQUIRE(cmpR->op == BinaryOp::LComp);
}

// ========= not binds tighter than and =========
TEST_CASE("Parser: not binds tighter than and", "[parser][expr][logic]") {
    // bool: b = not a and b \n
    auto toks = eof({
        T(TokenType::KwBool), T(TokenType::Colon), T(TokenType::Identifier, "b"),
        T(TokenType::Assign),

        T(TokenType::KwNot), T(TokenType::Identifier, "a"),
        T(TokenType::KwAnd),
        T(TokenType::Identifier, "b"),

        T(TokenType::Newline),
    });

    Parser p(toks);
    Module m = p.parse();

    const VarDecl* v = astx::asVarDecl(*astx::asStmt(m.items[0])); REQUIRE(v);
    const Binary* andNode = astx::asBinary(*v->init); REQUIRE(andNode);
    REQUIRE(andNode->op == BinaryOp::And);

    const Unary* notNode = astx::asUnary(*andNode->lhs); REQUIRE(notNode);
    REQUIRE(notNode->op == UnaryOp::Not);

    const Identifier* rhsId = astx::asIdent(*andNode->rhs); REQUIRE(rhsId);
    REQUIRE(rhsId->name == "b");
}

// ========= and binds tighter than or: a or b and c => a or (b and c) =========
TEST_CASE("Parser: and tighter than or", "[parser][expr][logic]") {
    // bool: x = a or b and c \n
    auto toks = eof({
        T(TokenType::KwBool), T(TokenType::Colon), T(TokenType::Identifier, "x"),
        T(TokenType::Assign),

        T(TokenType::Identifier, "a"),
        T(TokenType::KwOr),
        T(TokenType::Identifier, "b"),
        T(TokenType::KwAnd),
        T(TokenType::Identifier, "c"),

        T(TokenType::Newline),
    });

    Parser p(toks);
    Module m = p.parse();

    const VarDecl* v = astx::asVarDecl(*astx::asStmt(m.items[0])); REQUIRE(v);
    const Binary* orNode = astx::asBinary(*v->init); REQUIRE(orNode);
    REQUIRE(orNode->op == BinaryOp::Or);

    const Identifier* a = astx::asIdent(*orNode->lhs); REQUIRE(a);
    REQUIRE(a->name == "a");

    const Binary* andNode = astx::asBinary(*orNode->rhs); REQUIRE(andNode);
    REQUIRE(andNode->op == BinaryOp::And);

    const Identifier* b = astx::asIdent(*andNode->lhs); REQUIRE(b);
    const Identifier* c = astx::asIdent(*andNode->rhs); REQUIRE(c);
    REQUIRE(b->name == "b");
    REQUIRE(c->name == "c");
}

// ========= left-assoc for and/or: (a and b) and c; (a or b) or c =========
TEST_CASE("Parser: and/or are left-associative", "[parser][expr][logic]") {
    // bool: p = a and b and c \n
    // bool: q = a or b or c \n
    auto toks = eof({
        T(TokenType::KwBool), T(TokenType::Colon), T(TokenType::Identifier, "p"),
        T(TokenType::Assign),
        T(TokenType::Identifier, "a"), T(TokenType::KwAnd), T(TokenType::Identifier, "b"),
        T(TokenType::KwAnd), T(TokenType::Identifier, "c"),
        T(TokenType::Newline),

        T(TokenType::KwBool), T(TokenType::Colon), T(TokenType::Identifier, "q"),
        T(TokenType::Assign),
        T(TokenType::Identifier, "a"), T(TokenType::KwOr), T(TokenType::Identifier, "b"),
        T(TokenType::KwOr), T(TokenType::Identifier, "c"),
        T(TokenType::Newline),
    });

    Parser p(toks);
    Module m = p.parse();
    REQUIRE(m.items.size() == 2);

    // a and b and c  -> ((a and b) and c)
    {
        const VarDecl* vp = astx::asVarDecl(*astx::asStmt(m.items[0])); REQUIRE(vp);
        const Binary* and2 = astx::asBinary(*vp->init); REQUIRE(and2);
        REQUIRE(and2->op == BinaryOp::And);

        const Binary* and1 = astx::asBinary(*and2->lhs); REQUIRE(and1);
        REQUIRE(and1->op == BinaryOp::And);

        REQUIRE(astx::asIdent(*and1->lhs)->name == std::string("a"));
        REQUIRE(astx::asIdent(*and1->rhs)->name == std::string("b"));
        REQUIRE(astx::asIdent(*and2->rhs)->name == std::string("c"));
    }

    // a or b or c    -> ((a or b) or c)
    {
        const VarDecl* vq = astx::asVarDecl(*astx::asStmt(m.items[1])); REQUIRE(vq);
        const Binary* or2 = astx::asBinary(*vq->init); REQUIRE(or2);
        REQUIRE(or2->op == BinaryOp::Or);

        const Binary* or1 = astx::asBinary(*or2->lhs); REQUIRE(or1);
        REQUIRE(or1->op == BinaryOp::Or);

        REQUIRE(astx::asIdent(*or1->lhs)->name == std::string("a"));
        REQUIRE(astx::asIdent(*or1->rhs)->name == std::string("b"));
        REQUIRE(astx::asIdent(*or2->rhs)->name == std::string("c"));
    }
}

// ========= parentheses with logic: (a or b) and c =========
TEST_CASE("Parser: parentheses interact with and/or", "[parser][expr][logic][paren]") {
    // bool: z = (a or b) and c \n
    auto toks = eof({
        T(TokenType::KwBool), T(TokenType::Colon), T(TokenType::Identifier, "z"),
        T(TokenType::Assign),

        T(TokenType::LParen),
          T(TokenType::Identifier, "a"),
          T(TokenType::KwOr),
          T(TokenType::Identifier, "b"),
        T(TokenType::RParen),
        T(TokenType::KwAnd),
        T(TokenType::Identifier, "c"),

        T(TokenType::Newline),
    });

    Parser p(toks);
    Module m = p.parse();

    const VarDecl* v = astx::asVarDecl(*astx::asStmt(m.items[0])); REQUIRE(v);
    const Binary* andNode = astx::asBinary(*v->init); REQUIRE(andNode);
    REQUIRE(andNode->op == BinaryOp::And);

    const Binary* orNode = astx::asBinary(*andNode->lhs); REQUIRE(orNode);
    REQUIRE(orNode->op == BinaryOp::Or);

    REQUIRE(astx::asIdent(*orNode->lhs)->name == std::string("a"));
    REQUIRE(astx::asIdent(*orNode->rhs)->name == std::string("b"));
    REQUIRE(astx::asIdent(*andNode->rhs)->name == std::string("c"));
}
