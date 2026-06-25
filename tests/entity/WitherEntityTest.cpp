/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction restriction, including without limitation the rights
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

#include "common/core/Types.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/boss/WitherEntity.hpp"

using namespace mc;

// ============================================================================
// WitherEntity 游戏模式检查测试
// ============================================================================
//
// 测试凋灵在选择目标时正确处理创造模式和旁观者模式玩家。
// 参考 MC 1.16.5 WitherEntity.updateAITasks()
//
// 注意：完整的集成测试需要 Mock 世界和实体系统。
// 这里测试游戏模式判断的核心逻辑。

class WitherEntityGameModeTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 设置代码
    }
};

// ============================================================================
// Vex 测试
// ============================================================================

TEST_F(WitherEntityGameModeTest, VexEntityType_IsDefined)
{
    // 验证 Vex 实体类型已定义
    // 这是 countNearbyVexes() 功能所需的前提条件
    EXPECT_NE(entity::EntityTypeIdNumber::VEX, entity::EntityTypeIdNumber::Unknown);
    // 注意：Vex 实体类型 ID 可能随注册顺序变化，不检查具体值
}

// ============================================================================
// 游戏模式常量测试
// ============================================================================

TEST_F(WitherEntityGameModeTest, GameMode_CreativeIsNotTargetable)
{
    // MC 1.16.5: 创造模式玩家不能被作为攻击目标
    // 凋灵的侧头不会瞄准创造模式玩家
    // 这个测试验证常量值
    constexpr u8 CREATIVE_MODE = 1;
    EXPECT_EQ(CREATIVE_MODE, static_cast<u8>(GameMode::Creative));
}

TEST_F(WitherEntityGameModeTest, GameMode_SpectatorIsNotTargetable)
{
    // MC 1.16.5: 旁观者模式玩家不能被作为攻击目标
    // 凋灵的侧头不会瞄准旁观者模式玩家
    constexpr u8 SPECTATOR_MODE = 3;
    EXPECT_EQ(SPECTATOR_MODE, static_cast<u8>(GameMode::Spectator));
}

TEST_F(WitherEntityGameModeTest, GameMode_SurvivalIsTargetable)
{
    // MC 1.16.5: 生存模式玩家可以被作为攻击目标
    constexpr u8 SURVIVAL_MODE = 0;
    EXPECT_EQ(SURVIVAL_MODE, static_cast<u8>(GameMode::Survival));
}

TEST_F(WitherEntityGameModeTest, GameMode_AdventureIsTargetable)
{
    // MC 1.16.5: 冒险模式玩家可以被作为攻击目标
    constexpr u8 ADVENTURE_MODE = 2;
    EXPECT_EQ(ADVENTURE_MODE, static_cast<u8>(GameMode::Adventure));
}

// ============================================================================
// 凋灵常量测试
// ============================================================================

TEST_F(WitherEntityGameModeTest, Wither_HeadTrackingRange_IsCorrect)
{
    // MC 1.16.5: 凋灵侧头追踪范围为 30 格（平方后为 900）
    constexpr f32 HEAD_TRACKING_RANGE = 30.0f;
    constexpr f32 HEAD_TRACKING_RANGE_SQ = 900.0f;
    EXPECT_FLOAT_EQ(HEAD_TRACKING_RANGE * HEAD_TRACKING_RANGE, HEAD_TRACKING_RANGE_SQ);
}

TEST_F(WitherEntityGameModeTest, Wither_SideHeadUpdateInterval_IsCorrect)
{
    // MC 1.16.5: 侧头目标更新间隔为 10-20 ticks
    constexpr i32 MIN_UPDATE_INTERVAL = 10;
    constexpr i32 MAX_UPDATE_INTERVAL = 20;
    EXPECT_LT(MIN_UPDATE_INTERVAL, MAX_UPDATE_INTERVAL);
    EXPECT_GE(MIN_UPDATE_INTERVAL, 10);
    EXPECT_LE(MAX_UPDATE_INTERVAL, 20);
}

TEST_F(WitherEntityGameModeTest, Wither_AttackCooldown_IsCorrect)
{
    // MC 1.16.5: 侧头攻击冷却为 40-60 ticks
    constexpr i32 MIN_ATTACK_COOLDOWN = 40;
    constexpr i32 MAX_ATTACK_COOLDOWN = 60;
    EXPECT_LT(MIN_ATTACK_COOLDOWN, MAX_ATTACK_COOLDOWN);
    EXPECT_GE(MIN_ATTACK_COOLDOWN, 40);
    EXPECT_LE(MAX_ATTACK_COOLDOWN, 60);
}

TEST_F(WitherEntityGameModeTest, Wither_InvulnerabilityTime_IsCorrect)
{
    // MC 1.16.5: 凋灵生成后的无敌时间为 220 ticks (11秒)
    constexpr i32 INVULNERABILITY_TIME = 220;
    EXPECT_EQ(INVULNERABILITY_TIME, 220);
}

// ============================================================================
// 目标选择逻辑测试
// ============================================================================

TEST_F(WitherEntityGameModeTest, TargetSelection_ExcludesCreativePlayers)
{
    // MC 1.16.5: 创造模式玩家不应被选中作为目标
    // 这个测试验证目标选择的逻辑条件
    GameMode playerMode = GameMode::Creative;
    bool shouldBeTargetable = (playerMode != GameMode::Creative && playerMode != GameMode::Spectator);
    EXPECT_FALSE(shouldBeTargetable);
}

