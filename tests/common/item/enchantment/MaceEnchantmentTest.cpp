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

using namespace mc;
using namespace mc::item::enchant;

// ============================================================================
// 致密附魔(Density)测试
// ============================================================================

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

TEST_F(DensityEnchantmentTest, CostProgression)
{
    DensityEnchantment density;
    EXPECT_EQ(density.getMinCost(1), 5);
    EXPECT_EQ(density.getMinCost(2), 13);
    EXPECT_EQ(density.getMinCost(3), 21);
    EXPECT_EQ(density.getMinCost(5), 37);
    EXPECT_GT(density.getMaxCost(1), density.getMinCost(1));
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

TEST_F(DensityEnchantmentTest, IncompatibleWithSelf)
{
    DensityEnchantment density1;
    DensityEnchantment density2;
    EXPECT_FALSE(density1.isCompatibleWith(density2));
}

TEST_F(DensityEnchantmentTest, DamageBonusAtVariousFallDistances)
{
    f32 density1 = DensityEnchantment::getDamagePerFallenBlock(1);
    EXPECT_FLOAT_EQ(density1 * 1.5f, 0.75f);
    EXPECT_FLOAT_EQ(density1 * 3.0f, 1.5f);
    EXPECT_FLOAT_EQ(density1 * 8.0f, 4.0f);

    f32 density5 = DensityEnchantment::getDamagePerFallenBlock(5);
    EXPECT_FLOAT_EQ(density5 * 3.0f, 7.5f);
    EXPECT_FLOAT_EQ(density5 * 8.0f, 20.0f);
}

// ============================================================================
// 破甲附魔(Breach)测试
// ============================================================================

class BreachEnchantmentTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
    }

    void TearDown() override { EnchantmentRegistry::clear(); }
};

TEST_F(BreachEnchantmentTest, Properties)
{
    BreachEnchantment breach;
    EXPECT_EQ(breach.id(), "minecraft:breach");
    EXPECT_EQ(breach.minLevel(), 1);
    EXPECT_EQ(breach.maxLevel(), 4);
    EXPECT_EQ(breach.type(), EnchantmentType::Weapon);
    EXPECT_EQ(breach.rarity(), EnchantmentRarity::Uncommon);
    EXPECT_FALSE(breach.isTreasure());
}

TEST_F(BreachEnchantmentTest, ArmorEffectivenessModifier)
{
    EXPECT_FLOAT_EQ(BreachEnchantment::getArmorEffectivenessModifier(1), -0.15f);
    EXPECT_FLOAT_EQ(BreachEnchantment::getArmorEffectivenessModifier(2), -0.30f);
    EXPECT_FLOAT_EQ(BreachEnchantment::getArmorEffectivenessModifier(3), -0.45f);
    EXPECT_FLOAT_EQ(BreachEnchantment::getArmorEffectivenessModifier(4), -0.60f);
}

TEST_F(BreachEnchantmentTest, IncompatibleWithDamageEnchantments)
{
    BreachEnchantment breach;

    SharpnessEnchantment sharpness;
    EXPECT_FALSE(breach.isCompatibleWith(sharpness));

    SmiteEnchantment smite;
    EXPECT_FALSE(breach.isCompatibleWith(smite));

    BaneOfArthropodsEnchantment bane;
    EXPECT_FALSE(breach.isCompatibleWith(bane));
}

TEST_F(BreachEnchantmentTest, IncompatibleWithDensity)
{
    BreachEnchantment breach;
    DensityEnchantment density;
    EXPECT_FALSE(breach.isCompatibleWith(density));
}

TEST_F(BreachEnchantmentTest, IncompatibleWithImpaling)
{
    BreachEnchantment breach;
    ImpalingEnchantment impaling;
    EXPECT_FALSE(breach.isCompatibleWith(impaling));
}

TEST_F(BreachEnchantmentTest, CompatibleWithWindBurst)
{
    BreachEnchantment breach;
    WindBurstEnchantment windBurst;
    EXPECT_TRUE(breach.isCompatibleWith(windBurst));
}

TEST_F(BreachEnchantmentTest, ArmorEffectivenessCombinedWithArmor)
{
    // 20护甲(80%减伤) + 破甲 IV → 有效护甲率 = 0.8 - 0.6 = 0.2
    f32 armorRatio = 0.8f;
    f32 breach4 = BreachEnchantment::getArmorEffectivenessModifier(4);
    EXPECT_FLOAT_EQ(armorRatio + breach4, 0.2f);

    // 10护甲(40%减伤) + 破甲 I → 有效护甲率 = 0.4 - 0.15 = 0.25
    f32 armorRatio2 = 0.4f;
    f32 breach1 = BreachEnchantment::getArmorEffectivenessModifier(1);
    EXPECT_FLOAT_EQ(armorRatio2 + breach1, 0.25f);
}

// ============================================================================
// 风爆附魔(Wind Burst)测试
// ============================================================================

class WindBurstEnchantmentTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
    }

    void TearDown() override { EnchantmentRegistry::clear(); }
};

