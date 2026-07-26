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
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/util/Direction.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/blocks/building/StairsBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include "item/core/ItemStack.hpp"
#include "util/math/Vector3.hpp"

#include <memory>
#include <unordered_map>

namespace mc {
namespace blocks {
namespace {

// ============================================================================
// 测试世界
// ============================================================================

/**
 * @brief StairsBlock 测试用的模拟世界
 *
 * 提供方块状态存储和含水检测功能。
 * 未设置方块的位置返回空气方块状态。
 */
class StairsTestWorld : public test::BaseTestWorld {
public:
    StairsTestWorld() { m_airState = &VanillaBlocks::AIR->defaultState(); }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_blocks.find(packPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }
        return m_airState;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        i64 key = packPos(x, y, z);
        if (state == nullptr || state == m_airState) {
            m_blocks.erase(key);
        } else {
            m_blocks[key] = state;
        }
        return true;
    }

    [[nodiscard]] bool hasChunk(i32, i32) const override { return true; }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        if (state != nullptr) {
            const Block& block = state->getBlock();
            const fluid::FluidState* fluidState = block.getFluidState(*state);
            if (fluidState != nullptr && fluidState->isSource()) {
                return fluidState;
            }
        }
        return &fluid::Fluids::EMPTY()->defaultState();
    }

    static i64 packPos(i32 x, i32 y, i32 z)
    {
        return (static_cast<i64>(x) & 0x3FFFFFF) | ((static_cast<i64>(y) & 0xFFF) << 26) |
            ((static_cast<i64>(z) & 0x3FFFFFF) << 38);
    }

private:
    std::unordered_map<i64, const BlockState*> m_blocks;
    const BlockState* m_airState;
};

/**
 * @brief 创建放置上下文的辅助函数
 */
BlockItemUseContext makePlacementContext(
    IWorld& world, const BlockPos& pos, Direction face, f32 playerYaw, f32 hitY = 0.5f)
{
    Vector3 hitPos(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + hitY, static_cast<f32>(pos.z) + 0.5f);
    ItemStack stack;
    return BlockItemUseContext(world, nullptr, stack, hitPos, pos, face, playerYaw, 0.0f);
}

// ============================================================================
// 测试固件
// ============================================================================

class StairsBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();

        // 创建一个基于石头的楼梯方块用于测试
        const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
        stairs_ =
            std::make_unique<StairsBlock>(*stoneState, BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f));
    }

    std::unique_ptr<StairsBlock> stairs_;
};

// ============================================================================
// 基础属性测试
// ============================================================================

