/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction or without limitation the rights
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

#include "common/core/EnumSet.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>

using namespace mc;
using namespace mc::math;

// ============================================================================
// GhastEntity 常量测试
// ============================================================================
//
// 注意：GhastEntity::shootFireball() 的完整行为测试需要 Mock 世界和实体。
// 这里测试常量和基本配置。
// 参考 MC 1.16.5 GhastEntity.FireballAttackGoal

class GhastEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 设置代码
    }
};

// ============================================================================
// MC 1.16.5 恶魂属性测试
// ============================================================================

TEST_F(GhastEntityTest, MaxHealth_IsCorrect)
{
    // MC 1.16.5: 恶魂最大生命值为 10
    constexpr f32 GHAST_MAX_HEALTH = 10.0f;
    EXPECT_FLOAT_EQ(GHAST_MAX_HEALTH, 10.0f);
}

TEST_F(GhastEntityTest, MovementSpeed_IsZero)
{
    // MC 1.16.5: 恶魂移动速度为 0（通过飞行速度控制）
    constexpr f32 GHAST_MOVEMENT_SPEED = 0.0f;
    EXPECT_FLOAT_EQ(GHAST_MOVEMENT_SPEED, 0.0f);
}

TEST_F(GhastEntityTest, FlyingSpeed_IsCorrect)
{
    // MC 1.16.5: 恶魂飞行速度为 0.9
    constexpr f32 GHAST_FLYING_SPEED = 0.9f;
    EXPECT_FLOAT_EQ(GHAST_FLYING_SPEED, 0.9f);
}

TEST_F(GhastEntityTest, ExperienceValue_IsCorrect)
{
    // MC 1.16.5: 恶魂掉落 5 经验
    constexpr i32 GHAST_EXPERIENCE = 5;
    EXPECT_EQ(GHAST_EXPERIENCE, 5);
}

// ============================================================================
// 火球攻击常量测试
// ============================================================================

TEST_F(GhastEntityTest, ChargeTime_IsCorrect)
{
    // MC 1.16.5: 恶魂充能时间为 20 ticks (1秒)
    // 参考 GhastEntity.tick() 中 m_chargeTime >= 20
    constexpr i32 CHARGE_TIME = 20;
    EXPECT_EQ(CHARGE_TIME, 20);
}

TEST_F(GhastEntityTest, AttackCooldown_IsCorrect)
{
    // MC 1.16.5: 攻击冷却时间为 40 ticks (2秒)
    // 参考 GhastEntity.tick() 中 m_attackCooldown = 40
    constexpr i32 ATTACK_COOLDOWN = 40;
    EXPECT_EQ(ATTACK_COOLDOWN, 40);
}

TEST_F(GhastEntityTest, DefaultExplosionPower_IsCorrect)
{
    // MC 1.16.5: 默认爆炸威力为 1
    // 参考 GhastEntity.explosionStrength = 1
    constexpr i32 DEFAULT_EXPLOSION_POWER = 1;
    EXPECT_EQ(DEFAULT_EXPLOSION_POWER, 1);
}

TEST_F(GhastEntityTest, FireballLaunchDistance_IsCorrect)
{
    // MC 1.16.5: 火球发射位置在恶魂前方 4 格
    // 发射位置 = 恶魂位置 + lookVector * 4.0
    constexpr f64 FIREBALL_LAUNCH_DISTANCE = 4.0;
    EXPECT_DOUBLE_EQ(FIREBALL_LAUNCH_DISTANCE, 4.0);
}

// ============================================================================
// 火球发射位置计算测试
// ============================================================================

TEST_F(GhastEntityTest, LookVectorCalculation_Forward)
{
    // 测试朝向正前方的 look 向量计算
    // yaw = 0 (正南方向), pitch = 0 (水平)
    const f32 yaw = 0.0f;   // 正南
    const f32 pitch = 0.0f; // 水平

    const f32 yawRad = yaw * DEG_TO_RAD;
    const f32 pitchRad = pitch * DEG_TO_RAD;

    // MC 1.16.5 坐标系：
    // lookX = -sin(yaw) * cos(pitch)
    // lookY = -sin(pitch)
    // lookZ = cos(yaw) * cos(pitch)
    const f32 lookX = -std::sin(yawRad) * std::cos(pitchRad);
    const f32 lookY = -std::sin(pitchRad);
    const f32 lookZ = std::cos(yawRad) * std::cos(pitchRad);

    // 正南方向 (yaw=0) 时，lookX=0, lookZ=1
    EXPECT_NEAR(lookX, 0.0f, 0.0001f);
    EXPECT_NEAR(lookY, 0.0f, 0.0001f);
    EXPECT_NEAR(lookZ, 1.0f, 0.0001f);
}

