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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "entity/effect/EffectAttributeModifiers.hpp"
#include "entity/attribute/Attribute.hpp"
#include "entity/attribute/AttributeMap.hpp"
#include "entity/attribute/AttributeModifier.hpp"
#include "entity/attribute/Attributes.hpp"
#include "entity/effect/EffectType.hpp"
#include <string>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity::effect;
using namespace mc::entity::attribute;

/**
 * @brief EffectAttributeModifiers 测试
 *
 * 测试效果属性修改器的映射、计算和创建功能
 */
class EffectAttributeModifiersTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// ============================================================================
// getEffectModifiers 测试
// ============================================================================

TEST_F(EffectAttributeModifiersTest, SpeedHasModifier)
{
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::Speed);
    ASSERT_EQ(mods.size(), 1u);
    EXPECT_EQ(mods[0].attributeName, std::string(Attributes::MOVEMENT_SPEED));
    EXPECT_DOUBLE_EQ(mods[0].baseAmount, 0.2);
    EXPECT_EQ(mods[0].operation, Operation::MultiplyTotal);
}

TEST_F(EffectAttributeModifiersTest, SlownessHasModifier)
{
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::Slowness);
    ASSERT_EQ(mods.size(), 1u);
    EXPECT_EQ(mods[0].attributeName, std::string(Attributes::MOVEMENT_SPEED));
    EXPECT_DOUBLE_EQ(mods[0].baseAmount, -0.15);
    EXPECT_EQ(mods[0].operation, Operation::MultiplyTotal);
}

TEST_F(EffectAttributeModifiersTest, HasteHasModifier)
{
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::Haste);
    ASSERT_EQ(mods.size(), 1u);
    EXPECT_EQ(mods[0].attributeName, std::string(Attributes::ATTACK_SPEED));
    EXPECT_DOUBLE_EQ(mods[0].baseAmount, 0.1);
    EXPECT_EQ(mods[0].operation, Operation::MultiplyTotal);
}

TEST_F(EffectAttributeModifiersTest, MiningFatigueHasModifier)
{
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::MiningFatigue);
    ASSERT_EQ(mods.size(), 1u);
    EXPECT_EQ(mods[0].attributeName, std::string(Attributes::ATTACK_SPEED));
    EXPECT_DOUBLE_EQ(mods[0].baseAmount, -0.1);
    EXPECT_EQ(mods[0].operation, Operation::MultiplyTotal);
}

TEST_F(EffectAttributeModifiersTest, StrengthHasCorrectModifier)
{
    // 力量效果：每级 +3.0 攻击伤害（Addition 操作）
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::Strength);
    ASSERT_EQ(mods.size(), 1u);
    EXPECT_EQ(mods[0].attributeName, std::string(Attributes::ATTACK_DAMAGE));
    EXPECT_DOUBLE_EQ(mods[0].baseAmount, 3.0);
    EXPECT_EQ(mods[0].operation, Operation::Addition);
}

TEST_F(EffectAttributeModifiersTest, WeaknessHasCorrectModifier)
{
    // 虚弱效果：每级 -4.0 攻击伤害（Addition 操作）
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::Weakness);
    ASSERT_EQ(mods.size(), 1u);
    EXPECT_EQ(mods[0].attributeName, std::string(Attributes::ATTACK_DAMAGE));
    EXPECT_DOUBLE_EQ(mods[0].baseAmount, -4.0);
    EXPECT_EQ(mods[0].operation, Operation::Addition);
}

TEST_F(EffectAttributeModifiersTest, JumpBoostHasModifier)
{
    // 跳跃提升：每级 +0.1 跳跃力（JUMP_BOOST）且每级 +1 安全摔落距离（SAFE_FALL_DISTANCE）。
    // 对齐 vanilla MobEffects.JUMP_BOOST 同时挂 JUMP_STRENGTH 与 SAFE_FALL_DISTANCE 两个修饰符
    // （SAFE_FALL_DISTANCE 修饰符使跳跃增强药水延长安全摔落高度，对齐 EffectAttributeModifiers.cpp:60-66）。
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::JumpBoost);
    ASSERT_EQ(mods.size(), 2u);
    EXPECT_EQ(mods[0].attributeName, std::string(Attributes::JUMP_BOOST));
    EXPECT_DOUBLE_EQ(mods[0].baseAmount, 0.1);
    EXPECT_EQ(mods[0].operation, Operation::Addition);
    EXPECT_EQ(mods[1].attributeName, std::string(Attributes::SAFE_FALL_DISTANCE));
    EXPECT_DOUBLE_EQ(mods[1].baseAmount, 1.0);
    EXPECT_EQ(mods[1].operation, Operation::Addition);
}