TEST_F(StairsBlockTest, DefaultState_HasCorrectProperties)
{
    auto state = stairs_->defaultState();

    // 默认状态：朝北、下半、直梯、不含水
    EXPECT_EQ(state.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
    EXPECT_EQ(state.get(BlockStateProperties::DOUBLE_BLOCK_HALF()), BlockStateProperties::DoubleBlockHalf::Lower);
    EXPECT_EQ(state.get(BlockStateProperties::STAIRS_SHAPE()), BlockStateProperties::StairsShape::Straight);
    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(StairsBlockTest, DefaultState_IsStairs)
{
    auto state = stairs_->defaultState();
    EXPECT_TRUE(StairsBlock::isStairs(state));
}

TEST_F(StairsBlockTest, NonStairsBlock_IsNotStairs)
{
    auto state = VanillaBlocks::STONE->defaultState();
    EXPECT_FALSE(StairsBlock::isStairs(state));
}

TEST_F(StairsBlockTest, StateProperties_CanBeToggled)
{
    auto state = stairs_->defaultState();

    // 朝向
    state = state.with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    EXPECT_EQ(state.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);

    // 上半/下半
    state = state.with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Upper);
    EXPECT_EQ(state.get(BlockStateProperties::DOUBLE_BLOCK_HALF()), BlockStateProperties::DoubleBlockHalf::Upper);

    // 形状
    state = state.with(BlockStateProperties::STAIRS_SHAPE(), BlockStateProperties::StairsShape::InnerLeft);
    EXPECT_EQ(state.get(BlockStateProperties::STAIRS_SHAPE()), BlockStateProperties::StairsShape::InnerLeft);

    // 含水
    state = state.with(BlockStateProperties::WATERLOGGED(), true);
    EXPECT_TRUE(state.get(BlockStateProperties::WATERLOGGED()));
}

// ============================================================================
// 放置测试 - 通过公共接口间接测试 _calculateShape
// ============================================================================

TEST_F(StairsBlockTest, Placement_NoNeighbors_Straight)
{
    StairsTestWorld world;
    BlockPos pos(0, 0, 0);

    // 点击底面放置，朝北，无邻居，应该是直梯
    // 使用 Direction::Down 和低 hitY 确保 Lower half
    auto context = makePlacementContext(world, pos, Direction::Down, 180.0f, 0.3f);
    auto state = stairs_->getStateForPlacement(context);

    EXPECT_EQ(state.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
    EXPECT_EQ(state.get(BlockStateProperties::DOUBLE_BLOCK_HALF()), BlockStateProperties::DoubleBlockHalf::Lower);
    EXPECT_EQ(state.get(BlockStateProperties::STAIRS_SHAPE()), BlockStateProperties::StairsShape::Straight);
}

TEST_F(StairsBlockTest, Placement_TopHalf_WhenClickTop)
{
    StairsTestWorld world;
    BlockPos pos(0, 0, 0);

    // 点击顶面放置 -> 上半 (MC: clickedFace == Up 时 isTop = true)
    auto context = makePlacementContext(world, pos, Direction::Up, 180.0f);
    auto state = stairs_->getStateForPlacement(context);

    // MC 逻辑: clickedFace == Up 时 isTop = true -> Upper
    EXPECT_EQ(state.get(BlockStateProperties::DOUBLE_BLOCK_HALF()), BlockStateProperties::DoubleBlockHalf::Upper);
}

TEST_F(StairsBlockTest, Placement_TopHalf_WhenHitYAboveMiddle)
{
    StairsTestWorld world;
    BlockPos pos(0, 0, 0);

    // 点击侧面但 Y > 0.5 -> 上半
    auto context = makePlacementContext(world, pos, Direction::North, 180.0f, 0.7f);
    auto state = stairs_->getStateForPlacement(context);

    EXPECT_EQ(state.get(BlockStateProperties::DOUBLE_BLOCK_HALF()), BlockStateProperties::DoubleBlockHalf::Upper);
}

TEST_F(StairsBlockTest, Placement_BottomHalf_WhenHitYBelowMiddle)
{
    StairsTestWorld world;
    BlockPos pos(0, 0, 0);

    // 点击侧面且 Y <= 0.5 -> 下半
    auto context = makePlacementContext(world, pos, Direction::North, 180.0f, 0.3f);
    auto state = stairs_->getStateForPlacement(context);

    EXPECT_EQ(state.get(BlockStateProperties::DOUBLE_BLOCK_HALF()), BlockStateProperties::DoubleBlockHalf::Lower);
}

TEST_F(StairsBlockTest, Placement_TopHalf_WhenClickUpAndHitYAboveMiddle)
{
    StairsTestWorld world;
    BlockPos pos(0, 0, 0);

    // 点击顶面且 hitY > 0.5 -> 上半
    auto context = makePlacementContext(world, pos, Direction::Up, 180.0f, 0.7f);
    auto state = stairs_->getStateForPlacement(context);

    EXPECT_EQ(state.get(BlockStateProperties::DOUBLE_BLOCK_HALF()), BlockStateProperties::DoubleBlockHalf::Upper);
}

TEST_F(StairsBlockTest, Placement_BottomHalf_WhenClickDown)
{
    StairsTestWorld world;
    BlockPos pos(0, 0, 0);

    // 点击底面 -> 下半 (clickedFace == Down 且 hitY 不满足 > 0.5)
    auto context = makePlacementContext(world, pos, Direction::Down, 180.0f, 0.3f);
    auto state = stairs_->getStateForPlacement(context);

    EXPECT_EQ(state.get(BlockStateProperties::DOUBLE_BLOCK_HALF()), BlockStateProperties::DoubleBlockHalf::Lower);
}

TEST_F(StairsBlockTest, Placement_WithPerpendicularNeighborForward_OuterRight)
{
    StairsTestWorld world;
    BlockPos pos(0, 0, 0);

    // 在朝向方向(North即Z-)放置朝东的楼梯 - 形成外角
    // North facing, forward neighbor at (0,0,-1) facing East
    auto neighborState =
        stairs_->defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
    world.setBlockState(0, 0, -1, &neighborState);

    auto context = makePlacementContext(world, pos, Direction::Down, 180.0f, 0.3f);
    auto state = stairs_->getStateForPlacement(context);

    // East 是 North 的顺时针旋转 -> OuterRight
    EXPECT_EQ(state.get(BlockStateProperties::STAIRS_SHAPE()), BlockStateProperties::StairsShape::OuterRight);
}

TEST_F(StairsBlockTest, Placement_WithPerpendicularNeighborForward_OuterLeft)
{
    StairsTestWorld world;
    BlockPos pos(0, 0, 0);

    // 在朝向方向(North即Z-)放置朝西的楼梯 - 形成外角
    // North facing, forward neighbor at (0,0,-1) facing West
    auto neighborState =
        stairs_->defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::West)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
    world.setBlockState(0, 0, -1, &neighborState);

    auto context = makePlacementContext(world, pos, Direction::Down, 180.0f, 0.3f);
    auto state = stairs_->getStateForPlacement(context);

    // West 是 North 的逆时针旋转 -> OuterLeft
    EXPECT_EQ(state.get(BlockStateProperties::STAIRS_SHAPE()), BlockStateProperties::StairsShape::OuterLeft);
}

TEST_F(StairsBlockTest, Placement_WithPerpendicularNeighborBackward_InnerRight)
{
    StairsTestWorld world;
    BlockPos pos(0, 0, 0);

    // 在反方向(South即Z+)放置朝东的楼梯 - 形成内角
    auto neighborState =
        stairs_->defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
    world.setBlockState(0, 0, 1, &neighborState);

    auto context = makePlacementContext(world, pos, Direction::Down, 180.0f, 0.3f);
    auto state = stairs_->getStateForPlacement(context);

    // East 是 North 的顺时针旋转 -> InnerRight
    EXPECT_EQ(state.get(BlockStateProperties::STAIRS_SHAPE()), BlockStateProperties::StairsShape::InnerRight);
}

TEST_F(StairsBlockTest, Placement_WithPerpendicularNeighborBackward_InnerLeft)
{
    StairsTestWorld world;
    BlockPos pos(0, 0, 0);

    // 在反方向(South即Z+)放置朝西的楼梯 - 形成内角
    auto neighborState =
        stairs_->defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::West)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
    world.setBlockState(0, 0, 1, &neighborState);

    auto context = makePlacementContext(world, pos, Direction::Down, 180.0f, 0.3f);
    auto state = stairs_->getStateForPlacement(context);

    // West 是 North 的逆时针旋转 -> InnerLeft
    EXPECT_EQ(state.get(BlockStateProperties::STAIRS_SHAPE()), BlockStateProperties::StairsShape::InnerLeft);
}

