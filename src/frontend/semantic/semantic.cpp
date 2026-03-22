#include "semantic.h"

namespace alder::semantic {

// ** Scope management

void SemanticAnalyzer::enterScope() {
    scopes_.emplace_back();
}

void SemanticAnalyzer::exitScope() {
    if (!scopes_.empty()) scopes_.pop_back();
}

void SemanticAnalyzer::declare(const std::string& name, const std::string& type) {
    if (scopes_.empty()) return;
    scopes_.back()[name] = type;
}

std::optional<std::string> SemanticAnalyzer::lookup(const std::string& name) const {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        if (auto found = it->find(name); found != it->end())
            return found->second;
    }
    return std::nullopt;
}

// ** Public entry point

void SemanticAnalyzer::analyze(const ast::Module& module) {
    enterScope(); // global scope

    // Builtins
    for (const auto& name : {
        "print", "input", "range",
        "len", "append", "pop",
        "str", "int", "float",
        "abs", "min", "max", "pow", "sqrt", "floor", "ceil",
    }) declare(name, "fn");

    for (const auto& decl : module.declarations)
        analyzeDecl(decl);
    exitScope();
}

// ** Declarations

void SemanticAnalyzer::analyzeDecl(const ast::Decl& decl) {
    std::visit([this](const auto& d) {
        using T = std::decay_t<decltype(d)>;
        if constexpr (std::is_same_v<T, ast::Func>)
            analyzeFunc(d);
        else if constexpr (std::is_same_v<T, ast::GlobalStmt>)
            analyzeStmt(*d.stmt);
    }, decl);
}

void SemanticAnalyzer::analyzeFunc(const ast::Func& func) {
    declare(func.name.name, "fn");
    enterScope();
    for (const auto& param : func.params) {
        std::string t = param.type ? typeExprToStr(*param.type) : "auto";
        declare(param.name.name, t);
    }
    if (func.body) analyzeBlock(*func.body);
    exitScope();
}

// ** Blocks and statements

void SemanticAnalyzer::analyzeBlock(const ast::Block& block) {
    enterScope();
    for (const auto& stmt : block.body)
        analyzeStmt(*stmt);
    exitScope();
}

void SemanticAnalyzer::analyzeStmt(const ast::Stmt& stmt) {
    std::visit([this](const auto& s) {
        using T = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<T, ast::VarDecl>)       analyzeVarDecl(s);
        else if constexpr (std::is_same_v<T, ast::ForIn>)    analyzeForIn(s);
        else if constexpr (std::is_same_v<T, ast::While>)    analyzeWhile(s);
        else if constexpr (std::is_same_v<T, ast::BrChain>)  analyzeBrChain(s);
        else if constexpr (std::is_same_v<T, ast::Return>)   analyzeReturn(s);
        else if constexpr (std::is_same_v<T, ast::ExprStmt>) analyzeExprStmt(s);
    }, stmt);
}

void SemanticAnalyzer::analyzeVarDecl(const ast::VarDecl& decl) {
    if (decl.init) analyzeExpr(*decl.init);
    std::string t = decl.typeExpr ? typeExprToStr(*decl.typeExpr) : "auto";
    declare(decl.name.name, t);
}

void SemanticAnalyzer::analyzeForIn(const ast::ForIn& forIn) {
    if (forIn.iterable) analyzeExpr(*forIn.iterable);
    enterScope();
    for (const auto& vb : forIn.binding.varBindings) {
        std::string t = vb.typeExpr ? typeExprToStr(*vb.typeExpr) : "auto";
        declare(vb.name.name, t);
    }
    if (forIn.body) analyzeBlock(*forIn.body);
    exitScope();
}

void SemanticAnalyzer::analyzeWhile(const ast::While& loop) {
    if (loop.cond) analyzeExpr(*loop.cond);
    if (loop.body) analyzeBlock(*loop.body);
}

void SemanticAnalyzer::analyzeBrChain(const ast::BrChain& chain) {
    if (chain.ifBlock.cond) analyzeExpr(*chain.ifBlock.cond);
    if (chain.ifBlock.body) analyzeBlock(*chain.ifBlock.body);
    for (const auto& elif : chain.elifBranches) {
        if (elif.cond) analyzeExpr(*elif.cond);
        if (elif.body) analyzeBlock(*elif.body);
    }
    if (chain.elseBlock) analyzeBlock(*chain.elseBlock);
}

void SemanticAnalyzer::analyzeReturn(const ast::Return& ret) {
    if (ret.retExpr) analyzeExpr(*ret.retExpr);
}

void SemanticAnalyzer::analyzeExprStmt(const ast::ExprStmt& es) {
    if (es.expr) analyzeExpr(*es.expr);
}

// ** Expressions

void SemanticAnalyzer::analyzeExpr(const ast::Expr& expr) {
    std::visit([this](const auto& e) {
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, ast::Identifier>) {
            if (!lookup(e.name))
                throw SemanticError("Undefined variable: " + e.name);
        } else if constexpr (std::is_same_v<T, ast::Binary>) {
            analyzeExpr(*e.lhs);
            analyzeExpr(*e.rhs);
        } else if constexpr (std::is_same_v<T, ast::Unary>) {
            analyzeExpr(*e.rhs);
        } else if constexpr (std::is_same_v<T, ast::Call>) {
            analyzeExpr(*e.callee);
            for (const auto& arg : e.args) analyzeExpr(*arg);
        } else if constexpr (std::is_same_v<T, ast::Assignment>) {
            analyzeExpr(*e.target);
            analyzeExpr(*e.value);
        } else if constexpr (std::is_same_v<T, ast::Indices>) {
            analyzeExpr(*e.target);
            for (const auto& idx : e.indices) analyzeExpr(*idx);
        } else if constexpr (std::is_same_v<T, ast::ListLit>) {
            for (const auto& el : e.elements) analyzeExpr(*el);
        } else if constexpr (std::is_same_v<T, ast::Slice>) {
            analyzeExpr(*e.target);
            if (e.start) analyzeExpr(*e.start);
            if (e.stop)  analyzeExpr(*e.stop);
        }
        // Literals (IntLit, FloatLit, StringLit, BoolLit) need no checking
    }, expr);
}

// ** Helpers

std::string SemanticAnalyzer::typeExprToStr(const ast::Expr& expr) const {
    if (const auto* id = std::get_if<ast::Identifier>(&expr))
        return id->name;
    if (const auto* idx = std::get_if<ast::Indices>(&expr)) {
        std::string base = typeExprToStr(*idx->target);
        if (!idx->indices.empty()) {
            base += "[" + typeExprToStr(*idx->indices[0]) + "]";
        }
        return base;
    }
    return "auto";
}

} // namespace alder::semantic
