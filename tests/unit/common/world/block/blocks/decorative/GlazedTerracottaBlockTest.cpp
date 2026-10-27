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
 * @file GlazedTerracottaBlockTest.cpp
 * @brief 釉面陶瓦活塞推动旋转单元测试
 *
 * 覆盖 Block::getStateForMovement 的默认实现与 GlazedTerracottaBlock 的重写，
 * 以及 PistonBlockEntity 落地时按推动方向计算实际状态的链路。
 */

#include "common/world/block/blocks/decorative/GlazedTerracottaBlock.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/registry/ColoredBlocks.hpp"
#include "common/world/block/registry/RedstoneBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/interactive/PistonBlockEntity.hpp"
#include <gtest/gtest.h>

#include <memory>
#include <unordered_map>

namespace mc {
namespace blocks {
namespace test {

namespace {

/// 可按键记录方块状态的最小测试世界
class GlazedTestWorld final : public ::mc::test::BaseTestWorld {
public:
    using IWorld::getBlockState;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        return it != m_blocks.end() ? it->second : &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr) {
            m_blocks.erase(BlockPos(x, y, z));
        } else {
            m_blocks[BlockPos(x, y, z)] = state;
        }
        return true;
    }

    void setBlockAt(const BlockPos& pos, const BlockState* state)
    {
        setBlockState(pos.x, pos.y, pos.z, state);
    }

    /// 登记非拥有（栈上）方块实体，生命周期由测试自身管理
    void setBlockEntityAt(const BlockPos& pos, BlockEntity* entity)
    {
        m_blockEntities[pos] = entity;
    }

    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos) override
    {
        const auto it = m_blockEntities.find(pos);
        return it == m_blockEntities.end() ? nullptr : it->second;
    }

    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos& pos) const override
    {
        const auto it = m_blockEntities.find(pos);
        return it == m_blockEntities.end() ? nullptr : it->second;
    }

    void removeBlockEntity(const BlockPos& pos) override
    {
        m_blockEntities.erase(pos);
    }

private:
    std::unordered_map<BlockPos, const BlockState*> m_blocks;
    std::unordered_map<BlockPos, BlockEntity*> m_blockEntities;
};

Direction glazedFacing(const BlockState& state)
{
    return state.get(BlockStateProperties::HORIZONTAL_FACING());
}

} // namespace

class GlazedTerracottaBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
    }

    [[nodiscard]] const BlockState& _stateFacing(Direction facing) const
    {
        return block_registry::ColoredBlocks::WHITE_GLAZED_TERRACOTTA->defaultState().with(
            BlockStateProperties::HORIZONTAL_FACING(), facing);
    }
};

// ========== getStateForMovement 默认实现 ==========

TEST_F(GlazedTerracottaBlockTest, DefaultMovementKeepsState)
{
    // 无旋转语义的普通方块，任何推动方向都保持原状态
    const BlockState& stone = VanillaBlocks::STONE->defaultState();
    EXPECT_EQ(VanillaBlocks::STONE->getStateForMovement(stone, Direction::North).stateId(), stone.stateId());
    EXPECT_EQ(VanillaBlocks::STONE->getStateForMovement(stone, Direction::Up).stateId(), stone.stateId());
}

// ========== 平行推动：顺时针 90° ==========

TEST_F(GlazedTerracottaBlockTest, ParallelPushRotatesClockwise)
{
    const Block* terracotta = block_registry::ColoredBlocks::WHITE_GLAZED_TERRACOTTA;
    const BlockState& north = _stateFacing(Direction::North);

    // 沿 Z 轴推动（朝北的面的法线轴）：North → East
    const BlockState& pushedNorth = terracotta->getStateForMovement(north, Direction::South);
    EXPECT_EQ(glazedFacing(pushedNorth), Direction::East);

    // 沿 X 轴推动：East → South
    const BlockState& east = _stateFacing(Direction::East);
    EXPECT_EQ(glazedFacing(terracotta->getStateForMovement(east, Direction::West)), Direction::South);

    // South → West
    const BlockState& south = _stateFacing(Direction::South);
    EXPECT_EQ(glazedFacing(terracotta->getStateForMovement(south, Direction::North)), Direction::West);

    // West → North
    const BlockState& west = _stateFacing(Direction::West);
    EXPECT_EQ(glazedFacing(terracotta->getStateForMovement(west, Direction::East)), Direction::North);
}