TEST_F(StairsBlockTest, Placement_SameFacingNeighbor_NoCorner)
{
    StairsTestWorld world;
    BlockPos pos(0, 0, 0);

    // 在朝向方向放置同朝向的楼梯 - 朝向相同轴，不应形成角
    auto neighborState =
        stairs_->defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
    world.setBlockState(0, 0, -1, &neighborState);

    auto context = makePlacementContext(world, pos, Direction::Down, 180.0f, 0.3f);
    auto state = stairs_->getStateForPlacement(context);

    EXPECT_EQ(state.get(BlockStateProperties::STAIRS_SHAPE()), BlockStateProperties::StairsShape::Straight);
}

TEST_F(StairsBlockTest, Placement_DifferentHalf_NoCorner)
{
    StairsTestWorld world;
    BlockPos pos(0, 0, 0);

    // 不同层的楼梯不应形成角
    auto neighborState =
        stairs_->defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Upper);
    world.setBlockState(0, 0, -1, &neighborState);

    auto context = makePlacementContext(world, pos, Direction::Down, 180.0f, 0.3f);
    auto state = stairs_->getStateForPlacement(context);

    EXPECT_EQ(state.get(BlockStateProperties::STAIRS_SHAPE()), BlockStateProperties::StairsShape::Straight);
}

TEST_F(StairsBlockTest, Placement_TJunction_NoCorner)
{
    StairsTestWorld world;
    BlockPos pos(0, 0, 0);

    // 前方是朝东的楼梯，但在朝西方向（opposite(forwardFacing)）也有同朝向同层的楼梯 -> T型，不应形成角
    // canTakeShape 检查 pos.offset(opposite(East)) = pos.offset(West) = (-1,0,0)
    auto forwardState =
        stairs_->defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
    world.setBlockState(0, 0, -1, &forwardState);

    // 在西方位置(-1,0,0)放置同朝向(North)的楼梯，形成T型
    auto sameStairsState =
        stairs_->defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
    world.setBlockState(-1, 0, 0, &sameStairsState);

    auto context = makePlacementContext(world, pos, Direction::Down, 180.0f, 0.3f);
    auto state = stairs_->getStateForPlacement(context);

    // canTakeShape 检查发现第三位置有同朝向同层楼梯 -> 不形成角
    EXPECT_EQ(state.get(BlockStateProperties::STAIRS_SHAPE()), BlockStateProperties::StairsShape::Straight);
}

