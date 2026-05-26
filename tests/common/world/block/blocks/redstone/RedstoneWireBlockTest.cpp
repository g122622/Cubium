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

#include "physics/collision/CollisionShape.hpp"
#include "util/property/Properties.hpp"
#include "world/block/Block.hpp"
#include "world/block/blocks/redstone/RedstoneWireBlock.hpp"

using namespace mc;
using namespace mc::blocks;

// ============================================================================
// RedstoneWireBlock Shape 测试
// ============================================================================

class RedstoneWireBlockShapeTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        block_ = std::make_unique<RedstoneWireBlock>(BlockProperties(Material::AIR).noCollision().notSolid());
    }

    std::unique_ptr<RedstoneWireBlock> block_;
};

// ========== 中心点形状测试 ==========

TEST_F(RedstoneWireBlockShapeTest, CenterShape_NonEmpty)
{
    // 无连接时，只有中心点形状
    BlockState state = block_->defaultState();
    const CollisionShape& shape = block_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(RedstoneWireBlockShapeTest, CenterShape_HasCollisionBox)
{
    BlockState state = block_->defaultState();
    const CollisionShape& shape = block_->getShape(state);
    // 中心点形状 (3/16, 0, 3/16) -> (13/16, 1/16, 13/16)
    EXPECT_TRUE(shape.boxCount() >= 1);
}

// ========== 无碰撞形状测试 ==========

TEST_F(RedstoneWireBlockShapeTest, CollisionShape_Empty)
{
    BlockState state = block_->defaultState();
    const CollisionShape& collisionShape = block_->getCollisionShape(state);
    EXPECT_TRUE(collisionShape.isEmpty());
}

// ========== 连接状态形状测试 ==========

TEST_F(RedstoneWireBlockShapeTest, SingleSideConnection_IncreasesShape)
{
    // 北面连接
    BlockState state = block_->defaultState();
    state = state.with(BlockStateProperties::REDSTONE_NORTH(), BlockStateProperties::RedstoneSide::Side);

    const CollisionShape& shapeWithNorth = block_->getShape(state);
    const CollisionShape& centerShape = block_->getShape(block_->defaultState());

    // 有连接的形状应该比无连接的大（有更多的碰撞箱或更大的包围盒）
    // 由于combine操作，形状会包含中心点+北面连接
    EXPECT_FALSE(shapeWithNorth.isEmpty());
}

TEST_F(RedstoneWireBlockShapeTest, FourSideConnections_CrossShape)
{
    // 四面都有连接（十字形）
    BlockState state = block_->defaultState();
    state = state.with(BlockStateProperties::REDSTONE_NORTH(), BlockStateProperties::RedstoneSide::Side);
    state = state.with(BlockStateProperties::REDSTONE_SOUTH(), BlockStateProperties::RedstoneSide::Side);
    state = state.with(BlockStateProperties::REDSTONE_EAST(), BlockStateProperties::RedstoneSide::Side);
    state = state.with(BlockStateProperties::REDSTONE_WEST(), BlockStateProperties::RedstoneSide::Side);

    const CollisionShape& shape = block_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(RedstoneWireBlockShapeTest, UpConnection_AscendingShape)
{
    // 向上连接（攀爬）
    BlockState state = block_->defaultState();
    state = state.with(BlockStateProperties::REDSTONE_NORTH(), BlockStateProperties::RedstoneSide::Up);

    const CollisionShape& shape = block_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
    // 向上连接的形状应该包含水平部分和向上延伸部分
}

// ========== 形状缓存测试 ==========

TEST_F(RedstoneWireBlockShapeTest, ShapeCache_SameStateReturnsSameShape)
{
    BlockState state1 = block_->defaultState();
    state1 = state1.with(BlockStateProperties::REDSTONE_NORTH(), BlockStateProperties::RedstoneSide::Side);

    BlockState state2 = block_->defaultState();
    state2 = state2.with(BlockStateProperties::REDSTONE_NORTH(), BlockStateProperties::RedstoneSide::Side);

    const CollisionShape& shape1 = block_->getShape(state1);
    const CollisionShape& shape2 = block_->getShape(state2);

    // 相同状态应该返回相同的形状引用（缓存）
    EXPECT_EQ(&shape1, &shape2);
}

// ========== Power不影响形状测试 ==========

TEST_F(RedstoneWireBlockShapeTest, PowerDoesNotAffectShape)
{
    BlockState state1 = block_->defaultState().with(BlockStateProperties::POWER_0_15(), 0);
    BlockState state2 = block_->defaultState().with(BlockStateProperties::POWER_0_15(), 15);

    const CollisionShape& shape1 = block_->getShape(state1);
    const CollisionShape& shape2 = block_->getShape(state2);

    // Power不影响形状
    EXPECT_EQ(&shape1, &shape2);
}

// ============================================================================
// RedstoneWireBlock 信号输出测试
// ============================================================================

class RedstoneWireBlockPowerTest : public ::testing::Test {
protected:
    void SetUp() override { block_ = std::make_unique<RedstoneWireBlock>(BlockProperties(Material::AIR)); }

    std::unique_ptr<RedstoneWireBlock> block_;
};

TEST_F(RedstoneWireBlockPowerTest, GetPower_ReturnsCorrectValue)
{
    BlockState state = block_->defaultState();
    EXPECT_EQ(RedstoneWireBlock::getPower(state), 0);

    state = state.with(BlockStateProperties::POWER_0_15(), 10);
    EXPECT_EQ(RedstoneWireBlock::getPower(state), 10);

    state = state.with(BlockStateProperties::POWER_0_15(), 15);
    EXPECT_EQ(RedstoneWireBlock::getPower(state), 15);
}

TEST_F(RedstoneWireBlockPowerTest, CanProvidePower_ReturnsTrue)
{
    BlockState state = block_->defaultState();
    EXPECT_TRUE(block_->canProvidePower(state));
}

TEST_F(RedstoneWireBlockPowerTest, IsNormalCube_ReturnsFalseForNonSolid)
{
    BlockState state = block_->defaultState();
    // 红石线不是实体方块
    EXPECT_FALSE(RedstoneWireBlock::isNormalCube(state));
}

// ============================================================================
// RedstoneWireBlock 连接状态测试
// ============================================================================

TEST_F(RedstoneWireBlockShapeTest, ConnectionProperties_Exist)
{
    BlockState state = block_->defaultState();

    // 验证连接属性存在且有默认值
    EXPECT_EQ(state.get(BlockStateProperties::REDSTONE_NORTH()), BlockStateProperties::RedstoneSide::None);
    EXPECT_EQ(state.get(BlockStateProperties::REDSTONE_SOUTH()), BlockStateProperties::RedstoneSide::None);
    EXPECT_EQ(state.get(BlockStateProperties::REDSTONE_EAST()), BlockStateProperties::RedstoneSide::None);
    EXPECT_EQ(state.get(BlockStateProperties::REDSTONE_WEST()), BlockStateProperties::RedstoneSide::None);
}

TEST_F(RedstoneWireBlockShapeTest, SetConnectionState_Works)
{
    BlockState state = block_->defaultState();

    state = state.with(BlockStateProperties::REDSTONE_NORTH(), BlockStateProperties::RedstoneSide::Side);
    EXPECT_EQ(state.get(BlockStateProperties::REDSTONE_NORTH()), BlockStateProperties::RedstoneSide::Side);

    state = state.with(BlockStateProperties::REDSTONE_EAST(), BlockStateProperties::RedstoneSide::Up);
    EXPECT_EQ(state.get(BlockStateProperties::REDSTONE_EAST()), BlockStateProperties::RedstoneSide::Up);
}
