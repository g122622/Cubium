#pragma once

#include "server/test/runner/reporter/TestReporter.hpp"

namespace mc::test {

/**
 * @brief spdlog 控制台报告器。
 *
 * 对齐 Java `LogTestReporter`：required 失败用 `spdlog::error`，optional 失败用 `spdlog::warn`，
 * 通过用 `spdlog::info`。批次/全部完成时输出汇总计数。
 */
class LogTestReporter final : public TestReporter {
public:
    LogTestReporter() = default;

    void onTestPassed(const BaseGameTestInstance& test) override;
    void onTestFailed(const BaseGameTestInstance& test) override;
    void onBatchFinished(const MultipleTestTracker& tracker) override;
    void onAllFinished(const MultipleTestTracker& tracker) override;
};

} // namespace mc::test