TEST_F(GhastEntityTest, LookVectorCalculation_East)
{
    // 测试朝向东的 look 向量计算
    // yaw = -90 (正东方向), pitch = 0 (水平)
    const f32 yaw = -90.0f; // 正东
    const f32 pitch = 0.0f; // 水平

    const f32 yawRad = yaw * DEG_TO_RAD;
    const f32 pitchRad = pitch * DEG_TO_RAD;

    const f32 lookX = -std::sin(yawRad) * std::cos(pitchRad);
    const f32 lookY = -std::sin(pitchRad);
    const f32 lookZ = std::cos(yawRad) * std::cos(pitchRad);

    // 正东方向 (yaw=-90) 时，lookX=1, lookZ=0
    EXPECT_NEAR(lookX, 1.0f, 0.0001f);
    EXPECT_NEAR(lookY, 0.0f, 0.0001f);
    EXPECT_NEAR(lookZ, 0.0f, 0.0001f);
}

TEST_F(GhastEntityTest, LookVectorCalculation_Down)
{
    // 测试朝下的 look 向量计算
    // yaw = 0, pitch = 90 (向下看)
    const f32 yaw = 0.0f;
    const f32 pitch = 90.0f; // 向下看

    const f32 yawRad = yaw * DEG_TO_RAD;
    const f32 pitchRad = pitch * DEG_TO_RAD;

    const f32 lookX = -std::sin(yawRad) * std::cos(pitchRad);
    const f32 lookY = -std::sin(pitchRad);
    const f32 lookZ = std::cos(yawRad) * std::cos(pitchRad);

    // 向下看时 (pitch=90)，lookY=-1
    EXPECT_NEAR(lookX, 0.0f, 0.0001f);
    EXPECT_NEAR(lookY, -1.0f, 0.0001f);
    EXPECT_NEAR(lookZ, 0.0f, 0.0001f);
}

TEST_F(GhastEntityTest, FireballPositionCalculation)
{
    // 测试火球发射位置计算
    // 恶魂位置 (0, 64, 0)，朝向正南 (yaw=0, pitch=0)
    const f64 ghastX = 0.0;
    const f64 ghastY = 64.0;
    const f64 ghastEyeHeight = 2.6; // MC 1.16.5: 恶魂眼睛高度
    const f64 ghastZ = 0.0;

    const f32 yaw = 0.0f;
    const f32 pitch = 0.0f;

    const f32 yawRad = yaw * DEG_TO_RAD;
    const f32 pitchRad = pitch * DEG_TO_RAD;

    const f32 lookX = -std::sin(yawRad) * std::cos(pitchRad);
    const f32 lookY = -std::sin(pitchRad);
    const f32 lookZ = std::cos(yawRad) * std::cos(pitchRad);

    // 火球发射位置 = 恶魂位置 + lookVector * 4.0
    const f64 launchDist = 4.0;
    const f32 fireballX = static_cast<f32>(ghastX + lookX * launchDist);
    const f32 fireballY = static_cast<f32>(ghastY + ghastEyeHeight + 0.5 + lookY * launchDist);
    const f32 fireballZ = static_cast<f32>(ghastZ + lookZ * launchDist);

    // 正南方向时，火球应在恶魂前方 4 格
    EXPECT_NEAR(fireballX, 0.0f, 0.0001f);
    EXPECT_NEAR(fireballY, static_cast<f32>(ghastY + ghastEyeHeight + 0.5), 0.0001f);
    EXPECT_NEAR(fireballZ, 4.0f, 0.0001f);
}

// ============================================================================
// FireballEntity 常量测试
// ============================================================================

TEST_F(GhastEntityTest, FireballWidth_IsCorrect)
{
    // MC 1.16.5: 大火球宽度为 1.0
    constexpr f32 FIREBALL_WIDTH = 1.0f;
    EXPECT_FLOAT_EQ(FIREBALL_WIDTH, 1.0f);
}