TEST_F(EffectAttributeModifiersTest, HealthBoostHasModifier)
{
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::HealthBoost);
    ASSERT_EQ(mods.size(), 1u);
    EXPECT_EQ(mods[0].attributeName, std::string(Attributes::MAX_HEALTH));
    EXPECT_DOUBLE_EQ(mods[0].baseAmount, 4.0);
    EXPECT_EQ(mods[0].operation, Operation::Addition);
}

TEST_F(EffectAttributeModifiersTest, AbsorptionHasModifier)
{
    // 伤害吸收效果：每级 +4.0 最大吸收值
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::Absorption);
    ASSERT_EQ(mods.size(), 1u);
    EXPECT_EQ(mods[0].attributeName, std::string(Attributes::MAX_ABSORPTION));
    EXPECT_DOUBLE_EQ(mods[0].baseAmount, 4.0);
    EXPECT_EQ(mods[0].operation, Operation::Addition);
}

TEST_F(EffectAttributeModifiersTest, LuckHasModifier)
{
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::Luck);
    ASSERT_EQ(mods.size(), 1u);
    EXPECT_EQ(mods[0].attributeName, std::string(Attributes::LUCK));
    EXPECT_DOUBLE_EQ(mods[0].baseAmount, 1.0);
    EXPECT_EQ(mods[0].operation, Operation::Addition);
}

TEST_F(EffectAttributeModifiersTest, BadLuckHasModifier)
{
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::BadLuck);
    ASSERT_EQ(mods.size(), 1u);
    EXPECT_EQ(mods[0].attributeName, std::string(Attributes::LUCK));
    EXPECT_DOUBLE_EQ(mods[0].baseAmount, -1.0);
    EXPECT_EQ(mods[0].operation, Operation::Addition);
}

TEST_F(EffectAttributeModifiersTest, NoAttributeEffectsReturnEmpty)
{
    // 这些效果没有属性修改器（只有逻辑效果）
    EXPECT_TRUE(EffectAttributeModifiers::getEffectModifiers(EffectType::Resistance).empty());
    EXPECT_TRUE(EffectAttributeModifiers::getEffectModifiers(EffectType::FireResistance).empty());
    EXPECT_TRUE(EffectAttributeModifiers::getEffectModifiers(EffectType::WaterBreathing).empty());
    EXPECT_TRUE(EffectAttributeModifiers::getEffectModifiers(EffectType::Invisibility).empty());
    EXPECT_TRUE(EffectAttributeModifiers::getEffectModifiers(EffectType::NightVision).empty());
    EXPECT_TRUE(EffectAttributeModifiers::getEffectModifiers(EffectType::Poison).empty());
    EXPECT_TRUE(EffectAttributeModifiers::getEffectModifiers(EffectType::Wither).empty());
    EXPECT_TRUE(EffectAttributeModifiers::getEffectModifiers(EffectType::SlowFalling).empty());
    EXPECT_TRUE(EffectAttributeModifiers::getEffectModifiers(EffectType::ConduitPower).empty());
    EXPECT_TRUE(EffectAttributeModifiers::getEffectModifiers(EffectType::DolphinsGrace).empty());
    EXPECT_TRUE(EffectAttributeModifiers::getEffectModifiers(EffectType::Levitation).empty());
}

// ============================================================================
// hasAttributeModifiers 测试
// ============================================================================

TEST_F(EffectAttributeModifiersTest, HasAttributeModifiers_TrueForModifierEffects)
{
    EXPECT_TRUE(EffectAttributeModifiers::hasAttributeModifiers(EffectType::Speed));
    EXPECT_TRUE(EffectAttributeModifiers::hasAttributeModifiers(EffectType::Strength));
    EXPECT_TRUE(EffectAttributeModifiers::hasAttributeModifiers(EffectType::Weakness));
    EXPECT_TRUE(EffectAttributeModifiers::hasAttributeModifiers(EffectType::HealthBoost));
    EXPECT_TRUE(EffectAttributeModifiers::hasAttributeModifiers(EffectType::Absorption));
}

