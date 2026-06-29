/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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
#include "common/world/block/blocks/sculk/SculkBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/gameevent/VibrationSystem.hpp"

using namespace mc;
using namespace mc::blocks;
using namespace mc::gameevent;

// ============================================================================
// SculkSensorBlock 状态属性测试
// ============================================================================

class SculkSensorBlockTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    BlockProperties sculkProperties() const { return BlockProperties(Material::SCULK).noCollision().notSolid(); }
};

TEST_F(SculkSensorBlockTest, DefaultState_IsInactiveWithZeroPower)
{
    SculkSensorBlock block(sculkProperties());
    const BlockState& state = block.defaultState();

    EXPECT_EQ(state.get(BlockStateProperties::SCULK_SENSOR_PHASE()), BlockStateProperties::SculkSensorPhase::Inactive);
    EXPECT_EQ(state.get(BlockStateProperties::POWER_0_15()), 0);
    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(SculkSensorBlockTest, CanActivate_InactivePhase_ReturnsTrue)
{
    SculkSensorBlock block(sculkProperties());
    const BlockState& state = block.defaultState();

    // 默认状态是 Inactive，应该可以被激活
    EXPECT_TRUE(SculkSensorBlock::canActivate(state));
}

TEST_F(SculkSensorBlockTest, CanActivate_ActivePhase_ReturnsFalse)
{
    SculkSensorBlock block(sculkProperties());
    const BlockState& state = block.defaultState().with(
        BlockStateProperties::SCULK_SENSOR_PHASE(), BlockStateProperties::SculkSensorPhase::Active);

    EXPECT_FALSE(SculkSensorBlock::canActivate(state));
}

TEST_F(SculkSensorBlockTest, CanActivate_CooldownPhase_ReturnsFalse)
{
    SculkSensorBlock block(sculkProperties());
    const BlockState& state = block.defaultState().with(
        BlockStateProperties::SCULK_SENSOR_PHASE(), BlockStateProperties::SculkSensorPhase::Cooldown);

    EXPECT_FALSE(SculkSensorBlock::canActivate(state));
}

TEST_F(SculkSensorBlockTest, GetPhase_ReturnsCorrectPhase)
{
    SculkSensorBlock block(sculkProperties());

    EXPECT_EQ(SculkSensorBlock::getPhase(block.defaultState()), BlockStateProperties::SculkSensorPhase::Inactive);

    const BlockState& activeState = block.defaultState().with(
        BlockStateProperties::SCULK_SENSOR_PHASE(), BlockStateProperties::SculkSensorPhase::Active);
    EXPECT_EQ(SculkSensorBlock::getPhase(activeState), BlockStateProperties::SculkSensorPhase::Active);

    const BlockState& cooldownState = block.defaultState().with(
        BlockStateProperties::SCULK_SENSOR_PHASE(), BlockStateProperties::SculkSensorPhase::Cooldown);
    EXPECT_EQ(SculkSensorBlock::getPhase(cooldownState), BlockStateProperties::SculkSensorPhase::Cooldown);
}

TEST_F(SculkSensorBlockTest, GetWeakPower_ReturnsPowerPropertyValue)
{
    SculkSensorBlock block(sculkProperties());

    // POWER = 0 时，弱信号为 0
    const BlockState& state0 = block.defaultState();
    EXPECT_EQ(block.getWeakPower(state0, *static_cast<IWorld*>(nullptr), BlockPos(0, 0, 0), Direction::Up), 0);

    // POWER = 8 时，弱信号为 8
    const BlockState& state8 = block.defaultState().with(BlockStateProperties::POWER_0_15(), 8);
    EXPECT_EQ(block.getWeakPower(state8, *static_cast<IWorld*>(nullptr), BlockPos(0, 0, 0), Direction::Up), 8);

    // POWER = 15 时，弱信号为 15
    const BlockState& state15 = block.defaultState().with(BlockStateProperties::POWER_0_15(), 15);
    EXPECT_EQ(block.getWeakPower(state15, *static_cast<IWorld*>(nullptr), BlockPos(0, 0, 0), Direction::Up), 15);
}

TEST_F(SculkSensorBlockTest, CanProvidePower_ReturnsTrue)
{
    SculkSensorBlock block(sculkProperties());
    EXPECT_TRUE(block.canProvidePower(block.defaultState()));
}

TEST_F(SculkSensorBlockTest, ActiveTicks_Is30)
{
    SculkSensorBlock block(sculkProperties());
    EXPECT_EQ(SculkSensorBlock::ACTIVE_TICKS, 30);
    EXPECT_EQ(SculkSensorBlock::COOLDOWN_TICKS, 10);
}

TEST_F(SculkSensorBlockTest, GetActiveTicks_ReturnsCorrectValue)
{
    SculkSensorBlock block(sculkProperties());
    EXPECT_EQ(block.getActiveTicks(), 30);
}

// ============================================================================
// CalibratedSculkSensorBlock 测试
// ============================================================================

class CalibratedSculkSensorBlockTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    BlockProperties sculkProperties() const { return BlockProperties(Material::SCULK).noCollision().notSolid(); }
};

