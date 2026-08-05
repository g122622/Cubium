#pragma once

#include "server/test/runner/tracker/MultipleTestTracker.hpp"

#include <memory>
#include <string>

namespace mc::test {

class BaseGameTestInstance;

/**
 * @brief 测试报告器接口。
 *
 * 对齐 Java `TestReporter`：在测试生命周期事件（通过/失败）与批次/全部完成时被回调，输出报告。
 * 与 `IGameTestListener`（单实例状态反应）概念区分：reporter 聚合全局结果输出。
 *
 * 实现：`LogTestReporter`（spdlog）、`JUnitTestReporter`（JUnit XML）、`GlobalTestReporter`（静态委托）。
 */
class TestReporter {
public:
    virtual ~TestReporter() = default;

    virtual void onTestPassed(const BaseGameTestInstance& test) = 0;
    virtual void onTestFailed(const BaseGameTestInstance& test) = 0;
    virtual void onBatchFinished(const MultipleTestTracker& tracker) = 0;
    virtual void onAllFinished(const MultipleTestTracker& tracker) = 0;
};

} // namespace mc::test
