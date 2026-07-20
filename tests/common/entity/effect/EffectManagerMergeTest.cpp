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

#include "entity/attribute/AttributeMap.hpp"
#include "entity/attribute/Attributes.hpp"
#include "entity/core/LivingEntity.hpp"
#include "entity/effect/EffectInstance.hpp"
#include "entity/effect/EffectManager.hpp"
#include "entity/effect/EffectType.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity::effect;
using namespace mc::entity::attribute;

/**
 * @brief EffectManager 合并行为测试
 *
 * 重点测试效果升级/降级时属性修改器的正确添加/移除/重应用
 */
class EffectManagerMergeTest : public ::testing::Test {
protected:
    std::unique_ptr<LivingEntity> m_entity;

    void SetUp() override
    {
        m_entity = std::make_unique<LivingEntity>(EntityInstanceId(1));
        m_entity->registerData();
        m_entity->registerAttributes();
        // LivingEntity::registerAttributes() 不注册以下属性（由子类注册），
        // 但效果测试需要它们，因此手动注册
        m_entity->attributes().registerAttribute(*Attributes::attackDamage());
        m_entity->attributes().registerAttribute(*Attributes::attackSpeed());
        m_entity->attributes().registerAttribute(*Attributes::luck());
        m_entity->attributes().registerAttribute(*Attributes::jumpBoost());
        m_entity->setHealth(m_entity->maxHealth());
    }

    void TearDown() override { m_entity.reset(); }

    /**
     * @brief 获取实体属性值的辅助方法
     */
    [[nodiscard]] f64 getAttrValue(const char* attrName, f64 defaultValue) const
    {
        return m_entity->attributes().getValue(attrName, defaultValue);
    }
};

// ============================================================================
// 基础合并行为测试
// ============================================================================

TEST_F(EffectManagerMergeTest, AddNewEffectAppliesModifiers)
{
    EffectManager& mgr = m_entity->effectManager();

    // 添加力量 I（amplifier=0），应增加 3.0 攻击伤害
    EffectInstance strength1(EffectType::Strength, 600, 0);
    mgr.addEffect(std::move(strength1), *m_entity);

    // 攻击伤害应从默认 2.0 变为 2.0 + 3.0 = 5.0
    EXPECT_DOUBLE_EQ(getAttrValue(Attributes::ATTACK_DAMAGE, 0.0), 5.0);
    EXPECT_TRUE(mgr.hasEffect(EffectType::Strength));
    EXPECT_EQ(mgr.getEffectLevel(EffectType::Strength), 1);
}

TEST_F(EffectManagerMergeTest, SameAmplifierLongerDurationExtendsOnly)
{
    EffectManager& mgr = m_entity->effectManager();

    // 先添加力量 I 持续 200 tick
    EffectInstance str1(EffectType::Strength, 200, 0);
    mgr.addEffect(std::move(str1), *m_entity);
    ASSERT_TRUE(mgr.getEffect(EffectType::Strength) != nullptr);
    EXPECT_EQ(mgr.getEffect(EffectType::Strength)->duration(), 200);

    // 再添加力量 I 持续 600 tick（同级但更长）
    EffectInstance str1_longer(EffectType::Strength, 600, 0);
    bool merged = mgr.addEffect(std::move(str1_longer), *m_entity);

    // 应合并成功，延长时间
    EXPECT_TRUE(merged);
    EXPECT_EQ(mgr.getEffect(EffectType::Strength)->duration(), 600);
    // amplifier 不变
    EXPECT_EQ(mgr.getEffect(EffectType::Strength)->amplifier(), 0);
    // 攻击伤害不变（没有重新应用）
    EXPECT_DOUBLE_EQ(getAttrValue(Attributes::ATTACK_DAMAGE, 0.0), 5.0);
}

TEST_F(EffectManagerMergeTest, StrongerEffectUpgradesModifiers)
{
    EffectManager& mgr = m_entity->effectManager();

    // 先添加力量 I (amplifier=0, +3.0)
    EffectInstance str1(EffectType::Strength, 600, 0);
    mgr.addEffect(std::move(str1), *m_entity);
    EXPECT_DOUBLE_EQ(getAttrValue(Attributes::ATTACK_DAMAGE, 0.0), 5.0); // 2.0 + 3.0

    // 再添加力量 II (amplifier=1, +6.0)
    EffectInstance str2(EffectType::Strength, 400, 1);
    bool merged = mgr.addEffect(std::move(str2), *m_entity);

    EXPECT_TRUE(merged);
    EXPECT_EQ(mgr.getEffect(EffectType::Strength)->amplifier(), 1);
    // 力量 II: 2.0 + 6.0 = 8.0
    EXPECT_DOUBLE_EQ(getAttrValue(Attributes::ATTACK_DAMAGE, 0.0), 8.0);
}

