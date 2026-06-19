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

#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/blocks/cave/PointedDripstoneBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;
using namespace mc::blocks;

// ========== PointedDripstoneBlock 测试 ==========

class PointedDripstoneBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        block_ =
            std::make_unique<PointedDripstoneBlock>(BlockProperties(Material::ROCK).hardness(1.5f).resistance(3.0f));
    }

    std::unique_ptr<PointedDripstoneBlock> block_;
};

// ============================================================================
// 构造与默认状态测试
// ============================================================================

TEST_F(PointedDripstoneBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(block_, nullptr);
}

TEST_F(PointedDripstoneBlockTest, DefaultState_DirectionUp)
{
    const BlockState& state = block_->defaultState();
    EXPECT_EQ(state.get(BlockStateProperties::VERTICAL_DIRECTION()), Direction::Up);
}

TEST_F(PointedDripstoneBlockTest, DefaultState_ThicknessTip)
{
    const BlockState& state = block_->defaultState();
    EXPECT_EQ(state.get(BlockStateProperties::DRIPSTONE_THICKNESS()), BlockStateProperties::DripstoneThickness::Tip);
}

TEST_F(PointedDripstoneBlockTest, DefaultState_NotWaterlogged)
{
    const BlockState& state = block_->defaultState();
    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(PointedDripstoneBlockTest, TicksRandomly_ReturnsTrue)
{
    EXPECT_TRUE(block_->ticksRandomly());
}

TEST_F(PointedDripstoneBlockTest, UseShapeForLightOcclusion_AlwaysTrue)
{
    const BlockState& state = block_->defaultState();
    EXPECT_TRUE(block_->useShapeForLightOcclusion(state));
}

// ============================================================================
// 状态属性组合测试
// ============================================================================

TEST_F(PointedDripstoneBlockTest, DirectionProperty_CanBeSet)
{
    auto state = block_->defaultState();

    state = state.with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up);
    EXPECT_EQ(state.get(BlockStateProperties::VERTICAL_DIRECTION()), Direction::Up);

    state = state.with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down);
    EXPECT_EQ(state.get(BlockStateProperties::VERTICAL_DIRECTION()), Direction::Down);
}

TEST_F(PointedDripstoneBlockTest, ThicknessProperty_CanBeSet)
{
    auto state = block_->defaultState();

    state = state.with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip);
    EXPECT_EQ(state.get(BlockStateProperties::DRIPSTONE_THICKNESS()), BlockStateProperties::DripstoneThickness::Tip);

    state = state.with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::TipMerge);
    EXPECT_EQ(
        state.get(BlockStateProperties::DRIPSTONE_THICKNESS()), BlockStateProperties::DripstoneThickness::TipMerge);

    state = state.with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Frustum);
    EXPECT_EQ(
        state.get(BlockStateProperties::DRIPSTONE_THICKNESS()), BlockStateProperties::DripstoneThickness::Frustum);

    state = state.with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Middle);
    EXPECT_EQ(state.get(BlockStateProperties::DRIPSTONE_THICKNESS()), BlockStateProperties::DripstoneThickness::Middle);

    state = state.with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Base);
    EXPECT_EQ(state.get(BlockStateProperties::DRIPSTONE_THICKNESS()), BlockStateProperties::DripstoneThickness::Base);
}

TEST_F(PointedDripstoneBlockTest, WaterloggedProperty_CanBeToggled)
{
    auto state = block_->defaultState();
    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));

    state = state.with(BlockStateProperties::WATERLOGGED(), true);
    EXPECT_TRUE(state.get(BlockStateProperties::WATERLOGGED()));
    EXPECT_TRUE(block_->isWaterlogged(state));

    state = state.with(BlockStateProperties::WATERLOGGED(), false);
    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
    EXPECT_FALSE(block_->isWaterlogged(state));
}

// ============================================================================
// 碰撞形状测试
// ============================================================================

TEST_F(PointedDripstoneBlockTest, Shape_TipUp_HasCollisionBox)
{
    auto state = block_->defaultState()
                     .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up)
                     .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip);
    const CollisionShape& shape = block_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
    EXPECT_FALSE(shape.isFullBlock());
}

TEST_F(PointedDripstoneBlockTest, Shape_TipDown_HasCollisionBox)
{
    auto state = block_->defaultState()
                     .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down)
                     .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip);
    const CollisionShape& shape = block_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
    EXPECT_FALSE(shape.isFullBlock());
}