TEST_F(WindBurstEnchantmentTest, Properties)
{
    WindBurstEnchantment windBurst;
    EXPECT_EQ(windBurst.id(), "minecraft:wind_burst");
    EXPECT_EQ(windBurst.minLevel(), 1);
    EXPECT_EQ(windBurst.maxLevel(), 3);
    EXPECT_EQ(windBurst.type(), EnchantmentType::Weapon);
    EXPECT_EQ(windBurst.rarity(), EnchantmentRarity::Rare);
    EXPECT_TRUE(windBurst.isTreasure());
    EXPECT_FALSE(windBurst.canVillagerTrade());
    EXPECT_TRUE(windBurst.canGenerateInLoot());
}

TEST_F(WindBurstEnchantmentTest, ExplosionKnockbackMultiplier)
{
    EXPECT_FLOAT_EQ(WindBurstEnchantment::getExplosionKnockbackMultiplier(1), 1.2f);
    EXPECT_FLOAT_EQ(WindBurstEnchantment::getExplosionKnockbackMultiplier(2), 1.75f);
    EXPECT_FLOAT_EQ(WindBurstEnchantment::getExplosionKnockbackMultiplier(3), 2.2f);
}

TEST_F(WindBurstEnchantmentTest, ExplosionInteractionRange)
{
    EXPECT_FLOAT_EQ(WindBurstEnchantment::getExplosionInteractionRange(), 3.5f);
}

TEST_F(WindBurstEnchantmentTest, CompatibleWithDensity)
{
    WindBurstEnchantment windBurst;
    DensityEnchantment density;
    EXPECT_TRUE(windBurst.isCompatibleWith(density));
}

TEST_F(WindBurstEnchantmentTest, CompatibleWithBreach)
{
    WindBurstEnchantment windBurst;
    BreachEnchantment breach;
    EXPECT_TRUE(windBurst.isCompatibleWith(breach));
}

TEST_F(WindBurstEnchantmentTest, CompatibleWithDamageEnchantments)
{
    WindBurstEnchantment windBurst;

    SharpnessEnchantment sharpness;
    EXPECT_TRUE(windBurst.isCompatibleWith(sharpness));

    SmiteEnchantment smite;
    EXPECT_TRUE(windBurst.isCompatibleWith(smite));
}

// ============================================================================
// 穿刺附魔(Impaling)与重锤附魔互斥性测试
// ============================================================================

class ImpalingMaceExclusivityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
    }

    void TearDown() override { EnchantmentRegistry::clear(); }
};

TEST_F(ImpalingMaceExclusivityTest, IncompatibleWithDensity)
{
    ImpalingEnchantment impaling;
    DensityEnchantment density;
    EXPECT_FALSE(impaling.isCompatibleWith(density));
}

TEST_F(ImpalingMaceExclusivityTest, IncompatibleWithBreach)
{
    ImpalingEnchantment impaling;
    BreachEnchantment breach;
    EXPECT_FALSE(impaling.isCompatibleWith(breach));
}

TEST_F(ImpalingMaceExclusivityTest, IncompatibleWithDamageEnchantments)
{
    ImpalingEnchantment impaling;

    SharpnessEnchantment sharpness;
    EXPECT_FALSE(impaling.isCompatibleWith(sharpness));

    SmiteEnchantment smite;
    EXPECT_FALSE(impaling.isCompatibleWith(smite));

    BaneOfArthropodsEnchantment bane;
    EXPECT_FALSE(impaling.isCompatibleWith(bane));
}

// ============================================================================
// 伤害附魔基类与重锤附魔互斥性测试
// ============================================================================

class DamageEnchantmentMaceExclusivityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
    }

    void TearDown() override { EnchantmentRegistry::clear(); }
};

TEST_F(DamageEnchantmentMaceExclusivityTest, SharpnessIncompatibleWithMaceEnchantments)
{
    SharpnessEnchantment sharpness;

    DensityEnchantment density;
    EXPECT_FALSE(sharpness.isCompatibleWith(density));

    BreachEnchantment breach;
    EXPECT_FALSE(sharpness.isCompatibleWith(breach));

    ImpalingEnchantment impaling;
    EXPECT_FALSE(sharpness.isCompatibleWith(impaling));
}

TEST_F(DamageEnchantmentMaceExclusivityTest, SmiteIncompatibleWithMaceEnchantments)
{
    SmiteEnchantment smite;

    DensityEnchantment density;
    EXPECT_FALSE(smite.isCompatibleWith(density));

    BreachEnchantment breach;
    EXPECT_FALSE(smite.isCompatibleWith(breach));
}

TEST_F(DamageEnchantmentMaceExclusivityTest, BaneOfArthropodsIncompatibleWithMaceEnchantments)
{
    BaneOfArthropodsEnchantment bane;

    DensityEnchantment density;
    EXPECT_FALSE(bane.isCompatibleWith(density));

    BreachEnchantment breach;
    EXPECT_FALSE(bane.isCompatibleWith(breach));
}

// ============================================================================
// 注册测试
// ============================================================================

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