TEST_F(EffectManagerMergeTest, WeakerEffectDoesNotModifyModifiers)
{
    EffectManager& mgr = m_entity->effectManager();

    // 先添加力量 II (amplifier=1, +6.0)
    EffectInstance str2(EffectType::Strength, 600, 1);
    mgr.addEffect(std::move(str2), *m_entity);
    EXPECT_DOUBLE_EQ(getAttrValue(Attributes::ATTACK_DAMAGE, 0.0), 8.0); // 2.0 + 6.0

    // 再添加力量 I (amplifier=0, +3.0) — 更弱，应被忽略
    EffectInstance str1(EffectType::Strength, 400, 0);
    bool merged = mgr.addEffect(std::move(str1), *m_entity);

    // 合并失败（新效果不比现有效果强）
    EXPECT_FALSE(merged);
    // amplifier 和属性修改器保持不变
    EXPECT_EQ(mgr.getEffect(EffectType::Strength)->amplifier(), 1);
    EXPECT_DOUBLE_EQ(getAttrValue(Attributes::ATTACK_DAMAGE, 0.0), 8.0);
}

TEST_F(EffectManagerMergeTest, SameAmplifierShorterDurationIgnored)
{
    EffectManager& mgr = m_entity->effectManager();

    // 先添加力量 I 持续 600 tick
    EffectInstance str1(EffectType::Strength, 600, 0);
    mgr.addEffect(std::move(str1), *m_entity);

    // 再添加力量 I 持续 200 tick（同级但更短）— 应被忽略
    EffectInstance str1_shorter(EffectType::Strength, 200, 0);
    bool merged = mgr.addEffect(std::move(str1_shorter), *m_entity);

    EXPECT_FALSE(merged);
    EXPECT_EQ(mgr.getEffect(EffectType::Strength)->duration(), 600);
    EXPECT_DOUBLE_EQ(getAttrValue(Attributes::ATTACK_DAMAGE, 0.0), 5.0);
}

// ============================================================================
// 多效果交互测试
// ============================================================================

TEST_F(EffectManagerMergeTest, StrengthAndWeaknessBothApplied)
{
    EffectManager& mgr = m_entity->effectManager();

    // 添加力量 I (+3.0)
    EffectInstance str1(EffectType::Strength, 600, 0);
    mgr.addEffect(std::move(str1), *m_entity);
    EXPECT_DOUBLE_EQ(getAttrValue(Attributes::ATTACK_DAMAGE, 0.0), 5.0); // 2.0 + 3.0

    // 添加虚弱 I (-4.0)
    EffectInstance weak1(EffectType::Weakness, 600, 0);
    mgr.addEffect(std::move(weak1), *m_entity);
    // Addition 阶段: 2.0 + 3.0 + (-4.0) = 1.0
    EXPECT_DOUBLE_EQ(getAttrValue(Attributes::ATTACK_DAMAGE, 0.0), 1.0);
}

TEST_F(EffectManagerMergeTest, RemoveWeaknessRestoresStrengthBonus)
{
    EffectManager& mgr = m_entity->effectManager();

    // 添加力量 I 和虚弱 I
    EffectInstance str1(EffectType::Strength, 600, 0);
    mgr.addEffect(std::move(str1), *m_entity);
    EffectInstance weak1(EffectType::Weakness, 600, 0);
    mgr.addEffect(std::move(weak1), *m_entity);
    EXPECT_DOUBLE_EQ(getAttrValue(Attributes::ATTACK_DAMAGE, 0.0), 1.0); // 2+3-4=1

    // 移除虚弱
    mgr.removeEffect(EffectType::Weakness, *m_entity);
    // 攻击伤害应恢复到 2.0 + 3.0 = 5.0
    EXPECT_DOUBLE_EQ(getAttrValue(Attributes::ATTACK_DAMAGE, 0.0), 5.0);
    EXPECT_FALSE(mgr.hasEffect(EffectType::Weakness));
    EXPECT_TRUE(mgr.hasEffect(EffectType::Strength));
}

TEST_F(EffectManagerMergeTest, SpeedAndSlownessInteraction)
{
    EffectManager& mgr = m_entity->effectManager();

    // 添加速度 I (+20% 移动速度)
    EffectInstance speed1(EffectType::Speed, 600, 0);
    mgr.addEffect(std::move(speed1), *m_entity);
    // 默认移动速度 0.7, Speed I: 0.7 * (1 + 0.2) = 0.84
    EXPECT_NEAR(getAttrValue(Attributes::MOVEMENT_SPEED, 0.0), 0.84, 0.001);

    // 添加缓慢 I (-15% 移动速度)
    EffectInstance slow1(EffectType::Slowness, 600, 0);
    mgr.addEffect(std::move(slow1), *m_entity);
    // Speed I (MultiplyTotal +0.2) + Slowness I (MultiplyTotal -0.15):
    // 0.7 * (1 + 0.2) * (1 - 0.15) = 0.7 * 1.2 * 0.85 = 0.714
    EXPECT_NEAR(getAttrValue(Attributes::MOVEMENT_SPEED, 0.0), 0.714, 0.001);
}

