#include <gtest/gtest.h>

#include "item/enchantment/Enchantment.hpp"
#include "item/enchantment/EnchantmentRegistry.hpp"
#include "item/enchantment/EnchantmentHelper.hpp"
#include "item/enchantment/EnchantmentContainer.hpp"
#include "item/enchantment/enchantments/FortuneEnchantment.hpp"
#include "item/enchantment/enchantments/SilkTouchEnchantment.hpp"
#include "item/core/ItemStack.hpp"
#include "item/Items.hpp"
#include "util/math/random/Random.hpp"

using namespace mc;
using namespace mc::item::enchant;

// ============================================================================
// EnchantmentRegistry 测试
// ============================================================================

class EnchantmentRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
    }

    void TearDown() override {
        EnchantmentRegistry::clear();
    }
};

TEST_F(EnchantmentRegistryTest, InitializeRegistersEnchantments) {
    EXPECT_TRUE(EnchantmentRegistry::isInitialized());
    EXPECT_GT(EnchantmentRegistry::all().size(), 0u);
}

TEST_F(EnchantmentRegistryTest, GetFortuneEnchantment) {
    const Enchantment* fortune = EnchantmentRegistry::get("minecraft:fortune");
    ASSERT_NE(fortune, nullptr);
    EXPECT_EQ(fortune->id(), "minecraft:fortune");
    EXPECT_EQ(fortune->minLevel(), 1);
    EXPECT_EQ(fortune->maxLevel(), 3);
    EXPECT_EQ(fortune->type(), EnchantmentType::Digger);
    EXPECT_EQ(fortune->rarity(), EnchantmentRarity::Rare);
}

TEST_F(EnchantmentRegistryTest, GetSilkTouchEnchantment) {
    const Enchantment* silkTouch = EnchantmentRegistry::get("minecraft:silk_touch");
    ASSERT_NE(silkTouch, nullptr);
    EXPECT_EQ(silkTouch->id(), "minecraft:silk_touch");
    EXPECT_EQ(silkTouch->minLevel(), 1);
    EXPECT_EQ(silkTouch->maxLevel(), 1);  // 精准采集只有 I 级
    EXPECT_EQ(silkTouch->type(), EnchantmentType::Digger);
    EXPECT_EQ(silkTouch->rarity(), EnchantmentRarity::VeryRare);
}

TEST_F(EnchantmentRegistryTest, GetNonExistentEnchantment) {
    const Enchantment* enchant = EnchantmentRegistry::get("minecraft:nonexistent");
    EXPECT_EQ(enchant, nullptr);
}

TEST_F(EnchantmentRegistryTest, HasEnchantment) {
    EXPECT_TRUE(EnchantmentRegistry::has("minecraft:fortune"));
    EXPECT_TRUE(EnchantmentRegistry::has("minecraft:silk_touch"));
    EXPECT_FALSE(EnchantmentRegistry::has("minecraft:nonexistent"));
}

TEST_F(EnchantmentRegistryTest, DoubleInitializeIsSafe) {
    // 第二次初始化应该安全但不会重复注册
    size_t count = EnchantmentRegistry::all().size();
    EnchantmentRegistry::initialize();
    EXPECT_EQ(EnchantmentRegistry::all().size(), count);
}

TEST_F(EnchantmentRegistryTest, InitializeDoesNotThrow) {
    EnchantmentRegistry::clear();

    EXPECT_NO_THROW(EnchantmentRegistry::initialize());
    EXPECT_TRUE(EnchantmentRegistry::isInitialized());
    EXPECT_TRUE(EnchantmentRegistry::has("minecraft:protection"));
}

// ============================================================================
// FortuneEnchantment 测试
// ============================================================================

TEST(FortuneEnchantmentTest, GetMinCost) {
    FortuneEnchantment fortune;

    EXPECT_EQ(fortune.getMinCost(1), 15);  // 15 + (1-1) * 9 = 15
    EXPECT_EQ(fortune.getMinCost(2), 24);  // 15 + (2-1) * 9 = 24
    EXPECT_EQ(fortune.getMinCost(3), 33);  // 15 + (3-1) * 9 = 33
}

