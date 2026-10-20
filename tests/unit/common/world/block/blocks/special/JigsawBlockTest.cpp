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
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/blocks/special/JigsawBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/jigsaw/JigsawOrientation.hpp"

using namespace mc;
using namespace mc::blocks;
using world::gen::jigsaw::JigsawOrientation;

// 拼图方块测试：对齐 MC Java 1.21.11 JigsawBlock.getStateForPlacement / canAttach / getFrontFacing /
// getTopFacing。
namespace {

class JigsawTestWorld final : public mc::test::BaseTestWorld {};

/// 构造放置上下文：face 为点击面，yaw 为玩家水平朝向角（度）。
BlockItemUseContext makeContext(IWorld& world, const BlockPos& pos, Direction face, f32 yaw)
{
    return BlockItemUseContext(world,
        nullptr,
        ItemStack::EMPTY,
        Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + 0.5f, static_cast<f32>(pos.z) + 0.5f),
        pos,
        face,
        yaw,
        0.0f);
}

class JigsawBlockTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { VanillaBlocks::initialize(); }

    JigsawTestWorld world;
    Block* jigsaw = VanillaBlocks::JIGSAW;
};

} // namespace

// 点击 Up 面放置：front = Up；top 取玩家水平朝向的反方向（yaw 0° 面向南 → 水平朝向 South → top = North）。
TEST_F(JigsawBlockTest, Placement_UpFaceUsesPlayerHorizontalDirection)
{
    const BlockPos pos(0, 64, 0);
    BlockItemUseContext context = makeContext(world, pos, Direction::Up, 0.0f);
    const BlockState state = jigsaw->getStateForPlacement(context);

    const JigsawOrientation orientation = state.get(BlockStateProperties::ORIENTATION());
    EXPECT_EQ(world::gen::jigsaw::JigsawOrientations::getFacing(orientation), Direction::Up);
    EXPECT_EQ(world::gen::jigsaw::JigsawOrientations::getRotation(orientation), Direction::North);
}

// 点击南北侧面放置：front 为点击面，top 为 Up。
TEST_F(JigsawBlockTest, Placement_HorizontalFaceUsesUpTop)
{
    const BlockPos pos(0, 64, 0);
    BlockItemUseContext context = makeContext(world, pos, Direction::North, 0.0f);
    const BlockState state = jigsaw->getStateForPlacement(context);

    EXPECT_EQ(JigsawBlock::getFrontFacing(state), Direction::North);
    EXPECT_EQ(JigsawBlock::getTopFacing(state), Direction::Up);
}

// canAttach：front 互为反方向时名称匹配即可对接（rollable）。
TEST_F(JigsawBlockTest, CanAttach_RollableMatchesOppositeFront)
{
    EXPECT_TRUE(JigsawBlock::canAttach(
        JigsawOrientation::NorthUp, "minecraft:bottom", JigsawOrientation::SouthUp, "minecraft:bottom", true));
    // 名称不匹配时不可对接
    EXPECT_FALSE(JigsawBlock::canAttach(
        JigsawOrientation::NorthUp, "minecraft:bottom", JigsawOrientation::SouthUp, "minecraft:top", true));
    // front 不相反时不可对接
    EXPECT_FALSE(JigsawBlock::canAttach(
        JigsawOrientation::NorthUp, "minecraft:bottom", JigsawOrientation::NorthUp, "minecraft:bottom", true));
}

// canAttach：aligned 连接还要求两侧 top 朝向一致。
TEST_F(JigsawBlockTest, CanAttach_AlignedRequiresSameTop)
{
    EXPECT_TRUE(JigsawBlock::canAttach(
        JigsawOrientation::DownEast, "minecraft:a", JigsawOrientation::UpEast, "minecraft:a", false));
    EXPECT_FALSE(JigsawBlock::canAttach(
        JigsawOrientation::DownEast, "minecraft:a", JigsawOrientation::UpWest, "minecraft:a", false));
    EXPECT_TRUE(JigsawBlock::canAttach(
        JigsawOrientation::DownEast, "minecraft:a", JigsawOrientation::UpWest, "minecraft:a", true));
}

// getFrontFacing / getTopFacing 读取 ORIENTATION 的 front 与 rotation 分量。
TEST_F(JigsawBlockTest, FrontAndTopFacingReadOrientation)
{
    const BlockState& state =
        jigsaw->defaultState().with(BlockStateProperties::ORIENTATION(), JigsawOrientation::DownWest);
    EXPECT_EQ(JigsawBlock::getFrontFacing(state), Direction::Down);
    EXPECT_EQ(JigsawBlock::getTopFacing(state), Direction::West);
}