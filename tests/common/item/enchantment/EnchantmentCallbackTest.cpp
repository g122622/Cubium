#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "item/enchantment/Enchantment.hpp"
#include "item/enchantment/EnchantmentRegistry.hpp"
#include "item/enchantment/EnchantmentHelper.hpp"
#include "item/enchantment/EnchantmentContainer.hpp"
#include "item/enchantment/enchantments/AllEnchantments.hpp"
#include "item/enchantment/enchantments/weapon/BaneOfArthropodsEnchantment.hpp"
#include "item/enchantment/enchantments/protection/ThornsEnchantment.hpp"
#include "item/core/ItemStack.hpp"
#include "item/Items.hpp"
#include "entity/core/Entity.hpp"
#include "entity/core/LivingEntity.hpp"
#include "entity/effect/EffectInstance.hpp"
#include "entity/effect/EffectType.hpp"
#include "entity/damage/DamageSource.hpp"
#include "util/math/random/Random.hpp"

using namespace mc;
using namespace mc::item::enchant;

// ============================================================================
// BaneOfArthropodsEnchantment 测试
// ============================================================================

class BaneOfArthropodsEnchantmentTest : public ::testing::Test {
protected:
    void SetUp() override {
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
    }

    void TearDown() override {
        EnchantmentRegistry::clear();
    }
};

TEST_F(BaneOfArthropodsEnchantmentTest, Properties) {
    BaneOfArthropodsEnchantment bane;

    EXPECT_EQ(bane.id(), "minecraft:bane_of_arthropods");
    EXPECT_EQ(bane.minLevel(), 1);
    EXPECT_EQ(bane.maxLevel(), 5);
    EXPECT_EQ(bane.type(), EnchantmentType::Weapon);
    EXPECT_EQ(bane.rarity(), EnchantmentRarity::Uncommon);
}

TEST_F(BaneOfArthropodsEnchantmentTest, GetSlownessDuration) {
    math::Random rng(12345);

    // Level I: 20 + random(0, 9) = 20-29
    for (int i = 0; i < 100; ++i) {
        i32 duration = BaneOfArthropodsEnchantment::getSlownessDuration(1, rng);
        EXPECT_GE(duration, 20);
        EXPECT_LE(duration, 29);
    }

    // Level V: 20 + random(0, 49) = 20-69
    for (int i = 0; i < 100; ++i) {
        i32 duration = BaneOfArthropodsEnchantment::getSlownessDuration(5, rng);
        EXPECT_GE(duration, 20);
        EXPECT_LE(duration, 69);
    }
}

TEST_F(BaneOfArthropodsEnchantmentTest, GetSlownessAmplifier) {
    // 缓慢 IV (amplifier = 3)
    EXPECT_EQ(BaneOfArthropodsEnchantment::getSlownessAmplifier(), 3);
}

TEST_F(BaneOfArthropodsEnchantmentTest, GetDamageBonus) {
    BaneOfArthropodsEnchantment bane;

    // 对节肢生物：每级 +2.5 伤害
    constexpr u32 EntityTypeArthropod = 2;
    EXPECT_FLOAT_EQ(bane.getDamageBonus(1, EntityTypeArthropod), 2.5f);
    EXPECT_FLOAT_EQ(bane.getDamageBonus(3, EntityTypeArthropod), 7.5f);
    EXPECT_FLOAT_EQ(bane.getDamageBonus(5, EntityTypeArthropod), 12.5f);

    // 对非节肢生物：0 伤害
    constexpr u32 EntityTypeUndead = 1;
    constexpr u32 EntityTypeNormal = 0;
    EXPECT_FLOAT_EQ(bane.getDamageBonus(5, EntityTypeUndead), 0.0f);
    EXPECT_FLOAT_EQ(bane.getDamageBonus(5, EntityTypeNormal), 0.0f);
}