TEST_F(CalibratedSculkSensorBlockTest, DefaultState_HasFacing)
{
    CalibratedSculkSensorBlock block(sculkProperties());
    const BlockState& state = block.defaultState();

    EXPECT_EQ(state.get(BlockStateProperties::SCULK_SENSOR_PHASE()), BlockStateProperties::SculkSensorPhase::Inactive);
    EXPECT_EQ(state.get(BlockStateProperties::POWER_0_15()), 0);
    EXPECT_EQ(state.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
}

TEST_F(CalibratedSculkSensorBlockTest, GetActiveTicks_Returns10)
{
    CalibratedSculkSensorBlock block(sculkProperties());
    EXPECT_EQ(block.getActiveTicks(), 10);
}

TEST_F(CalibratedSculkSensorBlockTest, GetWeakPower_FacingDirectionReturnsZero)
{
    CalibratedSculkSensorBlock block(sculkProperties());
    const BlockState& state = block.defaultState()
                                  .with(BlockStateProperties::POWER_0_15(), 10)
                                  .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);

    // FACING 方向（输入面）应该返回 0
    EXPECT_EQ(block.getWeakPower(state, *static_cast<IWorld*>(nullptr), BlockPos(0, 0, 0), Direction::North), 0);
}

TEST_F(CalibratedSculkSensorBlockTest, GetWeakPower_NonFacingDirectionReturnsPower)
{
    CalibratedSculkSensorBlock block(sculkProperties());
    const BlockState& state = block.defaultState()
                                  .with(BlockStateProperties::POWER_0_15(), 10)
                                  .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);

    // 非 FACING 方向应该返回 POWER 值
    EXPECT_EQ(block.getWeakPower(state, *static_cast<IWorld*>(nullptr), BlockPos(0, 0, 0), Direction::South), 10);
    EXPECT_EQ(block.getWeakPower(state, *static_cast<IWorld*>(nullptr), BlockPos(0, 0, 0), Direction::East), 10);
    EXPECT_EQ(block.getWeakPower(state, *static_cast<IWorld*>(nullptr), BlockPos(0, 0, 0), Direction::West), 10);
    EXPECT_EQ(block.getWeakPower(state, *static_cast<IWorld*>(nullptr), BlockPos(0, 0, 0), Direction::Up), 10);
    EXPECT_EQ(block.getWeakPower(state, *static_cast<IWorld*>(nullptr), BlockPos(0, 0, 0), Direction::Down), 10);
}

// ============================================================================
// VibrationSystem::getRedstoneStrengthForDistance 测试
// ============================================================================

TEST(VibrationSystemRedstoneStrengthTest, DistanceZero_Returns15)
{
    // 距离为 0 时，信号最强为 15
    EXPECT_EQ(VibrationSystem::getRedstoneStrengthForDistance(0.0f, 8), 15);
}

TEST(VibrationSystemRedstoneStrengthTest, MaxDistance_Returns1)
{
    // 距离等于检测半径时，信号最弱为 1
    EXPECT_EQ(VibrationSystem::getRedstoneStrengthForDistance(8.0f, 8), 1);
}

TEST(VibrationSystemRedstoneStrengthTest, MidDistance_ReturnsIntermediate)
{
    // 距离 4（半径 8），计算：15 - floor(15/8 * 4) = 15 - floor(7.5) = 15 - 7 = 8
    EXPECT_EQ(VibrationSystem::getRedstoneStrengthForDistance(4.0f, 8), 8);
}

TEST(VibrationSystemRedstoneStrengthTest, CalibratedSensor_DoubleRadius)
{
    // 校准感测体半径 16，距离 8 时：15 - floor(15/16 * 8) = 15 - floor(7.5) = 15 - 7 = 8
    EXPECT_EQ(VibrationSystem::getRedstoneStrengthForDistance(8.0f, 16), 8);
}

// ============================================================================
// VibrationSystem::getResonanceEventByFrequency 测试
// ============================================================================

TEST(VibrationSystemResonanceTest, Frequency1_ReturnsResonate1)
{
    const GameEvent* event = VibrationSystem::getResonanceEventByFrequency(1);
    ASSERT_NE(event, nullptr);
    EXPECT_EQ(event->id(), "resonate_1");
}

TEST(VibrationSystemResonanceTest, Frequency15_ReturnsResonate15)
{
    const GameEvent* event = VibrationSystem::getResonanceEventByFrequency(15);
    ASSERT_NE(event, nullptr);
    EXPECT_EQ(event->id(), "resonate_15");
}

