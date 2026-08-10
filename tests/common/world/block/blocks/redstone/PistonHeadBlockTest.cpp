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
 * LIABILITY, WHETHER IN TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/**
 * @file PistonHeadBlockTest.cpp
 * @brief PistonHeadBlock 单元测试
 *
 * 测试活塞头方块的存活检查和更新逻辑。
 */

#include "common/TestWorldHelper.hpp"
#include "common/world/block/blocks/redstone/PistonHeadBlock.hpp"
#include "common/entity/entities/player/GameModeUtils.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/blocks/redstone/MovingPistonBlock.hpp"
#include "common/world/block/blocks/redstone/PistonBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "core/Constants.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <map>
#include <memory>

namespace mc {
namespace blocks {
namespace test {

/**
 * @brief 用于 PistonHeadBlock 测试的 Mock World 实现
 *
 * 继承 IBlockReader，提供方块读写和最小化测试环境
 */
class PistonHeadTestWorld final : public IBlockReader {
public:
    using IWorld::getBlockState;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const BlockPos pos(x, y, z);
        const auto it = m_blocks.find(pos);
        if (it != m_blocks.end()) {
            return it->second;
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        const BlockPos pos(x, y, z);
        if (state == nullptr || state->isAir()) {
            m_blocks.erase(pos);
        } else {
            m_blocks[pos] = state;
        }
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override { return nullptr; }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT;
    }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] DimensionId dimension() const override { return 0; }
    [[nodiscard]] u64 seed() const override { return 12345; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isClientSide() const override { return false; }
    [[nodiscard]] bool isUltraWarm() const override { return false; }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

    void clearBlockAt(const BlockPos& pos) { m_blocks.erase(pos); }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        ensureTickManager();
        return *m_tickManagerPtr;
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        const_cast<PistonHeadTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

    void clearState() { m_blocks.clear(); }

private:
    void ensureTickManager() const
    {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(const_cast<PistonHeadTestWorld&>(*this));
        }
    }

    std::map<BlockPos, const BlockState*> m_blocks;
    mutable std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    math::Random m_random{12345};
    world::border::WorldBorder m_worldBorder;
};

/**
 * @brief PistonHeadBlock 测试固件
 */
class PistonHeadBlockTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    void TearDown() override { m_world.clearState(); }

