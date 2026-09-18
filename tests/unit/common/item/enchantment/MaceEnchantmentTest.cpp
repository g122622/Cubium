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
 * THE SOFTWARE IS PROVIDED "AS IS", WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "item/enchantment/Enchantment.hpp"
#include "item/enchantment/EnchantmentContainer.hpp"
#include "item/enchantment/EnchantmentHelper.hpp"
#include "item/enchantment/EnchantmentRegistry.hpp"
#include "item/enchantment/enchantments/AllEnchantments.hpp"
#include "item/enchantment/enchantments/mace/BreachEnchantment.hpp"
#include "item/enchantment/enchantments/mace/DensityEnchantment.hpp"
#include "item/enchantment/enchantments/mace/WindBurstEnchantment.hpp"
#include "item/enchantment/enchantments/trident/ImpalingEnchantment.hpp"
#include "item/enchantment/enchantments/weapon/DamageEnchantment.hpp"
#include "item/items/trial/MaceMath.hpp"

using namespace mc;
using namespace mc::item;
using namespace mc::item::enchant;

// ============================================================================
// MaceMath 纯计算函数测试
// ============================================================================

class MaceMathTest : public ::testing::Test {};

// --- calculateSmashAttackDamage 分段函数测试 ---

TEST_F(MaceMathTest, SmashAttackDamage_ZeroOrNegative)
{
    EXPECT_FLOAT_EQ(MaceMath::calculateSmashAttackDamage(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(MaceMath::calculateSmashAttackDamage(-1.0f), 0.0f);
    EXPECT_FLOAT_EQ(MaceMath::calculateSmashAttackDamage(-0.01f), 0.0f);
}

TEST_F(MaceMathTest, SmashAttackDamage_FirstSegment)
{
    // 第一段：0~3格，4.0 * fallDistance
    EXPECT_FLOAT_EQ(MaceMath::calculateSmashAttackDamage(0.5f), 2.0f);
    EXPECT_FLOAT_EQ(MaceMath::calculateSmashAttackDamage(1.0f), 4.0f);
    EXPECT_FLOAT_EQ(MaceMath::calculateSmashAttackDamage(1.5f), 6.0f);
    EXPECT_FLOAT_EQ(MaceMath::calculateSmashAttackDamage(2.0f), 8.0f);
    EXPECT_FLOAT_EQ(MaceMath::calculateSmashAttackDamage(3.0f), 12.0f);
}

TEST_F(MaceMathTest, SmashAttackDamage_SecondSegment)
{
    // 第二段：3~8格，12.0 + 2.0 * (fd - 3.0)
    EXPECT_FLOAT_EQ(MaceMath::calculateSmashAttackDamage(4.0f), 14.0f);
    EXPECT_FLOAT_EQ(MaceMath::calculateSmashAttackDamage(5.0f), 16.0f);
    EXPECT_FLOAT_EQ(MaceMath::calculateSmashAttackDamage(6.5f), 19.0f);
    EXPECT_FLOAT_EQ(MaceMath::calculateSmashAttackDamage(8.0f), 22.0f);
}

TEST_F(MaceMathTest, SmashAttackDamage_ThirdSegment)
{
    // 第三段：8格以上，22.0 + (fd - 8.0)
    EXPECT_FLOAT_EQ(MaceMath::calculateSmashAttackDamage(9.0f), 23.0f);
    EXPECT_FLOAT_EQ(MaceMath::calculateSmashAttackDamage(10.0f), 24.0f);
    EXPECT_FLOAT_EQ(MaceMath::calculateSmashAttackDamage(15.0f), 29.0f);
    EXPECT_FLOAT_EQ(MaceMath::calculateSmashAttackDamage(20.0f), 34.0f);
    EXPECT_FLOAT_EQ(MaceMath::calculateSmashAttackDamage(50.0f), 64.0f);
}

TEST_F(MaceMathTest, SmashAttackDamage_SegmentBoundaries)
{
    // 第一段和第二段边界：3.0f
    EXPECT_FLOAT_EQ(MaceMath::calculateSmashAttackDamage(3.0f), 12.0f);
    EXPECT_FLOAT_EQ(12.0f + 2.0f * (3.0f - 3.0f), 12.0f); // 连续性验证

    // 第二段和第三段边界：8.0f
    EXPECT_FLOAT_EQ(MaceMath::calculateSmashAttackDamage(8.0f), 22.0f);
    EXPECT_FLOAT_EQ(22.0f + (8.0f - 8.0f), 22.0f); // 连续性验证
}

TEST_F(MaceMathTest, SmashAttackDamage_SmoothTransition)
{
    // 在分段点附近验证函数连续性（左右极限相等）
    f32 eps = 0.001f;

    // 3.0 附近
    f32 justBelow3 = MaceMath::calculateSmashAttackDamage(3.0f - eps);
    f32 at3 = MaceMath::calculateSmashAttackDamage(3.0f);
    EXPECT_NEAR(justBelow3, at3, 0.01f); // 允许微小误差

    // 8.0 附近
    f32 justBelow8 = MaceMath::calculateSmashAttackDamage(8.0f - eps);
    f32 at8 = MaceMath::calculateSmashAttackDamage(8.0f);
    EXPECT_NEAR(justBelow8, at8, 0.01f);
}

// --- isSmashAttackFallDistance 测试 ---

TEST_F(MaceMathTest, IsSmashAttackFallDistance)
{
    // 不满足条件
    EXPECT_FALSE(MaceMath::isSmashAttackFallDistance(0.0f));
    EXPECT_FALSE(MaceMath::isSmashAttackFallDistance(1.0f));
    EXPECT_FALSE(MaceMath::isSmashAttackFallDistance(1.5f)); // 边界：等于阈值不算

    // 满足条件
    EXPECT_TRUE(MaceMath::isSmashAttackFallDistance(1.51f));
    EXPECT_TRUE(MaceMath::isSmashAttackFallDistance(2.0f));
    EXPECT_TRUE(MaceMath::isSmashAttackFallDistance(5.0f));
    EXPECT_TRUE(MaceMath::isSmashAttackFallDistance(20.0f));
}

// --- isHeavySmashAttack 测试 ---

TEST_F(MaceMathTest, IsHeavySmashAttack)
{
    EXPECT_FALSE(MaceMath::isHeavySmashAttack(0.0f));
    EXPECT_FALSE(MaceMath::isHeavySmashAttack(3.0f));
    EXPECT_FALSE(MaceMath::isHeavySmashAttack(5.0f)); // 边界：等于阈值不算

    EXPECT_TRUE(MaceMath::isHeavySmashAttack(5.01f));
    EXPECT_TRUE(MaceMath::isHeavySmashAttack(6.0f));
    EXPECT_TRUE(MaceMath::isHeavySmashAttack(10.0f));
}

// --- calculateSmashAttackKnockbackPower 测试 ---

TEST_F(MaceMathTest, KnockbackPower_ZeroAtBoundary)
{
    // 距离等于半径时无击退
    EXPECT_FLOAT_EQ(MaceMath::calculateSmashAttackKnockbackPower(3.5f, 3.0f, 0.0f), 0.0f);
}

TEST_F(MaceMathTest, KnockbackPower_ZeroBeyondBoundary)
{
    // 距离超过半径时无击退
    EXPECT_FLOAT_EQ(MaceMath::calculateSmashAttackKnockbackPower(4.0f, 3.0f, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(MaceMath::calculateSmashAttackKnockbackPower(10.0f, 3.0f, 0.0f), 0.0f);
}

TEST_F(MaceMathTest, KnockbackPower_NormalHit)
{
    // 正常砸地（非重击），无击退抗性
    // power = (3.5 - 1.0) * 0.7 * 1.0 * (1 - 0) = 2.5 * 0.7 = 1.75
    EXPECT_FLOAT_EQ(MaceMath::calculateSmashAttackKnockbackPower(1.0f, 3.0f, 0.0f), 1.75f);
}

TEST_F(MaceMathTest, KnockbackPower_HeavyHitDoublesPower)
{
    // 重击（下落距离>5），击退翻倍
    // power = (3.5 - 1.0) * 0.7 * 2.0 * (1 - 0) = 2.5 * 0.7 * 2.0 = 3.5
    EXPECT_FLOAT_EQ(MaceMath::calculateSmashAttackKnockbackPower(1.0f, 6.0f, 0.0f), 3.5f);
}

TEST_F(MaceMathTest, KnockbackPower_KnockbackResistanceReduces)
{
    // 50% 击退抗性
    // power = (3.5 - 1.0) * 0.7 * 1.0 * (1 - 0.5) = 1.75 * 0.5 = 0.875
    EXPECT_FLOAT_EQ(MaceMath::calculateSmashAttackKnockbackPower(1.0f, 3.0f, 0.5f), 0.875f);
}

TEST_F(MaceMathTest, KnockbackPower_FullResistanceNoKnockback)
{
    // 100% 击退抗性 = 无击退
    EXPECT_FLOAT_EQ(MaceMath::calculateSmashAttackKnockbackPower(1.0f, 3.0f, 1.0f), 0.0f);
}

TEST_F(MaceMathTest, KnockbackPower_CloseRangeStronger)
{
    // 近距离击退更强
    f32 closeRange = MaceMath::calculateSmashAttackKnockbackPower(0.5f, 3.0f, 0.0f);
    f32 farRange = MaceMath::calculateSmashAttackKnockbackPower(3.0f, 3.0f, 0.0f);
    EXPECT_GT(closeRange, farRange);
}

// --- 常量验证测试 ---

TEST_F(MaceMathTest, ConstantsMatchExpectedValues)
{
    EXPECT_FLOAT_EQ(MaceMath::SMASH_ATTACK_FALL_THRESHOLD, 1.5f);
    EXPECT_FLOAT_EQ(MaceMath::SMASH_ATTACK_HEAVY_THRESHOLD, 5.0f);
    EXPECT_FLOAT_EQ(MaceMath::SMASH_ATTACK_KNOCKBACK_RADIUS, 3.5f);
    EXPECT_FLOAT_EQ(MaceMath::SMASH_ATTACK_KNOCKBACK_POWER, 0.7f);
    EXPECT_FLOAT_EQ(MaceMath::DEFAULT_ATTACK_DAMAGE, 5.0f);
    EXPECT_FLOAT_EQ(MaceMath::DEFAULT_ATTACK_SPEED, -3.4f);
    EXPECT_EQ(MaceMath::MAX_DURABILITY, 250);
}

// --- 致密魔咒伤害加成测试 ---

class DensityEnchantmentTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
    }

    void TearDown() override { EnchantmentRegistry::clear(); }
};

TEST_F(DensityEnchantmentTest, Properties)
{
    DensityEnchantment density;
    EXPECT_EQ(density.id(), "minecraft:density");
    EXPECT_EQ(density.minLevel(), 1);
    EXPECT_EQ(density.maxLevel(), 5);
    EXPECT_EQ(density.type(), EnchantmentType::Weapon);
    EXPECT_EQ(density.rarity(), EnchantmentRarity::Common);
    EXPECT_FALSE(density.isTreasure());
}

TEST_F(DensityEnchantmentTest, DamagePerFallenBlock)
{
    EXPECT_FLOAT_EQ(DensityEnchantment::getDamagePerFallenBlock(1), 0.5f);
    EXPECT_FLOAT_EQ(DensityEnchantment::getDamagePerFallenBlock(2), 1.0f);
    EXPECT_FLOAT_EQ(DensityEnchantment::getDamagePerFallenBlock(3), 1.5f);
    EXPECT_FLOAT_EQ(DensityEnchantment::getDamagePerFallenBlock(4), 2.0f);
    EXPECT_FLOAT_EQ(DensityEnchantment::getDamagePerFallenBlock(5), 2.5f);
}

TEST_F(DensityEnchantmentTest, IncompatibleWithDamageEnchantments)
{
    DensityEnchantment density;
    SharpnessEnchantment sharpness;
    EXPECT_FALSE(density.isCompatibleWith(sharpness));
    SmiteEnchantment smite;
    EXPECT_FALSE(density.isCompatibleWith(smite));
    BaneOfArthropodsEnchantment bane;
    EXPECT_FALSE(density.isCompatibleWith(bane));
}

TEST_F(DensityEnchantmentTest, IncompatibleWithBreach)
{
    DensityEnchantment density;
    BreachEnchantment breach;
    EXPECT_FALSE(density.isCompatibleWith(breach));
}

TEST_F(DensityEnchantmentTest, IncompatibleWithImpaling)
{
    DensityEnchantment density;
    ImpalingEnchantment impaling;
    EXPECT_FALSE(density.isCompatibleWith(impaling));
}

TEST_F(DensityEnchantmentTest, CompatibleWithWindBurst)
{
    DensityEnchantment density;
    WindBurstEnchantment windBurst;
    EXPECT_TRUE(density.isCompatibleWith(windBurst));
}

TEST_F(DensityEnchantmentTest, CombinedDamageWithSmashAttack)
{
    // 致密 V + 3格下落 = 12 + 2.5*3 = 19.5
    f32 baseDamage = MaceMath::calculateSmashAttackDamage(3.0f);
    f32 densityBonus = DensityEnchantment::getDamagePerFallenBlock(5) * 3.0f;
    EXPECT_FLOAT_EQ(baseDamage, 12.0f);
    EXPECT_FLOAT_EQ(densityBonus, 7.5f);
    EXPECT_FLOAT_EQ(baseDamage + densityBonus, 19.5f);

    // 致密 V + 8格下落 = 22 + 2.5*8 = 42
    f32 baseDamage8 = MaceMath::calculateSmashAttackDamage(8.0f);
    f32 densityBonus8 = DensityEnchantment::getDamagePerFallenBlock(5) * 8.0f;
    EXPECT_FLOAT_EQ(baseDamage8, 22.0f);
    EXPECT_FLOAT_EQ(densityBonus8, 20.0f);
    EXPECT_FLOAT_EQ(baseDamage8 + densityBonus8, 42.0f);
}

// --- 破甲附魔测试 ---

class BreachEnchantmentTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
    }

    void TearDown() override { EnchantmentRegistry::clear(); }
};

