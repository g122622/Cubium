#pragma once

#include "common/test/base/data/TestParameters.hpp"
#include "common/test/base/error/GameTestError.hpp"  // GameTestError/_failBatchEnvironment 参数
#include "common/test/base/error/GameTestResult.hpp" // GameTestResult（_applyBatchEnvironment* 返回值）
#include "common/test/framework/batch/GameTestBatch.hpp"
#include "common/test/framework/batch/GameTestBatchListener.hpp"
#include "common/test/framework/listener/IGameTestListener.hpp" // setInstanceListener 持有
#include "common/util/assert/AssertMacros.hpp"                  // MC_UNUSED

#include <memory>
#include <vector>

namespace mc::test {

class BaseGameTestInstance;
class GameTestTicker;

/**
 * @brief 批次 runner 抽象基类。
 *
 * 对齐基岩版 `BaseGameTestBatchRunner`：持有一组 `GameTestBatch` + `GameTestTicker` + `TestParameters`，
 * 按 batch 顺序运行：`start()` 启动第一批，`_runBatch(i)` 内对每个测试函数创建实例、加入 ticker；
 * 当前批次所有实例完成（`isDone`）后推进下一批。批次开始/结束触发 `GameTestBatchListener`。
 *
 * 纯虚方法（由 `MinecraftGameTestBatchRunner` 1C 阶段实现）：
 * - `_createGameTestInstance(function, rotation)`：创建具体 `BaseGameTestInstance`（含 helper provider）。
 * - `_runTest(instance)`：把实例加入 ticker 并触发结构放置。
 *
 * 实例所有权归 runner（`m_instances`），ticker 持裸指针。
 */
class BaseGameTestBatchRunner {
public:
    BaseGameTestBatchRunner(std::vector<GameTestBatch> batches, GameTestTicker& ticker, TestParameters params);
    virtual ~BaseGameTestBatchRunner() = default;

    void start();
    void addBatchListener(std::shared_ptr<GameTestBatchListener> listener);

    /**
     * @brief 设置实例级监听器（挂到每个新创建的 `BaseGameTestInstance`）。
     *
     * 由 `GameTestRunner` 注入 `_RunnerListener`，实例 succeed/fail 时更新 tracker +
     * 广播到 `GlobalTestReporter`。须在 `start()` 前调用。
     */
    void setInstanceListener(std::shared_ptr<IGameTestListener> listener);

    /**
     * @brief 每 tick 调用，检查当前批次是否完成，推进下一批。
     *
     * 由 `GameTestServer`/`IntegratedServer` 的 tick 末尾调用（或经 `GameTestTicker` 间接驱动）。
     */
    void tick();

    [[nodiscard]] bool isComplete() const noexcept;
    [[nodiscard]] std::size_t totalTestCount() const noexcept { return m_totalTestCount; }
    [[nodiscard]] std::size_t passedCount() const noexcept { return m_passedCount; }
    [[nodiscard]] std::size_t failedCount() const noexcept { return m_failedCount; }
    [[nodiscard]] std::size_t failedRequiredCount() const noexcept { return m_failedRequiredCount; }

protected:
    [[nodiscard]] virtual std::unique_ptr<BaseGameTestInstance> _createGameTestInstance(
        BaseGameTestFunction& function, Rotation rotation) = 0;
    virtual void _runTest(std::unique_ptr<BaseGameTestInstance> instance) = 0;

    /**
     * @brief 批次环境 setup 钩子（在 beforeBatch 回调后、创建实例前调用）。
     *
     * framework 层引擎无关，默认空实现（nullopt=成功）。minecraft 绑定层 override 此方法经
     * `MinecraftEnvironmentApplier` 把环境应用到 `ServerWorld`（天气/时间/游戏规则等）。
     * 返回非 nullopt 即批次 setup 失败——记为批次错误，跳过本批实例创建。
     *
     * @param batch 当前批次（取其 environment()）。
     * @return nullopt=成功；非 nullopt=批次 setup 失败。
     */
    virtual GameTestResult _applyBatchEnvironmentSetup(const GameTestBatch& batch)
    {
        MC_UNUSED(batch);
        return mc::test::pass();
    }

    /**
     * @brief 批次环境 teardown 钩子（在 afterBatch 回调前调用）。
     *
     * 默认空实现。minecraft 绑定层 override 还原世界状态（如 resetWeather）。
     */
    virtual GameTestResult _applyBatchEnvironmentTeardown(const GameTestBatch& batch)
    {
        MC_UNUSED(batch);
        return mc::test::pass();
    }

    GameTestTicker& _ticker() noexcept { return m_ticker; }
    const TestParameters& _params() const noexcept { return m_params; }

    /**
     * @brief 子类在 _runTest 中调用，把实例纳入批次跟踪（runner 持所有权，ticker 持裸指针）。
     */
    void _trackInstance(std::unique_ptr<BaseGameTestInstance> instance);

    /**
     * @brief 标记当前批次 setup 失败（环境应用返回错误时调用）。
     *
     * 把本批所有测试函数计为 failed（required 测试计入 failedRequiredCount），跳过实例创建。
     */
    void _failBatchEnvironment(GameTestBatch& batch, const GameTestError& error);

private:
    void _runBatch(std::size_t batchIndex);

    std::vector<GameTestBatch> m_batches;
    GameTestTicker& m_ticker;
    TestParameters m_params;
    std::vector<std::shared_ptr<GameTestBatchListener>> m_batchListeners;
    std::shared_ptr<IGameTestListener> m_instanceListener; // 挂到每个实例（_RunnerListener）
    std::vector<std::unique_ptr<BaseGameTestInstance>> m_currentBatchInstances;
    std::size_t m_currentBatch = 0;
    bool m_started = false;
    std::size_t m_totalTestCount = 0;
    std::size_t m_passedCount = 0;
    std::size_t m_failedCount = 0;
    std::size_t m_failedRequiredCount = 0;
};

} // namespace mc::test