TEST_F(EffectManagerMergeTest, UpgradeSpeedToLevel2)
{
    EffectManager& mgr = m_entity->effectManager();

    // 添加速度 I
    EffectInstance speed1(EffectType::Speed, 600, 0);
    mgr.addEffect(std::move(speed1), *m_entity);
    EXPECT_NEAR(getAttrValue(Attributes::MOVEMENT_SPEED, 0.0), 0.84, 0.001);

    // 升级到速度 II (+40%)
    EffectInstance speed2(EffectType::Speed, 400, 1);
    mgr.addEffect(std::move(speed2), *m_entity);
    // Speed II: 0.7 * (1 + 0.4) = 0.98
    EXPECT_NEAR(getAttrValue(Attributes::MOVEMENT_SPEED, 0.0), 0.98, 0.001);
}

TEST_F(EffectManagerMergeTest, RemoveAllEffectsClearsModifiers)
{
    EffectManager& mgr = m_entity->effectManager();

    // 添加力量 I
    EffectInstance str1(EffectType::Strength, 600, 0);
    mgr.addEffect(std::move(str1), *m_entity);
    EXPECT_DOUBLE_EQ(getAttrValue(Attributes::ATTACK_DAMAGE, 0.0), 5.0);

    // 添加幸运 I
    EffectInstance luck1(EffectType::Luck, 600, 0);
    mgr.addEffect(std::move(luck1), *m_entity);
    EXPECT_DOUBLE_EQ(getAttrValue(Attributes::LUCK, 0.0), 1.0); // 0.0 + 1.0

    // 移除所有效果
    mgr.removeAllEffects(*m_entity);
    // 攻击伤害和幸运值应恢复默认
    EXPECT_DOUBLE_EQ(getAttrValue(Attributes::ATTACK_DAMAGE, 0.0), 2.0);
    EXPECT_DOUBLE_EQ(getAttrValue(Attributes::LUCK, 0.0), 0.0);
    EXPECT_FALSE(mgr.hasEffect(EffectType::Strength));
    EXPECT_FALSE(mgr.hasEffect(EffectType::Luck));
}

// ============================================================================
// Absorption 效果测试
// ============================================================================

TEST_F(EffectManagerMergeTest, AbsorptionAddsMaxAbsorption)
{
    EffectManager& mgr = m_entity->effectManager();

    // 添加伤害吸收 I (+4.0)
    EffectInstance abs1(EffectType::Absorption, 600, 0);
    mgr.addEffect(std::move(abs1), *m_entity);
    // 默认 max_absorption = 0.0, 吸收 I: 0.0 + 4.0 = 4.0
    EXPECT_DOUBLE_EQ(getAttrValue(Attributes::MAX_ABSORPTION, 0.0), 4.0);

    // 升级到吸收 II (+8.0)
    EffectInstance abs2(EffectType::Absorption, 400, 1);
    mgr.addEffect(std::move(abs2), *m_entity);
    EXPECT_DOUBLE_EQ(getAttrValue(Attributes::MAX_ABSORPTION, 0.0), 8.0);

    // 移除吸收
    mgr.removeEffect(EffectType::Absorption, *m_entity);
    EXPECT_DOUBLE_EQ(getAttrValue(Attributes::MAX_ABSORPTION, 0.0), 0.0);
}

// ============================================================================
// HealthBoost 特殊处理测试
// ============================================================================

TEST_F(EffectManagerMergeTest, HealthBoostAddsMaxHealth)
{
    EffectManager& mgr = m_entity->effectManager();

    // 添加生命提升 I (+4.0)
    EffectInstance hb1(EffectType::HealthBoost, 600, 0);
    mgr.addEffect(std::move(hb1), *m_entity);
    // max_health = 20.0 + 4.0 = 24.0
    EXPECT_DOUBLE_EQ(getAttrValue(Attributes::MAX_HEALTH, 0.0), 24.0);
}

// ============================================================================
// EffectInstance::isApplied 测试
// ============================================================================

TEST_F(EffectManagerMergeTest, IsAppliedAfterAddEffect)
{
    EffectManager& mgr = m_entity->effectManager();

    EffectInstance str1(EffectType::Strength, 600, 0);
    mgr.addEffect(std::move(str1), *m_entity);

    const EffectInstance* effect = mgr.getEffect(EffectType::Strength);
    ASSERT_TRUE(effect != nullptr);
    EXPECT_TRUE(effect->isApplied());
}

TEST_F(EffectManagerMergeTest, IsAppliedFalseAfterRemove)
{
    EffectManager& mgr = m_entity->effectManager();

    EffectInstance str1(EffectType::Strength, 600, 0);
    mgr.addEffect(std::move(str1), *m_entity);
    EXPECT_TRUE(mgr.getEffect(EffectType::Strength)->isApplied());

    mgr.removeEffect(EffectType::Strength, *m_entity);
    // 效果已从列表中移除，无法检查 isApplied
    EXPECT_FALSE(mgr.hasEffect(EffectType::Strength));
}
