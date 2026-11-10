// ============================================================================
// Block::isValidSpawn 默认谓词测试
//
// 对应原版 `BlockBehaviour.BlockStateBase#isValidSpawn(BlockGetter, BlockPos, EntityType)`：
// 默认谓词为「该方块顶面是完整支撑面 && 自身发光等级 < 14」。传送门刷僵尸猪灵、结构生成等
// 「在此方块上方生成实体」的场景都以它做最后一道位置校验。
// ============================================================================

#include "common/TestWorldHelper.hpp"

#include <gtest/gtest.h>

#include <unordered_map>

#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/state/BlockState.hpp"
#include "common/world/IWorld.hpp"

namespace {

/**
 * @brief 按位置存储方块状态的测试世界
 *
 * `BaseTestWorld` 已实现 IWorld 的全部纯虚成员，但 getBlockState/setBlockState 是空桩。
 * 这里只补上按位置存取，使 isValidSpawn 这类需要查询邻接方块的判定可被测试。
 */
class MapBackedTestWorld final : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] const mc::BlockState* getBlockState(mc::i32 x, mc::i32 y, mc::i32 z) const override
    {
        const auto it = m_blocks.find(mc::BlockPos(x, y, z));
        return it == m_blocks.end() ? nullptr : it->second;
    }

    bool setBlockState(mc::i32 x, mc::i32 y, mc::i32 z, const mc::BlockState* state) override
    {
        m_blocks[mc::BlockPos(x, y, z)] = state;
        return true;
    }

private:
    std::unordered_map<mc::BlockPos, const mc::BlockState*> m_blocks;
};

/// 取一个实体类型用于 isValidSpawn 的类型参数（默认谓词不使用该参数）。
const mc::entity::EntityType& _probeEntityType()
{
    static const mc::entity::EntityType* type = nullptr;
    if (type == nullptr) {
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
    MapBackedTestWorld world;
    const auto* air = mc::BlockRegistry::instance().airState();
    ASSERT_NE(air, nullptr);

    EXPECT_FALSE(
        air->getBlock().isValidSpawn(world, mc::BlockPos(0, 64, 0), *air, _probeEntityType()));
}

TEST(BlockIsValidSpawnTest, StoneAllowsSpawn)
{
    // 石头顶面为完整支撑面且发光等级 0 < 14 → 满足默认谓词
    MapBackedTestWorld world;
    const auto& stone = mc::VanillaBlocks::STONE->defaultState();

    EXPECT_TRUE(
        stone.getBlock().isValidSpawn(world, mc::BlockPos(0, 64, 0), stone, _probeEntityType()));
}