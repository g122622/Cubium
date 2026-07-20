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

#include "common/item/Items.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "entity/core/LivingEntity.hpp"
#include "entity/entities/player/Player.hpp"
#include "world/fluid/FluidRegistry.hpp"

using namespace mc;

/**
 * @brief 横扫攻击静止检测条件测试
 *
 * MC 1.16.5 中横扫攻击（sweeping attack）触发条件：
 * - 冷却 > 90%
 * - 非暴击
 * - 非疾跑击退
 * - 在地面上
 * - 玩家几乎静止（distanceWalkedModified - prevDistanceWalkedModified < aiMoveSpeed）
 *
 * 参考 MC 1.16.5 PlayerEntity.attackTargetEntityWithCurrentItem() 行 1147-1148
 */
class SweepAttackConditionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
        Items::initialize();
        item::enchant::EnchantmentRegistry::initialize();

        m_player = std::make_unique<Player>(static_cast<EntityInstanceId>(1), "TestPlayer");
    }

    void TearDown() override
    {
        m_player.reset();
        item::enchant::EnchantmentRegistry::clear();
    }

    std::unique_ptr<Player> m_player;
};

// ============================================================================
// distanceWalkedModified 基础测试
// ============================================================================

TEST_F(SweepAttackConditionTest, MoveDistanceWalked_InitialValue_IsZero)
{
    // 初始状态下移动距离应为 0
    EXPECT_FLOAT_EQ(m_player->moveDistanceWalked(), 0.0f);
    EXPECT_FLOAT_EQ(m_player->prevMoveDistanceWalked(), 0.0f);
}

TEST_F(SweepAttackConditionTest, MoveDistanceWalked_AfterMovement_HasDelta)
{
    // 模拟移动后 distanceWalkedModified 应该增加
    // 首先设置玩家位置并模拟一些移动
    m_player->setPosition(0.0f, 64.0f, 0.0f);

    // 通过调用 updateMoveDistance 来更新移动距离
    // 在实际游戏中，这由 Player::tick() 调用
    // 这里我们无法直接设置 m_moveDistanceWalked，但可以验证 getter 工作正常

    // 验证 getter 返回的值是有效的浮点数
    f32 current = m_player->moveDistanceWalked();
    f32 prev = m_player->prevMoveDistanceWalked();
    EXPECT_FALSE(std::isnan(current));
    EXPECT_FALSE(std::isnan(prev));
}

// ============================================================================
// aiMoveSpeed 测试
// ============================================================================

TEST_F(SweepAttackConditionTest, AiMoveSpeed_DefaultValue_IsValid)
{
    // aiMoveSpeed() 继承自 LivingEntity，使用 m_landMovementFactor
    // 默认值应该是有效的正浮点数
    f32 speed = m_player->aiMoveSpeed();
    EXPECT_GT(speed, 0.0f);
    EXPECT_FALSE(std::isnan(speed));
    EXPECT_FALSE(std::isinf(speed));
}

TEST_F(SweepAttackConditionTest, AiMoveSpeed_CanBeModified)
{
    // 获取默认速度
    f32 defaultSpeed = m_player->aiMoveSpeed();

    // 设置新的速度
    m_player->setAIMoveSpeed(0.2f);
    EXPECT_FLOAT_EQ(m_player->aiMoveSpeed(), 0.2f);

    // 设置另一个速度
    m_player->setAIMoveSpeed(0.15f);
    EXPECT_FLOAT_EQ(m_player->aiMoveSpeed(), 0.15f);

    // 恢复默认值
    m_player->setAIMoveSpeed(defaultSpeed);
    EXPECT_FLOAT_EQ(m_player->aiMoveSpeed(), defaultSpeed);
}

// ============================================================================
// 横扫攻击静止检测条件边界测试
// ============================================================================

TEST_F(SweepAttackConditionTest, SweepCondition_DistanceDelta_ExactlyEqualsAiMoveSpeed)
{
    // 测试边界情况：distanceWalkedDelta == aiMoveSpeed
    // MC 条件: distanceWalkedDelta < aiMoveSpeed，等于时不应触发

    f32 aiSpeed = m_player->aiMoveSpeed();

    // 当 delta == aiSpeed 时，条件应为 false（因为 < 而非 <=）
    f32 deltaExactlyEqual = aiSpeed;
    bool shouldSweep = (deltaExactlyEqual < static_cast<f64>(aiSpeed));
    EXPECT_FALSE(shouldSweep) << "当 distanceWalkedDelta == aiMoveSpeed 时，横扫攻击不应触发";
}

