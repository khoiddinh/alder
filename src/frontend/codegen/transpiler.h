#pragma once

#include <string>
#include <sstream>
#include "../ast/ast.h"
#include <unordered_map>

namespace alder::codegen {

namespace ast = alder::ast;

class Transpiler {
public:
    // Returns complete C++ source for the given module.
    std::string transpile(const ast::Module& module);

private:
    std::ostringstream out_;
    int indentLevel_ = 0;


    // Indentation helpers
    void emit(const std::string& s);
    void emitLine(const std::string& s = "");
    void emitIndent();

    // Top-level
    void emitDecl(const ast::Decl& decl); // Ignores Macro Decl
    void emitFunc(const ast::Func& func);
		void emitMacro(const ast::Macro& macro);

    // Statements
    void emitBlock(const ast::Block& block);
    void emitStmt(const ast::Stmt& stmt);
    void emitVarDecl(const ast::VarDecl& decl);
    void emitForIn(const ast::ForIn& forIn);
    void emitWhile(const ast::While& loop);
    void emitBrChain(const ast::BrChain& chain);
    void emitReturn(const ast::Return& ret);
    void emitExprStmt(const ast::ExprStmt& es);

    // Stdlib preamble emitted at the top of every generated file
    void emitStdlibPreamble();

    // Expressions (return C++ string)
    std::string exprToStr(const ast::Expr& expr);
    std::string emitCall(const ast::Call& call);
    std::string typeToStr(const ast::Expr& expr);
    std::string binaryOpStr(ast::BinaryOp op) const;
    std::string unaryOpStr(ast::UnaryOp op) const;
    std::string mapTypeName(const std::string& name) const;
};

} // namespace alder::codegen
