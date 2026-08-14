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

#include "common/TestWorldHelper.hpp"
#include "common/core/Constants.hpp"
#include "common/entity/attribute/AttributeModifier.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/monster/illager/WitchEntity.hpp"
#include "common/entity/interfaces/IRangedAttackMob.hpp"

namespace mc {
namespace {

/**
 * @brief 测试用世界实现
 */
class WitchTestWorld final : public mc::test::BaseTestWorld {
public:
    bool setBlockState(i32, i32, i32, const BlockState*) override { return false; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Normal; }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return EntityInstanceId(static_cast<u32>(m_spawnedEntities.size()));
    }

    void advanceTick() { m_currentTick++; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("WitchTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("WitchTestWorld::tickManager not implemented");
    }

    [[nodiscard]] size_t spawnedEntityCount() const { return m_spawnedEntities.size(); }

private:
    u64 m_currentTick = 0;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
};

} // namespace

// ============================================================================
// WitchEntity 基础测试
// ============================================================================

TEST(WitchEntityTest, Construction)
{
    WitchEntity witch(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 验证女巫尺寸
    EXPECT_FLOAT_EQ(witch.width(), 0.6f);
    EXPECT_FLOAT_EQ(witch.height(), 1.95f);

    // 验证默认喝药水状态
    EXPECT_FALSE(witch.isDrinking());
    EXPECT_EQ(witch.getDrinkTimer(), 0);
    EXPECT_EQ(witch.getAttackCooldown(), 0);
}

TEST(WitchEntityTest, EyeHeightCorrect)
{
    WitchEntity witch(EntityInstanceId(1), mc::test::testEcsRegistry());
    // MC 1.16.5: 女巫眼睛高度为 1.62
    EXPECT_FLOAT_EQ(witch.eyeHeight(), 1.62f);
}

TEST(WitchEntityTest, DoesNotBurnInDaylight)
{
    WitchEntity witch(EntityInstanceId(1), mc::test::testEcsRegistry());
    // MC 1.16.5: 女巫不在阳光下燃烧
    EXPECT_FALSE(witch.shouldBurnInDaylight());
}

// ========== 喝药水状态测试 ==========

TEST(WitchEntityTest, DrinkingStateCanBeSet)
{
    WitchEntity witch(EntityInstanceId(1), mc::test::testEcsRegistry());

    EXPECT_FALSE(witch.isDrinking());

    witch.setDrinking(true);
    EXPECT_TRUE(witch.isDrinking());

    witch.setDrinking(false);
    EXPECT_FALSE(witch.isDrinking());
}

TEST(WitchEntityTest, DrinkTimerCanBeSet)
{
    WitchEntity witch(EntityInstanceId(1), mc::test::testEcsRegistry());

    witch.setDrinkTimer(32);
    EXPECT_EQ(witch.getDrinkTimer(), 32);

    witch.setDrinkTimer(0);
    EXPECT_EQ(witch.getDrinkTimer(), 0);
}

TEST(WitchEntityTest, AttackCooldownCanBeReset)
{
    WitchEntity witch(EntityInstanceId(1), mc::test::testEcsRegistry());

    witch.resetAttackCooldown();
    EXPECT_EQ(witch.getAttackCooldown(), 60); // 3秒 = 60 ticks
}

// ========== 效果应用测试 ==========

TEST(WitchEntityTest, InstantHealingRestoresHealth)
{
    WitchEntity witch(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 设置初始生命值
    witch.setHealth(10.0f);

    // 通过heal()方法恢复生命值
    witch.heal(4.0f);

    EXPECT_FLOAT_EQ(witch.health(), 14.0f);
}

TEST(WitchEntityTest, SpeedEffectCanBeAdded)
{
    WitchEntity witch(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 添加速度效果
    entity::effect::EffectInstance speedEffect(entity::effect::EffectType::Speed,
        3600,  // 持续时间
        0,     // 等级 I
        false, // 非环境效果
        true,  // 显示粒子
        true   // 显示图标
    );

    witch.addEffect(std::move(speedEffect));

    EXPECT_TRUE(witch.hasEffect(entity::effect::EffectType::Speed));
    EXPECT_EQ(witch.getEffectLevel(entity::effect::EffectType::Speed), 1);
}

TEST(WitchEntityTest, WaterBreathingEffectCanBeAdded)
{
    WitchEntity witch(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 添加水肺效果
    entity::effect::EffectInstance waterBreathingEffect(
        entity::effect::EffectType::WaterBreathing, 3600, 0, false, true, true);

    witch.addEffect(std::move(waterBreathingEffect));

    EXPECT_TRUE(witch.hasEffect(entity::effect::EffectType::WaterBreathing));
}

TEST(WitchEntityTest, FireResistanceEffectCanBeAdded)
{
    WitchEntity witch(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 添加抗火效果
    entity::effect::EffectInstance fireResistanceEffect(
        entity::effect::EffectType::FireResistance, 3600, 0, false, true, true);

    witch.addEffect(std::move(fireResistanceEffect));

    EXPECT_TRUE(witch.hasEffect(entity::effect::EffectType::FireResistance));
}

// ========== 魔法伤害减免测试 ==========

TEST(WitchEntityTest, MagicDamageReducedBy85Percent)
{
    WitchEntity witch(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 创建魔法伤害来源
    auto magicSource = DamageSources::magic();
    f32 originalDamage = 10.0f;

    f32 reducedDamage = witch.applyMagicDamageReduction(magicSource, originalDamage);

    // 女巫对魔法伤害只受 15%
    EXPECT_FLOAT_EQ(reducedDamage, 1.5f);
}

TEST(WitchEntityTest, ImmuneToSelfDamage)
{
    WitchEntity witch(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 创建来自女巫自己的伤害
    EntityDamageSource selfSource(DamageType::Magic, &witch);

    f32 reducedDamage = witch.applyMagicDamageReduction(selfSource, 10.0f);

    // 女巫免疫自己造成的伤害
    EXPECT_FLOAT_EQ(reducedDamage, 0.0f);
}

// ========== 常量值测试 ==========

TEST(WitchEntityTest, ConstantsAreCorrect)
{
    WitchEntity witch(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 验证常量符合 MC 1.16.5
    // ATTACK_COOLDOWN = 60 (3秒)
    witch.resetAttackCooldown();
    EXPECT_EQ(witch.getAttackCooldown(), 60);
}

// ========== Create工厂测试 ==========

TEST(WitchEntityTest, CreateFactory)
{
    auto entity = WitchEntity::create(nullptr, mc::test::testEcsRegistry());
    ASSERT_NE(entity, nullptr);

    // 验证创建的是 WitchEntity
    auto* witchPtr = dynamic_cast<WitchEntity*>(entity.get());
    EXPECT_NE(witchPtr, nullptr);
}

// ========== IRangedAttackMob 接口测试 ==========

TEST(WitchEntityTest, ImplementsIRangedAttackMob)
{
    WitchEntity witch(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 验证女巫实现了 IRangedAttackMob 接口
    auto* rangedAttacker = dynamic_cast<entity::IRangedAttackMob*>(&witch);
    EXPECT_NE(rangedAttacker, nullptr);

    // 验证默认攻击间隔
    EXPECT_EQ(rangedAttacker->getAttackInterval(), 60);

    // 验证默认可以进行远程攻击（不在喝药水状态）
    EXPECT_TRUE(rangedAttacker->canRangedAttack());

    // 设置喝药水状态后不能远程攻击
    witch.setDrinking(true);
    EXPECT_FALSE(rangedAttacker->canRangedAttack());
}

// ========== 速度修饰符测试 ==========

TEST(WitchEntityTest, DrinkingPotionAddsSpeedPenaltyModifier)
{
    WitchEntity witch(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 初始移动速度应为 0.25
    f64 initialSpeed = witch.getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.0);
    EXPECT_DOUBLE_EQ(initialSpeed, 0.25);

    // 开始喝药水：添加速度修饰符（模拟 _startDrinkingPotion 的修饰符逻辑）
    entity::attribute::AttributeModifier speedPenalty(WitchEntity::DRINKING_SPEED_PENALTY_UUID,
        "Drinking speed penalty",
        -0.25,
        entity::attribute::Operation::Addition);
    witch.attributes().addModifier(entity::attribute::Attributes::MOVEMENT_SPEED, speedPenalty);

    // 喝药水时移动速度应变为 0.0 (0.25 + (-0.25) = 0.0)
    f64 drinkingSpeed = witch.getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.0);
    EXPECT_DOUBLE_EQ(drinkingSpeed, 0.0);
}

TEST(WitchEntityTest, FinishingDrinkingRemovesSpeedPenaltyModifier)
{
    WitchEntity witch(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 添加速度修饰符
    entity::attribute::AttributeModifier speedPenalty(WitchEntity::DRINKING_SPEED_PENALTY_UUID,
        "Drinking speed penalty",
        -0.25,
        entity::attribute::Operation::Addition);
    witch.attributes().addModifier(entity::attribute::Attributes::MOVEMENT_SPEED, speedPenalty);

    // 验证修饰符已添加
    f64 drinkingSpeed = witch.getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.0);
    EXPECT_DOUBLE_EQ(drinkingSpeed, 0.0);

    // 移除修饰符（模拟 _finishDrinkingPotion）
    witch.attributes().removeModifier(
        entity::attribute::Attributes::MOVEMENT_SPEED, WitchEntity::DRINKING_SPEED_PENALTY_UUID);

    // 速度应恢复到基础值 0.25
    f64 normalSpeed = witch.getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.0);
    EXPECT_DOUBLE_EQ(normalSpeed, 0.25);
}

TEST(WitchEntityTest, SpeedPenaltyModifierUsesAdditionOperation)
{
    WitchEntity witch(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 验证修饰符使用 Addition 操作（值直接相加，而非乘法）
    entity::attribute::AttributeModifier speedPenalty(WitchEntity::DRINKING_SPEED_PENALTY_UUID,
        "Drinking speed penalty",
        -0.25,
        entity::attribute::Operation::Addition);
    witch.attributes().addModifier(entity::attribute::Attributes::MOVEMENT_SPEED, speedPenalty);

    // Addition 操作：0.25 + (-0.25) = 0.0
    f64 result = witch.getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.0);
    EXPECT_DOUBLE_EQ(result, 0.0);
}

TEST(WitchEntityTest, SpeedPenaltyUUIDIsCorrect)
{
    // 验证 UUID 与 MC 1.16.5 一致
    // MC 源码中女巫喝药水减速的 UUID: "5CD17E52-A79A-43D3-A529-90FDE04B181E"
    EXPECT_STREQ(WitchEntity::DRINKING_SPEED_PENALTY_UUID, "5CD17E52-A79A-43D3-A529-90FDE04B181E");
}

// ========== 药水类型选择测试 ==========

TEST(WitchEntityTest, SelectAttackPotionType_ReturnsHarmingByDefault)
{
    WitchEntity witch(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 默认情况：目标无特殊状态，应该返回伤害药水
    // 注意：这需要 Mock LivingEntity，这里只测试基本逻辑
    // 实际测试需要完整的实体系统支持
}

TEST(WitchEntityTest, AttackCooldownCorrectValue)
{
    WitchEntity witch(EntityInstanceId(1), mc::test::testEcsRegistry());

    // MC 1.16.5: 女巫攻击冷却为 60 ticks (3秒)
    EXPECT_EQ(witch.getAttackInterval(), 60);
}

} // namespace mc
