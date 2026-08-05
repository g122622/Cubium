#pragma once

#include "common/test/framework/batch/GameTestBatch.hpp"
#include "common/test/framework/function/BaseGameTestFunction.hpp"
#include "common/test/framework/listener/IGameTestListener.hpp" // m_instanceListener 成员类型
#include "common/test/framework/ticker/GameTestTicker.hpp"
#include "common/world/block/BlockPos.hpp"
#include "server/test/runner/tracker/MultipleTestTracker.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace mc::server {
class ServerWorld;
}

namespace mc::test {

class BaseGameTestBatchRunner;
class GameTestRunnerBuilder;

/**
 * @brief GameTest 运行编排器（内部，被 `GameTestServer`/`GameTestCommand` 门面封装）。
 *
 * 持 `ServerWorld&` + `GameTestTicker&` + 一组 `GameTestBatch`，经 `MinecraftGameTestBatchRunner`（1C）
 * 驱动批次运行；`tick()` 每帧推进 runner + ticker；完成时触发 `GlobalTestReporter`。
 *
 * 生命周期：
 * 1. `start()`：runner.start() 启动第一批。
 * 2. `tick()`：runner.tick() 推进当前批次 + ticker.tick() 推进实例。
 * 3. `isComplete()`：runner.isComplete()。
 * 4. `exitCode()`：失败的 required 测试数（0=全过）。
 *
 * 不对外——由 `GameTestServer`（1F）或 `GameTestCommand`（1F）门面间接持有。
 */
class GameTestRunner {
public:
    ~GameTestRunner();
    GameTestRunner(const GameTestRunner&) = delete;
    GameTestRunner& operator=(const GameTestRunner&) = delete;

    void start();
    void tick();
    [[nodiscard]] bool isComplete() const noexcept;
    [[nodiscard]] std::size_t failedRequiredCount() const noexcept;
    [[nodiscard]] std::size_t totalTestCount() const noexcept;
    [[nodiscard]] std::size_t passedCount() const noexcept;
    [[nodiscard]] std::size_t failedCount() const noexcept;
    // 供 GameTestServer 在 run() 末尾把进度喂给 GlobalTestReporter::onAllFinished。
    [[nodiscard]] const MultipleTestTracker& tracker() const noexcept { return m_tracker; }

    /**
     * @brief 构造 builder。
     */
    [[nodiscard]] static GameTestRunnerBuilder builder();

private:
    friend class GameTestRunnerBuilder;

    GameTestRunner(mc::server::ServerWorld& world,
        GameTestTicker& ticker,
        std::vector<GameTestBatch> batches,
        BlockPos gridStart,
        std::size_t testsPerRow);

    mc::server::ServerWorld& m_world;
    GameTestTicker& m_ticker;
    std::unique_ptr<BaseGameTestBatchRunner> m_batchRunner;
    MultipleTestTracker m_tracker;
    // 实例级监听器（_RunnerListener，cpp 内定义），挂到每个实例；持 shared_ptr 保活，
    // 避免实例回指悬垂（实例 addListener 持 shared_ptr 拷贝）。
    std::shared_ptr<IGameTestListener> m_instanceListener;
};

} // namespace mc::test