TEST_F(BreachEnchantmentTest, ArmorEffectivenessModifier)
{
    EXPECT_FLOAT_EQ(BreachEnchantment::getArmorEffectivenessModifier(1), -0.15f);
    EXPECT_FLOAT_EQ(BreachEnchantment::getArmorEffectivenessModifier(2), -0.30f);
    EXPECT_FLOAT_EQ(BreachEnchantment::getArmorEffectivenessModifier(3), -0.45f);
    EXPECT_FLOAT_EQ(BreachEnchantment::getArmorEffectivenessModifier(4), -0.60f);
}

TEST_F(BreachEnchantmentTest, CompatibleWithWindBurst)
{
    BreachEnchantment breach;
    WindBurstEnchantment windBurst;
    EXPECT_TRUE(breach.isCompatibleWith(windBurst));
}

TEST_F(BreachEnchantmentTest, ArmorEffectivenessCombined)
{
    // 20护甲(80%减伤) + 破甲 IV → 有效护甲率 = 0.8 - 0.6 = 0.2
    f32 armorRatio = 0.8f;
    f32 modifier = BreachEnchantment::getArmorEffectivenessModifier(4);
    EXPECT_FLOAT_EQ(std::clamp(armorRatio + modifier, 0.0f, 1.0f), 0.2f);

    // 低护甲 + 破甲 IV 不应低于 0
    f32 lowArmor = 0.1f;
    EXPECT_FLOAT_EQ(std::clamp(lowArmor + modifier, 0.0f, 1.0f), 0.0f);
}

