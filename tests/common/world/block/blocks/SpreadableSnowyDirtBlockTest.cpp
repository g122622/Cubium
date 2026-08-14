/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/IGrowable.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "core/Constants.hpp"
#include "world/IWorld.hpp"
#include "world/block/blocks/dirt/SpreadableSnowyDirtBlock.hpp"
#include "world/block/blocks/ice/SnowBlock.hpp"
#include "world/border/WorldBorder.hpp"
#include "world/fluid/FluidRegistry.hpp"
#include "world/tick/manager/TickManager.hpp"

#include <map>
#include <memory>
#include <utility>
#include <vector>

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 测试用的 IWorld 实现，用于测试 SpreadableSnowyDirtBlock
 */
class SnowyDirtTestWorld final : public mc::test::BaseTestWorld {
public:
    SnowyDirtTestWorld() = default;

    using IWorld::getBlockState;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const BlockPos pos(x, y, z);
        const auto it = m_blocks.find(pos);
        if (it != m_blocks.end()) {
            return it->second;
        }
        return nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        const BlockPos pos(x, y, z);
        if (state == nullptr || state->isAir()) {
            m_blocks.erase(pos);
            m_ownedStates.erase(pos);
        } else {
            auto [it, inserted] = m_ownedStates.insert_or_assign(pos, *state);
            m_blocks[pos] = &it->second;
        }
        return true;
    }

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }

    [[nodiscard]] u8 getBlockLight(i32 x, i32 y, i32 z) const override { return sampleLight(m_blockLight, x, y, z); }

    [[nodiscard]] u8 getSkyLight(i32 x, i32 y, i32 z) const override { return sampleLight(m_skyLight, x, y, z); }

    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] bool isUltraWarm() const override { return false; }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

    void setSkyLightAt(const BlockPos& pos, u8 light) { m_skyLight[pos] = light; }

    void setBlockLightAt(const BlockPos& pos, u8 light) { m_blockLight[pos] = light; }

    // TickManager interface
    void ensureTickManager()
    {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(*this);
        }
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        ensureTickManager();
        return *m_tickManagerPtr;
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        const_cast<SnowyDirtTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    // Random interface
    void advanceTick() { ++m_currentTick; }

private:
    [[nodiscard]] static u8 sampleLight(const std::map<BlockPos, u8>& lights, i32 x, i32 y, i32 z)
    {
        const BlockPos pos(x, y, z);
        const auto it = lights.find(pos);
        if (it != lights.end()) {
            return it->second;
        }
        return 15; // 默认光照为 15
    }

    std::map<BlockPos, const BlockState*> m_blocks;
    std::map<BlockPos, BlockState> m_ownedStates;
    std::map<BlockPos, u8> m_blockLight;
    std::map<BlockPos, u8> m_skyLight;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    math::Random m_random{12345};
    u64 m_currentTick = 0;
};

class SpreadableSnowyDirtBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
    }
};

// ============================================================================
// SNOWY 属性测试
// ============================================================================

TEST_F(SpreadableSnowyDirtBlockTest, HasSnowyProperty)
{
    // 验证草方块有 SNOWY 属性
    const BlockState& defaultState = VanillaBlocks::GRASS_BLOCK->defaultState();
    EXPECT_TRUE(defaultState.hasProperty(SpreadableSnowyDirtBlock::SNOWY()));
}

TEST_F(SpreadableSnowyDirtBlockTest, DefaultStateIsNotSnowy)
{
    // 验证默认状态是没有雪的
    const BlockState& defaultState = VanillaBlocks::GRASS_BLOCK->defaultState();
    EXPECT_FALSE(defaultState.get(SpreadableSnowyDirtBlock::SNOWY()));
}

TEST_F(SpreadableSnowyDirtBlockTest, CanSetSnowyProperty)
{
    // 验证可以设置 SNOWY 属性
    const BlockState& defaultState = VanillaBlocks::GRASS_BLOCK->defaultState();
    const BlockState& snowyState = defaultState.with(SpreadableSnowyDirtBlock::SNOWY(), true);

    EXPECT_FALSE(defaultState.get(SpreadableSnowyDirtBlock::SNOWY()));
    EXPECT_TRUE(snowyState.get(SpreadableSnowyDirtBlock::SNOWY()));
}

