#include "diagnostic.h"
#include <sstream>
#include <algorithm>

namespace alder::diag {

std::string format(
    const std::string&              filename,
    const std::vector<std::string>& sourceLines,
    const Diagnostic&               diag)
{
    std::ostringstream out;

    // file:line:col: error: message
    out << filename;
    if (diag.loc.line > 0) {
        out << ":" << diag.loc.line;
        if (diag.loc.col > 0)
            out << ":" << diag.loc.col;
    }
    out << ": error: " << diag.message << "\n";

    // Source snippet + caret
    if (diag.loc.line > 0 && diag.loc.line <= static_cast<int>(sourceLines.size())) {
        const std::string& src = sourceLines[diag.loc.line - 1];
        out << "    " << src << "\n";

        if (diag.loc.col > 0) {
            out << "    ";
            int limit = std::min(diag.loc.col - 1, static_cast<int>(src.size()));
            for (int i = 0; i < limit; ++i)
                out << (src[i] == '\t' ? '\t' : ' ');
            out << "^\n";
        }
    }

    return out.str();
}

} // namespace alder::diag