TEST(VibrationSystemResonanceTest, Frequency0_ReturnsNullptr)
{
    const GameEvent* event = VibrationSystem::getResonanceEventByFrequency(0);
    EXPECT_EQ(event, nullptr);
}

TEST(VibrationSystemResonanceTest, FrequencyOutOfRange_ReturnsNullptr)
{
    const GameEvent* event = VibrationSystem::getResonanceEventByFrequency(16);
    EXPECT_EQ(event, nullptr);
}

// ============================================================================
// VibrationSystem::getGameEventFrequency 测试（关键事件频率）
// ============================================================================

TEST(VibrationSystemFrequencyTest, Step_Frequency1)
{
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::STEP), 1);
}

TEST(VibrationSystemFrequencyTest, BlockActivate_Frequency5)
{
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::BLOCK_ACTIVATE), 5);
}

TEST(VibrationSystemFrequencyTest, EntityDamage_Frequency11)
{
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::ENTITY_DAMAGE), 11);
}

TEST(VibrationSystemFrequencyTest, EntityDie_Frequency11)
{
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::ENTITY_DIE), 11);
}

TEST(VibrationSystemFrequencyTest, Shriek_Frequency15)
{
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::SHRIEK), 15);
}

TEST(VibrationSystemFrequencyTest, SculkSensorTendrilsClicking_Frequency9)
{
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::SCULK_SENSOR_TENDRILS_CLICKING), 9);
}

// ============================================================================
// SculkShriekerBlock 状态属性测试
// ============================================================================

class SculkShriekerBlockTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    BlockProperties sculkProperties() const { return BlockProperties(Material::SCULK).noCollision().notSolid(); }
};

TEST_F(SculkShriekerBlockTest, DefaultState_NotShriekingNotSummonNotWaterlogged)
{
    SculkShriekerBlock block(sculkProperties());
    const BlockState& state = block.defaultState();

    EXPECT_FALSE(state.get(BlockStateProperties::SHRIEKING()));
    EXPECT_FALSE(state.get(BlockStateProperties::CAN_SUMMON()));
    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(SculkShriekerBlockTest, ShriekingTicksConstant_Is90)
{
    // SHRIEKING_TICKS = 90 (4.5秒)
    EXPECT_EQ(SculkShriekerBlock::SHRIEKING_TICKS, 90);
}

TEST_F(SculkShriekerBlockTest, HasBlockEntity)
{
    SculkShriekerBlock block(sculkProperties());
    EXPECT_TRUE(block.hasBlockEntity());
}

TEST_F(SculkShriekerBlockTest, BlockEntityType_IsSculkShrieker)
{
    SculkShriekerBlock block(sculkProperties());
    EXPECT_EQ(block.getBlockEntityType(), BlockEntityType::SculkShrieker);
}

TEST_F(SculkShriekerBlockTest, ShriekingState_CanBeSet)
{
    SculkShriekerBlock block(sculkProperties());
    const BlockState& defaultState = block.defaultState();
    EXPECT_FALSE(defaultState.get(BlockStateProperties::SHRIEKING()));

    const BlockState& shriekingState = defaultState.with(BlockStateProperties::SHRIEKING(), true);
    EXPECT_TRUE(shriekingState.get(BlockStateProperties::SHRIEKING()));

    // 其他属性应保持不变
    EXPECT_FALSE(shriekingState.get(BlockStateProperties::CAN_SUMMON()));
    EXPECT_FALSE(shriekingState.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(SculkShriekerBlockTest, CanSummonState_CanBeSet)
{
    SculkShriekerBlock block(sculkProperties());
    const BlockState& defaultState = block.defaultState();
    EXPECT_FALSE(defaultState.get(BlockStateProperties::CAN_SUMMON()));

    // 自然生成的尖啸体 CAN_SUMMON=true
    const BlockState& naturalState = defaultState.with(BlockStateProperties::CAN_SUMMON(), true);
    EXPECT_TRUE(naturalState.get(BlockStateProperties::CAN_SUMMON()));
}

TEST_F(SculkShriekerBlockTest, UseShapeForLightOcclusion)
{
    SculkShriekerBlock block(sculkProperties());
    EXPECT_TRUE(block.useShapeForLightOcclusion(block.defaultState()));
}

TEST_F(SculkShriekerBlockTest, IsWaterloggable)
{
    SculkShriekerBlock block(sculkProperties());
    const BlockState& state = block.defaultState();
    EXPECT_FALSE(block.isWaterlogged(state));

    const BlockState& waterloggedState = state.with(BlockStateProperties::WATERLOGGED(), true);
    EXPECT_TRUE(block.isWaterlogged(waterloggedState));
}
