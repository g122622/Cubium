// ============================================================================
// Block::isValidSpawn 默认谓词测试
//
// 对应原版 `BlockBehaviour.BlockStateBase#isValidSpawn(BlockGetter, BlockPos, EntityType)`：
// 默认谓词为「该方块顶面是完整支撑面 && 自身发光等级 < 14」。传送门刷僵尸猪灵、结构生成等
// 「在此方块上方生成实体」的场景都以它做最后一道位置校验。
// ============================================================================

#include "common/TestWorldHelper.hpp"

#include <gtest/gtest.h>

#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/state/BlockState.hpp"

namespace {

/// 取一个实体类型用于 isValidSpawn 的类型参数（默认谓词不使用该参数）。
const mc::entity::EntityType& _probeEntityType()
{
    static const mc::entity::EntityType* type = nullptr;
    if (type == nullptr) {
        // 单测二进制不注册实体类型（main.cpp 只初始化方块/物品/标签），这里补一次幂等注册。
        mc::entity::VanillaEntities::registerAll();
        type = mc::entity::EntityRegistry::instance().getType("minecraft:pig");
    }
    MC_ASSERT_RELEASE(type != nullptr);
    return *type;
}

} // namespace

TEST(BlockIsValidSpawnTest, AirDoesNotAllowSpawn)
{
    // 空气顶面不是完整支撑面 → 不满足默认谓词
    mc::test::MapBackedTestWorld world(mc::Difficulty::Normal);
    const auto* air = mc::BlockRegistry::instance().airState();
    ASSERT_NE(air, nullptr);

    EXPECT_FALSE(air->getBlock().isValidSpawn(world, mc::BlockPos(0, 64, 0), *air, _probeEntityType()));
}

TEST(BlockIsValidSpawnTest, StoneAllowsSpawn)
{
    // 石头顶面为完整支撑面且发光等级 0 < 14 → 满足默认谓词
    mc::test::MapBackedTestWorld world(mc::Difficulty::Normal);
    const auto& stone = mc::VanillaBlocks::STONE->defaultState();

    EXPECT_TRUE(stone.getBlock().isValidSpawn(world, mc::BlockPos(0, 64, 0), stone, _probeEntityType()));
}
