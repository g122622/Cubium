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
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::blocks;

// ============================================================================
// PointedDripstoneBlock 静态方法测试
// ============================================================================

TEST(PointedDripstoneBlockTest, IsStalactite_DownDirection_ReturnsTrue)
{
    BlockProperties props(Material::ROCK);
    PointedDripstoneBlock block(props);
    const BlockState& downState =
        block.defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip);

    EXPECT_TRUE(PointedDripstoneBlock::isStalactite(downState));
}

TEST(PointedDripstoneBlockTest, IsStalactite_UpDirection_ReturnsFalse)
{
    BlockProperties props(Material::ROCK);
    PointedDripstoneBlock block(props);
    const BlockState& upState =
        block.defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip);

    EXPECT_FALSE(PointedDripstoneBlock::isStalactite(upState));
}

TEST(PointedDripstoneBlockTest, IsStalagmite_UpDirection_ReturnsTrue)
{
    BlockProperties props(Material::ROCK);
    PointedDripstoneBlock block(props);
    const BlockState& upState =
        block.defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip);

    EXPECT_TRUE(PointedDripstoneBlock::isStalagmite(upState));
}

TEST(PointedDripstoneBlockTest, IsStalagmite_DownDirection_ReturnsFalse)
{
    BlockProperties props(Material::ROCK);
    PointedDripstoneBlock block(props);
    const BlockState& downState =
        block.defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip);

    EXPECT_FALSE(PointedDripstoneBlock::isStalagmite(downState));
}

TEST(PointedDripstoneBlockTest, CanDrip_TipDownNotWaterlogged_ReturnsTrue)
{
    BlockProperties props(Material::ROCK);
    PointedDripstoneBlock block(props);
    const BlockState& state =
        block.defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip)
            .with(BlockStateProperties::WATERLOGGED(), false);

    EXPECT_TRUE(PointedDripstoneBlock::canDrip(state));
}

TEST(PointedDripstoneBlockTest, CanDrip_TipDownWaterlogged_ReturnsFalse)
{
    BlockProperties props(Material::ROCK);
    PointedDripstoneBlock block(props);
    const BlockState& state =
        block.defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip)
            .with(BlockStateProperties::WATERLOGGED(), true);

    EXPECT_FALSE(PointedDripstoneBlock::canDrip(state));
}

TEST(PointedDripstoneBlockTest, CanDrip_BaseDown_ReturnsFalse)
{
    BlockProperties props(Material::ROCK);
    PointedDripstoneBlock block(props);
    const BlockState& state =
        block.defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Base)
            .with(BlockStateProperties::WATERLOGGED(), false);

    EXPECT_FALSE(PointedDripstoneBlock::canDrip(state));
}

TEST(PointedDripstoneBlockTest, GetDripParticlePosition_CalculatesCorrectOffset)
{
    // Y = blockPos.y + 0.25, X = blockPos.x + 0.5, Z = blockPos.z + 0.5
    BlockPos pos(10, 20, 30);
    Vector3 result = PointedDripstoneBlock::getDripParticlePosition(pos);

    EXPECT_FLOAT_EQ(result.x, 10.5f);
    EXPECT_FLOAT_EQ(result.y, 20.25f);
    EXPECT_FLOAT_EQ(result.z, 30.5f);
}

TEST(PointedDripstoneBlockTest, GetDripParticlePosition_OriginPosition)
{
    BlockPos pos(0, 0, 0);
    Vector3 result = PointedDripstoneBlock::getDripParticlePosition(pos);

    EXPECT_FLOAT_EQ(result.x, 0.5f);
    EXPECT_FLOAT_EQ(result.y, 0.25f);
    EXPECT_FLOAT_EQ(result.z, 0.5f);
}

TEST(PointedDripstoneBlockTest, GetDripParticlePosition_NegativeCoordinates)
{
    BlockPos pos(-5, -10, -15);
    Vector3 result = PointedDripstoneBlock::getDripParticlePosition(pos);

    EXPECT_FLOAT_EQ(result.x, -4.5f);
    EXPECT_FLOAT_EQ(result.y, -9.75f);
    EXPECT_FLOAT_EQ(result.z, -14.5f);
}

TEST(PointedDripstoneBlockTest, IsTip_TipAllowMergeTrue_ReturnsTrue)
{
    BlockProperties props(Material::ROCK);
    PointedDripstoneBlock block(props);
    const BlockState& tipState =
        block.defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip);

    EXPECT_TRUE(PointedDripstoneBlock::isTip(&tipState, true));
    EXPECT_TRUE(PointedDripstoneBlock::isTip(&tipState, false));
}

TEST(PointedDripstoneBlockTest, IsTip_TipMergeAllowMergeTrue_ReturnsTrue)
{
    BlockProperties props(Material::ROCK);
    PointedDripstoneBlock block(props);
    const BlockState& mergeState =
        block.defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::TipMerge);

    EXPECT_TRUE(PointedDripstoneBlock::isTip(&mergeState, true));
    EXPECT_FALSE(PointedDripstoneBlock::isTip(&mergeState, false));
}

TEST(PointedDripstoneBlockTest, IsTip_NullPtr_ReturnsFalse)
{
    EXPECT_FALSE(PointedDripstoneBlock::isTip(nullptr, true));
    EXPECT_FALSE(PointedDripstoneBlock::isTip(nullptr, false));
}

TEST(PointedDripstoneBlockTest, IsPointedDripstoneWithDirection_MatchingDirection_ReturnsTrue)
{
    BlockProperties props(Material::ROCK);
    PointedDripstoneBlock block(props);
    const BlockState& downState =
        block.defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip);

    EXPECT_TRUE(PointedDripstoneBlock::isPointedDripstoneWithDirection(&downState, Direction::Down));
    EXPECT_FALSE(PointedDripstoneBlock::isPointedDripstoneWithDirection(&downState, Direction::Up));
}

TEST(PointedDripstoneBlockTest, IsPointedDripstoneWithDirection_NullPtr_ReturnsFalse)
{
    EXPECT_FALSE(PointedDripstoneBlock::isPointedDripstoneWithDirection(nullptr, Direction::Down));
}