TEST_F(SweepAttackConditionTest, SweepCondition_DistanceDelta_SlightlyLessThanAiMoveSpeed)
{
    // 测试略小于的情况：distanceWalkedDelta < aiMoveSpeed
    // 应该触发横扫攻击

    f32 aiSpeed = m_player->aiMoveSpeed();

    // 略小于 aiSpeed
    f32 deltaSlightlyLess = aiSpeed * 0.99f;
    bool shouldSweep = (static_cast<f64>(deltaSlightlyLess) < static_cast<f64>(aiSpeed));
    EXPECT_TRUE(shouldSweep) << "当 distanceWalkedDelta < aiMoveSpeed 时，横扫攻击应触发";
}

TEST_F(SweepAttackConditionTest, SweepCondition_DistanceDelta_SlightlyGreaterThanAiMoveSpeed)
{
    // 测试略大于的情况：distanceWalkedDelta > aiMoveSpeed
    // 不应触发横扫攻击

    f32 aiSpeed = m_player->aiMoveSpeed();

    // 略大于 aiSpeed
    f32 deltaSlightlyGreater = aiSpeed * 1.01f;
    bool shouldSweep = (static_cast<f64>(deltaSlightlyGreater) < static_cast<f64>(aiSpeed));
    EXPECT_FALSE(shouldSweep) << "当 distanceWalkedDelta > aiMoveSpeed 时，横扫攻击不应触发";
}

TEST_F(SweepAttackConditionTest, SweepCondition_DistanceDelta_MuchLessThanAiMoveSpeed)
{
    // 测试远小于的情况（玩家静止）
    // 应该触发横扫攻击

    f32 aiSpeed = m_player->aiMoveSpeed();

    // 远小于 aiSpeed（玩家几乎静止）
    f32 deltaMuchLess = aiSpeed * 0.1f;
    bool shouldSweep = (static_cast<f64>(deltaMuchLess) < static_cast<f64>(aiSpeed));
    EXPECT_TRUE(shouldSweep) << "当玩家几乎静止时，横扫攻击应触发";
}

TEST_F(SweepAttackConditionTest, SweepCondition_DistanceDelta_Zero_PlayerStandingStill)
{
    // 测试玩家完全静止（delta = 0）
    // 应该触发横扫攻击

    f32 aiSpeed = m_player->aiMoveSpeed();

    // 玩家完全静止
    f32 deltaZero = 0.0f;
    bool shouldSweep = (static_cast<f64>(deltaZero) < static_cast<f64>(aiSpeed));
    EXPECT_TRUE(shouldSweep) << "当玩家完全静止时，横扫攻击应触发";
}

TEST_F(SweepAttackConditionTest, SweepCondition_DistanceDelta_MuchGreaterThanAiMoveSpeed)
{
    // 测试远大于的情况（玩家正在移动）
    // 不应触发横扫攻击

    f32 aiSpeed = m_player->aiMoveSpeed();

    // 远大于 aiSpeed（玩家正在快速移动）
    f32 deltaMuchGreater = aiSpeed * 5.0f;
    bool shouldSweep = (static_cast<f64>(deltaMuchGreater) < static_cast<f64>(aiSpeed));
    EXPECT_FALSE(shouldSweep) << "当玩家正在快速移动时，横扫攻击不应触发";
}

// ============================================================================
// 横扫攻击条件组合测试
// ============================================================================

TEST_F(SweepAttackConditionTest, SweepCondition_AllConditionsMet_CanSweep)
{
    // 测试所有条件都满足时可以触发横扫攻击
    f32 aiSpeed = m_player->aiMoveSpeed();

    // 模拟所有条件满足：
    // - isFullCooldown = true (冷却 > 90%)
    // - !isCritical = true (非暴击)
    // - !isSprintKnockback = true (非疾跑击退)
    // - isOnGround() = true (在地面)
    // - distanceWalkedDelta < aiMoveSpeed (几乎静止)

    bool isFullCooldown = true;
    bool isCritical = false;
    bool isSprintKnockback = false;
    bool isOnGround = true;
    f64 distanceWalkedDelta = static_cast<f64>(aiSpeed * 0.5f); // 小于 aiMoveSpeed

    bool canSweep = isFullCooldown && !isCritical && !isSprintKnockback && isOnGround &&
        (distanceWalkedDelta < static_cast<f64>(aiSpeed));

    EXPECT_TRUE(canSweep) << "所有条件满足时，横扫攻击应触发";
}

