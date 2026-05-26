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
#include "world/block/blocks/redstone/TripwireHookBlock.hpp"

using namespace mc;
using namespace mc::blocks;

// ============================================================================
// TripwireHookBlock 形状测试
// ============================================================================

class TripwireHookBlockShapeTest : public ::testing::Test {
protected:
    void SetUp() override { block_ = std::make_unique<TripWireHookBlock>(BlockProperties(Material::AIR)); }

    std::unique_ptr<TripWireHookBlock> block_;
};

// ========== 形状测试 ==========

TEST_F(TripwireHookBlockShapeTest, NorthFacingShape_HasCollisionBox)
{
    BlockState state = block_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);

    const CollisionShape& shape = block_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
    EXPECT_TRUE(shape.boxCount() >= 1);
}

TEST_F(TripwireHookBlockShapeTest, SouthFacingShape_HasCollisionBox)
{
    BlockState state = block_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South);

    const CollisionShape& shape = block_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
    EXPECT_TRUE(shape.boxCount() >= 1);
}

TEST_F(TripwireHookBlockShapeTest, EastFacingShape_HasCollisionBox)
{
    BlockState state = block_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);

    const CollisionShape& shape = block_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
    EXPECT_TRUE(shape.boxCount() >= 1);
}

TEST_F(TripwireHookBlockShapeTest, WestFacingShape_HasCollisionBox)
{
    BlockState state = block_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::West);

    const CollisionShape& shape = block_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
    EXPECT_TRUE(shape.boxCount() >= 1);
}

TEST_F(TripwireHookBlockShapeTest, DifferentFacings_HaveDifferentShapes)
{
    BlockState northState = block_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    BlockState southState = block_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South);
    BlockState eastState = block_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    BlockState westState = block_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::West);

    const CollisionShape& northShape = block_->getShape(northState);
    const CollisionShape& southShape = block_->getShape(southState);
    const CollisionShape& eastShape = block_->getShape(eastState);
    const CollisionShape& westShape = block_->getShape(westState);

    // 不同朝向应该有不同的形状
    EXPECT_NE(&northShape, &southShape);
    EXPECT_NE(&northShape, &eastShape);
    EXPECT_NE(&northShape, &westShape);
    EXPECT_NE(&southShape, &eastShape);
    EXPECT_NE(&southShape, &westShape);
    EXPECT_NE(&eastShape, &westShape);
}

TEST_F(TripwireHookBlockShapeTest, PoweredDoesNotAffectShape)
{
    BlockState unpowered = block_->defaultState()
                               .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                               .with(BlockStateProperties::POWERED(), false);

    BlockState powered = block_->defaultState()
                             .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                             .with(BlockStateProperties::POWERED(), true);

    const CollisionShape& unpoweredShape = block_->getShape(unpowered);
    const CollisionShape& poweredShape = block_->getShape(powered);

    // POWERED 属性不影响形状
    EXPECT_EQ(&unpoweredShape, &poweredShape);
}

TEST_F(TripwireHookBlockShapeTest, AttachedDoesNotAffectShape)
{
    BlockState detached = block_->defaultState()
                              .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                              .with(BlockStateProperties::ATTACHED(), false);

    BlockState attached = block_->defaultState()
                              .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                              .with(BlockStateProperties::ATTACHED(), true);

    const CollisionShape& detachedShape = block_->getShape(detached);
    const CollisionShape& attachedShape = block_->getShape(attached);

    // ATTACHED 属性不影响形状
    EXPECT_EQ(&detachedShape, &attachedShape);
}

// ============================================================================
// TripwireHookBlock 状态测试
// ============================================================================

class TripwireHookBlockStateTest : public ::testing::Test {
protected:
    void SetUp() override { block_ = std::make_unique<TripWireHookBlock>(BlockProperties(Material::AIR)); }

    std::unique_ptr<TripWireHookBlock> block_;
};

TEST_F(TripwireHookBlockStateTest, DefaultState_NotPoweredNotAttached)
{
    BlockState state = block_->defaultState();

    EXPECT_FALSE(TripWireHookBlock::isPowered(state));
    EXPECT_FALSE(TripWireHookBlock::isConnected(state));
}

TEST_F(TripwireHookBlockStateTest, DefaultState_FacesNorth)
{
    BlockState state = block_->defaultState();
    EXPECT_EQ(TripWireHookBlock::getFacing(state), Direction::North);
}

TEST_F(TripwireHookBlockStateTest, IsPowered_ReturnsCorrectValue)
{
    BlockState state = block_->defaultState();

    state = state.with(BlockStateProperties::POWERED(), true);
    EXPECT_TRUE(TripWireHookBlock::isPowered(state));

    state = state.with(BlockStateProperties::POWERED(), false);
    EXPECT_FALSE(TripWireHookBlock::isPowered(state));
}

TEST_F(TripwireHookBlockStateTest, IsConnected_ReturnsCorrectValue)
{
    BlockState state = block_->defaultState();

    state = state.with(BlockStateProperties::ATTACHED(), true);
    EXPECT_TRUE(TripWireHookBlock::isConnected(state));

    state = state.with(BlockStateProperties::ATTACHED(), false);
    EXPECT_FALSE(TripWireHookBlock::isConnected(state));
}

TEST_F(TripwireHookBlockStateTest, GetFacing_ReturnsCorrectValue)
{
    BlockState state = block_->defaultState();

    state = state.with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South);
    EXPECT_EQ(TripWireHookBlock::getFacing(state), Direction::South);

    state = state.with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    EXPECT_EQ(TripWireHookBlock::getFacing(state), Direction::East);

    state = state.with(BlockStateProperties::HORIZONTAL_FACING(), Direction::West);
    EXPECT_EQ(TripWireHookBlock::getFacing(state), Direction::West);
}

TEST_F(TripwireHookBlockStateTest, WithPowered_ChangesState)
{
    BlockState state = block_->defaultState();

    BlockState poweredState = TripWireHookBlock::withPowered(state, true);
    EXPECT_TRUE(TripWireHookBlock::isPowered(poweredState));

    BlockState unpoweredState = TripWireHookBlock::withPowered(state, false);
    EXPECT_FALSE(TripWireHookBlock::isPowered(unpoweredState));
}

TEST_F(TripwireHookBlockStateTest, WithConnected_ChangesState)
{
    BlockState state = block_->defaultState();

    BlockState connectedState = TripWireHookBlock::withConnected(state, true);
    EXPECT_TRUE(TripWireHookBlock::isConnected(connectedState));

    BlockState disconnectedState = TripWireHookBlock::withConnected(state, false);
    EXPECT_FALSE(TripWireHookBlock::isConnected(disconnectedState));
}

// ============================================================================
// TripwireHookBlock 信号输出测试
// ============================================================================

class TripwireHookBlockPowerTest : public ::testing::Test {
protected:
    void SetUp() override { block_ = std::make_unique<TripWireHookBlock>(BlockProperties(Material::AIR)); }

    std::unique_ptr<TripWireHookBlock> block_;
};

TEST_F(TripwireHookBlockPowerTest, CanProvidePower_ReturnsTrue)
{
    BlockState state = block_->defaultState();
    EXPECT_TRUE(block_->canProvidePower(state));
}

// 注意: getWeakPower 和 getStrongPower 需要 IWorld 参数，需要集成测试来验证
// 这里只验证 canProvidePower 返回 true
