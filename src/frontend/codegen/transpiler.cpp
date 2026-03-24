#include "transpiler.h"
#include <stdexcept>
#include <unordered_map>

namespace alder::codegen {

// Builtins that map directly to a C++ name with the same call signature.
static const std::unordered_map<std::string, std::string> SIMPLE_REMAP = {
    {"abs",   "std::abs"},
    {"min",   "std::min"},
    {"max",   "std::max"},
    {"pow",   "std::pow"},
    {"sqrt",  "std::sqrt"},
    {"floor", "std::floor"},
    {"ceil",  "std::ceil"},
};

// ** Public entry point

std::string Transpiler::transpile(const ast::Module& module) {
    out_.str("");
    out_.clear();
    indentLevel_ = 0;

    emitLine("#include <iostream>");
    emitLine("#include <string>");
    emitLine("#include <vector>");
    emitLine("#include <cmath>");
    emitLine("#include <algorithm>");
    emitLine();

    emitStdlibPreamble();
		
		// parse macros first
		for (const auto& decl : module.declarations) {
			if (auto* macro_decl = std::get_if<ast::Macro>(&decl)) {
				emitMacro(*macro_decl);
			}
		}

    for (const auto& decl : module.declarations)
        emitDecl(decl);

    return out_.str();
}

// ** Stdlib helper functions emitted into every output file

void Transpiler::emitStdlibPreamble() {
    // range(stop)
    emitLine("static std::vector<int> range(int stop) {");
    emitLine("    std::vector<int> v(stop);");
    emitLine("    for (int i = 0; i < stop; ++i) v[i] = i;");
    emitLine("    return v;");
    emitLine("}");

    // range(start, stop)
    emitLine("static std::vector<int> range(int start, int stop) {");
    emitLine("    std::vector<int> v;");
    emitLine("    v.reserve(stop - start);");
    emitLine("    for (int i = start; i < stop; ++i) v.push_back(i);");
    emitLine("    return v;");
    emitLine("}");

    // input(prompt)
    emitLine("static std::string input(const std::string& prompt = \"\") {");
    emitLine("    std::cout << prompt;");
    emitLine("    std::string _s;");
    emitLine("    std::getline(std::cin, _s);");
    emitLine("    return _s;");
    emitLine("}");

    // operator<< for list — lets print() work on list values
    emitLine("template<typename T>");
    emitLine("static std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {");
    emitLine("    os << \"[\";");
    emitLine("    for (size_t i = 0; i < v.size(); ++i) {");
    emitLine("        if (i > 0) os << \", \";");
    emitLine("        os << v[i];");
    emitLine("    }");
    emitLine("    return os << \"]\";");
    emitLine("}");
    emitLine();

    // _slice(v, start, stop) — backing for list slicing
    // start defaults to 0, stop defaults to a large sentinel meaning "end"
    emitLine("template<typename T>");
    emitLine("static std::vector<T> _slice(const std::vector<T>& v, int start = 0, int stop = 0x7fffffff) {");
    emitLine("    int n = (int)v.size();");
    emitLine("    if (stop > n) stop = n;");
    emitLine("    if (start < 0) start = std::max(0, n + start);");
    emitLine("    if (stop  < 0) stop  = std::max(0, n + stop);");
    emitLine("    start = std::min(start, n);");
    emitLine("    stop  = std::min(stop,  n);");
    emitLine("    if (stop <= start) return {};");
    emitLine("    return {v.begin() + start, v.begin() + stop};");
    emitLine("}");

    emitLine();
}

// ** Emit helpers

void Transpiler::emit(const std::string& s) {
    out_ << s;
}

void Transpiler::emitLine(const std::string& s) {
    out_ << s << "\n";
}

void Transpiler::emitIndent() {
    for (int i = 0; i < indentLevel_; ++i)
        out_ << "    ";
}

// ** Top-level declarations

void Transpiler::emitDecl(const ast::Decl& decl) {
    std::visit([this](const auto& d) {
        using T = std::decay_t<decltype(d)>;
        if constexpr (std::is_same_v<T, ast::Func>)
            emitFunc(d);
        else if constexpr (std::is_same_v<T, ast::GlobalStmt>)
            emitStmt(*d.stmt);
				// don't parse macros, they are preprocessed
    }, decl);
}

void Transpiler::emitFunc(const ast::Func& func) {
    std::string retType = func.retTypeExpr ? typeToStr(*func.retTypeExpr) : "void";

    std::string params;
    for (size_t i = 0; i < func.params.size(); ++i) {
        const auto& p = func.params[i];
        std::string ptype = p.type ? typeToStr(*p.type) : "auto";
        if (i > 0) params += ", ";
        params += ptype + " " + p.name.name;
    }

    emitIndent();
    emit(retType + " " + func.name.name + "(" + params + ") ");
    if (func.body) {
        emitLine("{");
        ++indentLevel_;
        for (const auto& stmt : func.body->body)
            emitStmt(*stmt);
        --indentLevel_;
        emitIndent();
        emitLine("}");
    } else {
        emitLine("{}");
    }
    emitLine();
}

void Transpiler::emitMacro(const ast::Macro& macro) {
	emitLine("#define " + macro.name.name + " " + exprToStr(*macro.value));
}

// ** Statements

void Transpiler::emitBlock(const ast::Block& block) {
    emitLine("{");
    ++indentLevel_;
    for (const auto& stmt : block.body)
        emitStmt(*stmt);
    --indentLevel_;
    emitIndent();
    emit("}");
}

void Transpiler::emitStmt(const ast::Stmt& stmt) {
    std::visit([this](const auto& s) {
        using T = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<T, ast::VarDecl>)       emitVarDecl(s);
        else if constexpr (std::is_same_v<T, ast::ForIn>)    emitForIn(s);
        else if constexpr (std::is_same_v<T, ast::While>)    emitWhile(s);
        else if constexpr (std::is_same_v<T, ast::BrChain>)  emitBrChain(s);
        else if constexpr (std::is_same_v<T, ast::Return>)   emitReturn(s);
        else if constexpr (std::is_same_v<T, ast::ExprStmt>) emitExprStmt(s);
    }, stmt);
}