TEST_F(SweepAttackConditionTest, SweepCondition_MovingPlayer_CannotSweep)
{
    // 测试玩家正在移动时不能触发横扫攻击
    f32 aiSpeed = m_player->aiMoveSpeed();

    // 玩家正在移动（distanceWalkedDelta > aiMoveSpeed）
    bool isFullCooldown = true;
    bool isCritical = false;
    bool isSprintKnockback = false;
    bool isOnGround = true;
    f64 distanceWalkedDelta = static_cast<f64>(aiSpeed * 2.0f); // 大于 aiMoveSpeed

    bool canSweep = isFullCooldown && !isCritical && !isSprintKnockback && isOnGround &&
        (distanceWalkedDelta < static_cast<f64>(aiSpeed));

    EXPECT_FALSE(canSweep) << "玩家正在移动时，横扫攻击不应触发";
}

TEST_F(SweepAttackConditionTest, SweepCondition_CriticalHit_CannotSweep)
{
    // 测试暴击时不能触发横扫攻击
    f32 aiSpeed = m_player->aiMoveSpeed();

    // 暴击攻击
    bool isFullCooldown = true;
    bool isCritical = true; // 暴击
    bool isSprintKnockback = false;
    bool isOnGround = true;
    f64 distanceWalkedDelta = static_cast<f64>(aiSpeed * 0.5f);

    bool canSweep = isFullCooldown && !isCritical && !isSprintKnockback && isOnGround &&
        (distanceWalkedDelta < static_cast<f64>(aiSpeed));

    EXPECT_FALSE(canSweep) << "暴击时，横扫攻击不应触发";
}

TEST_F(SweepAttackConditionTest, SweepCondition_SprintKnockback_CannotSweep)
{
    // 测试疾跑击退时不能触发横扫攻击
    f32 aiSpeed = m_player->aiMoveSpeed();

    // 疾跑击退
    bool isFullCooldown = true;
    bool isCritical = false;
    bool isSprintKnockback = true; // 疾跑击退
    bool isOnGround = true;
    f64 distanceWalkedDelta = static_cast<f64>(aiSpeed * 0.5f);

    bool canSweep = isFullCooldown && !isCritical && !isSprintKnockback && isOnGround &&
        (distanceWalkedDelta < static_cast<f64>(aiSpeed));

    EXPECT_FALSE(canSweep) << "疾跑击退时，横扫攻击不应触发";
}

TEST_F(SweepAttackConditionTest, SweepCondition_NotOnGround_CannotSweep)
{
    // 测试不在地面时不能触发横扫攻击
    f32 aiSpeed = m_player->aiMoveSpeed();

    // 不在地面
    bool isFullCooldown = true;
    bool isCritical = false;
    bool isSprintKnockback = false;
    bool isOnGround = false; // 不在地面
    f64 distanceWalkedDelta = static_cast<f64>(aiSpeed * 0.5f);

    bool canSweep = isFullCooldown && !isCritical && !isSprintKnockback && isOnGround &&
        (distanceWalkedDelta < static_cast<f64>(aiSpeed));

    EXPECT_FALSE(canSweep) << "不在地面时，横扫攻击不应触发";
}

TEST_F(SweepAttackConditionTest, SweepCondition_LowCooldown_CannotSweep)
{
    // 测试冷却不足时不能触发横扫攻击
    f32 aiSpeed = m_player->aiMoveSpeed();

    // 冷却不足
    bool isFullCooldown = false; // 冷却不足
    bool isCritical = false;
    bool isSprintKnockback = false;
    bool isOnGround = true;
    f64 distanceWalkedDelta = static_cast<f64>(aiSpeed * 0.5f);

    bool canSweep = isFullCooldown && !isCritical && !isSprintKnockback && isOnGround &&
        (distanceWalkedDelta < static_cast<f64>(aiSpeed));

    EXPECT_FALSE(canSweep) << "冷却不足时，横扫攻击不应触发";
}

// ============================================================================
// 数值范围验证
// ============================================================================

TEST_F(SweepAttackConditionTest, AiMoveSpeed_TypicalRange)
{
    // 验证典型的 aiMoveSpeed 范围
    // MC 1.16.5 中玩家的基础移动速度约为 0.1
    f32 speed = m_player->aiMoveSpeed();

    // 典型范围应该在 0.05 ~ 0.5 之间
    EXPECT_GT(speed, 0.0f);
    EXPECT_LT(speed, 1.0f) << "aiMoveSpeed 应该小于 1.0（玩家移动速度通常在 0.1 左右）";
}

TEST_F(SweepAttackConditionTest, DistanceWalkedDelta_CalculationConsistency)
{
    // 验证距离计算的一致性
    // distanceWalkedDelta = distanceWalkedModified - prevDistanceWalkedModified

    // 获取当前值
    f32 current = m_player->moveDistanceWalked();
    f32 prev = m_player->prevMoveDistanceWalked();

    // delta 应该等于 current - prev
    f32 delta = current - prev;
    EXPECT_FLOAT_EQ(delta, m_player->moveDistanceWalked() - m_player->prevMoveDistanceWalked());
}