TEST_F(GlazedTerracottaBlockTest, PushDirectionSignDoesNotMatter)
{
    // 同轴的相反推动方向都触发旋转且结果一致
    const BlockState& north = _stateFacing(Direction::North);
    EXPECT_EQ(glazedFacing(
                  block_registry::ColoredBlocks::WHITE_GLAZED_TERRACOTTA->getStateForMovement(north, Direction::South)),
        Direction::East);
    EXPECT_EQ(glazedFacing(
                  block_registry::ColoredBlocks::WHITE_GLAZED_TERRACOTTA->getStateForMovement(north, Direction::North)),
        Direction::East);
}

// ========== 垂直推动：保持朝向 ==========

TEST_F(GlazedTerracottaBlockTest, VerticalPushKeepsFacing)
{
    const BlockState& north = _stateFacing(Direction::North);
    EXPECT_EQ(
        glazedFacing(block_registry::ColoredBlocks::WHITE_GLAZED_TERRACOTTA->getStateForMovement(north, Direction::Up)),
        Direction::North);
    EXPECT_EQ(glazedFacing(
                  block_registry::ColoredBlocks::WHITE_GLAZED_TERRACOTTA->getStateForMovement(north, Direction::Down)),
        Direction::North);
}

// ========== 活塞方块实体落地链路 ==========

TEST_F(GlazedTerracottaBlockTest, PistonLandingAppliesRotation)
{
    GlazedTestWorld world;

    // 被推动的釉面陶瓦，朝北；从 (0, 64, 0) 沿 +Z（南）推到 (0, 64, 1)
    const BlockPos target(0, 64, 1);
    const BlockState& movedState = _stateFacing(Direction::North);

    const BlockState& movingPiston =
        VanillaBlocks::MOVING_PISTON->defaultState().with(BlockStateProperties::FACING(), Direction::South);
    world.setBlockAt(target, &movingPiston);

    blockentity::PistonBlockEntity piston(target, &movedState, Direction::South, true, false);
    world.setBlockEntityAt(target, &piston);

    // 两次 tick 推进到完成，第三次 tick 执行落地
    piston.tick(world);
    piston.tick(world);
    piston.tick(world);

    const BlockState* landed = world.getBlockState(target);
    ASSERT_NE(landed, nullptr);
    EXPECT_EQ(&landed->getBlock(), block_registry::ColoredBlocks::WHITE_GLAZED_TERRACOTTA);
    EXPECT_EQ(glazedFacing(*landed), Direction::East);
}

TEST_F(GlazedTerracottaBlockTest, PistonLandingWithoutRotationForOtherBlocks)
{
    GlazedTestWorld world;

    const BlockPos target(5, 64, 5);
    const BlockState& movedState = VanillaBlocks::STONE->defaultState();

    const BlockState& movingPiston =
        VanillaBlocks::MOVING_PISTON->defaultState().with(BlockStateProperties::FACING(), Direction::East);
    world.setBlockAt(target, &movingPiston);

    blockentity::PistonBlockEntity piston(target, &movedState, Direction::East, true, false);
    world.setBlockEntityAt(target, &piston);

    piston.tick(world);
    piston.tick(world);
    piston.tick(world);

    const BlockState* landed = world.getBlockState(target);
    ASSERT_NE(landed, nullptr);
    EXPECT_EQ(&landed->getBlock(), VanillaBlocks::STONE);
}

} // namespace test
} // namespace blocks
} // namespace mc
