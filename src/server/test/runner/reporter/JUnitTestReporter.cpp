#include "server/test/runner/reporter/JUnitTestReporter.hpp"

#include "common/test/framework/instance/BaseGameTestInstance.hpp"
#include "common/util/assert/AssertMacros.hpp" // MC_UNUSED

#include <spdlog/spdlog.h>

namespace mc::test {

JUnitTestReporter::JUnitTestReporter(std::filesystem::path reportPath)
    : m_reportPath(std::move(reportPath))
{}

void JUnitTestReporter::onTestPassed(const BaseGameTestInstance& test)
{
    _CaseRecord rec;
    rec.name = test.function().testName();
    rec.classname = test.function().data().structure();
    rec.timeSeconds = static_cast<double>(test.tickCount()) / 20.0; // 20 tps
    rec.passed = true;
    rec.required = test.function().data().required();
    m_records.push_back(std::move(rec));
}

void JUnitTestReporter::onTestFailed(const BaseGameTestInstance& test)
{
    _CaseRecord rec;
    rec.name = test.function().testName();
    rec.classname = test.function().data().structure();
    rec.timeSeconds = static_cast<double>(test.tickCount()) / 20.0;
    rec.passed = false;
    rec.required = test.function().data().required();
    const auto& err = test.error();
    rec.failureMessage = err.has_value() ? err->formattedMessage() : "unknown failure";
    m_records.push_back(std::move(rec));
}

void JUnitTestReporter::onBatchFinished(const MultipleTestTracker& tracker)
{
    MC_UNUSED(tracker);
    // 批次边界不写文件；onAllFinished 统一写
}

void JUnitTestReporter::onAllFinished(const MultipleTestTracker& tracker)
{
    MC_UNUSED(tracker);
    _writeXml();
}

void JUnitTestReporter::_xmlEscape(std::string& s)
{
    // JUnit XML 属性/文本转义
    for (std::size_t i = 0; i < s.size();) {
        const char c = s[i];
        std::string repl;
        switch (c) {
            case '<':
                repl = "&lt;";
                break;
            case '>':
                repl = "&gt;";
                break;
            case '&':
                repl = "&amp;";
                break;
            case '"':
                repl = "&quot;";
                break;
            case '\'':
                repl = "&apos;";
                break;
            default:
                ++i;
                continue;
        }
        s.replace(i, 1, repl);
        i += repl.size();
    }
}

void JUnitTestReporter::_writeXml()
{
    std::ofstream out(m_reportPath);
    if (!out.is_open()) {
        m_ioError = true;
        spdlog::warn("[GameTest] JUnitTestReporter: failed to open report path '{}'", m_reportPath.string());
        return;
    }

    std::size_t failures = 0;
    std::size_t skipped = 0;
    for (const auto& rec : m_records) {
        if (!rec.passed) {
            if (rec.required) {
                ++failures;
            } else {
                ++skipped;
            }
        }
    }

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<testsuites>\n";
    out << "  <testsuite name=\"GameTest\" tests=\"" << m_records.size() << "\" failures=\"" << failures
        << "\" skipped=\"" << skipped << "\">\n";

    for (const auto& rec : m_records) {
        std::string name = rec.name;
        std::string cls = rec.classname;
        _xmlEscape(name);
        _xmlEscape(cls);
        out << "    <testcase name=\"" << name << "\" classname=\"" << cls << "\" time=\"" << rec.timeSeconds
            << "\">\n";
        if (!rec.passed) {
            std::string msg = rec.failureMessage;
            _xmlEscape(msg);
            if (rec.required) {
                out << "      <failure message=\"" << msg << "\"/>\n";
            } else {
                out << "      <skipped message=\"" << msg << "\"/>\n";
            }
        }
        out << "    </testcase>\n";
    }

    out << "  </testsuite>\n";
    out << "</testsuites>\n";
    out.flush();
    if (out.bad()) {
        m_ioError = true;
        spdlog::warn("[GameTest] JUnitTestReporter: io error while writing report");
    }
}

} // namespace mc::test