TEST(FortuneEnchantmentTest, GetMaxCost) {
    FortuneEnchantment fortune;

    EXPECT_EQ(fortune.getMaxCost(1), 65);  // 15 + 50 = 65
    EXPECT_EQ(fortune.getMaxCost(2), 74);  // 24 + 50 = 74
    EXPECT_EQ(fortune.getMaxCost(3), 83);  // 33 + 50 = 83
}

TEST(FortuneEnchantmentTest, IsIncompatibleWithSilkTouch) {
    FortuneEnchantment fortune;
    SilkTouchEnchantment silkTouch;

    EXPECT_FALSE(fortune.isCompatibleWith(silkTouch));
    EXPECT_FALSE(silkTouch.isCompatibleWith(fortune));
}

TEST(FortuneEnchantmentTest, ApplyBonus) {
    math::Random random(12345);

    // Fortune 0: 无加成
    EXPECT_EQ(FortuneEnchantment::applyBonus(1, 0, random), 1);

    // Fortune I-III: 有概率加成
    // 注意：由于随机性，只能测试范围
    for (int level = 1; level <= 3; ++level) {
        math::Random testRandom(12345);
        i32 result = FortuneEnchantment::applyBonus(1, level, testRandom);
        EXPECT_GE(result, 1);
        EXPECT_LE(result, 1 + level);
    }
}

TEST(FortuneEnchantmentTest, ApplyUniformBonus) {
    math::Random random(12345);

    // Fortune I: 0-1
    i32 result1 = FortuneEnchantment::applyUniformBonus(1, random);
    EXPECT_GE(result1, 0);
    EXPECT_LE(result1, 1);

    // Fortune III: 0-3
    i32 result3 = FortuneEnchantment::applyUniformBonus(3, random);
    EXPECT_GE(result3, 0);
    EXPECT_LE(result3, 3);

    // Fortune 0: 0
    EXPECT_EQ(FortuneEnchantment::applyUniformBonus(0, random), 0);
}

TEST(FortuneEnchantmentTest, ApplyOreDropBonus) {
    math::Random random(12345);

    // 基础掉落 4-5，Fortune III
    i32 result = FortuneEnchantment::applyOreDropBonus(4, 5, 3, random);
    EXPECT_GE(result, 4);  // 至少基础掉落
    EXPECT_LE(result, 8);  // 最多基础 + 3
}

// ============================================================================
// SilkTouchEnchantment 测试
// ============================================================================

TEST(SilkTouchEnchantmentTest, Properties) {
    SilkTouchEnchantment silkTouch;

    EXPECT_EQ(silkTouch.id(), "minecraft:silk_touch");
    EXPECT_EQ(silkTouch.minLevel(), 1);
    EXPECT_EQ(silkTouch.maxLevel(), 1);
    EXPECT_EQ(silkTouch.type(), EnchantmentType::Digger);
    EXPECT_EQ(silkTouch.rarity(), EnchantmentRarity::VeryRare);
}

TEST(SilkTouchEnchantmentTest, GetMinCost) {
    SilkTouchEnchantment silkTouch;

    EXPECT_EQ(silkTouch.getMinCost(1), 15);
    EXPECT_EQ(silkTouch.getMaxCost(1), 61);  // MC 1.16.5: 15 + 50 - 4 = 61
}

// ============================================================================
// EnchantmentContainer 测试
// ============================================================================

class EnchantmentContainerTest : public ::testing::Test {
protected:
    void SetUp() override {
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
    }

    void TearDown() override {
        EnchantmentRegistry::clear();
    }
};

TEST_F(EnchantmentContainerTest, EmptyContainer) {
    EnchantmentContainer container;

    EXPECT_TRUE(container.isEmpty());
    EXPECT_EQ(container.size(), 0u);
    EXPECT_EQ(container.getLevel("minecraft:fortune"), 0);
    EXPECT_FALSE(container.has("minecraft:fortune"));
}

TEST_F(EnchantmentContainerTest, SetAndGetEnchantment) {
    EnchantmentContainer container;

    container.set("minecraft:fortune", 3);

    EXPECT_FALSE(container.isEmpty());
    EXPECT_EQ(container.size(), 1u);
    EXPECT_EQ(container.getLevel("minecraft:fortune"), 3);
    EXPECT_TRUE(container.has("minecraft:fortune"));
}

