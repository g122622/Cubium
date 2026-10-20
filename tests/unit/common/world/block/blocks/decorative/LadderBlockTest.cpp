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
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/block/registry/VegetationBlocks.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include "item/core/ItemStack.hpp"
#include "util/Direction.hpp"
#include "util/math/Vector3.hpp"
#include "world/block/BlockPos.hpp"
#include "world/block/blocks/decorative/LadderBlock.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

using namespace mc;
using namespace mc::blocks;

namespace {

/// 梯子测试世界：稀疏方块存储，返回存储副本的稳定指针
class LadderTestWorld final : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(packPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr) {
            m_blocks.erase(packPos(x, y, z));
            return true;
        }
        m_storedStates.push_back(std::make_unique<BlockState>(*state));
        m_blocks[packPos(x, y, z)] = m_storedStates.back().get();
        return true;
    }

    // 写入方块状态（携带 LadderTestWorld 静态类型，避免经 const BlockPos& 重载产生歧义）
    void set(const BlockPos& pos, const BlockState& state)
    {
        LadderTestWorld::setBlockState(pos.x, pos.y, pos.z, &state);
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        const fluid::FluidState* fluidState = state->getFluidState();
        return fluidState != nullptr ? fluidState : &fluid::Fluids::EMPTY()->defaultState();
    }

private:
    static i64 packPos(i32 x, i32 y, i32 z) { return (static_cast<i64>(x) << 42) ^ (static_cast<i64>(y) << 21) ^ z; }

    std::unordered_map<i64, const BlockState*> m_blocks;
    std::vector<std::unique_ptr<BlockState>> m_storedStates;
};

/// 构造一个在 blockPos 的 face 面被点击的放置上下文
BlockItemUseContext makeContext(
    LadderTestWorld& world, const BlockPos& blockPos, Direction face, f32 yaw = 0.0f, f32 pitch = 0.0f)
{
    static ItemStack s_stack; // 放置上下文仅需物品堆存在，梯子逻辑不读取其内容
    const Vector3 hitPos(
        static_cast<f32>(blockPos.x) + 0.5f, static_cast<f32>(blockPos.y) + 0.5f, static_cast<f32>(blockPos.z) + 0.5f);
    return BlockItemUseContext(world, nullptr, s_stack, hitPos, blockPos, face, yaw, pitch);
}

/// 取梯子状态的朝向属性
Direction facingOf(const BlockState& state)
{
    return state.get(BlockStateProperties::HORIZONTAL_FACING());
}

/// 梯子世界中可支撑梯子的实心方块状态
const BlockState& solidBlock()
{
    return VanillaBlocks::STONE->defaultState();
}

/// 泥土类实体方块（用于实体碰撞体积检查）
const BlockState& dirtBlock()
{
    return VanillaBlocks::DIRT->defaultState();
}

class LadderAttachmentTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    LadderTestWorld world;
};

} // namespace

// 候选朝向按视线排序：视线朝东、点击石头东面时，首个与放置位置对齐的方向即 East，
// FACING 取其 Opposite = West（梯子倚靠石头）
TEST_F(LadderAttachmentTest, Placement_AdjacentToClickedFaceOpposite)
{
    world.set(BlockPos(0, 64, 0), solidBlock());

    // 玩家朝东（yaw=90）、水平视线点击石头东面：放置位置 (1,64,0)；首个与放置位置对齐的视线方向
    // 为 Up（原版 getNearestLookingDirections 会把点击面反向提到首位），FACING=East 不可存活，
    // 随后尝试 East→West（倚靠石头）可存活
    auto context = makeContext(world, BlockPos(0, 64, 0), Direction::East, 90.0f, 0.0f);
    ASSERT_EQ(context.placementPos(), BlockPos(1, 64, 0));
    const BlockState state = VanillaBlocks::LADDER->getStateForPlacement(context);
    EXPECT_EQ(&state.getBlock(), VanillaBlocks::LADDER);
    EXPECT_EQ(facingOf(state), Direction::West);
    EXPECT_TRUE(
        VanillaBlocks::LADDER->isValidPosition(state, static_cast<IBlockReader&>(world), context.placementPos()));

    // 玩家朝北、水平视线点击石头北面：放置位置 (0,64,-1)。FACING=South 不可存活后尝试 N/W/S，
    // 最终 FACING=North（倚靠北侧 (0,64,-2) 的石块）可存活
    world.set(BlockPos(0, 64, -2), solidBlock());
    auto northContext = makeContext(world, BlockPos(0, 64, 0), Direction::North, 180.0f, 0.0f);
    ASSERT_EQ(northContext.placementPos(), BlockPos(0, 64, -1));
    const BlockState northState = VanillaBlocks::LADDER->getStateForPlacement(northContext);
    EXPECT_EQ(facingOf(northState), Direction::North);
    EXPECT_TRUE(VanillaBlocks::LADDER->isValidPosition(
        northState, static_cast<IBlockReader&>(world), northContext.placementPos()));
}

