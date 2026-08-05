#pragma once

#include "common/test/base/data/TestParameters.hpp"
#include "common/test/framework/batch/GameTestBatch.hpp"
#include "common/test/framework/batch/GameTestBatchListener.hpp"

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

    GameTestTicker& _ticker() noexcept { return m_ticker; }
    const TestParameters& _params() const noexcept { return m_params; }

    /**
     * @brief 子类在 _runTest 中调用，把实例纳入批次跟踪（runner 持所有权，ticker 持裸指针）。
     */
    void _trackInstance(std::unique_ptr<BaseGameTestInstance> instance);

private:
    void _runBatch(std::size_t batchIndex);

    std::vector<GameTestBatch> m_batches;
    GameTestTicker& m_ticker;
    TestParameters m_params;
    std::vector<std::shared_ptr<GameTestBatchListener>> m_batchListeners;
    std::vector<std::unique_ptr<BaseGameTestInstance>> m_currentBatchInstances;
    std::size_t m_currentBatch = 0;
    bool m_started = false;
    std::size_t m_totalTestCount = 0;
    std::size_t m_passedCount = 0;
    std::size_t m_failedCount = 0;
    std::size_t m_failedRequiredCount = 0;
};

} // namespace mc::test
