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
#include "world/block/blocks/redstone/AbstractPressurePlateBlock.hpp"

using namespace mc;
using namespace mc::blocks;

// ============================================================================
// 测试用压力板实现类
// ============================================================================

class TestPressurePlateBlock : public AbstractPressurePlateBlock {
public:
    TestPressurePlateBlock(const BlockProperties& properties, i32 tickDelay = 10)
        : AbstractPressurePlateBlock(properties)
        , m_tickDelay(tickDelay)
    {}

    [[nodiscard]] i32 calculateSignalStrength(IWorld& world, const BlockPos& pos) const override
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        // 简单测试实现：返回当前是否检测到实体
        return m_hasEntity ? 15 : 0;
    }

    [[nodiscard]] i32 getTickDelay(bool oldPowered, bool newPowered) const override
    {
        MC_UNUSED(oldPowered);
        MC_UNUSED(newPowered);
        return m_tickDelay;
    }

    void playClickSound(IWorld& world, const BlockPos& pos, bool pressed) const override
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(pressed);
    }

    void setHasEntity(bool hasEntity) { m_hasEntity = hasEntity; }

private:
    i32 m_tickDelay = 10;
    bool m_hasEntity = false;
};

// ============================================================================
// AbstractPressurePlateBlock 形状测试
// ============================================================================

class AbstractPressurePlateBlockShapeTest : public ::testing::Test {
protected:
    void SetUp() override { block_ = std::make_unique<TestPressurePlateBlock>(BlockProperties(Material::WOOD)); }

    std::unique_ptr<TestPressurePlateBlock> block_;
};

// ========== 形状测试 ==========

TEST_F(AbstractPressurePlateBlockShapeTest, UnpressedShape_HasCollisionBox)
{
    // 未按下状态（powered=false）
    BlockState state = block_->defaultState().with(BlockStateProperties::POWERED(), false);

    const CollisionShape& shape = block_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
    EXPECT_TRUE(shape.boxCount() >= 1);
}

TEST_F(AbstractPressurePlateBlockShapeTest, PressedShape_HasCollisionBox)
{
    // 按下状态（powered=true）
    BlockState state = block_->defaultState().with(BlockStateProperties::POWERED(), true);

    const CollisionShape& shape = block_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
    EXPECT_TRUE(shape.boxCount() >= 1);
}

TEST_F(AbstractPressurePlateBlockShapeTest, PressedShape_LowerThanUnpressed)
{
    // 按下状态比未按下状态更低
    BlockState unpressedState = block_->defaultState().with(BlockStateProperties::POWERED(), false);
    BlockState pressedState = block_->defaultState().with(BlockStateProperties::POWERED(), true);

    const CollisionShape& unpressedShape = block_->getShape(unpressedState);
    const CollisionShape& pressedShape = block_->getShape(pressedState);

    // 两者都应该有形状
    EXPECT_FALSE(unpressedShape.isEmpty());
    EXPECT_FALSE(pressedShape.isEmpty());

    // 形状应该是不同的
    EXPECT_NE(&unpressedShape, &pressedShape);
}

// ============================================================================
// AbstractPressurePlateBlock 信号测试
// ============================================================================

class AbstractPressurePlateBlockPowerTest : public ::testing::Test {
protected:
    void SetUp() override { block_ = std::make_unique<TestPressurePlateBlock>(BlockProperties(Material::WOOD)); }

    std::unique_ptr<TestPressurePlateBlock> block_;
};

TEST_F(AbstractPressurePlateBlockPowerTest, IsPowered_ReturnsCorrectValue)
{
    BlockState state = block_->defaultState();
    EXPECT_FALSE(AbstractPressurePlateBlock::isPowered(state));

    state = state.with(BlockStateProperties::POWERED(), true);
    EXPECT_TRUE(AbstractPressurePlateBlock::isPowered(state));

    state = state.with(BlockStateProperties::POWERED(), false);
    EXPECT_FALSE(AbstractPressurePlateBlock::isPowered(state));
}

TEST_F(AbstractPressurePlateBlockPowerTest, WithPowered_ReturnsCorrectState)
{
    BlockState state = block_->defaultState();

    BlockState stateOn = AbstractPressurePlateBlock::withPowered(state, true);
    EXPECT_TRUE(AbstractPressurePlateBlock::isPowered(stateOn));

    BlockState stateOff = AbstractPressurePlateBlock::withPowered(state, false);
    EXPECT_FALSE(AbstractPressurePlateBlock::isPowered(stateOff));
}

TEST_F(AbstractPressurePlateBlockPowerTest, CanProvidePower_ReturnsTrue)
{
    BlockState state = block_->defaultState();
    EXPECT_TRUE(block_->canProvidePower(state));
}

TEST_F(AbstractPressurePlateBlockPowerTest, DefaultPowered_IsFalse)
{
    BlockState state = block_->defaultState();
    EXPECT_FALSE(AbstractPressurePlateBlock::isPowered(state));
}

// ============================================================================
// AbstractPressurePlateBlock 状态属性测试
// ============================================================================

class AbstractPressurePlateBlockPropertyTest : public ::testing::Test {
protected:
    void SetUp() override { block_ = std::make_unique<TestPressurePlateBlock>(BlockProperties(Material::WOOD)); }

    std::unique_ptr<TestPressurePlateBlock> block_;
};

TEST_F(AbstractPressurePlateBlockPropertyTest, HasPoweredProperty)
{
    BlockState state = block_->defaultState();
    EXPECT_TRUE(state.hasProperty(BlockStateProperties::POWERED()));
}

TEST_F(AbstractPressurePlateBlockPropertyTest, PoweredValues_RoundTrip)
{
    BlockState state = block_->defaultState();

    state = state.with(BlockStateProperties::POWERED(), true);
    EXPECT_EQ(state.get(BlockStateProperties::POWERED()), true);

    state = state.with(BlockStateProperties::POWERED(), false);
    EXPECT_EQ(state.get(BlockStateProperties::POWERED()), false);
}