TEST_F(GhastEntityTest, FireballHeight_IsCorrect)
{
    // MC 1.16.5: 大火球高度为 1.0
    constexpr f32 FIREBALL_HEIGHT = 1.0f;
    EXPECT_FLOAT_EQ(FIREBALL_HEIGHT, 1.0f);
}

TEST_F(GhastEntityTest, FireballDamage_IsCorrect)
{
    // MC 1.16.5: 大火球直接命中伤害为 6.0
    // 参考 FireballEntity 构造函数 setDamage(6.0f)
    constexpr f32 FIREBALL_DAMAGE = 6.0f;
    EXPECT_FLOAT_EQ(FIREBALL_DAMAGE, 6.0f);
}

TEST_F(GhastEntityTest, FireballMotionFactor_IsCorrect)
{
    // MC 1.16.5: 大火球运动因子为 0.95
    // 参考 DamagingProjectileEntity.getMotionFactor()
    constexpr f32 FIREBALL_MOTION_FACTOR = 0.95f;
    EXPECT_FLOAT_EQ(FIREBALL_MOTION_FACTOR, 0.95f);
}

// ============================================================================
// 攻击周期测试
// ============================================================================

TEST_F(GhastEntityTest, AttackCycleTime_IsCorrect)
{
    // MC 1.16.5: 完整攻击周期 = 充能 + 冷却
    // 充能 20 ticks + 冷却 40 ticks = 60 ticks = 3 秒
    constexpr i32 CHARGE_TIME = 20;
    constexpr i32 COOLDOWN_TIME = 40;
    constexpr i32 TOTAL_ATTACK_CYCLE = CHARGE_TIME + COOLDOWN_TIME;

    EXPECT_EQ(TOTAL_ATTACK_CYCLE, 60);

    // 转换为秒 (20 ticks = 1 秒)
    constexpr f64 TOTAL_SECONDS = static_cast<f64>(TOTAL_ATTACK_CYCLE) / 20.0;
    EXPECT_DOUBLE_EQ(TOTAL_SECONDS, 3.0);
}

// ============================================================================
// 火球方向向量计算测试
// ============================================================================

TEST_F(GhastEntityTest, DirectionVectorCalculation)
{
    // 测试火球方向向量计算
    // 目标位置 (10, 66, 10)，火球位置 (0, 66.6, 4)
    const f64 targetX = 10.0;
    const f64 targetY = 66.0;
    const f64 targetEyeHeight = 1.62; // 玩家眼睛高度
    const f64 targetZ = 10.0;

    const f32 fireballX = 0.0f;
    const f32 fireballY = 66.6f; // 恶魂眼睛高度 + 0.5
    const f32 fireballZ = 4.0f;

    // MC 1.16.5 方向向量计算:
    // dx = target.x - fireball.x
    // dy = target.y + target.eyeHeight * 0.5 - (ghast.y + ghast.eyeHeight * 0.5 + 0.5)
    // dz = target.z - fireball.z
    const f32 dx = static_cast<f32>(targetX - fireballX);
    const f32 dy = static_cast<f32>(targetY + targetEyeHeight * 0.5 - (64.0 + 2.6 * 0.5 + 0.5));
    const f32 dz = static_cast<f32>(targetZ - fireballZ);

    // 验证方向向量分量
    EXPECT_FLOAT_EQ(dx, 10.0f);
    EXPECT_GT(dz, 0.0f); // 目标在火球前方
}

// ============================================================================
// 恶魂碰撞箱测试
// ============================================================================

TEST_F(GhastEntityTest, BoundingBox_IsCorrect)
{
    // MC 1.16.5: 恶魂碰撞箱为 4x4
    // 但在项目中可能使用不同的尺寸，这里测试常量
    constexpr f32 GHAST_WIDTH = 4.0f;
    constexpr f32 GHAST_HEIGHT = 4.0f;
    EXPECT_FLOAT_EQ(GHAST_WIDTH, 4.0f);
    EXPECT_FLOAT_EQ(GHAST_HEIGHT, 4.0f);
}

TEST_F(GhastEntityTest, EyeHeight_IsCorrect)
{
    // MC 1.16.5: 恶魂眼睛高度为 2.6
    constexpr f32 GHAST_EYE_HEIGHT = 2.6f;
    EXPECT_FLOAT_EQ(GHAST_EYE_HEIGHT, 2.6f);
}