TEST_F(EnchantmentContainerTest, UpdateEnchantment) {
    EnchantmentContainer container;

    container.set("minecraft:fortune", 1);
    EXPECT_EQ(container.getLevel("minecraft:fortune"), 1);

    container.set("minecraft:fortune", 3);
    EXPECT_EQ(container.getLevel("minecraft:fortune"), 3);
}

TEST_F(EnchantmentContainerTest, RemoveEnchantment) {
    EnchantmentContainer container;

    container.set("minecraft:fortune", 3);
    EXPECT_TRUE(container.has("minecraft:fortune"));

    EXPECT_TRUE(container.remove("minecraft:fortune"));
    EXPECT_FALSE(container.has("minecraft:fortune"));
    EXPECT_EQ(container.getLevel("minecraft:fortune"), 0);

    // 再次移除返回 false
    EXPECT_FALSE(container.remove("minecraft:fortune"));
}

TEST_F(EnchantmentContainerTest, ClearEnchantments) {
    EnchantmentContainer container;

    container.set("minecraft:fortune", 3);
    container.set("minecraft:unbreaking", 2);

    EXPECT_EQ(container.size(), 2u);

    container.clear();

    EXPECT_TRUE(container.isEmpty());
    EXPECT_EQ(container.size(), 0u);
}

TEST_F(EnchantmentContainerTest, MultipleEnchantments) {
    EnchantmentContainer container;

    container.set("minecraft:fortune", 3);
    container.set("minecraft:unbreaking", 2);
    container.set("minecraft:efficiency", 5);

    EXPECT_EQ(container.size(), 3u);
    EXPECT_EQ(container.getLevel("minecraft:fortune"), 3);
    EXPECT_EQ(container.getLevel("minecraft:unbreaking"), 2);
    EXPECT_EQ(container.getLevel("minecraft:efficiency"), 5);
}

TEST_F(EnchantmentContainerTest, CompatibilityCheck) {
    EnchantmentContainer container;

    // 添加时运
    container.set("minecraft:fortune", 3);

    // 精准采集与时运互斥
    EXPECT_FALSE(container.canAdd("minecraft:silk_touch"));

    // 注意：耐久附魔还未注册，所以 canAdd 会返回 false
    // 当耐久附魔注册后，这个测试应该改为 EXPECT_TRUE
    // EXPECT_TRUE(container.canAdd("minecraft:unbreaking"));
}

TEST_F(EnchantmentContainerTest, HasEnchantmentType) {
    EnchantmentContainer container;

    container.set("minecraft:fortune", 3);

    EXPECT_TRUE(container.hasType(EnchantmentType::Digger));
    EXPECT_FALSE(container.hasType(EnchantmentType::Weapon));
}

// ============================================================================
// Enchantment 测试
// ============================================================================

TEST(EnchantmentTest, RarityWeight) {
    EXPECT_EQ(Enchantment::getRarityWeight(EnchantmentRarity::Common), 10);
    EXPECT_EQ(Enchantment::getRarityWeight(EnchantmentRarity::Uncommon), 5);
    EXPECT_EQ(Enchantment::getRarityWeight(EnchantmentRarity::Rare), 2);
    EXPECT_EQ(Enchantment::getRarityWeight(EnchantmentRarity::VeryRare), 1);
}

TEST(EnchantmentTest, GetNameKey) {
    FortuneEnchantment fortune;

    EXPECT_EQ(fortune.getNameKey(), "enchantment.minecraft:fortune");
    EXPECT_EQ(fortune.getNameKey(3), "enchantment.minecraft:fortune");
}

// ============================================================================
// EnchantmentInstance 测试
// ============================================================================

TEST(EnchantmentInstanceTest, GetEnchantment) {
    EnchantmentRegistry::clear();
    EnchantmentRegistry::initialize();

    EnchantmentInstance instance("minecraft:fortune", 3);

    EXPECT_EQ(instance.enchantmentId, "minecraft:fortune");
    EXPECT_EQ(instance.level, 3);

    const Enchantment* enchant = instance.getEnchantment();
    ASSERT_NE(enchant, nullptr);
    EXPECT_EQ(enchant->id(), "minecraft:fortune");

    EnchantmentRegistry::clear();
}
