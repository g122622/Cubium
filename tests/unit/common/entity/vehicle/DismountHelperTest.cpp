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
 * @file DismountHelperTest.cpp
 * @brief 下马/落点搜索工具单元测试
 *
 * 覆盖地面高度计算（地板/无地板/可攀爬穿透）、安全模式的危险方块拒绝、
 * 玩家不可在末地传送门内部重生、安全模式与非安全模式的差异以及世界边界约束。
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Constants.hpp"
#include "common/entity/vehicle/DismountHelper.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/blocks/building/TrapDoorBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <cmath>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <vector>

using namespace mc;

namespace {

/// 玩家碰撞箱尺寸
constexpr f32 PLAYER_WIDTH = 0.6f;
constexpr f32 PLAYER_HEIGHT = 1.8f;

/**
 * @brief 以方块表为数据源的 IWorld 桩
 *
 * 碰撞检测由非空、非液体方块的完整碰撞箱推导；世界边界可配置。
 */
class DismountTestWorld final : public IWorld {
public:
    using IWorld::getBlockState;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        // 与生产代码一致：已加载列恒有方块状态，未登记即空气；只有区块未加载时 getBlockState
        // 才返回 nullptr（testUnloadedChunks 置位）。
        if (m_unloadedChunks) {
            return nullptr;
        }
        const auto it = m_blocks.find(BlockPos(x, y, z));
        return it != m_blocks.end() ? it->second : &VanillaBlocks::AIR->defaultState();
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

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        MC_UNUSED(flags);
        return setBlockState(x, y, z, state);
    }

    void put(i32 x, i32 y, i32 z, const BlockState* state)
    {
        (void)setBlockState(x, y, z, state);
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override
    {
        return &fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] mc::world::chunk::IChunkManager* chunkManager() override
    {
        return &m_stubChunks;
    }

    [[nodiscard]] const mc::world::chunk::IChunkManager* chunkManager() const override
    {
        return &m_stubChunks;
    }

    mc::test::StubChunkManager m_stubChunks{nullptr, true};

    [[nodiscard]] i32 getHeight(i32, i32) const override
    {
        return 64;
    }

    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override
    {
        return 15;
    }

    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override
    {
        return 15;
    }

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB& box) const override
    {
        return !getBlockCollisions(box).empty();
    }

    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB& box) const override
    {
        std::vector<AxisAlignedBB> collisions;
        for (const auto& [pos, state] : m_blocks) {
            if (state == nullptr) {
                continue;
            }
            // 用方块自身的碰撞形状（空气/液体/植物等空形状方块不产生碰撞）。
            for (const auto& localBox : state->owner().getCollisionShape(*state).boxes()) {
                const AxisAlignedBB blockBox(localBox.minX + static_cast<f32>(pos.x),
                    localBox.minY + static_cast<f32>(pos.y),
                    localBox.minZ + static_cast<f32>(pos.z),
                    localBox.maxX + static_cast<f32>(pos.x),
                    localBox.maxY + static_cast<f32>(pos.y),
                    localBox.maxZ + static_cast<f32>(pos.z));
                const bool overlaps = box.maxX > blockBox.minX && box.minX < blockBox.maxX &&
                    box.maxY > blockBox.minY && box.minY < blockBox.maxY && box.maxZ > blockBox.minZ &&
                    box.minZ < blockBox.maxZ;
                if (overlaps) {
                    collisions.push_back(blockBox);
                }
            }
        }
        return collisions;
    }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }

    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override
    {
        return false;
    }

    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] PhysicsEngine* physicsEngine() override
    {
        return nullptr;
    }

    [[nodiscard]] const PhysicsEngine* physicsEngine() const override
    {
        return nullptr;
    }

    [[nodiscard]] DimensionId dimension() const override
    {
        return 0;
    }

    [[nodiscard]] u64 seed() const override
    {
        return 0;
    }

    [[nodiscard]] u64 currentTick() const override
    {
        return 0;
    }

    [[nodiscard]] i64 dayTime() const override
    {
        return 0;
    }

    [[nodiscard]] bool isHardcore() const override
    {
        return false;
    }

    [[nodiscard]] Difficulty difficulty() const override
    {
        return Difficulty::Easy;
    }

    [[nodiscard]] bool isClientSide() const override
    {
        return false;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(*this);
        }
        return *m_tickManagerPtr;
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        return const_cast<DismountTestWorld*>(this)->tickManager();
    }

    [[nodiscard]] math::IRandom& getRandom() override
    {
        return m_random;
    }

    [[nodiscard]] const math::IRandom& getRandom() const override
    {
        return m_random;
    }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override
    {
        return m_worldBorder;
    }

    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override
    {
        return m_worldBorder;
    }

    /// 模拟区块未加载：所有查询返回 nullptr。
    void setUnloadedChunks(bool unloaded)
    {
        m_unloadedChunks = unloaded;
    }

private:
    bool m_unloadedChunks = false;
    std::map<BlockPos, const BlockState*> m_blocks;
    std::map<BlockPos, BlockState> m_ownedStates;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    world::border::WorldBorder m_worldBorder;
    math::Random m_random;
};

class DismountHelperTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
    }

    [[nodiscard]] const BlockState& stone() const
    {
        return VanillaBlocks::STONE->defaultState();
    }

    [[nodiscard]] std::optional<Vector3> findSafe(const BlockPos& pos, bool isPlayer, bool safeMode)
    {
        return DismountHelper::findSafeDismountLocation(
            world, pos, PLAYER_WIDTH, PLAYER_HEIGHT, isPlayer, false, safeMode);
    }

    DismountTestWorld world;
};