// 梯子朝向沿视线排序裁决：竖直视线使候选全为竖直方向（统一落到玩家水平朝向），
// 水平视线则按视线方向（同轴优先）逐级尝试。南/北两侧都有墙时，视线朝北的候选 North
// 先命中（FACING=North，倚靠北墙）；视线竖直则落到玩家水平朝向。
TEST_F(LadderAttachmentTest, Placement_ResolvesAlongLookOrder)
{
    world.set(BlockPos(0, 64, 0), solidBlock());
    world.set(BlockPos(0, 65, 1), solidBlock());
    world.set(BlockPos(0, 65, -1), solidBlock());

    // 俯视（视线竖直）：南侧墙可存活 → South
    {
        auto context = makeContext(world, BlockPos(0, 64, 0), Direction::Up, 0.0f, 90.0f);
        const BlockState state = VanillaBlocks::LADDER->getStateForPlacement(context);
        EXPECT_EQ(facingOf(state), Direction::South);
        EXPECT_TRUE(
            VanillaBlocks::LADDER->isValidPosition(state, static_cast<IBlockReader&>(world), context.placementPos()));
    }

    // 水平视线朝北：排序首为 North（FACING=North，北墙可存活）→ North
    {
        auto context = makeContext(world, BlockPos(0, 64, 0), Direction::Up, 180.0f, 0.0f);
        const BlockState state = VanillaBlocks::LADDER->getStateForPlacement(context);
        EXPECT_EQ(facingOf(state), Direction::North);
        EXPECT_TRUE(
            VanillaBlocks::LADDER->isValidPosition(state, static_cast<IBlockReader&>(world), context.placementPos()));
    }
}

// 梯子只接受完整实心面作为附着面
TEST_F(LadderAttachmentTest, CanSurvive_RequiresSolidAttachFace)
{
    auto& ladder = *VanillaBlocks::LADDER;
    auto& reader = static_cast<IBlockReader&>(world);

    BlockState state = ladder.defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    const BlockPos ladderPos(1, 64, 0);

    // 附着方块缺失（空气）
    EXPECT_FALSE(ladder.isValidPosition(state, reader, ladderPos));

    // 附着方块为石头：实心且带碰撞 → 可存活
    // 注意：FACING=East 表示梯子倚靠东侧（X+）的方块，故附着位为 (2,64,0)，而非放置位置 (1,64,0) 自身
    world.set(BlockPos(2, 64, 0), solidBlock());
    EXPECT_TRUE(ladder.isValidPosition(state, reader, ladderPos));

    // 附着方块为非实心方块（场景装饰类，无碰撞）→ 不可存活
    world.set(BlockPos(2, 64, 0), VanillaBlocks::SHORT_GRASS->defaultState());
    EXPECT_FALSE(ladder.isValidPosition(state, reader, ladderPos));
}

// 附着方块被移除时梯子随之破坏（仅当变化方向为附着方向）
TEST_F(LadderAttachmentTest, UpdatePostPlacement_AttachBlockRemovedDropsLadder)
{
    auto& ladder = *VanillaBlocks::LADDER;
    const BlockPos ladderPos(1, 64, 0);

    BlockState state = ladder.defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    world.set(BlockPos(0, 64, 0), solidBlock());

    const BlockState* airState = &VanillaBlocks::AIR->defaultState();
    BlockState updated =
        ladder.updatePostPlacement(state, Direction::East, *airState, world, ladderPos, BlockPos(0, 64, 0));
    EXPECT_EQ(&updated.getBlock(), VanillaBlocks::AIR);
}

// 非附着方向的邻居变化不改动梯子
TEST_F(LadderAttachmentTest, UpdatePostPlacement_UnrelatedNeighborKeepsLadder)
{
    auto& ladder = *VanillaBlocks::LADDER;
    const BlockPos ladderPos(1, 64, 0);

    BlockState state = ladder.defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    world.set(BlockPos(0, 64, 0), solidBlock());

    const BlockState* airState = &VanillaBlocks::AIR->defaultState();
    BlockState updated =
        ladder.updatePostPlacement(state, Direction::North, *airState, world, ladderPos, BlockPos(1, 64, -1));
    EXPECT_EQ(&updated.getBlock(), VanillaBlocks::LADDER);
    EXPECT_EQ(facingOf(updated), Direction::East);
}

// 旋转/镜像保持朝向正确换算
TEST_F(LadderAttachmentTest, RotateAndMirror_UpdatesFacing)
{
    auto& ladder = *VanillaBlocks::LADDER;
    BlockState state = ladder.defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);

    const BlockState& rotated = ladder.rotate(state, Rotation::Clockwise90);
    EXPECT_EQ(facingOf(rotated), Directions::rotateDirection(Direction::East, Rotation::Clockwise90));

    // 左右镜像对南北向朝向无影响
    const BlockState& mirrored = ladder.mirror(state, Mirror::LeftRight);
    EXPECT_EQ(facingOf(mirrored), Direction::East);
}