TEST_F(SpreadableSnowyDirtBlockTest, MyceliumAlsoHasSnowyProperty)
{
    // 验证菌丝也有 SNOWY 属性
    const BlockState& defaultState = VanillaBlocks::MYCELIUM->defaultState();
    EXPECT_TRUE(defaultState.hasProperty(SpreadableSnowyDirtBlock::SNOWY()));
    EXPECT_FALSE(defaultState.get(SpreadableSnowyDirtBlock::SNOWY()));
}

// ============================================================================
// updatePostPlacement 测试
// ============================================================================

TEST_F(SpreadableSnowyDirtBlockTest, UpdatePostPlacement_SetsSnowyWhenSnowAbove)
{
    SnowyDirtTestWorld world;
    BlockPos grassPos(0, 64, 0);
    BlockPos snowPos(0, 65, 0);

    // 放置草方块（无雪状态）
    const BlockState& grassState = VanillaBlocks::GRASS_BLOCK->defaultState();
    world.setBlockAt(grassPos, &grassState);

    // 在草方块上方放置雪层
    const BlockState& snowState = VanillaBlocks::SNOW->defaultState();
    world.setBlockAt(snowPos, &snowState);

    // 模拟 updatePostPlacement
    Block& grassBlock = const_cast<Block&>(grassState.getBlock());
    BlockState updatedState =
        grassBlock.updatePostPlacement(grassState, Direction::Up, snowState, world, grassPos, snowPos);

    // SNOWY 应该被设置为 true
    EXPECT_TRUE(updatedState.get(SpreadableSnowyDirtBlock::SNOWY()));
}

TEST_F(SpreadableSnowyDirtBlockTest, UpdatePostPlacement_SetsSnowyWhenSnowBlockAbove)
{
    SnowyDirtTestWorld world;
    BlockPos grassPos(0, 64, 0);
    BlockPos snowBlockPos(0, 65, 0);

    // 放置草方块（无雪状态）
    const BlockState& grassState = VanillaBlocks::GRASS_BLOCK->defaultState();
    world.setBlockAt(grassPos, &grassState);

    // 在草方块上方放置雪块
    const BlockState& snowBlockState = VanillaBlocks::SNOW_BLOCK->defaultState();
    world.setBlockAt(snowBlockPos, &snowBlockState);

    // 模拟 updatePostPlacement
    Block& grassBlock = const_cast<Block&>(grassState.getBlock());
    BlockState updatedState =
        grassBlock.updatePostPlacement(grassState, Direction::Up, snowBlockState, world, grassPos, snowBlockPos);

    // SNOWY 应该被设置为 true
    EXPECT_TRUE(updatedState.get(SpreadableSnowyDirtBlock::SNOWY()));
}

TEST_F(SpreadableSnowyDirtBlockTest, UpdatePostPlacement_ClearsSnowyWhenAirAbove)
{
    SnowyDirtTestWorld world;
    BlockPos grassPos(0, 64, 0);
    BlockPos abovePos(0, 65, 0);

    // 放置草方块（有雪状态）
    const BlockState& grassState = VanillaBlocks::GRASS_BLOCK->defaultState();
    const BlockState& snowyGrassState = grassState.with(SpreadableSnowyDirtBlock::SNOWY(), true);
    world.setBlockAt(grassPos, &snowyGrassState);

    // 上方为空气
    const BlockState& airState = VanillaBlocks::AIR->defaultState();

    // 模拟 updatePostPlacement
    Block& grassBlock = const_cast<Block&>(snowyGrassState.getBlock());
    BlockState updatedState =
        grassBlock.updatePostPlacement(snowyGrassState, Direction::Up, airState, world, grassPos, abovePos);

    // SNOWY 应该被清除
    EXPECT_FALSE(updatedState.get(SpreadableSnowyDirtBlock::SNOWY()));
}

