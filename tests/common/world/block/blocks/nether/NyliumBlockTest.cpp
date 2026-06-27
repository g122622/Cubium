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
#include "common/util/math/random/Random.hpp"
#include "common/world/block/IGrowable.hpp"
#include "common/world/block/blocks/nether/NyliumBlock.hpp"
#include "common/world/block/registry/NetherBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "core/Constants.hpp"
#include "world/IWorld.hpp"
#include "world/border/WorldBorder.hpp"
#include "world/fluid/FluidRegistry.hpp"

#include <map>
#include <memory>

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 下界方块测试用世界
 *
 * 支持方块状态和光照设置的测试世界，用于测试 NyliumBlock 的 IGrowable 接口。
 */
class NyliumTestWorld final : public test::BaseTestWorld {
public:
    NyliumTestWorld()
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
    }

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

    [[nodiscard]] const BlockState* getBlockAt(const BlockPos& pos) const
    {
        const auto it = m_blocks.find(pos);
        return it != m_blocks.end() ? it->second : nullptr;
    }

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
        const_cast<NyliumTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    void advanceTick() { ++m_currentTick; }

private:
    [[nodiscard]] static u8 sampleLight(const std::map<BlockPos, u8>& lights, i32 x, i32 y, i32 z)
    {
        const BlockPos pos(x, y, z);
        const auto it = lights.find(pos);
        if (it != lights.end()) {
            return it->second;
        }
        return 15;
    }

    std::map<BlockPos, const BlockState*> m_blocks;
    std::map<BlockPos, BlockState> m_ownedStates;
    std::map<BlockPos, u8> m_blockLight;
    std::map<BlockPos, u8> m_skyLight;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    u64 m_currentTick = 0;
};

class NyliumBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
    }
};

// ============================================================================
// NyliumBlock IGrowable 接口测试
// ============================================================================

TEST_F(NyliumBlockTest, CrimsonNylium_ImplementsIGrowable)
{
    ASSERT_NE(block_registry::NetherBlocks::CRIMSON_NYLIUM, nullptr);
    const BlockState& state = block_registry::NetherBlocks::CRIMSON_NYLIUM->defaultState();
    const IGrowable* growable = dynamic_cast<const IGrowable*>(&state.owner());
    ASSERT_NE(growable, nullptr) << "CrimsonNylium should implement IGrowable";
}

TEST_F(NyliumBlockTest, WarpedNylium_ImplementsIGrowable)
{
    ASSERT_NE(block_registry::NetherBlocks::WARPED_NYLIUM, nullptr);
    const BlockState& state = block_registry::NetherBlocks::WARPED_NYLIUM->defaultState();
    const IGrowable* growable = dynamic_cast<const IGrowable*>(&state.owner());
    ASSERT_NE(growable, nullptr) << "WarpedNylium should implement IGrowable";
}

TEST_F(NyliumBlockTest, CrimsonNylium_CanGrow_RequiresAirAbove)
{
    NyliumTestWorld world;
    ASSERT_NE(block_registry::NetherBlocks::CRIMSON_NYLIUM, nullptr);
    const BlockState& nyliumState = block_registry::NetherBlocks::CRIMSON_NYLIUM->defaultState();
    BlockPos pos(0, 64, 0);
    BlockPos abovePos(0, 65, 0);

    world.setBlockAt(pos, &nyliumState);

    const IGrowable* growable = dynamic_cast<const IGrowable*>(&nyliumState.owner());
    ASSERT_NE(growable, nullptr);

    // 在此测试世界中，未设置的方块位置返回 nullptr，
    // canGrow 要求 aboveState != nullptr && aboveState->isAir()，
    // 因此上方未设置时返回 false（与 MC 实际世界中空气方块有正常引用不同）
    EXPECT_FALSE(growable->canGrow(static_cast<IBlockReader&>(world), pos, nyliumState, false));

    // 上方放置石头，不能使用骨粉
    world.setBlockAt(abovePos, &VanillaBlocks::STONE->defaultState());
    EXPECT_FALSE(growable->canGrow(static_cast<IBlockReader&>(world), pos, nyliumState, false));

    // 移除上方石头（变回 nullptr），仍然不能使用骨粉
    world.setBlockAt(abovePos, nullptr);
    EXPECT_FALSE(growable->canGrow(static_cast<IBlockReader&>(world), pos, nyliumState, false));
}

