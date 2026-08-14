#include "server/test/minecraft/batch/MinecraftGameTestBatchRunner.hpp"

#include "common/util/assert/AssertMacros.hpp" // MC_UNUSED
#include "server/test/minecraft/environment/MinecraftEnvironmentApplier.hpp"
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
    , m_spawner(gridStart, _params().testsPerRow())
{}

std::unique_ptr<BaseGameTestInstance> MinecraftGameTestBatchRunner::_createGameTestInstance(
    BaseGameTestFunction& function, Rotation rotation)
{
    MC_UNUSED(rotation); // 旋转已由基类 _runBatch 内 Rotations::add 叠加到 function.data().rotation()，
                         // 此处不再二次叠加；实例经 function.data() 取最终旋转。
    auto helperProvider = std::make_unique<MinecraftGameTestHelperProvider>(m_world);
    // 先从网格布局器取得本测试原点（peekOrigin 不推进游标），构造实例并放置结构。
    const BlockPos origin = m_spawner.peekOrigin();
    auto instance = std::make_unique<MinecraftGameTestInstance>(function, std::move(helperProvider), m_world, origin);

    // 先放置结构，再用放置后的真实包围盒推进布局器游标（供下一测试）。
    // 此前在放置前取 instance->bounds()，此时 m_bounds 尚为 nullptr（spawnStructure 在 _runTest 才调），
    // spanX 退化为默认 1，致游标推进不足，相邻测试结构在世界中重叠——后一测试的 air 方块覆盖前一测试
    // 已放置的 button 等依附类方块，表现为"button 放置后变 air"。对齐 vanilla GameTestRunner：先放结构
    // （spawnStructure）再算下一原点。spawnStructureIfNeeded 幂等：已放置则跳过，_runTest 再调不会重复放置。
    instance->spawnStructureIfNeeded();

    // 用本测试旋转后真实尺寸 + padding 推进网格游标。SPACE_BETWEEN_COLUMNS/ROWS=32 覆盖实体 FOLLOW_RANGE
    // （默认 16，部分实体 32），确保相邻结构间距 > 目标搜索半径，从框架层根除跨测试 AI 目标搜索污染
    // （NearestAttackableTargetGoal 等用 getEntitiesInRange(pos, FOLLOW_RANGE) 球形搜索全维度，间距不足时
    // 覆盖邻结构，checkSight 射线被邻结构干扰致实体放弃本结构目标）。MinecraftStructurePlacer 已为每个
    // 结构区域单独加 forced chunk ticket，增大间距不会导致远 chunk 不加载。
    const auto* bounds = instance->bounds();
    const i32 sizeX = bounds ? bounds->rotatedSize().x : 1;
    const i32 sizeZ = bounds ? bounds->rotatedSize().z : 1;
    m_spawner.advance(sizeX, sizeZ, function.data().padding());

    return instance;
}

void MinecraftGameTestBatchRunner::_runTest(std::unique_ptr<BaseGameTestInstance> instance)
{
    // 触发结构放置（若尚未放置），纳入 ticker + runner 所有权。
    // spawnStructure() 为 protected 钩子，经公有 spawnStructureIfNeeded() 转发（tick() 内 _isTestReady
    // 依赖结构已就绪，故须在加入 ticker 前显式放一次）。
    instance->spawnStructureIfNeeded();
    // 结构放置成功后启动执行：设 tickCount = -(setupTicks+1)（setup 阶段负值，对齐 Java
    // startExecution）。tick() 内 ++tickCount 到 0 才触发测试函数；不调 startExecution 则 m_tickCount
    // 保持默认 0，++后恒 >=1，m_tickCount==0 永不成立，测试函数永不执行（alwaysSucceed 超时根因）。
    // 结构放置失败时 spawnStructure 已调 fail()（state=Failed），isDone 为真跳过 startExecution，
    // 避免 startExecution 把 state 重置回 NotStarted 掩盖放置失败。
    if (!isDone(instance->state())) {
        instance->startExecution();
    }
    _trackInstance(std::move(instance));
}

GameTestResult MinecraftGameTestBatchRunner::_applyBatchEnvironmentSetup(const GameTestBatch& batch)
{
    // 空 environment（如 "default" 空 AllOfEnvironment 或 nullptr）直接通过。
    const auto& env = batch.environment();
    if (!env) {
        return mc::test::pass();
    }
    return MinecraftEnvironmentApplier::applySetup(*env, m_world);
}

GameTestResult MinecraftGameTestBatchRunner::_applyBatchEnvironmentTeardown(const GameTestBatch& batch)
{
    const auto& env = batch.environment();
    if (!env) {
        return mc::test::pass();
    }
    return MinecraftEnvironmentApplier::applyTeardown(*env, m_world);
}

} // namespace mc::test
