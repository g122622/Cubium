#pragma once

#include "common/test/framework/batch/BaseGameTestBatchRunner.hpp"
#include "common/world/block/BlockPos.hpp"

namespace mc::server {
class ServerWorld;
}

namespace mc::test {

/**
 * @brief `BaseGameTestBatchRunner` 的 `ServerWorld` 具体实现。
 *
 * 持 `ServerWorld&` 引用 + 当前批次原点生成器（结构网格布局），实现两个纯虚：
 * - `_createGameTestInstance(function, rotation)`：用 `MinecraftGameTestHelperProvider` +
 *   `MinecraftGameTestInstance` 构造实例（旋转已由基类 `Rotations::add` 叠加，此处不再处理）。
 * - `_runTest(instance)`：经 `_trackInstance` 纳入 ticker + runner 所有权，触发结构放置。
 *
 * 原点布局：批次内每个测试按 `testsPerRow` 换行，间距由 `StructureGridSpawner`（1D runner/spawner/）算；
 * 本 runner 持 `m_nextOrigin` 游标，`_createGameTestInstance` 内推进。第一阶段简化为线性递增 X，
 * 完整网格布局由 1D `StructureGridSpawner` 接管（TODO 切换）。
 *
 * 不对外——由 `GameTestRunner`（1D）或 `GameTestServer`（1F）门面间接持有。
 */
class MinecraftGameTestBatchRunner final : public BaseGameTestBatchRunner {
public:
    MinecraftGameTestBatchRunner(std::vector<GameTestBatch> batches,
        GameTestTicker& ticker,
        TestParameters params,
        mc::server::ServerWorld& world,
        BlockPos gridStart);

protected:
    [[nodiscard]] std::unique_ptr<BaseGameTestInstance> _createGameTestInstance(
        BaseGameTestFunction& function, Rotation rotation) override;
    void _runTest(std::unique_ptr<BaseGameTestInstance> instance) override;

    // 环境 setup/teardown：经 MinecraftEnvironmentApplier 把批次环境应用到 ServerWorld
    // （天气/时间/游戏规则等），覆盖基类空实现。framework 层 TestEnvironmentDefinition::setup 不再被调用
    // （其具体子类返回 fail 是历史桩，applier 接管后为死代码）。
    GameTestResult _applyBatchEnvironmentSetup(const GameTestBatch& batch) override;
    GameTestResult _applyBatchEnvironmentTeardown(const GameTestBatch& batch) override;

private:
    mc::server::ServerWorld& m_world;
    BlockPos m_nextOrigin;
};

} // namespace mc::test