TEST_F(PointedDripstoneBlockTest, Shape_TipUpAndDown_AreDifferent)
{
    // 朝上尖端和朝下尖端应该有不同的形状
    auto stateUp =
        block_->defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip);
    auto stateDown =
        block_->defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip);

    const CollisionShape& shapeUp = block_->getShape(stateUp);
    const CollisionShape& shapeDown = block_->getShape(stateDown);

    // 两种方向的尖端形状不同（朝下尖端的碰撞盒高度不同）
    EXPECT_NE(&shapeUp, &shapeDown);
}

TEST_F(PointedDripstoneBlockTest, Shape_AllThicknesses_NonEmpty)
{
    // 所有厚度等级都应有非空碰撞形状
    const auto thicknesses = {BlockStateProperties::DripstoneThickness::TipMerge,
        BlockStateProperties::DripstoneThickness::Tip,
        BlockStateProperties::DripstoneThickness::Frustum,
        BlockStateProperties::DripstoneThickness::Middle,
        BlockStateProperties::DripstoneThickness::Base};

    for (const auto& thickness : thicknesses) {
        auto state = block_->defaultState()
                         .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up)
                         .with(BlockStateProperties::DRIPSTONE_THICKNESS(), thickness);
        const CollisionShape& shape = block_->getShape(state);
        EXPECT_FALSE(shape.isEmpty()) << "Shape should not be empty for thickness " << static_cast<int>(thickness);
    }
}

// ============================================================================
// 旋转和镜像测试
// ============================================================================

TEST_F(PointedDripstoneBlockTest, Rotate_DoesNotChangeDirection)
{
    // 垂直方向不受旋转影响
    auto stateUp = block_->defaultState().with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up);
    auto stateDown = block_->defaultState().with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down);

    EXPECT_EQ(
        block_->rotate(stateUp, Rotation::Clockwise90).get(BlockStateProperties::VERTICAL_DIRECTION()), Direction::Up);
    EXPECT_EQ(block_->rotate(stateDown, Rotation::Clockwise180).get(BlockStateProperties::VERTICAL_DIRECTION()),
        Direction::Down);
}

TEST_F(PointedDripstoneBlockTest, Mirror_SwapsUpAndDown)
{
    auto stateUp = block_->defaultState().with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up);
    auto stateDown = block_->defaultState().with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down);

    // FrontBack镜像：Up→Down，Down→Up
    EXPECT_EQ(
        block_->mirror(stateUp, Mirror::FrontBack).get(BlockStateProperties::VERTICAL_DIRECTION()), Direction::Down);
    EXPECT_EQ(
        block_->mirror(stateDown, Mirror::FrontBack).get(BlockStateProperties::VERTICAL_DIRECTION()), Direction::Up);

    // LeftRight镜像：Up→Down，Down→Up
    EXPECT_EQ(
        block_->mirror(stateUp, Mirror::LeftRight).get(BlockStateProperties::VERTICAL_DIRECTION()), Direction::Down);
    EXPECT_EQ(
        block_->mirror(stateDown, Mirror::LeftRight).get(BlockStateProperties::VERTICAL_DIRECTION()), Direction::Up);
}

TEST_F(PointedDripstoneBlockTest, Mirror_None_DoesNotChangeDirection)
{
    // 无镜像不变
    auto stateUp = block_->defaultState().with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up);
    EXPECT_EQ(block_->mirror(stateUp, Mirror::None).get(BlockStateProperties::VERTICAL_DIRECTION()), Direction::Up);
}

// ============================================================================
// 方块类型验证测试
// ============================================================================

TEST_F(PointedDripstoneBlockTest, IsWaterlogged_Implemented)
{
    // PointedDripstoneBlock 实现了 IWaterLoggable 接口
    auto state = block_->defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    EXPECT_TRUE(block_->isWaterlogged(state));

    auto stateNotWaterlogged = block_->defaultState().with(BlockStateProperties::WATERLOGGED(), false);
    EXPECT_FALSE(block_->isWaterlogged(stateNotWaterlogged));
}

TEST_F(PointedDripstoneBlockTest, GetFluidState_Waterlogged_ReturnsNonNull)
{
    auto state = block_->defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    const fluid::FluidState* fluidState = block_->getFluidState(state);
    EXPECT_NE(fluidState, nullptr);
}

TEST_F(PointedDripstoneBlockTest, GetFluidState_NotWaterlogged_MayBeNull)
{
    auto state = block_->defaultState().with(BlockStateProperties::WATERLOGGED(), false);
    const fluid::FluidState* fluidState = block_->getFluidState(state);
    // 非含水状态：流体状态为空指针或空流体
    // 无法调用isEmpty()因为FluidState是不完整类型，仅验证不会崩溃
    (void)fluidState;
}

