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

/**
 * @file SupportTypeTest.cpp
 * @brief SupportType / canSupportCenter / canSupportRigidBlock 单元测试
 *
 * 验证 MC 1.21.11 三种支撑类型（Full / Center / Rigid）的判定逻辑：
 * - Full：方块面投影必须覆盖整个 1×1 面
 * - Center：方块面投影必须包含中心 2×2 像素到 10×10 像素的"中心柱"
 * - Rigid：方块面投影必须覆盖 1×1 面除中心 12×12 像素柱以外的外环
 *
 * 同时验证：
 * - Block::canSupportCenter 在 UNSTABLE_BOTTOM_CENTER 标签（栅栏门）上返回 false
 * - Block::canSupportRigidBlock 用于铁轨支撑判定
 * - BlockState::isFaceSturdy 正确委托到 SupportType
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/SupportType.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>

using namespace mc;

namespace {

// ============================================================================
// 测试用世界：支持按位置设置/查询方块状态
// ============================================================================

class SupportTestWorld : public IBlockReader {
public:
    SupportTestWorld()
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
        m_airState = &VanillaBlocks::AIR->defaultState();
    }

    const BlockState* storeBlockState(const BlockState& state)
    {
        m_storedStates.push_back(std::make_unique<BlockState>(state));
        return m_storedStates.back().get();
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(packPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }
        return m_airState;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        const i64 key = packPos(x, y, z);
        if (state == nullptr || state == m_airState) {
            m_blocks.erase(key);
        } else {
            m_blocks[key] = state;
        }
        return true;
    }

    bool setBlockStateCopy(const BlockPos& pos, const BlockState& state)
    {
        const BlockState* stored = storeBlockState(state);
        return setBlockState(pos.x, pos.y, pos.z, stored);
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override
    {
        return &fluid::Fluids::EMPTY()->defaultState();
    }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(
        const AxisAlignedBB&, const Entity* = nullptr) const override
    {
        return {};
    }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] DimensionId dimension() const override { return 0; }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isClientSide() const override { return false; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("SupportTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("SupportTestWorld::tickManager not implemented");
    }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

private:
    [[nodiscard]] static i64 packPos(i32 x, i32 y, i32 z)
    {
        return (static_cast<i64>(x) & 0x3FFFFFF) | ((static_cast<i64>(y) & 0xFFF) << 26) |
            ((static_cast<i64>(z) & 0x3FFFFFF) << 38);
    }

    std::unordered_map<i64, const BlockState*> m_blocks;
    std::vector<std::unique_ptr<BlockState>> m_storedStates;
    const BlockState* m_airState;
    world::border::WorldBorder m_worldBorder;
    mutable math::Random m_random{12345};
};

} // namespace

// ============================================================================
// SupportType 静态实例测试
// ============================================================================

TEST(SupportTypeTest, StaticInstancesAreAccessible)
{
    // 仅验证静态实例存在且可取地址
    EXPECT_NE(&SupportType::Full, nullptr);
    EXPECT_NE(&SupportType::Center, nullptr);
    EXPECT_NE(&SupportType::Rigid, nullptr);
}

// ============================================================================
// BlockTags::UNSTABLE_BOTTOM_CENTER 标签测试
// ============================================================================

class SupportTypeTagTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
    }
};

TEST_F(SupportTypeTagTest, UnstableBottomCenterContainsOakFenceGate)
{
    ASSERT_NE(VanillaBlocks::OAK_FENCE_GATE, nullptr);
    EXPECT_TRUE(BlockTags::UNSTABLE_BOTTOM_CENTER().contains(*VanillaBlocks::OAK_FENCE_GATE));
}

TEST_F(SupportTypeTagTest, UnstableBottomCenterContainsCrimsonFenceGate)
{
    ASSERT_NE(VanillaBlocks::CRIMSON_FENCE_GATE, nullptr);
    EXPECT_TRUE(BlockTags::UNSTABLE_BOTTOM_CENTER().contains(*VanillaBlocks::CRIMSON_FENCE_GATE));
}

TEST_F(SupportTypeTagTest, UnstableBottomCenterDoesNotContainStone)
{
    ASSERT_NE(VanillaBlocks::STONE, nullptr);
    EXPECT_FALSE(BlockTags::UNSTABLE_BOTTOM_CENTER().contains(*VanillaBlocks::STONE));
}

TEST_F(SupportTypeTagTest, UnstableBottomCenterIdIsCorrect)
{
    EXPECT_EQ(BlockTags::UNSTABLE_BOTTOM_CENTER().getId(), ResourceLocation("minecraft", "unstable_bottom_center"));
}

// ============================================================================
// Block::canSupportCenter / canSupportRigidBlock 测试
// ============================================================================

class CanSupportCenterTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
    }
};

TEST_F(CanSupportCenterTest, FullBlock_ProvidesCenterSupportFromBelow)
{
    SupportTestWorld world;
    ASSERT_NE(VanillaBlocks::STONE, nullptr);
    const BlockState& stone = VanillaBlocks::STONE->defaultState();
    world.setBlockStateCopy(BlockPos(0, 0, 0), stone);

    // 石头方块顶面提供 Center 支撑
    EXPECT_TRUE(Block::canSupportCenter(world, BlockPos(0, 0, 0), Direction::Up));
    // 石头方块底面也提供 Center 支撑（向下悬挂判定）
    EXPECT_TRUE(Block::canSupportCenter(world, BlockPos(0, 0, 0), Direction::Down));
}

TEST_F(CanSupportCenterTest, FullBlock_ProvidesRigidSupport)
{
    SupportTestWorld world;
    const BlockState& stone = VanillaBlocks::STONE->defaultState();
    world.setBlockStateCopy(BlockPos(0, 0, 0), stone);

    EXPECT_TRUE(Block::canSupportRigidBlock(world, BlockPos(0, 0, 0)));
}

TEST_F(CanSupportCenterTest, Air_DoesNotProvideCenterSupport)
{
    SupportTestWorld world;
    EXPECT_FALSE(Block::canSupportCenter(world, BlockPos(0, 100, 0), Direction::Up));
    EXPECT_FALSE(Block::canSupportRigidBlock(world, BlockPos(0, 100, 0)));
}

TEST_F(CanSupportCenterTest, FenceGate_DoesNotProvideCenterSupportDownward)
{
    SupportTestWorld world;
    ASSERT_NE(VanillaBlocks::OAK_FENCE_GATE, nullptr);
    const BlockState& gate = VanillaBlocks::OAK_FENCE_GATE->defaultState();
    world.setBlockStateCopy(BlockPos(0, 5, 0), gate);

    // MC 1.21.11: 栅栏门属于 UNSTABLE_BOTTOM_CENTER 标签，
    // canSupportCenter(world, pos, DOWN) 必须返回 false（标签强制拒绝）
    EXPECT_FALSE(Block::canSupportCenter(world, BlockPos(0, 5, 0), Direction::Down));

    // Up 方向不受 UNSTABLE_BOTTOM_CENTER 标签影响（仅 DOWN 方向检查标签）。
    // 栅栏门默认朝向 North，碰撞形状为 cube(0,0,0.4375, 1,1,0.5625)（z 轴窄条）。
    // 顶面投影在 x 轴覆盖 [0,1]，在 z 轴覆盖 [0.4375,0.5625]=[7/16,9/16]，
    // 完全包含 CENTER 支撑柱（[7/16,9/16]×[7/16,9/16]），故 Up 方向提供 Center 支撑。
    EXPECT_TRUE(Block::canSupportCenter(world, BlockPos(0, 5, 0), Direction::Up));
}

TEST_F(CanSupportCenterTest, FenceGate_DoesNotProvideRigidSupport)
{
    SupportTestWorld world;
    const BlockState& gate = VanillaBlocks::OAK_FENCE_GATE->defaultState();
    world.setBlockStateCopy(BlockPos(0, 5, 0), gate);

    // 栅栏门碰撞形状不是完整方块，外环不完整，不提供 Rigid 支撑
    EXPECT_FALSE(Block::canSupportRigidBlock(world, BlockPos(0, 5, 0)));
}

TEST_F(CanSupportCenterTest, Dirt_ProvidesCenterAndRigidSupport)
{
    SupportTestWorld world;
    ASSERT_NE(VanillaBlocks::DIRT, nullptr);
    const BlockState& dirt = VanillaBlocks::DIRT->defaultState();
    world.setBlockStateCopy(BlockPos(2, 3, 2), dirt);

    EXPECT_TRUE(Block::canSupportCenter(world, BlockPos(2, 3, 2), Direction::Up));
    EXPECT_TRUE(Block::canSupportRigidBlock(world, BlockPos(2, 3, 2)));
}

// ============================================================================
// BlockState::isFaceSturdy 测试
// ============================================================================

TEST_F(CanSupportCenterTest, IsFaceSturdy_FullBlock_AllSupportTypes)
{
    SupportTestWorld world;
    const BlockState& stone = VanillaBlocks::STONE->defaultState();
    world.setBlockStateCopy(BlockPos(0, 0, 0), stone);

    const BlockState* state = world.getBlockState(0, 0, 0);
    ASSERT_NE(state, nullptr);

    // 完整方块在所有方向都满足 Full / Center / Rigid 支撑
    EXPECT_TRUE(state->isFaceSturdy(world, BlockPos(0, 0, 0), Direction::Up, SupportType::Full));
    EXPECT_TRUE(state->isFaceSturdy(world, BlockPos(0, 0, 0), Direction::Down, SupportType::Full));
    EXPECT_TRUE(state->isFaceSturdy(world, BlockPos(0, 0, 0), Direction::Up, SupportType::Center));
    EXPECT_TRUE(state->isFaceSturdy(world, BlockPos(0, 0, 0), Direction::Down, SupportType::Center));
    EXPECT_TRUE(state->isFaceSturdy(world, BlockPos(0, 0, 0), Direction::Up, SupportType::Rigid));
    EXPECT_TRUE(state->isFaceSturdy(world, BlockPos(0, 0, 0), Direction::Down, SupportType::Rigid));
}

TEST_F(CanSupportCenterTest, IsFaceSturdy_Air_NoSupport)
{
    SupportTestWorld world;
    const BlockState* air = world.getBlockState(0, 100, 0);
    ASSERT_NE(air, nullptr);

    EXPECT_FALSE(air->isFaceSturdy(world, BlockPos(0, 100, 0), Direction::Up, SupportType::Full));
    EXPECT_FALSE(air->isFaceSturdy(world, BlockPos(0, 100, 0), Direction::Up, SupportType::Center));
    EXPECT_FALSE(air->isFaceSturdy(world, BlockPos(0, 100, 0), Direction::Up, SupportType::Rigid));
}