TEST_F(DismountHelperTest, FloorValidityRequiresFiniteAndBelowOne)
{
    EXPECT_TRUE(DismountHelper::isBlockFloorValid(0.0));
    EXPECT_TRUE(DismountHelper::isBlockFloorValid(0.5));
    EXPECT_FALSE(DismountHelper::isBlockFloorValid(1.0));
    EXPECT_FALSE(DismountHelper::isBlockFloorValid(1.5));
    EXPECT_FALSE(DismountHelper::isBlockFloorValid(std::numeric_limits<f64>::infinity()));
}

TEST_F(DismountHelperTest, FloorHeightIsZeroOnSolidBlock)
{
    world.put(0, 63, 0, &stone());
}

TEST_F(DismountHelperTest, FloorHeightAboveAirWithoutFloorIsInfinite)
{
    // 当前方块与下方方块都是空气 → 无地板。
    EXPECT_TRUE(
        std::isinf(DismountHelper::getBlockFloorHeight(world, BlockPos(0, 64, 0), PLAYER_WIDTH, PLAYER_HEIGHT)));
}

TEST_F(DismountHelperTest, DismountOnFloorSucceeds)
{
    world.put(0, 63, 0, &stone());
    const auto position = findSafe(BlockPos(0, 64, 0), true, true);
    ASSERT_TRUE(position.has_value());
    EXPECT_FLOAT_EQ(position->x, 0.5f);
    EXPECT_FLOAT_EQ(position->y, 64.0f);
    EXPECT_FLOAT_EQ(position->z, 0.5f);
}

TEST_F(DismountHelperTest, SafeModeRejectsWitherRose)
{
    // 凋灵玫瑰没有碰撞箱，危险判定只由安全模式承担：安全模式拒绝，非安全模式接受。
    world.put(0, 63, 0, &stone());
    world.put(0, 64, 0, &VanillaBlocks::WITHER_ROSE->defaultState());

    EXPECT_TRUE(DismountHelper::isBlockDangerous(VanillaBlocks::WITHER_ROSE->defaultState(), false));
    EXPECT_FALSE(findSafe(BlockPos(0, 64, 0), true, true).has_value());
    EXPECT_TRUE(findSafe(BlockPos(0, 64, 0), true, false).has_value());
}

TEST_F(DismountHelperTest, SafeModeRejectsStandingOnDangerousFloor)
{
    // 脚下是岩浆块（实心地板且危险）：安全模式拒绝，非安全模式接受。
    world.put(0, 63, 0, &VanillaBlocks::MAGMA->defaultState());
    EXPECT_TRUE(DismountHelper::isBlockDangerous(VanillaBlocks::MAGMA->defaultState(), false));
    EXPECT_FALSE(findSafe(BlockPos(0, 64, 0), true, true).has_value());

    const auto position = findSafe(BlockPos(0, 64, 0), true, false);
    ASSERT_TRUE(position.has_value());
    EXPECT_FLOAT_EQ(position->y, 64.0f);
}

TEST_F(DismountHelperTest, FireImmuneEntityIgnoresBurningBlock)
{
    // 火没有实心碰撞箱：免疫与否只影响危险判定，不影响碰撞。
    world.put(0, 63, 0, &stone());
    world.put(0, 64, 0, &VanillaBlocks::FIRE->defaultState());

    const BlockState& fire = VanillaBlocks::FIRE->defaultState();
    EXPECT_TRUE(DismountHelper::isBlockDangerous(fire, false));
    EXPECT_FALSE(DismountHelper::isBlockDangerous(fire, true));
    EXPECT_FALSE(findSafe(BlockPos(0, 64, 0), false, true).has_value());
    EXPECT_TRUE(DismountHelper::findSafeDismountLocation(
        world, BlockPos(0, 64, 0), PLAYER_WIDTH, PLAYER_HEIGHT, false, true, true)
            .has_value());
}

TEST_F(DismountHelperTest, OpenSpaceWithoutFloorIsRejected)
{
    // 悬空（无地板）：地面高度无限，任何模式都拒绝。
    EXPECT_FALSE(findSafe(BlockPos(0, 64, 0), true, true).has_value());
    EXPECT_FALSE(findSafe(BlockPos(0, 64, 0), true, false).has_value());
}

TEST_F(DismountHelperTest, BlockedSpaceIsRejected)
{
    world.put(0, 63, 0, &stone());
    world.put(0, 65, 0, &stone()); // 玩家头部位置被占
    EXPECT_FALSE(findSafe(BlockPos(0, 64, 0), true, true).has_value());
}

TEST_F(DismountHelperTest, OpenTrapdoorIsNotAFloor)
{
    // 开启的活板门不提供地板：地面高度来自下一格（无地板 → 无限）。
    const auto* trapdoor = dynamic_cast<const blocks::TrapDoorBlock*>(VanillaBlocks::OAK_TRAPDOOR);
    ASSERT_NE(trapdoor, nullptr);
    BlockState openTrapdoor = trapdoor->defaultState().with(BlockStateProperties::OPEN(), true);
    world.put(0, 64, 0, &openTrapdoor);

    EXPECT_TRUE(
        std::isinf(DismountHelper::getBlockFloorHeight(world, BlockPos(0, 64, 0), PLAYER_WIDTH, PLAYER_HEIGHT)));
}

TEST_F(DismountHelperTest, OutsideWorldBorderIsRejected)
{
    world.put(0, 63, 0, &stone());
    world.worldBorder().setCenter(0.0, 0.0);
    world.worldBorder().setSize(10.0);

    EXPECT_TRUE(findSafe(BlockPos(0, 64, 0), true, true).has_value());
    EXPECT_FALSE(findSafe(BlockPos(100, 64, 0), true, true).has_value());
}

} // namespace
