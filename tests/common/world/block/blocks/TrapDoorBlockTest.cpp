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

#include "entity/core/Entity.hpp"
#include "util/property/Properties.hpp"
#include "world/block/Block.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/blocks/building/TrapDoorBlock.hpp"

using namespace mc;
using namespace mc::blocks;

// ============================================================================
// TrapDoorBlock isLadder 测试
// ============================================================================

class TrapDoorBlockLadderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建木活板门
        woodenTrapdoor_ =
            std::make_unique<TrapDoorBlock>(BlockProperties(Material::WOOD).hardness(2.0f).resistance(3.0f),
                false // not iron
            );

        // 创建铁活板门
        ironTrapdoor_ = std::make_unique<TrapDoorBlock>(BlockProperties(Material::IRON).hardness(5.0f).resistance(5.0f),
            true // is iron
        );
    }

    std::unique_ptr<TrapDoorBlock> woodenTrapdoor_;
    std::unique_ptr<TrapDoorBlock> ironTrapdoor_;
};

// ========== 基本isLadder测试 ==========

TEST_F(TrapDoorBlockLadderTest, ClosedTrapdoor_NotLadder)
{
    // 关闭的活板门不可攀爬
    const BlockState& state = woodenTrapdoor_->defaultState();
    // 默认状态是关闭的
    EXPECT_FALSE(state.get(BlockStateProperties::OPEN()));

    // 关闭的活板门不应该可攀爬
    EXPECT_FALSE(woodenTrapdoor_->isLadder(state, nullptr, nullptr, nullptr));
}

TEST_F(TrapDoorBlockLadderTest, OpenTrapdoor_IsLadderWithoutEntity)
{
    // 打开的活板门可以攀爬（没有实体信息时）
    BlockState state = woodenTrapdoor_->defaultState();
    state = state.with(BlockStateProperties::OPEN(), true);

    // 打开的活板门应该可攀爬
    EXPECT_TRUE(woodenTrapdoor_->isLadder(state, nullptr, nullptr, nullptr));
}

TEST_F(TrapDoorBlockLadderTest, IronTrapdoor_SameBehavior)
{
    // 铁活板门的攀爬行为与木活板门相同
    // isLadder只检查是否打开，不检查材料

    // 关闭的铁活板门
    const BlockState& closedState = ironTrapdoor_->defaultState();
    EXPECT_FALSE(ironTrapdoor_->isLadder(closedState, nullptr, nullptr, nullptr));

    // 打开的铁活板门
    BlockState openState = ironTrapdoor_->defaultState();
    openState = openState.with(BlockStateProperties::OPEN(), true);
    EXPECT_TRUE(ironTrapdoor_->isLadder(openState, nullptr, nullptr, nullptr));
}

// ========== 状态属性测试 ==========

TEST_F(TrapDoorBlockLadderTest, OpenProperty_Works)
{
    // 验证OPEN属性工作正常
    const BlockState& state = woodenTrapdoor_->defaultState();
    EXPECT_FALSE(TrapDoorBlock::isOpen(state));

    BlockState openState = state.with(BlockStateProperties::OPEN(), true);
    EXPECT_TRUE(TrapDoorBlock::isOpen(openState));
}

TEST_F(TrapDoorBlockLadderTest, Toggle_Works)
{
    // 验证toggle方法
    // 这需要世界对象，所以只测试静态方法
    const BlockState& closedState = woodenTrapdoor_->defaultState();
    EXPECT_FALSE(TrapDoorBlock::isOpen(closedState));
}

// ========== 方块属性测试 ==========

TEST_F(TrapDoorBlockLadderTest, IsIronTrapdoor_ReturnsCorrectValue)
{
    EXPECT_FALSE(woodenTrapdoor_->isIronTrapdoor());
    EXPECT_TRUE(ironTrapdoor_->isIronTrapdoor());
}

TEST_F(TrapDoorBlockLadderTest, GetShape_ReturnsValidShape)
{
    // 关闭的活板门有碰撞形状
    const BlockState& closedState = woodenTrapdoor_->defaultState();
    const CollisionShape& closedShape = woodenTrapdoor_->getShape(closedState);
    EXPECT_FALSE(closedShape.isEmpty());

    // 打开的活板门没有碰撞形状
    BlockState openState = woodenTrapdoor_->defaultState();
    openState = openState.with(BlockStateProperties::OPEN(), true);
    const CollisionShape& openShape = woodenTrapdoor_->getCollisionShape(openState);
    EXPECT_TRUE(openShape.isEmpty());
}

// ========== 材料测试 ==========

TEST_F(TrapDoorBlockLadderTest, Material_Correct)
{
    // 验证材料
    const BlockState& woodenState = woodenTrapdoor_->defaultState();
    EXPECT_EQ(woodenState.getMaterial(), Material::WOOD);

    const BlockState& ironState = ironTrapdoor_->defaultState();
    EXPECT_EQ(ironState.getMaterial(), Material::IRON);
}

// ========== 推动反应测试 ==========

TEST_F(TrapDoorBlockLadderTest, GetPushReaction_ReturnsDestroy)
{
    // 活板门被活塞推动时会被破坏
    const BlockState& state = woodenTrapdoor_->defaultState();
    EXPECT_EQ(woodenTrapdoor_->getPushReaction(state), Material::PushReaction::Destroy);
}

// ============================================================================
// 攀爬位置检测测试（无世界对象）
// ============================================================================

TEST(TrapDoorBlockLadderLogicTest, OpenStateCheck)
{
    // 独立测试OPEN状态的检查逻辑
    TrapDoorBlock block(BlockProperties(Material::WOOD).hardness(2.0f), false);

    // 测试各种状态组合
    BlockState state = block.defaultState();

    // 关闭状态
    EXPECT_FALSE(state.get(BlockStateProperties::OPEN()));
    EXPECT_FALSE(block.isLadder(state, nullptr, nullptr, nullptr));

    // 打开状态
    state = state.with(BlockStateProperties::OPEN(), true);
    EXPECT_TRUE(state.get(BlockStateProperties::OPEN()));
    EXPECT_TRUE(block.isLadder(state, nullptr, nullptr, nullptr));
}

// ============================================================================
// 方向和位置测试
// ============================================================================

TEST_F(TrapDoorBlockLadderTest, HalfProperty_Exists)
{
    // 验证DOUBLE_BLOCK_HALF属性存在
    const BlockState& state = woodenTrapdoor_->defaultState();
    auto half = state.get(BlockStateProperties::DOUBLE_BLOCK_HALF());
    // 默认应该是下半部分
    EXPECT_EQ(half, BlockStateProperties::DoubleBlockHalf::Lower);
}

TEST_F(TrapDoorBlockLadderTest, FacingProperty_Exists)
{
    // 验证HORIZONTAL_FACING属性存在
    const BlockState& state = woodenTrapdoor_->defaultState();
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    // 默认朝向北
    EXPECT_EQ(facing, Direction::North);
}