// --- 风爆附魔测试 ---

class WindBurstEnchantmentTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
    }

    void TearDown() override { EnchantmentRegistry::clear(); }
};

TEST_F(WindBurstEnchantmentTest, ExplosionKnockbackMultiplier)
{
    EXPECT_FLOAT_EQ(WindBurstEnchantment::getExplosionKnockbackMultiplier(1), 1.2f);
    EXPECT_FLOAT_EQ(WindBurstEnchantment::getExplosionKnockbackMultiplier(2), 1.75f);
    EXPECT_FLOAT_EQ(WindBurstEnchantment::getExplosionKnockbackMultiplier(3), 2.2f);
}

TEST_F(WindBurstEnchantmentTest, IsTreasure)
{
    WindBurstEnchantment wb;
    EXPECT_TRUE(wb.isTreasure());
    EXPECT_FALSE(wb.canVillagerTrade());
}

// --- 互斥性完整性测试 ---

class DamageExclusivityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
    }

    void TearDown() override { EnchantmentRegistry::clear(); }
};

TEST_F(DamageExclusivityTest, SharpnessIncompatibleWithAllDamageGroup)
{
    SharpnessEnchantment sharpness;
    DensityEnchantment density;
    BreachEnchantment breach;
    ImpalingEnchantment impaling;

    EXPECT_FALSE(sharpness.isCompatibleWith(density));
    EXPECT_FALSE(sharpness.isCompatibleWith(breach));
    EXPECT_FALSE(sharpness.isCompatibleWith(impaling));
}

