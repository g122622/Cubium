/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HAVING BEEN CLAIMED FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "common/mod/bedrock/addon/diagnostics/ScriptDiagnostics.hpp"
#include "common/core/Types.hpp"

#include <algorithm>
#include <cstddef>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace mc::mod::bedrock::addon {

static const char* diagnosticLevelName(DiagnosticLevel level)
{
    switch (level) {
        case DiagnosticLevel::Info:
            return "INFO";
        case DiagnosticLevel::Warning:
            return "WARN";
        case DiagnosticLevel::Error:
            return "ERROR";
    }
    return "UNKNOWN";
}

void ScriptDiagnostics::add(
    DiagnosticLevel level, std::string source, std::string message, std::string filename, i32 line, i32 column)
{
    m_entries.push_back({level, std::move(source), std::move(message), std::move(filename), line, column});
}

const std::vector<DiagnosticEntry>& ScriptDiagnostics::entries() const
{
    return m_entries;
}

size_t ScriptDiagnostics::count(DiagnosticLevel level) const
{
    return std::count_if(
        m_entries.begin(), m_entries.end(), [level](const DiagnosticEntry& e) { return e.level == level; });
}

void ScriptDiagnostics::clear()
{
    m_entries.clear();
}

std::string ScriptDiagnostics::generateReport() const
{
    if (m_entries.empty()) {
        return "No diagnostics.";
    }

    std::ostringstream oss;
    oss << "Script Diagnostics Report (" << m_entries.size() << " entries):\n";

    for (const auto& entry : m_entries) {
        oss << "  [" << diagnosticLevelName(entry.level) << "] " << entry.source;
        if (!entry.filename.empty()) {
            oss << " (" << entry.filename;
            if (entry.line >= 0) {
                oss << ":" << entry.line;
                if (entry.column >= 0) {
                    oss << ":" << entry.column;
                }
            }
            oss << ")";
        }
        oss << ": " << entry.message << "\n";
    }

    oss << "\nSummary: " << count(DiagnosticLevel::Error) << " errors, " << count(DiagnosticLevel::Warning)
        << " warnings, " << count(DiagnosticLevel::Info) << " info";

    return oss.str();
}

} // namespace mc::mod::bedrock::addon