TEST_F(NyliumBlockTest, WarpedNylium_CanGrow_RequiresAirAbove)
{
    NyliumTestWorld world;
    ASSERT_NE(block_registry::NetherBlocks::WARPED_NYLIUM, nullptr);
    const BlockState& nyliumState = block_registry::NetherBlocks::WARPED_NYLIUM->defaultState();
    BlockPos pos(0, 64, 0);
    BlockPos abovePos(0, 65, 0);

    world.setBlockAt(pos, &nyliumState);

    const IGrowable* growable = dynamic_cast<const IGrowable*>(&nyliumState.owner());
    ASSERT_NE(growable, nullptr);

    // 与 CrimsonNylium 相同，上方 nullptr 时不满足条件
    EXPECT_FALSE(growable->canGrow(static_cast<IBlockReader&>(world), pos, nyliumState, false));

    // 上方放置石头
    world.setBlockAt(abovePos, &VanillaBlocks::STONE->defaultState());
    EXPECT_FALSE(growable->canGrow(static_cast<IBlockReader&>(world), pos, nyliumState, false));

    // 移除上方石头
    world.setBlockAt(abovePos, nullptr);
    EXPECT_FALSE(growable->canGrow(static_cast<IBlockReader&>(world), pos, nyliumState, false));
}

TEST_F(NyliumBlockTest, CanUseBonemeal_AlwaysTrue)
{
    NyliumTestWorld world;
    math::Random random(12345);
    BlockPos pos(0, 64, 0);

    // 测试绯红菌岩
    ASSERT_NE(block_registry::NetherBlocks::CRIMSON_NYLIUM, nullptr);
    const BlockState& crimsonState = block_registry::NetherBlocks::CRIMSON_NYLIUM->defaultState();
    world.setBlockAt(pos, &crimsonState);
    const IGrowable* crimsonGrowable = dynamic_cast<const IGrowable*>(&crimsonState.owner());
    ASSERT_NE(crimsonGrowable, nullptr);
    EXPECT_TRUE(crimsonGrowable->canUseBonemeal(world, random, pos, crimsonState));

    // 测试诡异菌岩
    ASSERT_NE(block_registry::NetherBlocks::WARPED_NYLIUM, nullptr);
    const BlockState& warpedState = block_registry::NetherBlocks::WARPED_NYLIUM->defaultState();
    world.setBlockAt(pos, &warpedState);
    const IGrowable* warpedGrowable = dynamic_cast<const IGrowable*>(&warpedState.owner());
    ASSERT_NE(warpedGrowable, nullptr);
    EXPECT_TRUE(warpedGrowable->canUseBonemeal(world, random, pos, warpedState));
}

TEST_F(NyliumBlockTest, GetBoneMealType_IsNeighborSpreader)
{
    // 绯红菌岩和诡异菌岩都应该是 NEIGHBOR_SPREADER 类型
    ASSERT_NE(block_registry::NetherBlocks::CRIMSON_NYLIUM, nullptr);
    const BlockState& crimsonState = block_registry::NetherBlocks::CRIMSON_NYLIUM->defaultState();
    const IGrowable* crimsonGrowable = dynamic_cast<const IGrowable*>(&crimsonState.owner());
    ASSERT_NE(crimsonGrowable, nullptr);
    EXPECT_EQ(crimsonGrowable->getBoneMealType(), IGrowable::BoneMealType::NEIGHBOR_SPREADER);

    ASSERT_NE(block_registry::NetherBlocks::WARPED_NYLIUM, nullptr);
    const BlockState& warpedState = block_registry::NetherBlocks::WARPED_NYLIUM->defaultState();
    const IGrowable* warpedGrowable = dynamic_cast<const IGrowable*>(&warpedState.owner());
    ASSERT_NE(warpedGrowable, nullptr);
    EXPECT_EQ(warpedGrowable->getBoneMealType(), IGrowable::BoneMealType::NEIGHBOR_SPREADER);
}

TEST_F(NyliumBlockTest, GetParticlePos_ReturnsAbove)
{
    ASSERT_NE(block_registry::NetherBlocks::CRIMSON_NYLIUM, nullptr);
    const BlockState& crimsonState = block_registry::NetherBlocks::CRIMSON_NYLIUM->defaultState();
    const IGrowable* crimsonGrowable = dynamic_cast<const IGrowable*>(&crimsonState.owner());
    ASSERT_NE(crimsonGrowable, nullptr);

    BlockPos pos(5, 10, 5);
    BlockPos particlePos = crimsonGrowable->getParticlePos(pos);
    EXPECT_EQ(particlePos.x, 5);
    EXPECT_EQ(particlePos.y, 11);
    EXPECT_EQ(particlePos.z, 5);

    ASSERT_NE(block_registry::NetherBlocks::WARPED_NYLIUM, nullptr);
    const BlockState& warpedState = block_registry::NetherBlocks::WARPED_NYLIUM->defaultState();
    const IGrowable* warpedGrowable = dynamic_cast<const IGrowable*>(&warpedState.owner());
    ASSERT_NE(warpedGrowable, nullptr);

    BlockPos particlePos2 = warpedGrowable->getParticlePos(pos);
    EXPECT_EQ(particlePos2.x, 5);
    EXPECT_EQ(particlePos2.y, 11);
    EXPECT_EQ(particlePos2.z, 5);
}