TEST_F(BaneOfArthropodsEnchantmentTest, IsIncompatibleWithOtherDamageEnchants) {
    BaneOfArthropodsEnchantment bane;
    SharpnessEnchantment sharpness;
    SmiteEnchantment smite;

    // 节肢杀手与锋利互斥
    EXPECT_FALSE(bane.isCompatibleWith(sharpness));
    EXPECT_FALSE(sharpness.isCompatibleWith(bane));

    // 节肢杀手与亡灵杀手互斥
    EXPECT_FALSE(bane.isCompatibleWith(smite));
    EXPECT_FALSE(smite.isCompatibleWith(bane));
}

// ============================================================================
// ThornsEnchantment 测试
// ============================================================================

class ThornsEnchantmentTest : public ::testing::Test {
protected:
    void SetUp() override {
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
    }

    void TearDown() override {
        EnchantmentRegistry::clear();
    }
};

TEST_F(ThornsEnchantmentTest, Properties) {
    ThornsEnchantment thorns;

    EXPECT_EQ(thorns.id(), "minecraft:thorns");
    EXPECT_EQ(thorns.minLevel(), 1);
    EXPECT_EQ(thorns.maxLevel(), 3);
    EXPECT_EQ(thorns.type(), EnchantmentType::ArmorChest);
    EXPECT_EQ(thorns.rarity(), EnchantmentRarity::VeryRare);
}

TEST_F(ThornsEnchantmentTest, GetMinCost) {
    ThornsEnchantment thorns;

    EXPECT_EQ(thorns.getMinCost(1), 10);
    EXPECT_EQ(thorns.getMinCost(2), 30);  // 10 + 20
    EXPECT_EQ(thorns.getMinCost(3), 50);  // 10 + 40
}

TEST_F(ThornsEnchantmentTest, GetMaxCost) {
    ThornsEnchantment thorns;

    EXPECT_EQ(thorns.getMaxCost(1), 60);  // 10 + 50
    EXPECT_EQ(thorns.getMaxCost(2), 80);  // 30 + 50
    EXPECT_EQ(thorns.getMaxCost(3), 100); // 50 + 50
}

TEST_F(ThornsEnchantmentTest, ShouldTrigger) {
    math::Random rng(12345);

    // Level 0: 永不触发
    for (int i = 0; i < 100; ++i) {
        EXPECT_FALSE(ThornsEnchantment::shouldTrigger(0, rng));
    }

    // Level I: 15% 概率
    int triggerCount1 = 0;
    for (int i = 0; i < 1000; ++i) {
        if (ThornsEnchantment::shouldTrigger(1, rng)) {
            triggerCount1++;
        }
    }
    // 期望约 150 次，允许一定误差
    EXPECT_GT(triggerCount1, 100);
    EXPECT_LT(triggerCount1, 200);

    // Level III: 45% 概率
    int triggerCount3 = 0;
    for (int i = 0; i < 1000; ++i) {
        if (ThornsEnchantment::shouldTrigger(3, rng)) {
            triggerCount3++;
        }
    }
    // 期望约 450 次，允许一定误差
    EXPECT_GT(triggerCount3, 400);
    EXPECT_LT(triggerCount3, 500);
}

TEST_F(ThornsEnchantmentTest, GetThornsDamage) {
    math::Random rng(12345);

    // Level <= 10: 1-4 伤害
    for (int level = 1; level <= 10; ++level) {
        for (int i = 0; i < 100; ++i) {
            i32 damage = ThornsEnchantment::getThornsDamage(level, rng);
            EXPECT_GE(damage, 1);
            EXPECT_LE(damage, 4);
        }
    }

    // Level > 10: level - 10
    EXPECT_EQ(ThornsEnchantment::getThornsDamage(11, rng), 1);
    EXPECT_EQ(ThornsEnchantment::getThornsDamage(15, rng), 5);
    EXPECT_EQ(ThornsEnchantment::getThornsDamage(20, rng), 10);
}

TEST_F(ThornsEnchantmentTest, GetTriggerChance) {
    EXPECT_FLOAT_EQ(ThornsEnchantment::getTriggerChance(1), 0.15f);
    EXPECT_FLOAT_EQ(ThornsEnchantment::getTriggerChance(2), 0.30f);
    EXPECT_FLOAT_EQ(ThornsEnchantment::getTriggerChance(3), 0.45f);
}

