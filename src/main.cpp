#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <filesystem>
#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "transpiler.h"
#include "diagnostic.h"

namespace fs = std::filesystem;

static std::string stem(const std::string& path) {
    return fs::path(path).stem().string();
}

static std::string findCompiler() {
    if (std::system("clang++ --version > /dev/null 2>&1") == 0) return "clang++";
    if (std::system("g++ --version > /dev/null 2>&1") == 0)     return "g++";
    return "";
}

int main(int argc, char* argv[]) {
    // Parse CLI args
    // Usage:
    //   alder <source.adr>                 — compile to binary (same name as source)
    //   alder <source.adr> -o <bin>        — compile to named binary
    //   alder <source.adr> --emit-cpp <f>  — write C++ only, no compilation

    if (argc < 2) {
        std::cerr << "Usage: alder <source.adr> [-o <binary>] [--emit-cpp <file>]\n";
        return 1;
    }

    std::string sourceFile = argv[1];
    std::string outputBin;
    std::string emitCpp;

    for (int i = 2; i < argc; ++i) {
        std::string flag = argv[i];
        if (flag == "-o" && i + 1 < argc) {
            outputBin = argv[++i];
        } else if (flag == "--emit-cpp" && i + 1 < argc) {
            emitCpp = argv[++i];
        } else {
            std::cerr << "Unknown argument: " << flag << "\n";
            return 1;
        }
    }

    // Default binary name: source stem (e.g. foo.adr -> foo)
    if (outputBin.empty() && emitCpp.empty()) {
        outputBin = stem(sourceFile);
    }

    // Read source
    std::ifstream file(sourceFile);
    if (!file) {
        std::cerr << "Error: cannot open '" << sourceFile << "'\n";
        return 1;
    }
    std::ostringstream buf;
    buf << file.rdbuf();
    std::string source = buf.str();

    // Split source into lines for error reporting
    std::vector<std::string> sourceLines;
    {
        std::istringstream ss(source);
        std::string ln;
        while (std::getline(ss, ln))
            sourceLines.push_back(ln);
    }

    // Lex
    alder::lexer::Lexer lexer(source);
    std::vector<alder::token::Token> tokens;
    while (true) {
        alder::token::Token t = lexer.next();
        tokens.push_back(t);
        if (t.type == alder::token::TokenType::Eof) break;
    }

    // Parse
    alder::parser::Parser parser(std::move(tokens));
    alder::ast::Module module;
    try {
        module = parser.parse();
    } catch (const alder::diag::CompileError& e) {
        std::cerr << alder::diag::format(sourceFile, sourceLines, e.diagnostic);
        return 1;
    }

    // Semantic analysis
    alder::semantic::SemanticAnalyzer analyzer;
    try {
        analyzer.analyze(module);
    } catch (const alder::diag::CompileError& e) {
        std::cerr << alder::diag::format(sourceFile, sourceLines, e.diagnostic);
        return 1;
    }

    // Transpile
    alder::codegen::Transpiler transpiler;
    std::string cppSource = transpiler.transpile(module);

    // --emit-cpp: just write the C++ and stop
    if (!emitCpp.empty()) {
        std::ofstream out(emitCpp);
        if (!out) {
            std::cerr << "Error: cannot write to '" << emitCpp << "'\n";
            return 1;
        }
        out << cppSource;
        return 0;
    }

    // Write C++ to a temp file, compile it, then delete it
    std::string tmpCpp = stem(sourceFile) + ".alder_tmp.cpp";

    {
        std::ofstream out(tmpCpp);
        if (!out) {
            std::cerr << "Error: cannot write temporary file '" << tmpCpp << "'\n";
            return 1;
        }
        out << cppSource;
    }

    std::string compiler = findCompiler();
    if (compiler.empty()) {
        std::cerr << "Error: no C++ compiler found (tried clang++, g++)\n";
        fs::remove(tmpCpp);
        return 1;
    }

    std::string cmd = compiler + " -std=c++20 -w " + tmpCpp + " -o " + outputBin;
    int ret = std::system(cmd.c_str());
    fs::remove(tmpCpp);

    if (ret != 0) {
        std::cerr << "Compilation failed\n";
        return 1;
    }

    return 0;
}
