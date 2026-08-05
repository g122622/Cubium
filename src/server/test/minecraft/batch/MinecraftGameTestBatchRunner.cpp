#include "server/test/minecraft/batch/MinecraftGameTestBatchRunner.hpp"

#include "common/util/assert/AssertMacros.hpp" // MC_UNUSED
#include "server/test/minecraft/helper/MinecraftGameTestHelperProvider.hpp"
#include "server/test/minecraft/instance/MinecraftGameTestInstance.hpp"

namespace mc::test {

MinecraftGameTestBatchRunner::MinecraftGameTestBatchRunner(std::vector<GameTestBatch> batches,
    GameTestTicker& ticker,
    TestParameters params,
    mc::server::ServerWorld& world,
    BlockPos gridStart)
    : BaseGameTestBatchRunner(std::move(batches), ticker, std::move(params))
    , m_world(world)
    , m_nextOrigin(gridStart)
{}

std::unique_ptr<BaseGameTestInstance> MinecraftGameTestBatchRunner::_createGameTestInstance(
    BaseGameTestFunction& function, Rotation rotation)
{
    MC_UNUSED(rotation); // 旋转已由基类 _runBatch 内 Rotations::add 叠加到 function.data().rotation()，
                         // 此处不再二次叠加；实例经 function.data() 取最终旋转。
    auto helperProvider = std::make_unique<MinecraftGameTestHelperProvider>(m_world);
    auto instance =
        std::make_unique<MinecraftGameTestInstance>(function, std::move(helperProvider), m_world, m_nextOrigin);

    // TODO: 原点布局切换为 1D StructureGridSpawner 的网格算式（testsPerRow 换行 + 旋转后包围盒间距）。
    // 第一阶段简化为线性递增 X（按结构 X 跨度 + padding 间隔），避免重叠。
    const auto* bounds = instance->bounds();
    const i32 spanX = bounds ? bounds->rotatedSize().x : 1;
    m_nextOrigin.x += spanX + function.data().padding() * 2 + 2;

    return instance;
}

void MinecraftGameTestBatchRunner::_runTest(std::unique_ptr<BaseGameTestInstance> instance)
{
    // 触发结构放置（若尚未放置），纳入 ticker + runner 所有权。
    // spawnStructure() 为 protected 钩子，经公有 spawnStructureIfNeeded() 转发（tick() 内 _isTestReady
    // 依赖结构已就绪，故须在加入 ticker 前显式放一次）。
    instance->spawnStructureIfNeeded();
    _trackInstance(std::move(instance));
}

} // namespace mc::test