void Transpiler::emitVarDecl(const ast::VarDecl& decl) {
    std::string type = decl.typeExpr ? typeToStr(*decl.typeExpr) : "auto";
    std::string init = decl.init     ? exprToStr(*decl.init)     : "";

    emitIndent();
    if (decl.isFinal) emit("const ");
    emit(type + " " + decl.name.name);
    if (!init.empty()) emit(" = " + init);
    emitLine(";");
}

void Transpiler::emitForIn(const ast::ForIn& forIn) {
    std::string binding;
    const auto& bindings = forIn.binding.varBindings;
    if (bindings.size() == 1) {
        const auto& vb = bindings[0];
        std::string t = vb.typeExpr ? typeToStr(*vb.typeExpr) : "auto";
        binding = t + " " + vb.name.name;
    } else {
        binding = "auto [";
        for (size_t i = 0; i < bindings.size(); ++i) {
            if (i > 0) binding += ", ";
            binding += bindings[i].name.name;
        }
        binding += "]";
    }

    std::string iterable = forIn.iterable ? exprToStr(*forIn.iterable) : "";

    emitIndent();
    emit("for (" + binding + " : " + iterable + ") ");
    if (forIn.body) {
        emitBlock(*forIn.body);
        emitLine();
    } else {
        emitLine("{}");
    }
}

void Transpiler::emitWhile(const ast::While& loop) {
    std::string cond = loop.cond ? exprToStr(*loop.cond) : "true";
    emitIndent();
    emit("while (" + cond + ") ");
    if (loop.body) {
        emitBlock(*loop.body);
        emitLine();
    } else {
        emitLine("{}");
    }
}

