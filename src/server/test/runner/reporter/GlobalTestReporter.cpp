#include "server/test/runner/reporter/GlobalTestReporter.hpp"

namespace mc::test {

GlobalTestReporter& GlobalTestReporter::instance() noexcept
{
    static GlobalTestReporter s_instance;
    return s_instance;
}

void GlobalTestReporter::addReporter(std::shared_ptr<TestReporter> reporter)
{
    if (reporter) {
        m_reporters.push_back(std::move(reporter));
    }
}

void GlobalTestReporter::clear() noexcept
{
    m_reporters.clear();
}

void GlobalTestReporter::onTestPassed(const BaseGameTestInstance& test)
{
    for (auto& r : m_reporters) {
        r->onTestPassed(test);
    }
}

void GlobalTestReporter::onTestFailed(const BaseGameTestInstance& test)
{
    for (auto& r : m_reporters) {
        r->onTestFailed(test);
    }
}

void GlobalTestReporter::onBatchFinished(const MultipleTestTracker& tracker)
{
    for (auto& r : m_reporters) {
        r->onBatchFinished(tracker);
    }
}

void GlobalTestReporter::onAllFinished(const MultipleTestTracker& tracker)
{
    for (auto& r : m_reporters) {
        r->onAllFinished(tracker);
    }
}

} // namespace mc::test