TEST_F(EffectAttributeModifiersTest, HasAttributeModifiers_FalseForNonModifierEffects)
{
    EXPECT_FALSE(EffectAttributeModifiers::hasAttributeModifiers(EffectType::Resistance));
    EXPECT_FALSE(EffectAttributeModifiers::hasAttributeModifiers(EffectType::FireResistance));
    EXPECT_FALSE(EffectAttributeModifiers::hasAttributeModifiers(EffectType::Poison));
}

// ============================================================================
// calculateAmount 测试
// ============================================================================

TEST_F(EffectAttributeModifiersTest, CalculateAmount_SpeedLevel1)
{
    // Speed I: 0.2 * (0 + 1) = 0.2
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::Speed);
    ASSERT_FALSE(mods.empty());
    EXPECT_DOUBLE_EQ(mods[0].calculateAmount(0), 0.2);
}

TEST_F(EffectAttributeModifiersTest, CalculateAmount_SpeedLevel2)
{
    // Speed II: 0.2 * (1 + 1) = 0.4
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::Speed);
    ASSERT_FALSE(mods.empty());
    EXPECT_DOUBLE_EQ(mods[0].calculateAmount(1), 0.4);
}

TEST_F(EffectAttributeModifiersTest, CalculateAmount_StrengthLevel1)
{
    // Strength I: 3.0 * (0 + 1) = 3.0
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::Strength);
    ASSERT_FALSE(mods.empty());
    EXPECT_DOUBLE_EQ(mods[0].calculateAmount(0), 3.0);
}

TEST_F(EffectAttributeModifiersTest, CalculateAmount_StrengthLevel2)
{
    // Strength II: 3.0 * (1 + 1) = 6.0
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::Strength);
    ASSERT_FALSE(mods.empty());
    EXPECT_DOUBLE_EQ(mods[0].calculateAmount(1), 6.0);
}

TEST_F(EffectAttributeModifiersTest, CalculateAmount_WeaknessLevel1)
{
    // Weakness I: -4.0 * (0 + 1) = -4.0
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::Weakness);
    ASSERT_FALSE(mods.empty());
    EXPECT_DOUBLE_EQ(mods[0].calculateAmount(0), -4.0);
}

TEST_F(EffectAttributeModifiersTest, CalculateAmount_WeaknessLevel2)
{
    // Weakness II: -4.0 * (1 + 1) = -8.0
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::Weakness);
    ASSERT_FALSE(mods.empty());
    EXPECT_DOUBLE_EQ(mods[0].calculateAmount(1), -8.0);
}

TEST_F(EffectAttributeModifiersTest, CalculateAmount_HealthBoostLevel2)
{
    // Health Boost II: 4.0 * (1 + 1) = 8.0
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::HealthBoost);
    ASSERT_FALSE(mods.empty());
    EXPECT_DOUBLE_EQ(mods[0].calculateAmount(1), 8.0);
}

TEST_F(EffectAttributeModifiersTest, CalculateAmount_AbsorptionLevel1)
{
    // Absorption I: 4.0 * (0 + 1) = 4.0
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::Absorption);
    ASSERT_FALSE(mods.empty());
    EXPECT_DOUBLE_EQ(mods[0].calculateAmount(0), 4.0);
}

// ============================================================================
// createModifier 测试
// ============================================================================

TEST_F(EffectAttributeModifiersTest, CreateModifier_NameFormat)
{
    // 修改器名称应使用 MC 原版格式: effect.minecraft.<resource_name>.<level>
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::Speed);
    ASSERT_FALSE(mods.empty());
    AttributeModifier modifier = EffectAttributeModifiers::createModifier(mods[0], EffectType::Speed, 0);
    EXPECT_EQ(modifier.name(), std::string("effect.minecraft.speed.1"));

    AttributeModifier modifier2 = EffectAttributeModifiers::createModifier(mods[0], EffectType::Speed, 1);
    EXPECT_EQ(modifier2.name(), std::string("effect.minecraft.speed.2"));
}

TEST_F(EffectAttributeModifiersTest, CreateModifier_StrengthName)
{
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::Strength);
    ASSERT_FALSE(mods.empty());
    AttributeModifier modifier = EffectAttributeModifiers::createModifier(mods[0], EffectType::Strength, 0);
    EXPECT_EQ(modifier.name(), std::string("effect.minecraft.strength.1"));
}

TEST_F(EffectAttributeModifiersTest, CreateModifier_WeaknessName)
{
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::Weakness);
    ASSERT_FALSE(mods.empty());
    AttributeModifier modifier = EffectAttributeModifiers::createModifier(mods[0], EffectType::Weakness, 0);
    EXPECT_EQ(modifier.name(), std::string("effect.minecraft.weakness.1"));
}