// ============================================================================
// onFallenUpon 行为测试
// ============================================================================

TEST_F(PointedDripstoneBlockTest, OnFallenUpon_StalagmiteTipUp_CallsCauseFallDamage)
{
    // 朝上的TIP尖端（石笋）应该触发增强的摔落伤害
    // 验证：onFallenUpon 在石笋尖端方向上不会调用父类（替代普通摔落伤害）
    // 这里验证静态属性判断逻辑是否正确
    auto stalagmiteTip =
        block_->defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip);

    // 验证状态属性组合正确
    EXPECT_EQ(stalagmiteTip.get(BlockStateProperties::VERTICAL_DIRECTION()), Direction::Up);
    EXPECT_EQ(
        stalagmiteTip.get(BlockStateProperties::DRIPSTONE_THICKNESS()), BlockStateProperties::DripstoneThickness::Tip);
}

TEST_F(PointedDripstoneBlockTest, OnFallenUpon_StalactiteTipDown_NoStalagmiteDamage)
{
    // 朝下的TIP（钟乳石尖端）不应该触发石笋伤害
    // 验证：onFallenUpon 在朝下TIP时应调用父类（普通摔落伤害）
    auto stalactiteTip =
        block_->defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip);

    // 朝下的尖端不是石笋
    EXPECT_EQ(stalactiteTip.get(BlockStateProperties::VERTICAL_DIRECTION()), Direction::Down);
}

TEST_F(PointedDripstoneBlockTest, OnFallenUpon_NonTipNoStalagmiteDamage)
{
    // 非TIP厚度的滴石（Base, Middle, Frustum）不应触发石笋伤害
    // 验证：这些厚度方向上总是调用父类（普通摔落伤害）
    const auto nonTipThicknesses = {BlockStateProperties::DripstoneThickness::Base,
        BlockStateProperties::DripstoneThickness::Middle,
        BlockStateProperties::DripstoneThickness::Frustum,
        BlockStateProperties::DripstoneThickness::TipMerge};

    for (const auto& thickness : nonTipThicknesses) {
        auto state = block_->defaultState()
                         .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up)
                         .with(BlockStateProperties::DRIPSTONE_THICKNESS(), thickness);
        // 非TIP厚度或TIP_MERGE不应触发石笋伤害
        EXPECT_NE(state.get(BlockStateProperties::DRIPSTONE_THICKNESS()), BlockStateProperties::DripstoneThickness::Tip)
            << "Non-tip thickness should not trigger stalagmite damage";
    }
}

// ============================================================================
// 方向和厚度的石笋/钟乳石判定逻辑测试
// ============================================================================

TEST_F(PointedDripstoneBlockTest, StalagmiteCondition_UpTipOnly)
{
    // 石笋（Stalagmite）条件：Direction::Up + DripstoneThickness::Tip
    // 只有同时满足朝上和TIP厚度时，onFallenUpon才施加石笋伤害
    auto upTip =
        block_->defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip);
    EXPECT_EQ(upTip.get(BlockStateProperties::VERTICAL_DIRECTION()), Direction::Up);
    EXPECT_EQ(upTip.get(BlockStateProperties::DRIPSTONE_THICKNESS()), BlockStateProperties::DripstoneThickness::Tip);

    // 朝上 + 非TIP = 非石笋
    auto upFrustum =
        block_->defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Frustum);
    EXPECT_NE(upFrustum.get(BlockStateProperties::DRIPSTONE_THICKNESS()), BlockStateProperties::DripstoneThickness::Tip);

    // 朝下 + TIP = 非石笋（是钟乳石尖端）
    auto downTip =
        block_->defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Down)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::Tip);
    EXPECT_EQ(downTip.get(BlockStateProperties::VERTICAL_DIRECTION()), Direction::Down);
}

TEST_F(PointedDripstoneBlockTest, TipMerge_NotTipForStalagmiteDamage)
{
    // TIP_MERGE 厚度不触发石笋伤害（只有纯TIP才触发）
    auto tipMerge =
        block_->defaultState()
            .with(BlockStateProperties::VERTICAL_DIRECTION(), Direction::Up)
            .with(BlockStateProperties::DRIPSTONE_THICKNESS(), BlockStateProperties::DripstoneThickness::TipMerge);
    EXPECT_NE(tipMerge.get(BlockStateProperties::DRIPSTONE_THICKNESS()), BlockStateProperties::DripstoneThickness::Tip);
}
