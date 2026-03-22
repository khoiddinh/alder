#pragma once

#include <string>
#include <vector>
#include <stdexcept>

namespace alder::diag {

// ** Source location **

struct SourceLocation {
    int line = 0;  // 1-based; 0 = unknown
    int col  = 0;  // 1-based; 0 = unknown
};

// ** Diagnostic (message + location) **

struct Diagnostic {
    std::string    message;
    SourceLocation loc;
};

// ** Formatted output **
//
// Produces output like:
//
//   foo.adr:10:5: error: expected ':' after type
//       result: int = 1
//       ^
//
// If loc.line is 0, the source snippet is omitted.

std::string format(
    const std::string&              filename,
    const std::vector<std::string>& sourceLines,
    const Diagnostic&               diag
);

// ** Exception type **

struct CompileError : std::runtime_error {
    Diagnostic diagnostic;

    CompileError(std::string message, SourceLocation loc = {})
        : std::runtime_error(message)
        , diagnostic{std::move(message), loc} {}
};

} // namespace alder::diag
