#pragma once

#include "common/test/framework/batch/BaseGameTestBatchRunner.hpp"
#include "common/world/block/BlockPos.hpp"
#include "server/test/runner/spawner/StructureGridSpawner.hpp"

namespace mc::server {
class ServerWorld;
}

namespace mc::test {

/**
 * @brief `BaseGameTestBatchRunner` 的 `ServerWorld` 具体实现。
 *
 * 持 `ServerWorld&` 引用 + `StructureGridSpawner` 网格布局器，实现两个纯虚：
 * - `_createGameTestInstance(function, rotation)`：用 `MinecraftGameTestHelperProvider` +
 *   `MinecraftGameTestInstance` 构造实例（旋转已由基类 `Rotations::add` 叠加，此处不再处理）。
 * - `_runTest(instance)`：经 `_trackInstance` 纳入 ticker + runner 所有权，触发结构放置。
 *
 * 原点布局：批次内每个测试由 `StructureGridSpawner` 按 `testsPerRow` 换行网格排列，间距
 * `SPACE_BETWEEN_COLUMNS/ROWS=32` 覆盖实体 FOLLOW_RANGE，避免相邻结构跨测试目标搜索污染。
 * 游标跨 batch 累积，整个运行连续网格编号。
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
    StructureGridSpawner m_spawner;
};

} // namespace mc::test