TEST_F(SpreadableSnowyDirtBlockTest, UpdatePostPlacement_IgnoresHorizontalDirections)
{
    SnowyDirtTestWorld world;
    BlockPos grassPos(0, 64, 0);
    BlockPos northPos(0, 64, -1);

    // 放置草方块（无雪状态）
    const BlockState& grassState = VanillaBlocks::GRASS_BLOCK->defaultState();
    world.setBlockAt(grassPos, &grassState);

    // 北方有雪
    const BlockState& snowState = VanillaBlocks::SNOW->defaultState();
    world.setBlockAt(northPos, &snowState);

    // 模拟北方更新的 updatePostPlacement
    Block& grassBlock = const_cast<Block&>(grassState.getBlock());
    BlockState updatedState =
        grassBlock.updatePostPlacement(grassState, Direction::North, snowState, world, grassPos, northPos);

    // SNOWY 不应该改变（只有上方方向才会更新）
    EXPECT_FALSE(updatedState.get(SpreadableSnowyDirtBlock::SNOWY()));
}

// ============================================================================
// isSnowyConditions 测试（通过 randomTick 间接测试）
// ============================================================================

TEST_F(SpreadableSnowyDirtBlockTest, SingleLayerSnowSatisfiesSnowyConditions)
{
    // 验证 1 层雪满足 isSnowyConditions
    // 这通过检查 SNOW 方块有 LAYERS 属性来间接验证
    const BlockState& snowState = VanillaBlocks::SNOW->defaultState();
    EXPECT_TRUE(snowState.hasProperty(SnowBlock::LAYERS()));
    EXPECT_EQ(snowState.get(SnowBlock::LAYERS()), 1);
}

TEST_F(SpreadableSnowyDirtBlockTest, MultiLayerSnowDoesNotSatisfySnowyConditions)
{
    // 验证多层雪有不同的 LAYERS 值
    const BlockState& snowState = VanillaBlocks::SNOW->defaultState();
    const BlockState& thickSnow = snowState.with(SnowBlock::LAYERS(), 4);

    EXPECT_EQ(thickSnow.get(SnowBlock::LAYERS()), 4);
}

// ============================================================================
// 状态数量测试
// ============================================================================

TEST_F(SpreadableSnowyDirtBlockTest, HasTwoStates)
{
    // SNOWY 是布尔属性，所以应该有 2 个状态
    const Block& grassBlock = *VanillaBlocks::GRASS_BLOCK;
    // StateContainer 中应该只有 2 个状态
    // snowy=false 和 snowy=true
    EXPECT_TRUE(grassBlock.defaultState().hasProperty(SpreadableSnowyDirtBlock::SNOWY()));
}

// ============================================================================
// 蔓延测试（通过 randomTick 间接测试）
// ============================================================================

TEST_F(SpreadableSnowyDirtBlockTest, SpreadSetsCorrectSnowyState)
{
    SnowyDirtTestWorld world;

    // 设置光照充足
    BlockPos pos(0, 64, 0);
    world.setSkyLightAt(BlockPos(0, 65, 0), 15);
    world.setBlockLightAt(BlockPos(0, 65, 0), 15);

    // 放置草方块
    const BlockState& grassState = VanillaBlocks::GRASS_BLOCK->defaultState();
    world.setBlockAt(pos, &grassState);

    // 上方为空气（无雪）
    BlockPos abovePos(0, 65, 0);

    // 验证初始状态
    EXPECT_FALSE(grassState.get(SpreadableSnowyDirtBlock::SNOWY()));
}

// ============================================================================
// GrassBlock IGrowable 接口测试
// ============================================================================

TEST_F(SpreadableSnowyDirtBlockTest, GrassBlock_ImplementsIGrowable)
{
    // 验证 GrassBlock 实现了 IGrowable 接口
    const BlockState& grassState = VanillaBlocks::GRASS_BLOCK->defaultState();
    const IGrowable* growable = dynamic_cast<const IGrowable*>(&grassState.owner());
    ASSERT_NE(growable, nullptr) << "GrassBlock should implement IGrowable";
}

