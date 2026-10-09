#include "server/test/runner/reporter/JUnitTestReporter.hpp"

#include "common/util/assert/AssertMacros.hpp" // MC_UNUSED
#include "server/test/framework/instance/BaseGameTestInstance.hpp"

#include <algorithm>
#include <spdlog/spdlog.h>

namespace mc::test {

JUnitTestReporter::JUnitTestReporter(std::filesystem::path reportPath)
    : m_reportPath(std::move(reportPath))
{}

JUnitTestReporter::_CaseRecord& JUnitTestReporter::_recordFor(const BaseGameTestFunction& test)
{
    auto it =
        std::find_if(m_records.begin(), m_records.end(), [&](const auto& rec) { return rec.name == test.testName(); });
    if (it != m_records.end()) {
        return *it;
    }
    _CaseRecord rec;
    rec.name = test.testName();
    rec.classname = test.data().structure();
    rec.required = test.data().required();
    m_records.push_back(std::move(rec));
    return m_records.back();
}

void JUnitTestReporter::prepareTests(const std::vector<std::shared_ptr<BaseGameTestFunction>>& tests)
{
    for (const auto& test : tests) {
        auto& rec = _recordFor(*test);
        rec.passed = false;
        rec.required = false;
        rec.failureMessage = "Not started: run ended before this test could execute";
    }
    _writeXml();
}

void JUnitTestReporter::onTestSkipped(const BaseGameTestFunction& test, std::string reason)
{
    auto& rec = _recordFor(test);
    rec.passed = false;
    rec.required = false;
    rec.failureMessage = std::move(reason);
    _writeXml();
}

void JUnitTestReporter::onTestStarted(const BaseGameTestInstance& test)
{
    auto& rec = _recordFor(test.function());
    rec.passed = false;
    rec.incomplete = true;
    rec.failureMessage = "Execution interrupted before test completed";
    _writeXml();
}

void JUnitTestReporter::onTestPassed(const BaseGameTestInstance& test)
{
    auto& rec = _recordFor(test.function());
    rec.timeSeconds = test.wallTimeSeconds();
    rec.passed = true;
    rec.incomplete = false;
    rec.required = test.function().data().required();
    rec.failureMessage.clear();
    _writeXml();
}

void JUnitTestReporter::onTestFailed(const BaseGameTestInstance& test)
{
    auto& rec = _recordFor(test.function());
    rec.timeSeconds = test.wallTimeSeconds();
    rec.passed = false;
    rec.incomplete = false;
    rec.required = test.function().data().required();
    const auto& err = test.error();
    rec.failureMessage = err.has_value() ? err->formattedMessage() : "unknown failure";
    _writeXml();
}

void JUnitTestReporter::onBatchFinished(const MultipleTestTracker& tracker)
{
    MC_UNUSED(tracker);
    _writeXml();
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
    // 先写完整临时文件再替换；看门狗终止进程时至少保留上一份完整快照。
    auto temporaryPath = m_reportPath;
    temporaryPath += ".tmp";
    std::ofstream out(temporaryPath);
    if (!out.is_open()) {
        m_ioError = true;
        spdlog::warn("[GameTest] JUnitTestReporter: failed to open report path '{}'", m_reportPath.string());
        return;
    }

    std::size_t failures = 0;
    std::size_t skipped = 0;
    std::size_t errors = 0;
    for (const auto& rec : m_records) {
        if (rec.incomplete) {
            ++errors;
        } else if (!rec.passed) {
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
        << "\" skipped=\"" << skipped << "\" errors=\"" << errors << "\">\n";

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
            if (rec.incomplete) {
                out << "      <error message=\"" << msg << "\"/>\n";
            } else if (rec.required) {
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
    out.close();
    if (out.fail()) {
        m_ioError = true;
        spdlog::warn("[GameTest] JUnitTestReporter: io error while writing report");
        return;
    }
    std::error_code error;
    std::filesystem::rename(temporaryPath, m_reportPath, error);
    if (error) {
        m_ioError = true;
        spdlog::warn("[GameTest] JUnitTestReporter: failed to publish report: {}", error.message());
    }
}

} // namespace mc::test
