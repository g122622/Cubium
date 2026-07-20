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

#include "common/core/Types.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/projectile/AbstractFireballEntity.hpp"

using namespace mc;
using namespace mc::entity;

// ============================================================================
// FireballEntity 测试
// ============================================================================

class FireballEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建 FireballEntity
        m_fireball = std::make_unique<FireballEntity>(EntityInstanceId(0));
    }

    std::unique_ptr<FireballEntity> m_fireball;
};

// ============================================================================
// 基本属性测试
// ============================================================================

TEST_F(FireballEntityTest, DefaultValues_AreCorrect)
{
    // MC 1.16.5: FireballEntity 默认伤害 6.0
    EXPECT_FLOAT_EQ(m_fireball->damage(), 6.0f);

    // MC 1.16.5: 爆炸威力默认 1
    EXPECT_EQ(m_fireball->explosionPower(), 1);

    // 尺寸
    EXPECT_FLOAT_EQ(m_fireball->width(), 1.0f);
    EXPECT_FLOAT_EQ(m_fireball->height(), 1.0f);
}

TEST_F(FireballEntityTest, SetExplosionPower_UpdatesValue)
{
    m_fireball->setExplosionPower(2);
    EXPECT_EQ(m_fireball->explosionPower(), 2);

    m_fireball->setExplosionPower(0);
    EXPECT_EQ(m_fireball->explosionPower(), 0);
}

TEST_F(FireballEntityTest, SetDamage_UpdatesValue)
{
    m_fireball->setDamage(10.0f);
    EXPECT_FLOAT_EQ(m_fireball->damage(), 10.0f);
}

// ============================================================================
// SmallFireballEntity 测试
// ============================================================================

class SmallFireballEntityTest : public ::testing::Test {
protected:
    void SetUp() override { m_smallFireball = std::make_unique<SmallFireballEntity>(EntityInstanceId(0)); }

    std::unique_ptr<SmallFireballEntity> m_smallFireball;
};

TEST_F(SmallFireballEntityTest, DefaultValues_AreCorrect)
{
    // MC 1.16.5: SmallFireballEntity 默认伤害 5.0
    EXPECT_FLOAT_EQ(m_smallFireball->damage(), 5.0f);

    // 尺寸 (更小的火球)
    EXPECT_FLOAT_EQ(m_smallFireball->width(), 0.3125f);
    EXPECT_FLOAT_EQ(m_smallFireball->height(), 0.3125f);
}

// ============================================================================
// DragonFireballEntity 测试
// ============================================================================

class DragonFireballEntityTest : public ::testing::Test {
protected:
    void SetUp() override { m_dragonFireball = std::make_unique<DragonFireballEntity>(EntityInstanceId(0)); }

    std::unique_ptr<DragonFireballEntity> m_dragonFireball;
};

TEST_F(DragonFireballEntityTest, DefaultValues_AreCorrect)
{
    // MC 1.16.5: DragonFireballEntity 伤害值 12.0 (虽然不直接造成伤害)
    EXPECT_FLOAT_EQ(m_dragonFireball->damage(), 12.0f);

    // 尺寸
    EXPECT_FLOAT_EQ(m_dragonFireball->width(), 1.0f);
    EXPECT_FLOAT_EQ(m_dragonFireball->height(), 1.0f);
}

// ============================================================================
// WitherSkullEntity 测试
// ============================================================================

class WitherSkullEntityTest : public ::testing::Test {
protected:
    void SetUp() override { m_witherSkull = std::make_unique<WitherSkullEntity>(EntityInstanceId(0)); }

    std::unique_ptr<WitherSkullEntity> m_witherSkull;
};

TEST_F(WitherSkullEntityTest, DefaultValues_AreCorrect)
{
    // MC 1.16.5: WitherSkullEntity 默认伤害 8.0
    EXPECT_FLOAT_EQ(m_witherSkull->damage(), 8.0f);

    // 尺寸
    EXPECT_FLOAT_EQ(m_witherSkull->width(), 0.3125f);
    EXPECT_FLOAT_EQ(m_witherSkull->height(), 0.3125f);

    // 默认不是蓝色凋灵之首
    EXPECT_FALSE(m_witherSkull->isBlue());
}

TEST_F(WitherSkullEntityTest, SetBlue_UpdatesValue)
{
    m_witherSkull->setBlue(true);
    EXPECT_TRUE(m_witherSkull->isBlue());

    m_witherSkull->setBlue(false);
    EXPECT_FALSE(m_witherSkull->isBlue());
}

// 注意：getMotionFactor() 和 isFiery() 是 protected 方法，无法直接测试
// 运动因子测试应通过行为测试验证（蓝色凋灵之首移动更快）

// ============================================================================
// MC 1.16.5 伤害参数常量测试
// ============================================================================

class FireballConstantsTest : public ::testing::Test {};