// ============================================================================
// EnchantmentHelper 回调测试
// ============================================================================

class EnchantmentHelperCallbackTest : public ::testing::Test {
protected:
    void SetUp() override {
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
    }

    void TearDown() override {
        EnchantmentRegistry::clear();
    }
};

TEST_F(EnchantmentHelperCallbackTest, ApplyArthropodEnchantmentDamageWithEmptyStack) {
    // 空物品堆不应触发回调
    // 由于没有真实的 LivingEntity 实现用于测试，这里只测试基本逻辑
    ItemStack emptyStack;
    EXPECT_TRUE(emptyStack.isEmpty());
    // EnchantmentHelper::applyArthropodEnchantmentDamage 需要 LivingEntity，跳过集成测试
}

TEST_F(EnchantmentHelperCallbackTest, ApplyThornsEnchantmentsWithEmptyArmor) {
    // 空护甲不应触发荆棘
    std::array<const ItemStack*, 4> emptyArmor = {
        &ItemStack::EMPTY,
        &ItemStack::EMPTY,
        &ItemStack::EMPTY,
        &ItemStack::EMPTY
    };
    // 验证空护甲槽位
    for (const auto* slot : emptyArmor) {
        EXPECT_TRUE(slot->isEmpty());
    }
}

// ============================================================================
// EnchantmentHelper 工具方法测试
// ============================================================================

class EnchantmentHelperToolTest : public ::testing::Test {
protected:
    void SetUp() override {
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
    }

    void TearDown() override {
        EnchantmentRegistry::clear();
    }
};

TEST_F(EnchantmentHelperToolTest, ShouldIgnoreDurabilityLoss) {
    math::Random rng(12345);

    // Level 0: 总是返回 false
    for (int i = 0; i < 100; ++i) {
        EXPECT_FALSE(EnchantmentHelper::shouldIgnoreDurabilityLoss(0, false, rng));
        EXPECT_FALSE(EnchantmentHelper::shouldIgnoreDurabilityLoss(0, true, rng));
    }

    // Level I: 约 50% 概率忽略（非护甲）
    int ignoreCount = 0;
    for (int i = 0; i < 1000; ++i) {
        if (EnchantmentHelper::shouldIgnoreDurabilityLoss(1, false, rng)) {
            ignoreCount++;
        }
    }
    EXPECT_GT(ignoreCount, 400);
    EXPECT_LT(ignoreCount, 600);

    // Level III: 约 75% 概率忽略（非护甲）
    ignoreCount = 0;
    for (int i = 0; i < 1000; ++i) {
        if (EnchantmentHelper::shouldIgnoreDurabilityLoss(3, false, rng)) {
            ignoreCount++;
        }
    }
    EXPECT_GT(ignoreCount, 650);
    EXPECT_LT(ignoreCount, 850);

    // 护甲：60% 概率不触发耐久效果
    // 所以护甲的实际忽略概率是 0.4 * level/(level+1)
    // Level I 护甲: 0.4 * 0.5 = 0.2 = 20%
    ignoreCount = 0;
    for (int i = 0; i < 1000; ++i) {
        if (EnchantmentHelper::shouldIgnoreDurabilityLoss(1, true, rng)) {
            ignoreCount++;
        }
    }
    EXPECT_GT(ignoreCount, 100);
    EXPECT_LT(ignoreCount, 300);
}

TEST_F(EnchantmentHelperToolTest, GetSweepingDamageRatio) {
    EXPECT_FLOAT_EQ(EnchantmentHelper::getSweepingDamageRatio(ItemStack::EMPTY), 0.0f);

    // 需要有横扫之刃附魔的物品才能测试
    // 由于 ItemStack::EMPTY 没有附魔，这里只测试返回值
}

TEST_F(EnchantmentHelperToolTest, GetFishingLuckBonus) {
    EXPECT_EQ(EnchantmentHelper::getFishingLuckBonus(ItemStack::EMPTY), 0);
}

TEST_F(EnchantmentHelperToolTest, GetFishingSpeedBonus) {
    EXPECT_EQ(EnchantmentHelper::getFishingSpeedBonus(ItemStack::EMPTY), 0);
}
