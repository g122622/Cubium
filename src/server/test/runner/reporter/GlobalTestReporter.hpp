#pragma once

#include "server/test/runner/reporter/TestReporter.hpp"
#include "server/test/runner/tracker/MultipleTestTracker.hpp"

#include <memory>
#include <vector>

namespace mc::test {

class BaseGameTestInstance;

/**
 * @brief 全局报告器：静态委托给一组 `TestReporter`。
 *
 * 对齐 Java `GlobalTestReporter`：runner 把所有报告器注册到全局单例，事件广播到全部。
 * 单例（`instance()`），`GameTestServer`/`GameTestCommand` 启动期注册 `LogTestReporter`/`JUnitTestReporter`。
 */
class GlobalTestReporter {
public:
    [[nodiscard]] static GlobalTestReporter& instance() noexcept;

    void addReporter(std::shared_ptr<TestReporter> reporter);
    void clear() noexcept;

    void onTestPassed(const BaseGameTestInstance& test);
    void onTestFailed(const BaseGameTestInstance& test);
    void onBatchFinished(const MultipleTestTracker& tracker);
    void onAllFinished(const MultipleTestTracker& tracker);

private:
    GlobalTestReporter() = default;

    std::vector<std::shared_ptr<TestReporter>> m_reporters;
};

} // namespace mc::test