TEST_F(EffectAttributeModifiersTest, CreateModifier_MultiWordEffectName)
{
    // 多词效果如 mining_fatigue 应使用下划线格式
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::MiningFatigue);
    ASSERT_FALSE(mods.empty());
    AttributeModifier modifier = EffectAttributeModifiers::createModifier(mods[0], EffectType::MiningFatigue, 2);
    EXPECT_EQ(modifier.name(), std::string("effect.minecraft.mining_fatigue.3"));
}

TEST_F(EffectAttributeModifiersTest, CreateModifier_Amount)
{
    // 验证修改器金额正确计算
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::Strength);
    ASSERT_FALSE(mods.empty());
    // Strength II (amplifier=1): amount = 3.0 * (1+1) = 6.0
    AttributeModifier modifier = EffectAttributeModifiers::createModifier(mods[0], EffectType::Strength, 1);
    EXPECT_DOUBLE_EQ(modifier.amount(), 6.0);
    EXPECT_EQ(modifier.operation(), Operation::Addition);
}

TEST_F(EffectAttributeModifiersTest, CreateModifier_UUID)
{
    // 验证修改器使用正确的 UUID
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::Speed);
    ASSERT_FALSE(mods.empty());
    AttributeModifier modifier = EffectAttributeModifiers::createModifier(mods[0], EffectType::Speed, 0);
    EXPECT_EQ(modifier.id(), std::string(EffectAttributeModifiers::SPEED_UUID));
}

// ============================================================================
// 属性系统积分测试 - 验证修改器正确应用到 AttributeMap
// ============================================================================

TEST_F(EffectAttributeModifiersTest, StrengthModifierIncreasesAttackDamage)
{
    // 注册 ATTACK_DAMAGE 属性
    AttributeMap attrMap;
    attrMap.registerAttribute(*Attributes::attackDamage());
    // 攻击伤害属性默认值为 2.0
    EXPECT_DOUBLE_EQ(attrMap.getValue(Attributes::ATTACK_DAMAGE, 0.0), 2.0);

    // 应用力量 I 修改器
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::Strength);
    ASSERT_FALSE(mods.empty());
    AttributeModifier modifier = EffectAttributeModifiers::createModifier(mods[0], EffectType::Strength, 0);
    attrMap.addModifier(Attributes::ATTACK_DAMAGE, modifier);

    // 力量 I: 基础 2.0 + 3.0 = 5.0
    EXPECT_DOUBLE_EQ(attrMap.getValue(Attributes::ATTACK_DAMAGE, 0.0), 5.0);
}

TEST_F(EffectAttributeModifiersTest, WeaknessModifierDecreasesAttackDamage)
{
    AttributeMap attrMap;
    attrMap.registerAttribute(*Attributes::attackDamage());
    EXPECT_DOUBLE_EQ(attrMap.getValue(Attributes::ATTACK_DAMAGE, 0.0), 2.0);

    // 应用虚弱 I 修改器
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::Weakness);
    ASSERT_FALSE(mods.empty());
    AttributeModifier modifier = EffectAttributeModifiers::createModifier(mods[0], EffectType::Weakness, 0);
    attrMap.addModifier(Attributes::ATTACK_DAMAGE, modifier);

    // 虚弱 I: 基础 2.0 + (-4.0) = -2.0, 但属性下限为 0.0
    EXPECT_DOUBLE_EQ(attrMap.getValue(Attributes::ATTACK_DAMAGE, 0.0), 0.0);
}

TEST_F(EffectAttributeModifiersTest, StrengthAndWeaknessCancel)
{
    AttributeMap attrMap;
    attrMap.registerAttribute(*Attributes::attackDamage());

    // 同时应用力量 I 和虚弱 I
    const auto& strMods = EffectAttributeModifiers::getEffectModifiers(EffectType::Strength);
    const auto& weakMods = EffectAttributeModifiers::getEffectModifiers(EffectType::Weakness);
    ASSERT_FALSE(strMods.empty());
    ASSERT_FALSE(weakMods.empty());

    AttributeModifier strMod = EffectAttributeModifiers::createModifier(strMods[0], EffectType::Strength, 0);
    AttributeModifier weakMod = EffectAttributeModifiers::createModifier(weakMods[0], EffectType::Weakness, 0);

    attrMap.addModifier(Attributes::ATTACK_DAMAGE, strMod);
    attrMap.addModifier(Attributes::ATTACK_DAMAGE, weakMod);

    // 力量 I (+3.0) + 虚弱 I (-4.0) + 基础 2.0 = 1.0
    EXPECT_DOUBLE_EQ(attrMap.getValue(Attributes::ATTACK_DAMAGE, 0.0), 1.0);
}