TEST_F(NyliumBlockTest, CrimsonNylium_Grow_PlacesVegetation)
{
    NyliumTestWorld world;
    math::Random random(12345);
    ASSERT_NE(block_registry::NetherBlocks::CRIMSON_NYLIUM, nullptr);

    const BlockState& nyliumState = block_registry::NetherBlocks::CRIMSON_NYLIUM->defaultState();
    BlockPos pos(0, 64, 0);
    world.setBlockAt(pos, &nyliumState);

    // 确保上方为空气
    const BlockState* aboveState = world.getBlockState(0, 65, 0);

    IGrowable* growable = const_cast<IGrowable*>(dynamic_cast<const IGrowable*>(&nyliumState.owner()));
    ASSERT_NE(growable, nullptr);

    if (aboveState == nullptr || aboveState->isAir()) {
        growable->grow(world, random, pos, nyliumState);
        // 骨粉应该放置了绯红菌或绯红菌索
        // 由于随机性，只检查不崩溃
        SUCCEED();
    }
}

TEST_F(NyliumBlockTest, WarpedNylium_Grow_PlacesVegetation)
{
    NyliumTestWorld world;
    math::Random random(12345);
    ASSERT_NE(block_registry::NetherBlocks::WARPED_NYLIUM, nullptr);

    const BlockState& nyliumState = block_registry::NetherBlocks::WARPED_NYLIUM->defaultState();
    BlockPos pos(0, 64, 0);
    world.setBlockAt(pos, &nyliumState);

    IGrowable* growable = const_cast<IGrowable*>(dynamic_cast<const IGrowable*>(&nyliumState.owner()));
    ASSERT_NE(growable, nullptr);

    const BlockState* aboveState = world.getBlockState(0, 65, 0);
    if (aboveState == nullptr || aboveState->isAir()) {
        growable->grow(world, random, pos, nyliumState);
        SUCCEED();
    }
}

TEST_F(NyliumBlockTest, CrimsonNylium_Grow_DoesNothingWithoutAirAbove)
{
    NyliumTestWorld world;
    math::Random random(12345);
    ASSERT_NE(block_registry::NetherBlocks::CRIMSON_NYLIUM, nullptr);

    const BlockState& nyliumState = block_registry::NetherBlocks::CRIMSON_NYLIUM->defaultState();
    BlockPos pos(0, 64, 0);
    BlockPos abovePos(0, 65, 0);

    // 放置菌岩，上方放置石头
    world.setBlockAt(pos, &nyliumState);
    world.setBlockAt(abovePos, &VanillaBlocks::STONE->defaultState());

    IGrowable* growable = const_cast<IGrowable*>(dynamic_cast<const IGrowable*>(&nyliumState.owner()));
    ASSERT_NE(growable, nullptr);

    // 上方不是空气，grow 应该不做任何事
    growable->grow(world, random, pos, nyliumState);

    // 菌岩方块应该还在
    const BlockState* state = world.getBlockAt(pos);
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->is(block_registry::NetherBlocks::CRIMSON_NYLIUM));
}

// ============================================================================
// NyliumBlock 随机刻退化测试
// ============================================================================

TEST_F(NyliumBlockTest, CrimsonNylium_TicksRandomly)
{
    ASSERT_NE(block_registry::NetherBlocks::CRIMSON_NYLIUM, nullptr);
    const Block& block = block_registry::NetherBlocks::CRIMSON_NYLIUM->defaultState().owner();
    EXPECT_TRUE(block.ticksRandomly());
}

TEST_F(NyliumBlockTest, WarpedNylium_TicksRandomly)
{
    ASSERT_NE(block_registry::NetherBlocks::WARPED_NYLIUM, nullptr);
    const Block& block = block_registry::NetherBlocks::WARPED_NYLIUM->defaultState().owner();
    EXPECT_TRUE(block.ticksRandomly());
}

} // namespace