void Transpiler::emitBrChain(const ast::BrChain& chain) {
    std::string cond = chain.ifBlock.cond ? exprToStr(*chain.ifBlock.cond) : "true";
    emitIndent();
    emit("if (" + cond + ") ");
    if (chain.ifBlock.body) emitBlock(*chain.ifBlock.body);
    else emit("{}");

    for (const auto& elif : chain.elifBranches) {
        std::string ec = elif.cond ? exprToStr(*elif.cond) : "true";
        emit(" else if (" + ec + ") ");
        if (elif.body) emitBlock(*elif.body);
        else emit("{}");
    }

    if (chain.elseBlock) {
        emit(" else ");
        emitBlock(*chain.elseBlock);
    }
    emitLine();
}

void Transpiler::emitReturn(const ast::Return& ret) {
    emitIndent();
    if (ret.retExpr)
        emitLine("return " + exprToStr(*ret.retExpr) + ";");
    else
        emitLine("return;");
}

void Transpiler::emitExprStmt(const ast::ExprStmt& es) {
    emitIndent();
    emitLine(exprToStr(*es.expr) + ";");
}

// ** Expressions

std::string Transpiler::exprToStr(const ast::Expr& expr) {
    return std::visit([this](const auto& e) -> std::string {
        using T = std::decay_t<decltype(e)>;

        if constexpr (std::is_same_v<T, ast::Identifier>) {
            return e.name;
        }
        else if constexpr (std::is_same_v<T, ast::IntLit>) {
            return e.lexeme;
        }
        else if constexpr (std::is_same_v<T, ast::FloatLit>) {
            return e.lexeme;
        }
        else if constexpr (std::is_same_v<T, ast::StringLit>) {
            return "\"" + e.value + "\"";
        }
        else if constexpr (std::is_same_v<T, ast::BoolLit>) {
            return e.value;
        }
        else if constexpr (std::is_same_v<T, ast::ListLit>) {
            std::string s = "{";
            for (size_t i = 0; i < e.elements.size(); ++i) {
                if (i > 0) s += ", ";
                s += exprToStr(*e.elements[i]);
            }
            return s + "}";
        }
        else if constexpr (std::is_same_v<T, ast::Unary>) {
            return unaryOpStr(e.op) + exprToStr(*e.rhs);
        }
        else if constexpr (std::is_same_v<T, ast::Binary>) {
            return "(" + exprToStr(*e.lhs) + " " + binaryOpStr(e.op) + " " + exprToStr(*e.rhs) + ")";
        }
        else if constexpr (std::is_same_v<T, ast::Call>) {
            return emitCall(e);
        }
        else if constexpr (std::is_same_v<T, ast::Assignment>) {
            return exprToStr(*e.target) + " = " + exprToStr(*e.value);
        }
        else if constexpr (std::is_same_v<T, ast::Indices>) {
            std::string s = exprToStr(*e.target);
            for (const auto& idx : e.indices)
                s += "[" + exprToStr(*idx) + "]";
            return s;
        }
        else if constexpr (std::is_same_v<T, ast::Slice>) {
            std::string target = exprToStr(*e.target);
            std::string start  = e.start ? exprToStr(*e.start) : "0";
            std::string stop   = e.stop  ? exprToStr(*e.stop)  : "0x7fffffff";
            return "_slice(" + target + ", " + start + ", " + stop + ")";
        }
        return "";
    }, expr);
}

