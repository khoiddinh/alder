#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include "diagnostic.h"
#include "../ast/ast.h"

namespace alder::semantic {

namespace ast = alder::ast;

// SemanticError carries a location when one is available.
// AST nodes don't yet store locations, so loc defaults to {0,0} (no snippet shown).
using SemanticError = diag::CompileError;

class SemanticAnalyzer {
public:
    void analyze(const ast::Module& module);

private:
    using SymbolTable = std::unordered_map<std::string, std::string>; // name -> type

    std::vector<SymbolTable> scopes_;

    void enterScope();
    void exitScope();
    void declare(const std::string& name, const std::string& type);
    std::optional<std::string> lookup(const std::string& name) const;

    void analyzeDecl(const ast::Decl& decl);
    void analyzeFunc(const ast::Func& func);
    void analyzeBlock(const ast::Block& block);
    void analyzeStmt(const ast::Stmt& stmt);
    void analyzeVarDecl(const ast::VarDecl& decl);
    void analyzeForIn(const ast::ForIn& forIn);
    void analyzeWhile(const ast::While& loop);
    void analyzeBrChain(const ast::BrChain& chain);
    void analyzeReturn(const ast::Return& ret);
    void analyzeExprStmt(const ast::ExprStmt& es);
    void analyzeExpr(const ast::Expr& expr);

    std::string typeExprToStr(const ast::Expr& expr) const;
};

} // namespace alder::semantic