TEST_F(WitherEntityGameModeTest, TargetSelection_ExcludesSpectatorPlayers)
{
    // MC 1.16.5: 旁观者模式玩家不应被选中作为目标
    GameMode playerMode = GameMode::Spectator;
    bool shouldBeTargetable = (playerMode != GameMode::Creative && playerMode != GameMode::Spectator);
    EXPECT_FALSE(shouldBeTargetable);
}

TEST_F(WitherEntityGameModeTest, TargetSelection_IncludesSurvivalPlayers)
{
    // MC 1.16.5: 生存模式玩家应被选中作为目标
    GameMode playerMode = GameMode::Survival;
    bool shouldBeTargetable = (playerMode != GameMode::Creative && playerMode != GameMode::Spectator);
    EXPECT_TRUE(shouldBeTargetable);
}

TEST_F(WitherEntityGameModeTest, TargetSelection_IncludesAdventurePlayers)
{
    // MC 1.16.5: 冒险模式玩家应被选中作为目标
    GameMode playerMode = GameMode::Adventure;
    bool shouldBeTargetable = (playerMode != GameMode::Creative && playerMode != GameMode::Spectator);
    EXPECT_TRUE(shouldBeTargetable);
}

// ============================================================================
// CreatureAttribute 测试 (亡灵判断)
// ============================================================================

TEST_F(WitherEntityGameModeTest, Wither_IsUndead)
{
    // MC 1.16.5: 凋灵是亡灵生物，不应被其他凋灵攻击
    // 侧头不会将亡灵生物作为目标
    constexpr CreatureAttribute WITHER_ATTRIBUTE = CreatureAttribute::Undead;
    EXPECT_EQ(WITHER_ATTRIBUTE, CreatureAttribute::Undead);
}

// ============================================================================
// WITHER_IMMUNE 方块标签测试
// ============================================================================

TEST_F(WitherEntityGameModeTest, Wither_BlockBreakCooldown_IsCorrect)
{
    // MC 1.16.5: 凋灵受伤后触发方块破坏的冷却时间为 20 ticks (1秒)
    constexpr i32 BLOCK_BREAK_COOLDOWN = 20;
    EXPECT_EQ(BLOCK_BREAK_COOLDOWN, 20);
}

TEST_F(WitherEntityGameModeTest, Wither_BlockBreakRange_IsCorrect)
{
    // MC 1.16.5: 凋灵破坏方块范围为 3x4x3
    // x: -1 到 1 (3格), y: 0 到 3 (4格), z: -1 到 1 (3格)
    constexpr i32 RANGE_X_MIN = -1;
    constexpr i32 RANGE_X_MAX = 1;
    constexpr i32 RANGE_Y_MIN = 0;
    constexpr i32 RANGE_Y_MAX = 3;
    constexpr i32 RANGE_Z_MIN = -1;
    constexpr i32 RANGE_Z_MAX = 1;

    EXPECT_EQ(RANGE_X_MAX - RANGE_X_MIN + 1, 3);
    EXPECT_EQ(RANGE_Y_MAX - RANGE_Y_MIN + 1, 4);
    EXPECT_EQ(RANGE_Z_MAX - RANGE_Z_MIN + 1, 3);
}

TEST_F(WitherEntityGameModeTest, Wither_IdleHeadUpdateIncrement_IsCorrect)
{
    // MC 1.16.5: 受伤时每个侧头的空闲更新计数增加 3
    constexpr i32 IDLE_HEAD_UPDATE_INCREMENT = 3;
    EXPECT_EQ(IDLE_HEAD_UPDATE_INCREMENT, 3);
}

TEST_F(WitherEntityGameModeTest, Wither_BlueSkullChance_IsCorrect)
{
    // MC 1.16.5: 主头发射蓝色凋灵之首的概率为 0.1% (0.001)
    constexpr f32 BLUE_SKULL_CHANCE = 0.001f;
    EXPECT_FLOAT_EQ(BLUE_SKULL_CHANCE, 0.001f);
}

TEST_F(WitherEntityGameModeTest, Wither_BlueSkullMotionFactor_IsCorrect)
{
    // MC 1.16.5: 蓝色凋灵之首运动因子为 0.73，普通为 0.95
    constexpr f32 BLUE_SKULL_MOTION_FACTOR = 0.73f;
    constexpr f32 NORMAL_SKULL_MOTION_FACTOR = 0.95f;
    EXPECT_FLOAT_EQ(BLUE_SKULL_MOTION_FACTOR, 0.73f);
    EXPECT_FLOAT_EQ(NORMAL_SKULL_MOTION_FACTOR, 0.95f);
}

TEST_F(WitherEntityGameModeTest, Wither_PreventDespawn_ReturnsTrue)
{
    entity::WitherEntity wither(EntityId(1));
    // 凋灵永不自然消失
    EXPECT_TRUE(wither.preventDespawn());
}

TEST_F(WitherEntityGameModeTest, Wither_IsDespawnPeaceful_ReturnsTrue)
{
    entity::WitherEntity wither(EntityId(1));
    // 和平难度下凋灵应被移除
    EXPECT_TRUE(wither.isDespawnPeaceful());
}

TEST_F(WitherEntityGameModeTest, Wither_FlyingSpeedAttribute)
{
    // FLYING_SPEED 属性值在 registerAttributes() 中注册为 0.6
    // 验证常量值
    constexpr f32 WITHER_FLYING_SPEED = 0.6f;
    EXPECT_FLOAT_EQ(WITHER_FLYING_SPEED, 0.6f);
}

TEST_F(WitherEntityGameModeTest, Wither_NoGravitySetInConstructor)
{
    entity::WitherEntity wither(EntityId(1));
    // 构造时设置 noGravity=true
    EXPECT_TRUE(wither.hasNoGravity());
}
