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

#include "../src/common/util/property/Properties.hpp"
#include "../src/common/util/property/StateContainer.hpp"
#include "../src/common/world/block/Block.hpp"
#include "../src/common/world/block/blocks/cave/PointedDripstoneBlock.hpp"
#include "../src/common/world/block/registry/VanillaBlocks.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::blocks;

// ============================================================================
// PointedDripstoneBlock 静态方法测试
//
// isStalactite/isStalagmite/isTip/isPointedDripstoneWithDirection/canDrip 等
// 静态方法内部用 state->is(VanillaBlocks::POINTED_DRIPSTONE) 校验方块身份
// （按指针比较），因此必须使用 VanillaBlocks::POINTED_DRIPSTONE 的状态，
// 而非临时构造的 PointedDripstoneBlock 实例的状态，否则 is() 恒为 false。
// ============================================================================

class PointedDripstoneBlockTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    // 从注册的 POINTED_DRIPSTONE 默认状态派生一个指定方向/厚度/含水状态的状态
    static const BlockState& makeState(
        Direction dir, BlockStateProperties::DripstoneThickness thickness, bool waterlogged = false)
    {
        return VanillaBlocks::POINTED_DRIPSTONE->defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), dir)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), thickness)
            .with(BlockStateProperties::WATERLOGGED(), waterlogged);
    }
};

TEST_F(PointedDripstoneBlockTest, IsStalactite_DownDirection_ReturnsTrue)
{
    const BlockState& downState = makeState(Direction::Down, BlockStateProperties::DripstoneThickness::Tip);
    EXPECT_TRUE(PointedDripstoneBlock::isStalactite(downState));
}

TEST_F(PointedDripstoneBlockTest, IsStalactite_UpDirection_ReturnsFalse)
{
    const BlockState& upState = makeState(Direction::Up, BlockStateProperties::DripstoneThickness::Tip);
    EXPECT_FALSE(PointedDripstoneBlock::isStalactite(upState));
}

TEST_F(PointedDripstoneBlockTest, IsStalagmite_UpDirection_ReturnsTrue)
{
    const BlockState& upState = makeState(Direction::Up, BlockStateProperties::DripstoneThickness::Tip);
    EXPECT_TRUE(PointedDripstoneBlock::isStalagmite(upState));
}

TEST_F(PointedDripstoneBlockTest, IsStalagmite_DownDirection_ReturnsFalse)
{
    const BlockState& downState = makeState(Direction::Down, BlockStateProperties::DripstoneThickness::Tip);
    EXPECT_FALSE(PointedDripstoneBlock::isStalagmite(downState));
}

TEST_F(PointedDripstoneBlockTest, CanDrip_TipDownNotWaterlogged_ReturnsTrue)
{
    const BlockState& state = makeState(Direction::Down, BlockStateProperties::DripstoneThickness::Tip, false);
    EXPECT_TRUE(PointedDripstoneBlock::canDrip(state));
}

TEST_F(PointedDripstoneBlockTest, CanDrip_TipDownWaterlogged_ReturnsFalse)
{
    const BlockState& state = makeState(Direction::Down, BlockStateProperties::DripstoneThickness::Tip, true);
    EXPECT_FALSE(PointedDripstoneBlock::canDrip(state));
}

TEST_F(PointedDripstoneBlockTest, CanDrip_BaseDown_ReturnsFalse)
{
    const BlockState& state = makeState(Direction::Down, BlockStateProperties::DripstoneThickness::Base, false);
    EXPECT_FALSE(PointedDripstoneBlock::canDrip(state));
}

TEST_F(PointedDripstoneBlockTest, GetDripParticlePosition_CalculatesCorrectOffset)
{
    // Y = blockPos.y + 0.25, X = blockPos.x + 0.5, Z = blockPos.z + 0.5
    BlockPos pos(10, 20, 30);
    Vector3 result = PointedDripstoneBlock::getDripParticlePosition(pos);

    EXPECT_FLOAT_EQ(result.x, 10.5f);
    EXPECT_FLOAT_EQ(result.y, 20.25f);
    EXPECT_FLOAT_EQ(result.z, 30.5f);
}

TEST_F(PointedDripstoneBlockTest, GetDripParticlePosition_OriginPosition)
{
    BlockPos pos(0, 0, 0);
    Vector3 result = PointedDripstoneBlock::getDripParticlePosition(pos);

    EXPECT_FLOAT_EQ(result.x, 0.5f);
    EXPECT_FLOAT_EQ(result.y, 0.25f);
    EXPECT_FLOAT_EQ(result.z, 0.5f);
}

TEST_F(PointedDripstoneBlockTest, GetDripParticlePosition_NegativeCoordinates)
{
    BlockPos pos(-5, -10, -15);
    Vector3 result = PointedDripstoneBlock::getDripParticlePosition(pos);

    EXPECT_FLOAT_EQ(result.x, -4.5f);
    EXPECT_FLOAT_EQ(result.y, -9.75f);
    EXPECT_FLOAT_EQ(result.z, -14.5f);
}

TEST_F(PointedDripstoneBlockTest, IsTip_TipAllowMergeTrue_ReturnsTrue)
{
    const BlockState& tipState = makeState(Direction::Down, BlockStateProperties::DripstoneThickness::Tip);

    EXPECT_TRUE(PointedDripstoneBlock::isTip(&tipState, true));
    EXPECT_TRUE(PointedDripstoneBlock::isTip(&tipState, false));
}

TEST_F(PointedDripstoneBlockTest, IsTip_TipMergeAllowMergeTrue_ReturnsTrue)
{
    const BlockState& mergeState = makeState(Direction::Down, BlockStateProperties::DripstoneThickness::TipMerge);

    EXPECT_TRUE(PointedDripstoneBlock::isTip(&mergeState, true));
    EXPECT_FALSE(PointedDripstoneBlock::isTip(&mergeState, false));
}

TEST_F(PointedDripstoneBlockTest, IsTip_NullPtr_ReturnsFalse)
{
    EXPECT_FALSE(PointedDripstoneBlock::isTip(nullptr, true));
    EXPECT_FALSE(PointedDripstoneBlock::isTip(nullptr, false));
}

TEST_F(PointedDripstoneBlockTest, IsPointedDripstoneWithDirection_MatchingDirection_ReturnsTrue)
{
    const BlockState& downState = makeState(Direction::Down, BlockStateProperties::DripstoneThickness::Tip);

    EXPECT_TRUE(PointedDripstoneBlock::isPointedDripstoneWithDirection(&downState, Direction::Down));
    EXPECT_FALSE(PointedDripstoneBlock::isPointedDripstoneWithDirection(&downState, Direction::Up));
}

TEST_F(PointedDripstoneBlockTest, IsPointedDripstoneWithDirection_NullPtr_ReturnsFalse)
{
    EXPECT_FALSE(PointedDripstoneBlock::isPointedDripstoneWithDirection(nullptr, Direction::Down));
}