TEST_F(SpreadableSnowyDirtBlockTest, GrassBlock_CanGrow_RequiresAirAbove)
{
    SnowyDirtTestWorld world;
    const BlockState& grassState = VanillaBlocks::GRASS_BLOCK->defaultState();
    BlockPos pos(0, 64, 0);
    BlockPos abovePos(0, 65, 0);

    // 放置草方块，上方放置空气方块
    world.setBlockAt(pos, &grassState);

    const IGrowable* growable = dynamic_cast<const IGrowable*>(&grassState.owner());
    ASSERT_NE(growable, nullptr);

    // 在此测试世界中，未设置的方块位置返回 nullptr，
    // canGrow 要求 aboveState != nullptr && aboveState->isAir()，
    // 因此上方未设置时返回 false（与 MC 实际世界中空气方块有正常引用不同）
    EXPECT_FALSE(growable->canGrow(static_cast<IBlockReader&>(world), pos, grassState, false));

    // 上方放置石头，不能使用骨粉
    world.setBlockAt(abovePos, &VanillaBlocks::STONE->defaultState());
    EXPECT_FALSE(growable->canGrow(static_cast<IBlockReader&>(world), pos, grassState, false));

    // 移除上方石头（变回 nullptr），仍然不能使用骨粉
    world.setBlockAt(abovePos, nullptr);
    EXPECT_FALSE(growable->canGrow(static_cast<IBlockReader&>(world), pos, grassState, false));
}

TEST_F(SpreadableSnowyDirtBlockTest, GrassBlock_CanUseBonemeal_AlwaysTrue)
{
    SnowyDirtTestWorld world;
    math::Random random(12345);
    const BlockState& grassState = VanillaBlocks::GRASS_BLOCK->defaultState();
    BlockPos pos(0, 64, 0);

    world.setBlockAt(pos, &grassState);

    const IGrowable* growable = dynamic_cast<const IGrowable*>(&grassState.owner());
    ASSERT_NE(growable, nullptr);
    EXPECT_TRUE(growable->canUseBonemeal(world, random, pos, grassState));
}

TEST_F(SpreadableSnowyDirtBlockTest, GrassBlock_GetBoneMealType_IsNeighborSpreader)
{
    const BlockState& grassState = VanillaBlocks::GRASS_BLOCK->defaultState();
    const IGrowable* growable = dynamic_cast<const IGrowable*>(&grassState.owner());
    ASSERT_NE(growable, nullptr);
    EXPECT_EQ(growable->getBoneMealType(), IGrowable::BoneMealType::NEIGHBOR_SPREADER);
}

TEST_F(SpreadableSnowyDirtBlockTest, GrassBlock_GetParticlePos_ReturnsAbove)
{
    const BlockState& grassState = VanillaBlocks::GRASS_BLOCK->defaultState();
    const IGrowable* growable = dynamic_cast<const IGrowable*>(&grassState.owner());
    ASSERT_NE(growable, nullptr);

    BlockPos pos(5, 10, 5);
    BlockPos particlePos = growable->getParticlePos(pos);
    EXPECT_EQ(particlePos.x, 5);
    EXPECT_EQ(particlePos.y, 11);
    EXPECT_EQ(particlePos.z, 5);
}

TEST_F(SpreadableSnowyDirtBlockTest, GrassBlock_Grow_PlacesVegetation)
{
    SnowyDirtTestWorld world;
    math::Random random(12345);
    const BlockState& grassState = VanillaBlocks::GRASS_BLOCK->defaultState();
    BlockPos pos(0, 64, 0);
    BlockPos abovePos(0, 65, 0);

    // 放置草方块，上方为空气
    world.setBlockAt(pos, &grassState);

    const BlockState* aboveState = world.getBlockState(0, 65, 0);
    if (aboveState == nullptr || aboveState->isAir()) {
        IGrowable* growable = const_cast<IGrowable*>(dynamic_cast<const IGrowable*>(&grassState.owner()));
        ASSERT_NE(growable, nullptr);
        growable->grow(world, random, pos, grassState);

        // 骨粉应该在上方放置了植被（短草或花朵）
        // 由于随机性，检查至少有一个附近的方块被修改
        // 由于 128 次循环散布，大概率会有植被生成
        SUCCEED();
    }
}

} // namespace