TEST_F(EffectAttributeModifiersTest, SpeedModifierMultiplies)
{
    AttributeMap attrMap;
    attrMap.registerAttribute(*Attributes::movementSpeed());
    // 默认移动速度 = 0.7 (来自 Attributes::movementSpeed() 工厂函数)
    EXPECT_DOUBLE_EQ(attrMap.getValue(Attributes::MOVEMENT_SPEED, 0.0), 0.7);

    // 应用速度 I 修改器
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::Speed);
    ASSERT_FALSE(mods.empty());
    AttributeModifier modifier = EffectAttributeModifiers::createModifier(mods[0], EffectType::Speed, 0);
    attrMap.addModifier(Attributes::MOVEMENT_SPEED, modifier);

    // 速度 I: 0.7 * (1 + 0.2) = 0.84
    EXPECT_NEAR(attrMap.getValue(Attributes::MOVEMENT_SPEED, 0.0), 0.84, 0.001);
}

TEST_F(EffectAttributeModifiersTest, HealthBoostModifierIncreasesMaxHealth)
{
    AttributeMap attrMap;
    attrMap.registerAttribute(*Attributes::maxHealth());
    // 默认最大生命值 = 20.0
    EXPECT_DOUBLE_EQ(attrMap.getValue(Attributes::MAX_HEALTH, 0.0), 20.0);

    // 应用生命提升 II 修改器
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::HealthBoost);
    ASSERT_FALSE(mods.empty());
    AttributeModifier modifier = EffectAttributeModifiers::createModifier(mods[0], EffectType::HealthBoost, 1);
    attrMap.addModifier(Attributes::MAX_HEALTH, modifier);

    // 生命提升 II: 20.0 + 4.0 * 2 = 28.0
    EXPECT_DOUBLE_EQ(attrMap.getValue(Attributes::MAX_HEALTH, 0.0), 28.0);
}

TEST_F(EffectAttributeModifiersTest, AbsorptionModifierIncreasesMaxAbsorption)
{
    AttributeMap attrMap;
    attrMap.registerAttribute(*Attributes::maxAbsorption());
    // 默认最大吸收值 = 0.0
    EXPECT_DOUBLE_EQ(attrMap.getValue(Attributes::MAX_ABSORPTION, 0.0), 0.0);

    // 应用伤害吸收 II 修改器
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::Absorption);
    ASSERT_FALSE(mods.empty());
    AttributeModifier modifier = EffectAttributeModifiers::createModifier(mods[0], EffectType::Absorption, 1);
    attrMap.addModifier(Attributes::MAX_ABSORPTION, modifier);

    // 伤害吸收 II: 0.0 + 4.0 * 2 = 8.0
    EXPECT_DOUBLE_EQ(attrMap.getValue(Attributes::MAX_ABSORPTION, 0.0), 8.0);
}

TEST_F(EffectAttributeModifiersTest, RemoveModifierRestoresValue)
{
    AttributeMap attrMap;
    attrMap.registerAttribute(*Attributes::attackDamage());
    // 默认攻击伤害 = 2.0
    EXPECT_DOUBLE_EQ(attrMap.getValue(Attributes::ATTACK_DAMAGE, 0.0), 2.0);

    // 应用力量 I
    const auto& mods = EffectAttributeModifiers::getEffectModifiers(EffectType::Strength);
    ASSERT_FALSE(mods.empty());
    AttributeModifier modifier = EffectAttributeModifiers::createModifier(mods[0], EffectType::Strength, 0);
    attrMap.addModifier(Attributes::ATTACK_DAMAGE, modifier);
    // 力量 I: 2.0 + 3.0 = 5.0
    EXPECT_DOUBLE_EQ(attrMap.getValue(Attributes::ATTACK_DAMAGE, 0.0), 5.0);

    // 移除修改器
    attrMap.removeModifier(Attributes::ATTACK_DAMAGE, mods[0].uuid);
    // 恢复默认值 2.0
    EXPECT_DOUBLE_EQ(attrMap.getValue(Attributes::ATTACK_DAMAGE, 0.0), 2.0);
}
