#pragma once

#include <memory>

namespace mc::test {

class BaseGameTestInstance;

/**
 * @brief 测试状态监听器接口（6 回调）。
 *
 * 对齐基岩版 `IGameTestListener`（见校正 6）：6 个回调，参数均为 `BaseGameTestInstance&`。
 * 挂在 `BaseGameTestInstance` 上反应状态变化，由 runner/报告器/可视化器实现：
 * - `ConsoleGameTestListener`：控制台日志（runner/reporter/）。
 * - `WorldVisualizationListener`：游戏内信标光束（minecraft/listener/）。
 * - `GameTestBatchRunnerGameTestListener`：批次 runner 内部协调（framework/batch/）。
 *
 * 与 `*Reporter`（`GlobalTestReporter`，全局报告输出）概念区分：listener 反应单实例状态，
 * reporter 聚合全局结果。
 */
class IGameTestListener {
public:
    virtual ~IGameTestListener() = default;

    /** @brief 结构已加载（放置完成，即将开始 setup）。 */
    virtual void onTestStructureLoaded(BaseGameTestInstance& test) = 0;
    /** @brief 测试已开始（setup 结束，正式运行）。 */
    virtual void onTestStarted(BaseGameTestInstance& test) = 0;
    /** @brief 测试已通过。 */
    virtual void onTestPassed(BaseGameTestInstance& test) = 0;
    /** @brief 测试已失败。 */
    virtual void onTestFailed(BaseGameTestInstance& test) = 0;
    /** @brief 重试已开始（flaky 测试再次运行）。 */
    virtual void onTestRetryStarted(BaseGameTestInstance& test) = 0;
    /** @brief 重试已结束（flaky 测试重试完成）。 */
    virtual void onTestRetryFinished(BaseGameTestInstance& test) = 0;
};

} // namespace mc::test