TEST_F(DamageExclusivityTest, ImpalingIncompatibleWithMaceEnchantments)
{
    ImpalingEnchantment impaling;
    DensityEnchantment density;
    BreachEnchantment breach;

    EXPECT_FALSE(impaling.isCompatibleWith(density));
    EXPECT_FALSE(impaling.isCompatibleWith(breach));
}

TEST_F(DamageExclusivityTest, WindBurstCompatibleWithAllDamageGroup)
{
    WindBurstEnchantment wb;
    DensityEnchantment density;
    BreachEnchantment breach;
    SharpnessEnchantment sharpness;

    EXPECT_TRUE(wb.isCompatibleWith(density));
    EXPECT_TRUE(wb.isCompatibleWith(breach));
    EXPECT_TRUE(wb.isCompatibleWith(sharpness));
}

// --- 注册测试 ---

class MaceEnchantmentRegistryTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
    }

    void TearDown() override { EnchantmentRegistry::clear(); }
};

TEST_F(MaceEnchantmentRegistryTest, AllMaceEnchantmentsRegistered)
{
    const Enchantment* density = EnchantmentRegistry::get("minecraft:density");
    ASSERT_NE(density, nullptr);
    EXPECT_EQ(density->maxLevel(), 5);

    const Enchantment* breach = EnchantmentRegistry::get("minecraft:breach");
    ASSERT_NE(breach, nullptr);
    EXPECT_EQ(breach->maxLevel(), 4);

    const Enchantment* windBurst = EnchantmentRegistry::get("minecraft:wind_burst");
    ASSERT_NE(windBurst, nullptr);
    EXPECT_EQ(windBurst->maxLevel(), 3);
    EXPECT_TRUE(windBurst->isTreasure());
}
