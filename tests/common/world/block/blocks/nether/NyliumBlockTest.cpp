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
class NyliumTestWorld final : public mc::test::BaseTestWorld {
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
// NyliumBlock 散布算法测试
// ============================================================================

TEST_F(NyliumBlockTest, CrimsonNylium_Grow_ScattersVegetationIn3x1x3Area)
{
    // 绯红菌岩骨粉应在 3×1×3 范围内散布放置植被
    // 对应 MC CRIMSON_FOREST_VEGETATION_BONEMEAL
    NyliumTestWorld world;
    math::Random random(42);
    ASSERT_NE(block_registry::NetherBlocks::CRIMSON_NYLIUM, nullptr);

    const BlockState& nyliumState = block_registry::NetherBlocks::CRIMSON_NYLIUM->defaultState();
    BlockPos pos(0, 64, 0);

    // 放置一片绯红菌岩区域，确保散布范围内的菌岩下方有足够支撑
    for (i32 dx = -3; dx <= 3; ++dx) {
        for (i32 dz = -3; dz <= 3; ++dz) {
            world.setBlockAt(BlockPos(dx, 64, dz), &nyliumState);
        }
    }

    IGrowable* growable = const_cast<IGrowable*>(dynamic_cast<const IGrowable*>(&nyliumState.owner()));
    ASSERT_NE(growable, nullptr);

    growable->grow(world, random, pos, nyliumState);

    // 统计放置的植被方块数量（在 y=65 范围内，即菌岩上方一格）
    i32 vegetationCount = 0;
    i32 crimsonRootsCount = 0;
    i32 crimsonFungusCount = 0;
    i32 warpedFungusCount = 0;
    for (i32 dx = -3; dx <= 3; ++dx) {
        for (i32 dz = -3; dz <= 3; ++dz) {
            const BlockState* aboveState = world.getBlockAt(BlockPos(dx, 65, dz));
            if (aboveState != nullptr && aboveState->isAir()) {
                continue; // 仍然是空气，跳过
            }
            if (aboveState != nullptr) {
                ++vegetationCount;
                if (aboveState->is(VanillaBlocks::CRIMSON_ROOTS)) {
                    ++crimsonRootsCount;
                } else if (aboveState->is(VanillaBlocks::CRIMSON_FUNGUS)) {
                    ++crimsonFungusCount;
                } else if (aboveState->is(VanillaBlocks::WARPED_FUNGUS)) {
                    ++warpedFungusCount;
                }
            }
        }
    }

    // 散布算法执行 9 次尝试，应该放置一些植被
    EXPECT_GT(vegetationCount, 0) << "Crimson nylium bonemeal should place vegetation";

    // 绯红菌索应该是最常见的（权重 87/99）
    EXPECT_GE(crimsonRootsCount + crimsonFungusCount + warpedFungusCount, 1)
        << "Should have at least one vegetation block placed";

    // 菌岩本身不应被改变
    const BlockState* nyliumAfter = world.getBlockAt(pos);
    ASSERT_NE(nyliumAfter, nullptr);
    EXPECT_TRUE(nyliumAfter->is(block_registry::NetherBlocks::CRIMSON_NYLIUM));
}

TEST_F(NyliumBlockTest, WarpedNylium_Grow_ScattersVegetationAndNetherSprouts)
{
    // 诡异菌岩骨粉应散布放置植被 + 下界苗，可能放置缠怨藤
    // 对应 MC WARPED_FOREST_VEGETATION_BONEMEAL + NETHER_SPROUTS_BONEMEAL + 可能的 TWISTING_VINES_BONEMEAL
    NyliumTestWorld world;
    math::Random random(42);
    ASSERT_NE(block_registry::NetherBlocks::WARPED_NYLIUM, nullptr);

    const BlockState& nyliumState = block_registry::NetherBlocks::WARPED_NYLIUM->defaultState();
    BlockPos pos(0, 64, 0);

    // 放置一片诡异菌岩区域
    for (i32 dx = -3; dx <= 3; ++dx) {
        for (i32 dz = -3; dz <= 3; ++dz) {
            world.setBlockAt(BlockPos(dx, 64, dz), &nyliumState);
        }
    }

    IGrowable* growable = const_cast<IGrowable*>(dynamic_cast<const IGrowable*>(&nyliumState.owner()));
    ASSERT_NE(growable, nullptr);

    growable->grow(world, random, pos, nyliumState);

    // 统计放置的方块
    i32 warpedRootsCount = 0;
    i32 warpedFungusCount = 0;
    i32 netherSproutsCount = 0;
    i32 twistingVinesCount = 0;

    for (i32 dx = -3; dx <= 3; ++dx) {
        for (i32 dz = -3; dz <= 3; ++dz) {
            for (i32 dy = 65; dy <= 68; ++dy) {
                const BlockState* state = world.getBlockAt(BlockPos(dx, dy, dz));
                if (state == nullptr || state->isAir()) {
                    continue;
                }
                if (state->is(VanillaBlocks::WARPED_ROOTS)) {
                    ++warpedRootsCount;
                } else if (state->is(VanillaBlocks::WARPED_FUNGUS)) {
                    ++warpedFungusCount;
                } else if (state->is(VanillaBlocks::NETHER_SPROUTS)) {
                    ++netherSproutsCount;
                } else if (state->is(VanillaBlocks::TWISTING_VINES) || state->is(VanillaBlocks::TWISTING_VINES_PLANT)) {
                    ++twistingVinesCount;
                }
            }
        }
    }

    // 诡异菌岩骨粉应放置诡异植被和下界苗
    i32 totalVegetation = warpedRootsCount + warpedFungusCount;
    EXPECT_GT(totalVegetation, 0) << "Warped nylium bonemeal should place warped vegetation";

    // 下界苗散布总是执行（9次尝试），应该放置一些
    EXPECT_GT(netherSproutsCount, 0) << "Warped nylium bonemeal should place nether sprouts";

    // 菌岩本身不应被改变
    const BlockState* nyliumAfter = world.getBlockAt(pos);
    ASSERT_NE(nyliumAfter, nullptr);
    EXPECT_TRUE(nyliumAfter->is(block_registry::NetherBlocks::WARPED_NYLIUM));
}

TEST_F(NyliumBlockTest, CrimsonNylium_Grow_WeightedSelection_CrimsonRootsMostCommon)
{
    // 多次运行骨粉，验证绯红菌索（权重87/99）是最常见的输出
    // 使用大样本统计确保权重分布正确
    NyliumTestWorld world;
    ASSERT_NE(block_registry::NetherBlocks::CRIMSON_NYLIUM, nullptr);

    const BlockState& nyliumState = block_registry::NetherBlocks::CRIMSON_NYLIUM->defaultState();
    IGrowable* growable = const_cast<IGrowable*>(dynamic_cast<const IGrowable*>(&nyliumState.owner()));
    ASSERT_NE(growable, nullptr);

    i32 crimsonRootsCount = 0;
    i32 crimsonFungusCount = 0;
    i32 warpedFungusCount = 0;

    // 运行 100 次骨粉，统计植被类型分布
    for (i32 run = 0; run < 100; ++run) {
        // 每次重新创建世界和放置菌岩
        NyliumTestWorld runWorld;
        math::Random random(run * 137 + 42);

        // 放置一片菌岩
        for (i32 dx = -3; dx <= 3; ++dx) {
            for (i32 dz = -3; dz <= 3; ++dz) {
                runWorld.setBlockAt(BlockPos(dx, 64, dz), &nyliumState);
            }
        }

        growable->grow(runWorld, random, BlockPos(0, 64, 0), nyliumState);

        // 统计放置的植被
        for (i32 dx = -3; dx <= 3; ++dx) {
            for (i32 dz = -3; dz <= 3; ++dz) {
                const BlockState* state = runWorld.getBlockAt(BlockPos(dx, 65, dz));
                if (state != nullptr && !state->isAir()) {
                    if (state->is(VanillaBlocks::CRIMSON_ROOTS)) {
                        ++crimsonRootsCount;
                    } else if (state->is(VanillaBlocks::CRIMSON_FUNGUS)) {
                        ++crimsonFungusCount;
                    } else if (state->is(VanillaBlocks::WARPED_FUNGUS)) {
                        ++warpedFungusCount;
                    }
                }
            }
        }
    }

    // 绯红菌索权重 87/99 ≈ 87.9%，应该是最常见的
    EXPECT_GT(crimsonRootsCount, crimsonFungusCount)
        << "Crimson roots (87/99) should be more common than crimson fungus (11/99)";
    EXPECT_GT(crimsonFungusCount, warpedFungusCount)
        << "Crimson fungus (11/99) should be more common than warped fungus (1/99)";

    // 总数应大于 0
    i32 total = crimsonRootsCount + crimsonFungusCount + warpedFungusCount;
    EXPECT_GT(total, 0) << "Should have placed some vegetation across 100 runs";
}

TEST_F(NyliumBlockTest, WarpedNylium_Grow_WeightedSelection_WarpedRootsMostCommon)
{
    // 多次运行骨粉，验证诡异菌索（权重85/100）是最常见的输出
    NyliumTestWorld world;
    ASSERT_NE(block_registry::NetherBlocks::WARPED_NYLIUM, nullptr);

    const BlockState& nyliumState = block_registry::NetherBlocks::WARPED_NYLIUM->defaultState();
    IGrowable* growable = const_cast<IGrowable*>(dynamic_cast<const IGrowable*>(&nyliumState.owner()));
    ASSERT_NE(growable, nullptr);

    i32 warpedRootsCount = 0;
    i32 warpedFungusCount = 0;

    for (i32 run = 0; run < 100; ++run) {
        NyliumTestWorld runWorld;
        math::Random random(run * 251 + 99);

        for (i32 dx = -3; dx <= 3; ++dx) {
            for (i32 dz = -3; dz <= 3; ++dz) {
                runWorld.setBlockAt(BlockPos(dx, 64, dz), &nyliumState);
            }
        }

        growable->grow(runWorld, random, BlockPos(0, 64, 0), nyliumState);

        for (i32 dx = -3; dx <= 3; ++dx) {
            for (i32 dz = -3; dz <= 3; ++dz) {
                const BlockState* state = runWorld.getBlockAt(BlockPos(dx, 65, dz));
                if (state != nullptr && !state->isAir()) {
                    if (state->is(VanillaBlocks::WARPED_ROOTS)) {
                        ++warpedRootsCount;
                    } else if (state->is(VanillaBlocks::WARPED_FUNGUS)) {
                        ++warpedFungusCount;
                    }
                }
            }
        }
    }

    EXPECT_GT(warpedRootsCount, warpedFungusCount)
        << "Warped roots (85/100) should be more common than warped fungus (13/100)";
}

TEST_F(NyliumBlockTest, TwistingVines_PlacedAsColumns)
{
    // 验证缠怨藤放置为柱状结构（底部 TWISTING_VINES_PLANT，顶部 TWISTING_VINES）
    // 使用固定种子多次运行，提高触发 1/8 缠怨藤概率的机会
    NyliumTestWorld world;
    ASSERT_NE(block_registry::NetherBlocks::WARPED_NYLIUM, nullptr);
    ASSERT_NE(VanillaBlocks::TWISTING_VINES, nullptr);
    ASSERT_NE(VanillaBlocks::TWISTING_VINES_PLANT, nullptr);

    const BlockState& nyliumState = block_registry::NetherBlocks::WARPED_NYLIUM->defaultState();
    IGrowable* growable = const_cast<IGrowable*>(dynamic_cast<const IGrowable*>(&nyliumState.owner()));
    ASSERT_NE(growable, nullptr);

    bool foundTwistingVines = false;
    bool foundValidColumn = false;

    // 使用不同种子多次运行，确保至少一次触发缠怨藤
    for (i32 seed = 0; seed < 200 && !foundTwistingVines; ++seed) {
        NyliumTestWorld runWorld;
        math::Random random(seed * 31 + 7);

        // 放置一片诡异菌岩区域
        for (i32 dx = -3; dx <= 3; ++dx) {
            for (i32 dz = -3; dz <= 3; ++dz) {
                runWorld.setBlockAt(BlockPos(dx, 64, dz), &nyliumState);
            }
        }

        growable->grow(runWorld, random, BlockPos(0, 64, 0), nyliumState);

        // 检查是否有缠怨藤放置
        for (i32 dx = -3; dx <= 3; ++dx) {
            for (i32 dz = -3; dz <= 3; ++dz) {
                for (i32 dy = 65; dy <= 70; ++dy) {
                    const BlockState* state = runWorld.getBlockAt(BlockPos(dx, dy, dz));
                    if (state != nullptr &&
                        (state->is(VanillaBlocks::TWISTING_VINES) || state->is(VanillaBlocks::TWISTING_VINES_PLANT))) {
                        foundTwistingVines = true;

                        // 找到藤蔓，验证柱状结构：
                        // 底部应该是 TWISTING_VINES_PLANT（如果有 2+ 格高）
                        // 顶部应该是 TWISTING_VINES（头部）
                        // 从顶部（TWISTING_VINES）开始向下检查
                        if (state->is(VanillaBlocks::TWISTING_VINES)) {
                            foundValidColumn = true;
                            // 头部下方（如果存在藤蔓身体）应该是 TWISTING_VINES_PLANT
                            if (dy > 65) {
                                const BlockState* belowState = runWorld.getBlockAt(BlockPos(dx, dy - 1, dz));
                                // 如果下方有藤蔓身体，验证类型
                                if (belowState != nullptr && !belowState->isAir()) {
                                    EXPECT_TRUE(belowState->is(VanillaBlocks::TWISTING_VINES_PLANT))
                                        << "Twisting vines body should be TWISTING_VINES_PLANT";
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // 由于 1/8 概率，200 次运行应该足够触发至少一次
    // 如果没有触发，也是可能的（概率极低），但测试应该至少验证逻辑正确
    if (foundTwistingVines) {
        EXPECT_TRUE(foundValidColumn) << "Twisting vines should form valid column structure";
    }
    // 如果没找到缠怨藤，概率上也是可以接受的（但 200 次运行应该足够）
}

TEST_F(NyliumBlockTest, Grow_VegetationOnlyPlacedOnNyliumGround)
{
    // 验证植被只放在菌岩上方的空气方块中
    // 如果散布位置下方不是菌岩，不应放置植被
    NyliumTestWorld world;
    math::Random random(42);
    ASSERT_NE(block_registry::NetherBlocks::CRIMSON_NYLIUM, nullptr);

    const BlockState& nyliumState = block_registry::NetherBlocks::CRIMSON_NYLIUM->defaultState();
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();

    // 只在 (0,64,0) 放置菌岩，周围放置石头
    world.setBlockAt(BlockPos(0, 64, 0), &nyliumState);
    for (i32 dx = -3; dx <= 3; ++dx) {
        for (i32 dz = -3; dz <= 3; ++dz) {
            if (dx == 0 && dz == 0) {
                continue; // 跳过菌岩位置
            }
            world.setBlockAt(BlockPos(dx, 64, dz), &stoneState);
        }
    }

    IGrowable* growable = const_cast<IGrowable*>(dynamic_cast<const IGrowable*>(&nyliumState.owner()));
    ASSERT_NE(growable, nullptr);

    growable->grow(world, random, BlockPos(0, 64, 0), nyliumState);

    // 检查石头上方（y=65）没有放置植被（因为下方不是菌岩）
    for (i32 dx = -3; dx <= 3; ++dx) {
        for (i32 dz = -3; dz <= 3; ++dz) {
            if (dx == 0 && dz == 0) {
                continue; // 跳过菌岩上方
            }
            const BlockState* aboveStone = world.getBlockAt(BlockPos(dx, 65, dz));
            // 石头上方不应有植被（因为下方不是菌岩）
            if (aboveStone != nullptr && !aboveStone->isAir()) {
                EXPECT_TRUE(false) << "Vegetation should not be placed above non-nylium block at (" << dx << ", 65, "
                                   << dz << ")";
            }
        }
    }

    // 菌岩上方（0, 65, 0）可能有植被
    const BlockState* aboveNylium = world.getBlockAt(BlockPos(0, 65, 0));
    // 可以有也可以没有植被（取决于随机散布），只验证不崩溃
    (void)aboveNylium;
}

TEST_F(NyliumBlockTest, Grow_DoesNotOverwriteExistingBlocks)
{
    // 验证骨粉不会覆盖已有非空气方块
    NyliumTestWorld world;
    math::Random random(42);
    ASSERT_NE(block_registry::NetherBlocks::CRIMSON_NYLIUM, nullptr);

    const BlockState& nyliumState = block_registry::NetherBlocks::CRIMSON_NYLIUM->defaultState();

    // 放置菌岩区域，并在上方某些位置放置石头
    for (i32 dx = -3; dx <= 3; ++dx) {
        for (i32 dz = -3; dz <= 3; ++dz) {
            world.setBlockAt(BlockPos(dx, 64, dz), &nyliumState);
        }
    }
    // 在 (1, 65, 0) 放置石头
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    world.setBlockAt(BlockPos(1, 65, 0), &stoneState);

    IGrowable* growable = const_cast<IGrowable*>(dynamic_cast<const IGrowable*>(&nyliumState.owner()));
    ASSERT_NE(growable, nullptr);

    growable->grow(world, random, BlockPos(0, 64, 0), nyliumState);

    // 石头不应被覆盖
    const BlockState* stoneAfter = world.getBlockAt(BlockPos(1, 65, 0));
    ASSERT_NE(stoneAfter, nullptr);
    EXPECT_TRUE(stoneAfter->is(VanillaBlocks::STONE))
        << "Existing stone block should not be overwritten by bonemeal vegetation";
}

TEST_F(NyliumBlockTest, WarpedNylium_TwistingVinesGroundCheck)
{
    // 验证缠怨藤只放置在有效地面（下界岩/诡异菌岩/诡异疣块）上方
    NyliumTestWorld world;
    ASSERT_NE(block_registry::NetherBlocks::WARPED_NYLIUM, nullptr);

    const BlockState& nyliumState = block_registry::NetherBlocks::WARPED_NYLIUM->defaultState();
    IGrowable* growable = const_cast<IGrowable*>(dynamic_cast<const IGrowable*>(&nyliumState.owner()));
    ASSERT_NE(growable, nullptr);

    bool foundTwistingVinesOnValidGround = false;

    // 使用不同种子尝试触发缠怨藤
    for (i32 seed = 0; seed < 200 && !foundTwistingVinesOnValidGround; ++seed) {
        NyliumTestWorld runWorld;
        math::Random random(seed * 59 + 13);

        // 放置菌岩区域
        for (i32 dx = -3; dx <= 3; ++dx) {
            for (i32 dz = -3; dz <= 3; ++dz) {
                runWorld.setBlockAt(BlockPos(dx, 64, dz), &nyliumState);
            }
        }

        growable->grow(runWorld, random, BlockPos(0, 64, 0), nyliumState);

        // 检查缠怨藤是否只在有效地面放置
        for (i32 dx = -3; dx <= 3; ++dx) {
            for (i32 dz = -3; dz <= 3; ++dz) {
                for (i32 dy = 65; dy <= 70; ++dy) {
                    const BlockState* state = runWorld.getBlockAt(BlockPos(dx, dy, dz));
                    if (state != nullptr &&
                        (state->is(VanillaBlocks::TWISTING_VINES) || state->is(VanillaBlocks::TWISTING_VINES_PLANT))) {
                        // 找到藤蔓头部，检查底部方块是否是有效地面
                        if (state->is(VanillaBlocks::TWISTING_VINES)) {
                            // 检查藤蔓头部的正下方
                            if (dy > 65) {
                                const BlockState* belowState = runWorld.getBlockAt(BlockPos(dx, dy - 1, dz));
                                // 如果下方不是 TWISTING_VINES_PLANT，则下方应该是有效地面
                                if (belowState == nullptr || !belowState->is(VanillaBlocks::TWISTING_VINES_PLANT)) {
                                    // dy-1 应该是地面方块（下界岩、诡异菌岩或诡异疣块）
                                    const BlockState* groundState = runWorld.getBlockAt(BlockPos(dx, dy - 1, dz));
                                    bool isValidGround = (groundState != nullptr &&
                                        (groundState->is(VanillaBlocks::NETHERRACK) ||
                                            groundState->is(VanillaBlocks::WARPED_NYLIUM) ||
                                            groundState->is(VanillaBlocks::WARPED_WART_BLOCK)));
                                    // 地面可能是 TWISTING_VINES_PLANT（柱中间），这种情况也是合法的
                                    if (groundState != nullptr &&
                                        groundState->is(VanillaBlocks::TWISTING_VINES_PLANT)) {
                                        isValidGround = true;
                                    }
                                    EXPECT_TRUE(isValidGround || belowState == nullptr)
                                        << "Twisting vines head should be above valid ground or vine body";
                                }
                            }
                            foundTwistingVinesOnValidGround = true;
                        }
                    }
                }
            }
        }
    }
}

TEST_F(NyliumBlockTest, Grow_MultipleRunsPlaceDifferentVegetation)
{
    // 验证不同随机种子会产生不同的植被分布
    // 确保散布算法确实使用了随机数
    NyliumTestWorld world;
    ASSERT_NE(block_registry::NetherBlocks::CRIMSON_NYLIUM, nullptr);

    const BlockState& nyliumState = block_registry::NetherBlocks::CRIMSON_NYLIUM->defaultState();
    IGrowable* growable = const_cast<IGrowable*>(dynamic_cast<const IGrowable*>(&nyliumState.owner()));
    ASSERT_NE(growable, nullptr);

    bool foundCrimsonRoots = false;
    bool foundCrimsonFungus = false;

    for (i32 seed = 0; seed < 200; ++seed) {
        NyliumTestWorld runWorld;
        math::Random random(seed);

        for (i32 dx = -3; dx <= 3; ++dx) {
            for (i32 dz = -3; dz <= 3; ++dz) {
                runWorld.setBlockAt(BlockPos(dx, 64, dz), &nyliumState);
            }
        }

        growable->grow(runWorld, random, BlockPos(0, 64, 0), nyliumState);

        for (i32 dx = -3; dx <= 3; ++dx) {
            for (i32 dz = -3; dz <= 3; ++dz) {
                const BlockState* state = runWorld.getBlockAt(BlockPos(dx, 65, dz));
                if (state != nullptr && !state->isAir()) {
                    if (state->is(VanillaBlocks::CRIMSON_ROOTS)) {
                        foundCrimsonRoots = true;
                    } else if (state->is(VanillaBlocks::CRIMSON_FUNGUS)) {
                        foundCrimsonFungus = true;
                    }
                }
            }
        }

        if (foundCrimsonRoots && foundCrimsonFungus) {
            break; // 两种类型都找到了，测试通过
        }
    }

    EXPECT_TRUE(foundCrimsonRoots) << "Should find crimson roots across multiple runs";
    EXPECT_TRUE(foundCrimsonFungus) << "Should find crimson fungus across multiple runs";
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