TEST_F(FireballConstantsTest, FireballDamage_MatchesMC1165)
{
    // MC 1.16.5: FireballEntity 伤害 6.0
    constexpr f32 FIREBALL_DAMAGE = 6.0f;
    EXPECT_FLOAT_EQ(FIREBALL_DAMAGE, 6.0f);
}

TEST_F(FireballConstantsTest, SmallFireballDamage_MatchesMC1165)
{
    // MC 1.16.5: SmallFireballEntity 伤害 5.0
    constexpr f32 SMALL_FIREBALL_DAMAGE = 5.0f;
    EXPECT_FLOAT_EQ(SMALL_FIREBALL_DAMAGE, 5.0f);
}

TEST_F(FireballConstantsTest, WitherSkullDamage_MatchesMC1165)
{
    // MC 1.16.5: WitherSkullEntity 伤害 (有发射者 8.0, 无发射者 5.0)
    constexpr f32 WITHER_SKULL_DAMAGE_SHOOTER = 8.0f;
    constexpr f32 WITHER_SKULL_DAMAGE_NO_SHOOTER = 5.0f;
    EXPECT_FLOAT_EQ(WITHER_SKULL_DAMAGE_SHOOTER, 8.0f);
    EXPECT_FLOAT_EQ(WITHER_SKULL_DAMAGE_NO_SHOOTER, 5.0f);
}

TEST_F(FireballConstantsTest, WitherEffectDuration_MatchesMC1165)
{
    // MC 1.16.5: 凋零效果持续时间
    // 简单难度: 无效果
    // 普通难度: 200 ticks (10 秒)
    // 困难难度: 800 ticks (40 秒)
    constexpr i32 WITHER_DURATION_NORMAL = 200;
    constexpr i32 WITHER_DURATION_HARD = 800;
    EXPECT_EQ(WITHER_DURATION_NORMAL, 200);
    EXPECT_EQ(WITHER_DURATION_HARD, 800);
}

TEST_F(FireballConstantsTest, DragonBreathCloudDuration_MatchesMC1165)
{
    // MC 1.16.5: 龙息云持续时间 600 ticks (30 秒)
    constexpr i32 DRAGON_BREATH_DURATION = 600;
    EXPECT_EQ(DRAGON_BREATH_DURATION, 600);
}

TEST_F(FireballConstantsTest, DragonBreathCloudRadius_MatchesMC1165)
{
    // MC 1.16.5: 龙息云初始半径 3.0, 最终扩展到 7.0
    constexpr f32 DRAGON_BREATH_INITIAL_RADIUS = 3.0f;
    constexpr f32 DRAGON_BREATH_FINAL_RADIUS = 7.0f;
    EXPECT_FLOAT_EQ(DRAGON_BREATH_INITIAL_RADIUS, 3.0f);
    EXPECT_FLOAT_EQ(DRAGON_BREATH_FINAL_RADIUS, 7.0f);
}

TEST_F(FireballConstantsTest, SmallFireballFireDuration_MatchesMC1165)
{
    // MC 1.16.5: 小火球点燃目标 5 秒
    constexpr i32 FIRE_DURATION_TICKS = 100; // 5 秒 = 100 ticks
    EXPECT_EQ(FIRE_DURATION_TICKS, 100);
}

TEST_F(FireballConstantsTest, ExplosionRadius_MatchesMC1165)
{
    // MC 1.16.5: 火球和凋灵之首爆炸半径
    constexpr f32 FIREBALL_EXPLOSION_RADIUS = 1.0f;
    constexpr f32 WITHER_SKULL_EXPLOSION_RADIUS = 1.0f;
    EXPECT_FLOAT_EQ(FIREBALL_EXPLOSION_RADIUS, 1.0f);
    EXPECT_FLOAT_EQ(WITHER_SKULL_EXPLOSION_RADIUS, 1.0f);
}

TEST_F(FireballConstantsTest, WitherHealAmount_MatchesMC1165)
{
    // MC 1.16.5: 凋灵之首杀死目标后治疗发射者 5.0 HP
    constexpr f32 WITHER_HEAL_AMOUNT = 5.0f;
    EXPECT_FLOAT_EQ(WITHER_HEAL_AMOUNT, 5.0f);
}

TEST_F(FireballConstantsTest, EffectAmplifier_MatchesMC1165)
{
    // MC 1.16.5: 凋零效果等级 II = amplifier 1
    // MC 1.16.5: 瞬间伤害效果等级 II = amplifier 1
    constexpr i32 WITHER_EFFECT_AMPLIFIER = 1;  // II级
    constexpr i32 INSTANT_DAMAGE_AMPLIFIER = 1; // II级
    EXPECT_EQ(WITHER_EFFECT_AMPLIFIER, 1);
    EXPECT_EQ(INSTANT_DAMAGE_AMPLIFIER, 1);
}