    PistonHeadTestWorld m_world;
};

// ========== isFittingBase 测试 ==========

/**
 * @brief 测试 isFittingBase - 普通活塞头匹配已伸出的普通活塞
 */
TEST_F(PistonHeadBlockTest, IsFittingBase_NormalPistonHead_MatchesExtendedPiston)
{
    // 活塞头朝北，类型为 Normal
    const BlockState& headState = VanillaBlocks::PISTON_HEAD->defaultState()
                                      .with(BlockStateProperties::FACING(), Direction::North)
                                      .with(PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Normal);

    // 活塞基座朝北，已伸出
    const BlockState& pistonState = VanillaBlocks::PISTON->defaultState()
                                        .with(BlockStateProperties::FACING(), Direction::North)
                                        .with(BlockStateProperties::EXTENDED(), true);

    EXPECT_TRUE(PistonHeadBlock::isFittingBase(headState, pistonState));
}

/**
 * @brief 测试 isFittingBase - 粘性活塞头匹配已伸出的粘性活塞
 */
TEST_F(PistonHeadBlockTest, IsFittingBase_StickyPistonHead_MatchesExtendedStickyPiston)
{
    // 粘性活塞头朝北
    const BlockState& headState = VanillaBlocks::PISTON_HEAD->defaultState()
                                      .with(BlockStateProperties::FACING(), Direction::North)
                                      .with(PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Sticky);

    // 粘性活塞基座朝北，已伸出
    const BlockState& stickyPistonState = VanillaBlocks::STICKY_PISTON->defaultState()
                                              .with(BlockStateProperties::FACING(), Direction::North)
                                              .with(BlockStateProperties::EXTENDED(), true);

    EXPECT_TRUE(PistonHeadBlock::isFittingBase(headState, stickyPistonState));
}

/**
 * @brief 测试 isFittingBase - 类型不匹配时不匹配
 */
TEST_F(PistonHeadBlockTest, IsFittingBase_TypeMismatch_ReturnsFalse)
{
    // 普通活塞头
    const BlockState& headState = VanillaBlocks::PISTON_HEAD->defaultState()
                                      .with(BlockStateProperties::FACING(), Direction::North)
                                      .with(PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Normal);

    // 粘性活塞（类型不匹配）
    const BlockState& stickyPistonState = VanillaBlocks::STICKY_PISTON->defaultState()
                                              .with(BlockStateProperties::FACING(), Direction::North)
                                              .with(BlockStateProperties::EXTENDED(), true);

    EXPECT_FALSE(PistonHeadBlock::isFittingBase(headState, stickyPistonState));
}

/**
 * @brief 测试 isFittingBase - 活塞未伸出时不匹配
 */
TEST_F(PistonHeadBlockTest, IsFittingBase_NotExtended_ReturnsFalse)
{
    const BlockState& headState = VanillaBlocks::PISTON_HEAD->defaultState()
                                      .with(BlockStateProperties::FACING(), Direction::North)
                                      .with(PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Normal);

    // 活塞未伸出（EXTENDED=false）
    const BlockState& pistonState = VanillaBlocks::PISTON->defaultState()
                                        .with(BlockStateProperties::FACING(), Direction::North)
                                        .with(BlockStateProperties::EXTENDED(), false);

    EXPECT_FALSE(PistonHeadBlock::isFittingBase(headState, pistonState));
}

/**
 * @brief 测试 isFittingBase - 朝向不一致时不匹配
 */
TEST_F(PistonHeadBlockTest, IsFittingBase_FacingMismatch_ReturnsFalse)
{
    // 活塞头朝北
    const BlockState& headState = VanillaBlocks::PISTON_HEAD->defaultState()
                                      .with(BlockStateProperties::FACING(), Direction::North)
                                      .with(PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Normal);

    // 活塞朝南（朝向不一致）
    const BlockState& pistonState = VanillaBlocks::PISTON->defaultState()
                                        .with(BlockStateProperties::FACING(), Direction::South)
                                        .with(BlockStateProperties::EXTENDED(), true);

    EXPECT_FALSE(PistonHeadBlock::isFittingBase(headState, pistonState));
}

/**
 * @brief 测试 isFittingBase - 非活塞方块不匹配
 */
TEST_F(PistonHeadBlockTest, IsFittingBase_NonPistonBlock_ReturnsFalse)
{
    const BlockState& headState = VanillaBlocks::PISTON_HEAD->defaultState()
                                      .with(BlockStateProperties::FACING(), Direction::North)
                                      .with(PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Normal);

    // 石头不是活塞
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    EXPECT_FALSE(PistonHeadBlock::isFittingBase(headState, stoneState));
}

// ========== isValidPosition 测试 ==========

/**
 * @brief 测试 isValidPosition - 有匹配的已伸出活塞基座时返回 true
 */
TEST_F(PistonHeadBlockTest, IsValidPosition_WithExtendedPistonBase_ReturnsTrue)
{
    // 活塞基座在 (0, 64, 0)，朝北，已伸出
    const BlockState& pistonState = VanillaBlocks::PISTON->defaultState()
                                        .with(BlockStateProperties::FACING(), Direction::North)
                                        .with(BlockStateProperties::EXTENDED(), true);
    m_world.setBlockAt(BlockPos(0, 64, 0), &pistonState);

    // 活塞头在 (0, 64, -1)（朝北方向的下一格），朝北
    const BlockState& headState = VanillaBlocks::PISTON_HEAD->defaultState()
                                      .with(BlockStateProperties::FACING(), Direction::North)
                                      .with(PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Normal);

    BlockPos headPos(0, 64, -1);
    EXPECT_TRUE(VanillaBlocks::PISTON_HEAD->isValidPosition(headState, m_world, headPos));
}

/**
 * @brief 测试 isValidPosition - 活塞未伸出时返回 false
 */
TEST_F(PistonHeadBlockTest, IsValidPosition_PistonNotExtended_ReturnsFalse)
{
    // 活塞基座在 (0, 64, 0)，朝北，未伸出
    const BlockState& pistonState = VanillaBlocks::PISTON->defaultState()
                                        .with(BlockStateProperties::FACING(), Direction::North)
                                        .with(BlockStateProperties::EXTENDED(), false);
    m_world.setBlockAt(BlockPos(0, 64, 0), &pistonState);

    const BlockState& headState = VanillaBlocks::PISTON_HEAD->defaultState()
                                      .with(BlockStateProperties::FACING(), Direction::North)
                                      .with(PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Normal);

    BlockPos headPos(0, 64, -1);
    EXPECT_FALSE(VanillaBlocks::PISTON_HEAD->isValidPosition(headState, m_world, headPos));
}

/**
 * @brief 测试 isValidPosition - 活塞头方向有 MOVING_PISTON 时返回 true
 */
TEST_F(PistonHeadBlockTest, IsValidPosition_WithMovingPiston_ReturnsTrue)
{
    // MOVING_PISTON 在 (0, 64, 0)，朝北
    const BlockState& movingPistonState = VanillaBlocks::MOVING_PISTON->defaultState()
                                              .with(BlockStateProperties::FACING(), Direction::North)
                                              .with(PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Normal);
    m_world.setBlockAt(BlockPos(0, 64, 0), &movingPistonState);

    const BlockState& headState = VanillaBlocks::PISTON_HEAD->defaultState()
                                      .with(BlockStateProperties::FACING(), Direction::North)
                                      .with(PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Normal);

    BlockPos headPos(0, 64, -1);
    EXPECT_TRUE(VanillaBlocks::PISTON_HEAD->isValidPosition(headState, m_world, headPos));
}

/**
 * @brief 测试 isValidPosition - MOVING_PISTON 朝向不一致时返回 false
 */
TEST_F(PistonHeadBlockTest, IsValidPosition_MovingPistonFacingMismatch_ReturnsFalse)
{
    // MOVING_PISTON 在 (0, 64, 0)，朝南（与活塞头朝向不匹配）
    const BlockState& movingPistonState = VanillaBlocks::MOVING_PISTON->defaultState()
                                              .with(BlockStateProperties::FACING(), Direction::South)
                                              .with(PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Normal);
    m_world.setBlockAt(BlockPos(0, 64, 0), &movingPistonState);

    const BlockState& headState = VanillaBlocks::PISTON_HEAD->defaultState()
                                      .with(BlockStateProperties::FACING(), Direction::North)
                                      .with(PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Normal);

    BlockPos headPos(0, 64, -1);
    EXPECT_FALSE(VanillaBlocks::PISTON_HEAD->isValidPosition(headState, m_world, headPos));
}

/**
 * @brief 测试 isValidPosition - 反方向为空气时返回 false
 */
TEST_F(PistonHeadBlockTest, IsValidPosition_AirBase_ReturnsFalse)
{
    // 反方向只有空气
    const BlockState& headState = VanillaBlocks::PISTON_HEAD->defaultState()
                                      .with(BlockStateProperties::FACING(), Direction::North)
                                      .with(PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Normal);

    BlockPos headPos(0, 64, -1);
    EXPECT_FALSE(VanillaBlocks::PISTON_HEAD->isValidPosition(headState, m_world, headPos));
}

/**
 * @brief 测试 isValidPosition - 粘性活塞头与普通活塞不匹配
 */
TEST_F(PistonHeadBlockTest, IsValidPosition_StickyHeadNormalPiston_ReturnsFalse)
{
    // 普通活塞基座
    const BlockState& pistonState = VanillaBlocks::PISTON->defaultState()
                                        .with(BlockStateProperties::FACING(), Direction::North)
                                        .with(BlockStateProperties::EXTENDED(), true);
    m_world.setBlockAt(BlockPos(0, 64, 0), &pistonState);

    // 粘性活塞头
    const BlockState& headState = VanillaBlocks::PISTON_HEAD->defaultState()
                                      .with(BlockStateProperties::FACING(), Direction::North)
                                      .with(PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Sticky);

    BlockPos headPos(0, 64, -1);
    EXPECT_FALSE(VanillaBlocks::PISTON_HEAD->isValidPosition(headState, m_world, headPos));
}

// ========== updatePostPlacement 测试 ==========

/**
 * @brief 测试 updatePostPlacement - 活塞基座更新时活塞头消失
 */
TEST_F(PistonHeadBlockTest, UpdatePostPlacement_PistonBaseRemoved_ReturnsAir)
{
    const BlockState& headState = VanillaBlocks::PISTON_HEAD->defaultState()
                                      .with(BlockStateProperties::FACING(), Direction::North)
                                      .with(PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Normal);

    // 活塞头在 (0, 64, -1)，反方向（南）的邻居 (0, 64, 0) 是空气
    // 更新来自南方（活塞基座方向），facingState 是空气
    const BlockState& airState = VanillaBlocks::AIR->defaultState();
    BlockPos headPos(0, 64, -1);
    BlockPos basePos(0, 64, 0);

    // facing = South（更新来自南方，即活塞基座方向）
    // Directions::opposite(South) == North == 活塞头朝向
    BlockState result = VanillaBlocks::PISTON_HEAD->updatePostPlacement(
        headState, Direction::South, airState, m_world, headPos, basePos);

    // 活塞基座不存在，活塞头应变为空气
    EXPECT_TRUE(result.isAir());
}

/**
 * @brief 测试 updatePostPlacement - 更新来自非活塞基座方向时不影响
 */
TEST_F(PistonHeadBlockTest, UpdatePostPlacement_UpdateFromOtherDirection_NoChange)
{
    const BlockState& headState = VanillaBlocks::PISTON_HEAD->defaultState()
                                      .with(BlockStateProperties::FACING(), Direction::North)
                                      .with(PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Normal);

    const BlockState& airState = VanillaBlocks::AIR->defaultState();
    BlockPos headPos(0, 64, -1);
    BlockPos northPos(0, 64, -2);

    // facing = North（更新来自北方，非活塞基座方向）
    // Directions::opposite(North) == South != North（活塞头朝向）
    BlockState result = VanillaBlocks::PISTON_HEAD->updatePostPlacement(
        headState, Direction::North, airState, m_world, headPos, northPos);

    // 来自非基座方向的更新不影响活塞头
    EXPECT_FALSE(result.isAir());
}

/**
 * @brief 测试 updatePostPlacement - 有匹配的已伸出活塞基座时活塞头不变
 */
TEST_F(PistonHeadBlockTest, UpdatePostPlacement_WithExtendedPistonBase_NoChange)
{
    // 设置已伸出的活塞基座
    const BlockState& pistonState = VanillaBlocks::PISTON->defaultState()
                                        .with(BlockStateProperties::FACING(), Direction::North)
                                        .with(BlockStateProperties::EXTENDED(), true);
    m_world.setBlockAt(BlockPos(0, 64, 0), &pistonState);

    const BlockState& headState = VanillaBlocks::PISTON_HEAD->defaultState()
                                      .with(BlockStateProperties::FACING(), Direction::North)
                                      .with(PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Normal);

    BlockPos headPos(0, 64, -1);
    BlockPos basePos(0, 64, 0);

    // 更新来自南方（活塞基座方向）
    BlockState result = VanillaBlocks::PISTON_HEAD->updatePostPlacement(
        headState, Direction::South, pistonState, m_world, headPos, basePos);

    // 活塞基座存在且已伸出，活塞头不变
    EXPECT_FALSE(result.isAir());
}

// ========== getFacing / getType / withType 测试 ==========

/**
 * @brief 测试 getFacing - 朝向属性
 */
TEST_F(PistonHeadBlockTest, GetFacing)
{
    const BlockState& headNorth =
        VanillaBlocks::PISTON_HEAD->defaultState().with(BlockStateProperties::FACING(), Direction::North);
    EXPECT_EQ(PistonHeadBlock::getFacing(headNorth), Direction::North);

    const BlockState& headUp =
        VanillaBlocks::PISTON_HEAD->defaultState().with(BlockStateProperties::FACING(), Direction::Up);
    EXPECT_EQ(PistonHeadBlock::getFacing(headUp), Direction::Up);
}

/**
 * @brief 测试 getType - 类型属性
 */
TEST_F(PistonHeadBlockTest, GetType)
{
    const BlockState& normalHead = VanillaBlocks::PISTON_HEAD->defaultState().with(
        PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Normal);
    EXPECT_EQ(PistonHeadBlock::getType(normalHead), PistonHeadBlock::Type::Normal);

    const BlockState& stickyHead = VanillaBlocks::PISTON_HEAD->defaultState().with(
        PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Sticky);
    EXPECT_EQ(PistonHeadBlock::getType(stickyHead), PistonHeadBlock::Type::Sticky);
}

/**
 * @brief 测试 withType - 修改类型属性
 */
TEST_F(PistonHeadBlockTest, WithType)
{
    const BlockState& normalHead = VanillaBlocks::PISTON_HEAD->defaultState().with(
        PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Normal);

    BlockState stickyHead = PistonHeadBlock::withType(normalHead, PistonHeadBlock::Type::Sticky);
    EXPECT_EQ(PistonHeadBlock::getType(stickyHead), PistonHeadBlock::Type::Sticky);

    // 原始状态不变
    EXPECT_EQ(PistonHeadBlock::getType(normalHead), PistonHeadBlock::Type::Normal);
}

// ========== neighborChanged 测试 ==========

/**
 * @brief 测试 neighborChanged - 活塞头存活时转发通知到活塞基座
 *
 * 当活塞头区域收到邻居变化，且活塞头自身仍能存活（基座存在且已伸出），
 * 通知应被转发到活塞基座方向，确保红石信号能传导到活塞基座。
 */
TEST_F(PistonHeadBlockTest, NeighborChanged_ForwardToBaseWhenValid)
{
    // 设置已伸出的活塞基座在 (0, 64, 0)
    const BlockState& pistonState = VanillaBlocks::PISTON->defaultState()
                                        .with(BlockStateProperties::FACING(), Direction::North)
                                        .with(BlockStateProperties::EXTENDED(), true);
    m_world.setBlockAt(BlockPos(0, 64, 0), &pistonState);

    // 设置活塞头在 (0, 64, -1)
    const BlockState& headState = VanillaBlocks::PISTON_HEAD->defaultState()
                                      .with(BlockStateProperties::FACING(), Direction::North)
                                      .with(PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Normal);
    m_world.setBlockAt(BlockPos(0, 64, -1), &headState);

    // neighborChanged 不应崩溃（基座存在时转发通知）
    // 无法直接验证通知转发（因为需要 mock 基座的 neighborChanged），
    // 但至少确保不会产生异常或崩溃
    BlockPos headPos(0, 64, -1);
    EXPECT_NO_THROW(VanillaBlocks::PISTON_HEAD->neighborChanged(
        m_world, headPos, *VanillaBlocks::STONE, BlockPos(0, 64, -2), false));
}

/**
 * @brief 测试 neighborChanged - 活塞头无法存活时不转发通知
 *
 * 当活塞头无法存活（基座不存在或未伸出）时，不应转发通知。
 */
TEST_F(PistonHeadBlockTest, NeighborChanged_NoForwardWhenInvalid)
{
    // 活塞头在 (0, 64, -1)，但反方向没有匹配的活塞基座
    const BlockState& headState = VanillaBlocks::PISTON_HEAD->defaultState()
                                      .with(BlockStateProperties::FACING(), Direction::North)
                                      .with(PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Normal);
    m_world.setBlockAt(BlockPos(0, 64, -1), &headState);

    BlockPos headPos(0, 64, -1);
    // isValidPosition 将返回 false，不应转发通知
    EXPECT_NO_THROW(VanillaBlocks::PISTON_HEAD->neighborChanged(
        m_world, headPos, *VanillaBlocks::STONE, BlockPos(0, 64, -2), false));
}

// ========== onBlockRemoved 测试 ==========

/**
 * @brief 测试 onBlockRemoved - 有匹配基座时销毁活塞基座
 *
 * 活塞头被移除时，如果反方向有匹配的已伸出活塞基座，
 * 活塞基座应被设为空气（级联销毁）。
 */
TEST_F(PistonHeadBlockTest, OnBlockRemoved_WithFittingBase_SetsBaseToAir)
{
    // 设置已伸出的活塞基座在 (0, 64, 0)
    const BlockState& pistonState = VanillaBlocks::PISTON->defaultState()
                                        .with(BlockStateProperties::FACING(), Direction::North)
                                        .with(BlockStateProperties::EXTENDED(), true);
    m_world.setBlockAt(BlockPos(0, 64, 0), &pistonState);

    // 活塞头在 (0, 64, -1)，朝北
    const BlockState& headState = VanillaBlocks::PISTON_HEAD->defaultState()
                                      .with(BlockStateProperties::FACING(), Direction::North)
                                      .with(PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Normal);

    BlockPos headPos(0, 64, -1);
    BlockPos basePos(0, 64, 0);

    // 验证基座存在
    const BlockState* baseBefore = m_world.getBlockState(basePos);
    ASSERT_NE(baseBefore, nullptr);
    EXPECT_TRUE(baseBefore->is(VanillaBlocks::PISTON));

    // 调用 onBlockRemoved（ItemDropHelper::spawnItemEntity 可能因测试世界不完整而返回 nullptr，但不应崩溃）
    EXPECT_NO_THROW(VanillaBlocks::PISTON_HEAD->onBlockRemoved(m_world, headPos, headState));

    // 验证基座已被设为空气
    const BlockState* baseAfter = m_world.getBlockState(basePos);
    ASSERT_NE(baseAfter, nullptr);
    EXPECT_TRUE(baseAfter->isAir());
}

/**
 * @brief 测试 onBlockRemoved - 无匹配基座时不修改世界
 *
 * 活塞头被移除时，如果反方向没有匹配的已伸出活塞基座，
 * 不应修改任何方块。
 */
TEST_F(PistonHeadBlockTest, OnBlockRemoved_NoFittingBase_NoChange)
{
    // 活塞头在 (0, 64, -1)，朝北，反方向 (0, 64, 0) 没有匹配的活塞基座
    const BlockState& headState = VanillaBlocks::PISTON_HEAD->defaultState()
                                      .with(BlockStateProperties::FACING(), Direction::North)
                                      .with(PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Normal);

    BlockPos headPos(0, 64, -1);
    BlockPos basePos(0, 64, 0);

    // 反方向原本是空气
    const BlockState* baseBefore = m_world.getBlockState(basePos);
    ASSERT_NE(baseBefore, nullptr);
    EXPECT_TRUE(baseBefore->isAir());

    // 调用 onBlockRemoved
    EXPECT_NO_THROW(VanillaBlocks::PISTON_HEAD->onBlockRemoved(m_world, headPos, headState));

    // 反方向应仍然为空气（未被修改）
    const BlockState* baseAfter = m_world.getBlockState(basePos);
    ASSERT_NE(baseAfter, nullptr);
    EXPECT_TRUE(baseAfter->isAir());
}

/**
 * @brief 测试 onBlockRemoved - 基座未伸出时不级联销毁
 *
 * 活塞头被移除时，如果反方向有活塞基座但未伸出（EXTENDED=false），
 * isFittingBase 返回 false，不应级联销毁。
 */
TEST_F(PistonHeadBlockTest, OnBlockRemoved_BaseNotExtended_NoCascade)
{
    // 设置未伸出的活塞基座在 (0, 64, 0)
    const BlockState& pistonState = VanillaBlocks::PISTON->defaultState()
                                        .with(BlockStateProperties::FACING(), Direction::North)
                                        .with(BlockStateProperties::EXTENDED(), false);
    m_world.setBlockAt(BlockPos(0, 64, 0), &pistonState);

    const BlockState& headState = VanillaBlocks::PISTON_HEAD->defaultState()
                                      .with(BlockStateProperties::FACING(), Direction::North)
                                      .with(PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Normal);

    BlockPos headPos(0, 64, -1);
    BlockPos basePos(0, 64, 0);

    // 调用 onBlockRemoved
    EXPECT_NO_THROW(VanillaBlocks::PISTON_HEAD->onBlockRemoved(m_world, headPos, headState));

    // 基座应仍然存在（未被销毁）
    const BlockState* baseAfter = m_world.getBlockState(basePos);
    ASSERT_NE(baseAfter, nullptr);
    EXPECT_TRUE(baseAfter->is(VanillaBlocks::PISTON));
}

// ========== getPushReaction 测试 ==========

/**
 * @brief 测试 getPushReaction - 活塞头的推动反应应为 Block
 *
 * 活塞头不能被活塞推动（应阻止推动）。
 */
TEST_F(PistonHeadBlockTest, GetPushReaction_ReturnsBlock)
{
    const BlockState& headState = VanillaBlocks::PISTON_HEAD->defaultState()
                                      .with(BlockStateProperties::FACING(), Direction::North)
                                      .with(PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Normal);
    EXPECT_EQ(VanillaBlocks::PISTON_HEAD->getPushReaction(headState), Material::PushReaction::Block);
}

// ========== playerWillDestroy 测试 ==========

/**
 * @brief 测试 playerWillDestroy - 创造模式下销毁匹配的活塞基座
 *
 * 创造模式破坏活塞头时，应同时销毁匹配的已伸出活塞基座且不产生掉落物。
 * playerWillDestroy 将基座设为空气后，后续 onBlockRemoved 的 isFittingBase 检查会失败，
 * 避免重复销毁。
 */
TEST_F(PistonHeadBlockTest, PlayerWillDestroy_CreativeMode_DestroysFittingBase)
{
    // 设置已伸出的活塞基座在 (0, 64, 0)
    const BlockState& pistonState = VanillaBlocks::PISTON->defaultState()
                                        .with(BlockStateProperties::FACING(), Direction::North)
                                        .with(BlockStateProperties::EXTENDED(), true);
    m_world.setBlockAt(BlockPos(0, 64, 0), &pistonState);

    // 活塞头在 (0, 64, -1)，朝北
    const BlockState& headState = VanillaBlocks::PISTON_HEAD->defaultState()
                                      .with(BlockStateProperties::FACING(), Direction::North)
                                      .with(PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Normal);

    BlockPos headPos(0, 64, -1);
    BlockPos basePos(0, 64, 0);

    // 创建创造模式玩家
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    player.setGameMode(GameMode::Creative);
    ASSERT_TRUE(player.isCreative());

    // 验证基座存在
    const BlockState* baseBefore = m_world.getBlockState(basePos);
    ASSERT_NE(baseBefore, nullptr);
    EXPECT_TRUE(baseBefore->is(VanillaBlocks::PISTON));

    // 调用 playerWillDestroy
    EXPECT_NO_THROW(VanillaBlocks::PISTON_HEAD->playerWillDestroy(m_world, headPos, headState, player));

    // 验证基座已被设为空气（创造模式下不产生掉落物）
    const BlockState* baseAfter = m_world.getBlockState(basePos);
    ASSERT_NE(baseAfter, nullptr);
    EXPECT_TRUE(baseAfter->isAir());
}

/**
 * @brief 测试 playerWillDestroy - 创造模式下无匹配基座时不修改世界
 *
 * 创造模式破坏活塞头时，如果反方向没有匹配的活塞基座，
 * playerWillDestroy 不应修改任何方块。
 */
TEST_F(PistonHeadBlockTest, PlayerWillDestroy_CreativeMode_NoFittingBase_NoChange)
{
    // 活塞头在 (0, 64, -1)，朝北，反方向 (0, 64, 0) 没有匹配的活塞基座
    const BlockState& headState = VanillaBlocks::PISTON_HEAD->defaultState()
                                      .with(BlockStateProperties::FACING(), Direction::North)
                                      .with(PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Normal);

    BlockPos headPos(0, 64, -1);
    BlockPos basePos(0, 64, 0);

    // 创建创造模式玩家
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    player.setGameMode(GameMode::Creative);

    // 反方向原本是空气
    const BlockState* baseBefore = m_world.getBlockState(basePos);
    ASSERT_NE(baseBefore, nullptr);
    EXPECT_TRUE(baseBefore->isAir());

    // 调用 playerWillDestroy
    EXPECT_NO_THROW(VanillaBlocks::PISTON_HEAD->playerWillDestroy(m_world, headPos, headState, player));

    // 反方向应仍然为空气（未被修改）
    const BlockState* baseAfter = m_world.getBlockState(basePos);
    ASSERT_NE(baseAfter, nullptr);
    EXPECT_TRUE(baseAfter->isAir());
}

/**
 * @brief 测试 playerWillDestroy - 生存模式下不销毁基座
 *
 * 生存模式破坏活塞头时，playerWillDestroy 不执行任何操作。
 * 基座的级联销毁和掉落物由 onBlockRemoved 处理。
 */
TEST_F(PistonHeadBlockTest, PlayerWillDestroy_SurvivalMode_DoesNotDestroyBase)
{
    // 设置已伸出的活塞基座在 (0, 64, 0)
    const BlockState& pistonState = VanillaBlocks::PISTON->defaultState()
                                        .with(BlockStateProperties::FACING(), Direction::North)
                                        .with(BlockStateProperties::EXTENDED(), true);
    m_world.setBlockAt(BlockPos(0, 64, 0), &pistonState);

    const BlockState& headState = VanillaBlocks::PISTON_HEAD->defaultState()
                                      .with(BlockStateProperties::FACING(), Direction::North)
                                      .with(PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Normal);

    BlockPos headPos(0, 64, -1);
    BlockPos basePos(0, 64, 0);

    // 创建生存模式玩家
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    player.setGameMode(GameMode::Survival);
    ASSERT_FALSE(player.isCreative());

    // 验证基座存在
    const BlockState* baseBefore = m_world.getBlockState(basePos);
    ASSERT_NE(baseBefore, nullptr);
    EXPECT_TRUE(baseBefore->is(VanillaBlocks::PISTON));

    // 调用 playerWillDestroy
    EXPECT_NO_THROW(VanillaBlocks::PISTON_HEAD->playerWillDestroy(m_world, headPos, headState, player));

    // 生存模式下基座应保持不变（由 onBlockRemoved 处理级联销毁和掉落物）
    const BlockState* baseAfter = m_world.getBlockState(basePos);
    ASSERT_NE(baseAfter, nullptr);
    EXPECT_TRUE(baseAfter->is(VanillaBlocks::PISTON));
}

/**
 * @brief 测试 playerWillDestroy - 创造模式下基座未伸出时不销毁
 *
 * 创造模式破坏活塞头时，如果反方向有活塞基座但未伸出（EXTENDED=false），
 * isFittingBase 返回 false，不应级联销毁。
 */
TEST_F(PistonHeadBlockTest, PlayerWillDestroy_CreativeMode_BaseNotExtended_NoCascade)
{
    // 设置未伸出的活塞基座在 (0, 64, 0)
    const BlockState& pistonState = VanillaBlocks::PISTON->defaultState()
                                        .with(BlockStateProperties::FACING(), Direction::North)
                                        .with(BlockStateProperties::EXTENDED(), false);
    m_world.setBlockAt(BlockPos(0, 64, 0), &pistonState);

    const BlockState& headState = VanillaBlocks::PISTON_HEAD->defaultState()
                                      .with(BlockStateProperties::FACING(), Direction::North)
                                      .with(PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Normal);

    BlockPos headPos(0, 64, -1);
    BlockPos basePos(0, 64, 0);

    // 创建创造模式玩家
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    player.setGameMode(GameMode::Creative);

    // 调用 playerWillDestroy
    EXPECT_NO_THROW(VanillaBlocks::PISTON_HEAD->playerWillDestroy(m_world, headPos, headState, player));

    // 基座应保持不变（未伸出，不匹配 isFittingBase）
    const BlockState* baseAfter = m_world.getBlockState(basePos);
    ASSERT_NE(baseAfter, nullptr);
    EXPECT_TRUE(baseAfter->is(VanillaBlocks::PISTON));
}

/**
 * @brief 测试 playerWillDestroy 与 onBlockRemoved 的协同 - 创造模式不重复销毁
 *
 * 创造模式下，playerWillDestroy 先将基座设为空气，
 * 后续 onBlockRemoved 中 isFittingBase 检查会失败（基座已不存在），
 * 因此不会重复销毁或产生掉落物。
 */
TEST_F(PistonHeadBlockTest, PlayerWillDestroy_CreativeMode_ThenOnBlockRemoved_NoDuplicate)
{
    // 设置已伸出的活塞基座在 (0, 64, 0)
    const BlockState& pistonState = VanillaBlocks::PISTON->defaultState()
                                        .with(BlockStateProperties::FACING(), Direction::North)
                                        .with(BlockStateProperties::EXTENDED(), true);
    m_world.setBlockAt(BlockPos(0, 64, 0), &pistonState);

    const BlockState& headState = VanillaBlocks::PISTON_HEAD->defaultState()
                                      .with(BlockStateProperties::FACING(), Direction::North)
                                      .with(PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Normal);

    BlockPos headPos(0, 64, -1);
    BlockPos basePos(0, 64, 0);

    // 创建创造模式玩家
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    player.setGameMode(GameMode::Creative);

    // 先调用 playerWillDestroy（创造模式下销毁基座）
    EXPECT_NO_THROW(VanillaBlocks::PISTON_HEAD->playerWillDestroy(m_world, headPos, headState, player));

    // 验证基座已被 playerWillDestroy 设为空气
    const BlockState* baseAfterPlayerWill = m_world.getBlockState(basePos);
    ASSERT_NE(baseAfterPlayerWill, nullptr);
    EXPECT_TRUE(baseAfterPlayerWill->isAir());

    // 然后调用 onBlockRemoved（模拟方块实际被移除时的回调）
    // 由于基座已被设为空气，isFittingBase 应返回 false，不会重复销毁
    EXPECT_NO_THROW(VanillaBlocks::PISTON_HEAD->onBlockRemoved(m_world, headPos, headState));

    // 基座应仍然为空气（未被重复操作）
    const BlockState* baseAfterOnRemoved = m_world.getBlockState(basePos);
    ASSERT_NE(baseAfterOnRemoved, nullptr);
    EXPECT_TRUE(baseAfterOnRemoved->isAir());
}

/**
 * @brief 测试 playerWillDestroy - 粘性活塞头匹配粘性活塞基座
 *
 * 创造模式下破坏粘性活塞头时，应销毁粘性活塞基座（而非普通活塞基座）。
 */
TEST_F(PistonHeadBlockTest, PlayerWillDestroy_CreativeMode_StickyPistonHead_MatchesStickyBase)
{
    // 设置已伸出的粘性活塞基座在 (0, 64, 0)
    const BlockState& stickyPistonState = VanillaBlocks::STICKY_PISTON->defaultState()
                                              .with(BlockStateProperties::FACING(), Direction::North)
                                              .with(BlockStateProperties::EXTENDED(), true);
    m_world.setBlockAt(BlockPos(0, 64, 0), &stickyPistonState);

    // 粘性活塞头在 (0, 64, -1)，朝北
    const BlockState& stickyHeadState = VanillaBlocks::PISTON_HEAD->defaultState()
                                            .with(BlockStateProperties::FACING(), Direction::North)
                                            .with(PistonHeadBlock::getTypeProperty(), PistonHeadBlock::Type::Sticky);

    BlockPos headPos(0, 64, -1);
    BlockPos basePos(0, 64, 0);

    // 创建创造模式玩家
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    player.setGameMode(GameMode::Creative);

    // 调用 playerWillDestroy
    EXPECT_NO_THROW(VanillaBlocks::PISTON_HEAD->playerWillDestroy(m_world, headPos, stickyHeadState, player));

    // 验证粘性活塞基座已被设为空气
    const BlockState* baseAfter = m_world.getBlockState(basePos);
    ASSERT_NE(baseAfter, nullptr);
    EXPECT_TRUE(baseAfter->isAir());
}

} // namespace test
} // namespace blocks
} // namespace mc