// ============================================================================
// 邻居更新测试
// ============================================================================

TEST_F(StairsBlockTest, UpdatePostPlacement_RecalculatesShape)
{
    StairsTestWorld world;
    BlockPos pos(0, 0, 0);

    auto state = stairs_->defaultState()
                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                     .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower)
                     .with(BlockStateProperties::STAIRS_SHAPE(), BlockStateProperties::StairsShape::Straight);

    // 放置一个垂直方向邻居
    auto neighborState =
        stairs_->defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
    world.setBlockState(0, 0, -1, &neighborState);

    // 通知北方向有更新
    auto newState =
        stairs_->updatePostPlacement(state, Direction::North, neighborState, world, pos, BlockPos(0, 0, -1));

    EXPECT_EQ(newState.get(BlockStateProperties::STAIRS_SHAPE()), BlockStateProperties::StairsShape::OuterRight);
}

TEST_F(StairsBlockTest, UpdatePostPlacement_VerticalUpdate_NoShapeChange)
{
    StairsTestWorld world;
    BlockPos pos(0, 0, 0);

    auto state = stairs_->defaultState()
                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                     .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower)
                     .with(BlockStateProperties::STAIRS_SHAPE(), BlockStateProperties::StairsShape::Straight);

    auto neighborState =
        stairs_->defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
    world.setBlockState(0, 1, 0, &neighborState);

    // 垂直方向更新不应改变形状
    auto newState = stairs_->updatePostPlacement(state, Direction::Up, neighborState, world, pos, BlockPos(0, 1, 0));

    EXPECT_EQ(newState.get(BlockStateProperties::STAIRS_SHAPE()), BlockStateProperties::StairsShape::Straight);
}

// ============================================================================
// 镜像和旋转测试
// ============================================================================

TEST_F(StairsBlockTest, Rotate_CyclesFacing)
{
    auto state = stairs_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);

    state = stairs_->rotate(state, Rotation::Clockwise90);
    EXPECT_EQ(state.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);

    state = stairs_->rotate(state, Rotation::Clockwise90);
    EXPECT_EQ(state.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South);
}

TEST_F(StairsBlockTest, Mirror_LeftRight_NorthFacing_SwapsOuterShapes)
{
    auto state = stairs_->defaultState()
                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                     .with(BlockStateProperties::STAIRS_SHAPE(), BlockStateProperties::StairsShape::OuterLeft);

    auto mirrored = stairs_->mirror(state, Mirror::LeftRight);
    EXPECT_EQ(mirrored.get(BlockStateProperties::STAIRS_SHAPE()), BlockStateProperties::StairsShape::OuterRight);
}

TEST_F(StairsBlockTest, Mirror_FrontBack_EastFacing_SwapsOuterShapes)
{
    auto state = stairs_->defaultState()
                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East)
                     .with(BlockStateProperties::STAIRS_SHAPE(), BlockStateProperties::StairsShape::OuterLeft);

    auto mirrored = stairs_->mirror(state, Mirror::FrontBack);
    EXPECT_EQ(mirrored.get(BlockStateProperties::STAIRS_SHAPE()), BlockStateProperties::StairsShape::OuterRight);
}

TEST_F(StairsBlockTest, Mirror_None_NoChange)
{
    auto state = stairs_->defaultState()
                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                     .with(BlockStateProperties::STAIRS_SHAPE(), BlockStateProperties::StairsShape::InnerLeft);

    auto mirrored = stairs_->mirror(state, Mirror::None);
    EXPECT_EQ(mirrored.get(BlockStateProperties::STAIRS_SHAPE()), BlockStateProperties::StairsShape::InnerLeft);
    EXPECT_EQ(mirrored.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
}

TEST_F(StairsBlockTest, Mirror_LeftRight_EastFacing_NoEffect)
{
    // LEFT_RIGHT 镜像对 X 轴朝向无效果
    auto state = stairs_->defaultState()
                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East)
                     .with(BlockStateProperties::STAIRS_SHAPE(), BlockStateProperties::StairsShape::Straight);

    auto mirrored = stairs_->mirror(state, Mirror::LeftRight);
    EXPECT_EQ(mirrored.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);
    EXPECT_EQ(mirrored.get(BlockStateProperties::STAIRS_SHAPE()), BlockStateProperties::StairsShape::Straight);
}

