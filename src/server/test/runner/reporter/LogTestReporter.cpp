#include "server/test/runner/reporter/LogTestReporter.hpp"

#include "common/test/framework/instance/BaseGameTestInstance.hpp"

#include <spdlog/spdlog.h>

namespace mc::test {

void LogTestReporter::onTestPassed(const BaseGameTestInstance& test)
{
    spdlog::info("[GameTest] PASSED: {}", test.function().testName());
}

void LogTestReporter::onTestFailed(const BaseGameTestInstance& test)
{
    const auto& err = test.error();
    const std::string name = test.function().testName();
    if (test.function().data().required()) {
        if (err.has_value()) {
            spdlog::error("[GameTest] FAILED (required): {} - {}", name, err->formattedMessage());
        } else {
            spdlog::error("[GameTest] FAILED (required): {}", name);
        }
    } else {
        if (err.has_value()) {
            spdlog::warn("[GameTest] FAILED (optional): {} - {}", name, err->formattedMessage());
        } else {
            spdlog::warn("[GameTest] FAILED (optional): {}", name);
        }
    }
}

void LogTestReporter::onBatchFinished(const MultipleTestTracker& tracker)
{
    spdlog::info(
        "[GameTest] batch finished: {}/{} passed, {} failed", tracker.passed(), tracker.total(), tracker.failed());
}

void LogTestReporter::onAllFinished(const MultipleTestTracker& tracker)
{
    spdlog::info(
        "[GameTest] all finished: {}/{} passed, {} failed", tracker.passed(), tracker.total(), tracker.failed());
}

} // namespace mc::test
