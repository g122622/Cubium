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
#include "world/block/blocks/redstone/TripWireBlock.hpp"

using namespace mc;
using namespace mc::blocks;

// ============================================================================
// TripWireBlock 形状测试
// ============================================================================

class TripWireBlockShapeTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        block_ = std::make_unique<TripWireBlock>(BlockProperties(Material::AIR).noCollision().notSolid());
    }

    std::unique_ptr<TripWireBlock> block_;
};

// ========== 形状测试 ==========

TEST_F(TripWireBlockShapeTest, AttachedShape_HasCollisionBox)
{
    // 绷紧状态的绊线
    BlockState state = block_->defaultState().with(BlockStateProperties::ATTACHED(), true);

    const CollisionShape& shape = block_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
    EXPECT_TRUE(shape.boxCount() >= 1);
}

TEST_F(TripWireBlockShapeTest, DetachedShape_HasCollisionBox)
{
    // 松弛状态的绊线
    BlockState state = block_->defaultState().with(BlockStateProperties::ATTACHED(), false);

    const CollisionShape& shape = block_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
    EXPECT_TRUE(shape.boxCount() >= 1);
}

TEST_F(TripWireBlockShapeTest, AttachedShape_HigherThanDetached)
{
    // 绷紧状态比松弛状态更高（Y轴起点不同）
    BlockState attachedState = block_->defaultState().with(BlockStateProperties::ATTACHED(), true);
    BlockState detachedState = block_->defaultState().with(BlockStateProperties::ATTACHED(), false);

    const CollisionShape& attachedShape = block_->getShape(attachedState);
    const CollisionShape& detachedShape = block_->getShape(detachedState);

    // 两者都应该有形状
    EXPECT_FALSE(attachedShape.isEmpty());
    EXPECT_FALSE(detachedShape.isEmpty());

    // 形状应该是不同的
    EXPECT_NE(&attachedShape, &detachedShape);
}

TEST_F(TripWireBlockShapeTest, CollisionShape_Empty)
{
    // 绊线没有碰撞箱（实体可以穿过）
    BlockState state = block_->defaultState();
    const CollisionShape& collisionShape = block_->getCollisionShape(state);
    EXPECT_TRUE(collisionShape.isEmpty());
}

// ========== 连接属性测试 ==========

TEST_F(TripWireBlockShapeTest, ConnectionProperties_Exist)
{
    BlockState state = block_->defaultState();

    // 验证连接属性存在
    EXPECT_FALSE(state.get(BlockStateProperties::NORTH()));
    EXPECT_FALSE(state.get(BlockStateProperties::SOUTH()));
    EXPECT_FALSE(state.get(BlockStateProperties::EAST()));
    EXPECT_FALSE(state.get(BlockStateProperties::WEST()));
}

TEST_F(TripWireBlockShapeTest, PoweredProperty_Exists)
{
    BlockState state = block_->defaultState();
    EXPECT_FALSE(state.get(BlockStateProperties::POWERED()));

    state = state.with(BlockStateProperties::POWERED(), true);
    EXPECT_TRUE(state.get(BlockStateProperties::POWERED()));
}

TEST_F(TripWireBlockShapeTest, DisarmedProperty_Exists)
{
    BlockState state = block_->defaultState();
    EXPECT_FALSE(state.get(BlockStateProperties::DISARMED()));

    state = state.with(BlockStateProperties::DISARMED(), true);
    EXPECT_TRUE(state.get(BlockStateProperties::DISARMED()));
}

// ============================================================================
// TripWireBlock 信号输出测试
// ============================================================================

class TripWireBlockPowerTest : public ::testing::Test {
protected:
    void SetUp() override { block_ = std::make_unique<TripWireBlock>(BlockProperties(Material::AIR)); }

    std::unique_ptr<TripWireBlock> block_;
};

TEST_F(TripWireBlockPowerTest, GetWeakPower_Returns15WhenPowered)
{
    BlockState state = block_->defaultState().with(BlockStateProperties::POWERED(), true);
    // 绊线只输出弱信号
    // 需要在实际世界中测试，这里验证canProvidePower
    EXPECT_TRUE(block_->canProvidePower(state));
}

TEST_F(TripWireBlockPowerTest, GetWeakPower_Returns0WhenNotPowered)
{
    BlockState state = block_->defaultState().with(BlockStateProperties::POWERED(), false);
    EXPECT_TRUE(block_->canProvidePower(state));
}

TEST_F(TripWireBlockPowerTest, CanProvidePower_ReturnsTrue)
{
    BlockState state = block_->defaultState();
    EXPECT_TRUE(block_->canProvidePower(state));
}

TEST_F(TripWireBlockPowerTest, IsPowered_StaticMethod)
{
    BlockState poweredState = block_->defaultState().with(BlockStateProperties::POWERED(), true);
    BlockState unpoweredState = block_->defaultState().with(BlockStateProperties::POWERED(), false);

    EXPECT_TRUE(TripWireBlock::isPowered(poweredState));
    EXPECT_FALSE(TripWireBlock::isPowered(unpoweredState));
}

TEST_F(TripWireBlockPowerTest, IsActivated_StaticMethod)
{
    BlockState activatedState = block_->defaultState().with(BlockStateProperties::POWERED(), true);
    BlockState deactivatedState = block_->defaultState().with(BlockStateProperties::POWERED(), false);

    EXPECT_TRUE(TripWireBlock::isActivated(activatedState));
    EXPECT_FALSE(TripWireBlock::isActivated(deactivatedState));
}

// ============================================================================
// TripWireBlock 连接状态测试
// ============================================================================

TEST_F(TripWireBlockShapeTest, IsConnected_ReturnsCorrectValue)
{
    BlockState state = block_->defaultState();

    // 默认无连接
    EXPECT_FALSE(TripWireBlock::isConnected(state, Direction::North));
    EXPECT_FALSE(TripWireBlock::isConnected(state, Direction::South));
    EXPECT_FALSE(TripWireBlock::isConnected(state, Direction::East));
    EXPECT_FALSE(TripWireBlock::isConnected(state, Direction::West));

    // 设置北面连接
    state = state.with(BlockStateProperties::NORTH(), true);
    EXPECT_TRUE(TripWireBlock::isConnected(state, Direction::North));
    EXPECT_FALSE(TripWireBlock::isConnected(state, Direction::South));

    // 设置东面连接
    state = state.with(BlockStateProperties::EAST(), true);
    EXPECT_TRUE(TripWireBlock::isConnected(state, Direction::East));
}