TEST_F(StairsBlockTest, Mirror_LeftRight_NorthFacing_SwapsInnerShapes)
{
    auto state = stairs_->defaultState()
                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                     .with(BlockStateProperties::STAIRS_SHAPE(), BlockStateProperties::StairsShape::InnerRight);

    auto mirrored = stairs_->mirror(state, Mirror::LeftRight);
    EXPECT_EQ(mirrored.get(BlockStateProperties::STAIRS_SHAPE()), BlockStateProperties::StairsShape::InnerLeft);
}

TEST_F(StairsBlockTest, Mirror_FrontBack_EastFacing_SwapsInnerShapes)
{
    auto state = stairs_->defaultState()
                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East)
                     .with(BlockStateProperties::STAIRS_SHAPE(), BlockStateProperties::StairsShape::InnerRight);

    auto mirrored = stairs_->mirror(state, Mirror::FrontBack);
    EXPECT_EQ(mirrored.get(BlockStateProperties::STAIRS_SHAPE()), BlockStateProperties::StairsShape::InnerRight);
}

TEST_F(StairsBlockTest, Mirror_FrontBack_NorthFacing_NoEffect)
{
    // FRONT_BACK 镜像对 Z 轴朝向无效果
    auto state = stairs_->defaultState()
                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                     .with(BlockStateProperties::STAIRS_SHAPE(), BlockStateProperties::StairsShape::Straight);

    auto mirrored = stairs_->mirror(state, Mirror::FrontBack);
    EXPECT_EQ(mirrored.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
    EXPECT_EQ(mirrored.get(BlockStateProperties::STAIRS_SHAPE()), BlockStateProperties::StairsShape::Straight);
}

// ============================================================================
// isStairs 测试
// ============================================================================

TEST_F(StairsBlockTest, IsStairs_StairsBlock_ReturnsTrue)
{
    EXPECT_TRUE(StairsBlock::isStairs(stairs_->defaultState()));
}

TEST_F(StairsBlockTest, IsStairs_NonStairsBlock_ReturnsFalse)
{
    EXPECT_FALSE(StairsBlock::isStairs(VanillaBlocks::STONE->defaultState()));
}

// ============================================================================
// 形状更新测试（通过 updatePostPlacement 间接测试）
// ============================================================================

TEST_F(StairsBlockTest, ShapeUpdate_SameAxisNeighborBackward_Straight)
{
    StairsTestWorld world;
    BlockPos pos(0, 0, 0);

    auto state = stairs_->defaultState()
                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                     .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);

    // 在反方向放置同轴朝向的楼梯（South facing，同轴不同朝向）
    auto neighborState =
        stairs_->defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
    world.setBlockState(0, 0, 1, &neighborState);

    auto newState = stairs_->updatePostPlacement(state, Direction::South, neighborState, world, pos, BlockPos(0, 0, 1));

    // 同轴朝向不应形成角
    EXPECT_EQ(newState.get(BlockStateProperties::STAIRS_SHAPE()), BlockStateProperties::StairsShape::Straight);
}

TEST_F(StairsBlockTest, ShapeUpdate_RemoveCornerNeighbor_RevertsToStraight)
{
    StairsTestWorld world;
    BlockPos pos(0, 0, 0);

    // 先设置有角落邻居的环境
    auto neighborState =
        stairs_->defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
    world.setBlockState(0, 0, -1, &neighborState);

    auto stateWithCorner =
        stairs_->defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);

    auto newState =
        stairs_->updatePostPlacement(stateWithCorner, Direction::North, neighborState, world, pos, BlockPos(0, 0, -1));
    EXPECT_EQ(newState.get(BlockStateProperties::STAIRS_SHAPE()), BlockStateProperties::StairsShape::OuterRight);

    // 移除邻居（空气），应该回到直梯
    world.setBlockState(0, 0, -1, nullptr);
    auto airState = VanillaBlocks::AIR->defaultState();
    auto revertedState =
        stairs_->updatePostPlacement(stateWithCorner, Direction::North, airState, world, pos, BlockPos(0, 0, -1));
    EXPECT_EQ(revertedState.get(BlockStateProperties::STAIRS_SHAPE()), BlockStateProperties::StairsShape::Straight);
}

} // namespace
} // namespace blocks
} // namespace mc