std::string Transpiler::emitCall(const ast::Call& call) {
    // Collect stringified args upfront
    std::vector<std::string> args;
    for (const auto& a : call.args)
        args.push_back(exprToStr(*a));

    auto joinArgs = [&]() {
        std::string s;
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) s += ", ";
            s += args[i];
        }
        return s;
    };

    const auto* id = std::get_if<ast::Identifier>(call.callee.get());
    if (!id) {
        // Indirect call (e.g. function pointer)
        return exprToStr(*call.callee) + "(" + joinArgs() + ")";
    }

    const std::string& name = id->name;

    // print(...) → std::cout << a << b << ... << "\n"
    if (name == "print") {
        if (args.empty()) return "std::cout << \"\\n\"";
        std::string s = "std::cout";
        for (const auto& a : args) s += " << " + a;
        s += " << \"\\n\"";
        return s;
    }

    // len(x) → (int)(x).size()
    if (name == "len" && args.size() == 1)
        return "(int)(" + args[0] + ").size()";

    // str(x) → std::to_string(x)
    if (name == "str" && args.size() == 1)
        return "std::to_string(" + args[0] + ")";

    // int(x) → (int)(x)
    if (name == "int" && args.size() == 1)
        return "(int)(" + args[0] + ")";

    // float(x) → (double)(x)
    if (name == "float" && args.size() == 1)
        return "(double)(" + args[0] + ")";

    // append(list, val) → list.push_back(val)
    if (name == "append" && args.size() == 2)
        return args[0] + ".push_back(" + args[1] + ")";

    // pop(list) → list.pop_back()
    if (name == "pop" && args.size() == 1)
        return args[0] + ".pop_back()";

    // abs / min / max / pow / sqrt / floor / ceil → std:: equivalent
    if (auto it = SIMPLE_REMAP.find(name); it != SIMPLE_REMAP.end())
        return it->second + "(" + joinArgs() + ")";

    // Everything else (user functions, input, range, ...) passes through as-is
    return name + "(" + joinArgs() + ")";
}

std::string Transpiler::typeToStr(const ast::Expr& expr) {
    if (const auto* id = std::get_if<ast::Identifier>(&expr))
        return mapTypeName(id->name);
    if (const auto* idx = std::get_if<ast::Indices>(&expr)) {
        std::string base = typeToStr(*idx->target);
        if (base == "std::vector" && !idx->indices.empty())
            return "std::vector<" + typeToStr(*idx->indices[0]) + ">";
        std::string s = base + "<";
        for (size_t i = 0; i < idx->indices.size(); ++i) {
            if (i > 0) s += ", ";
            s += typeToStr(*idx->indices[i]);
        }
        return s + ">";
    }
    return "auto";
}

std::string Transpiler::mapTypeName(const std::string& name) const {
    if (name == "int")    return "int";
    if (name == "float")  return "double";
    if (name == "string") return "std::string";
    if (name == "bool")   return "bool";
    if (name == "void")   return "void";
    if (name == "list")   return "std::vector";
    return name;
}

std::string Transpiler::binaryOpStr(ast::BinaryOp op) const {
    switch (op) {
        case ast::BinaryOp::Add:      return "+";
        case ast::BinaryOp::Sub:      return "-";
        case ast::BinaryOp::Mul:      return "*";
        case ast::BinaryOp::Div:      return "/";
        case ast::BinaryOp::Eq:       return "==";
        case ast::BinaryOp::NotEq:    return "!=";
        case ast::BinaryOp::Lt:       return "<";
        case ast::BinaryOp::Lte:      return "<=";
        case ast::BinaryOp::Gt:       return ">";
        case ast::BinaryOp::Gte:      return ">=";
        case ast::BinaryOp::And:      return "&&";
        case ast::BinaryOp::Or:       return "||";
        case ast::BinaryOp::Shl:      return "<<";
        case ast::BinaryOp::Shr:      return ">>";
        case ast::BinaryOp::LogicShr: return ">>";
    }
    return "?";
}

std::string Transpiler::unaryOpStr(ast::UnaryOp op) const {
    switch (op) {
        case ast::UnaryOp::Neg: return "-";
        case ast::UnaryOp::Not: return "!";
    }
    return "?";
}

} // namespace alder::codegen
