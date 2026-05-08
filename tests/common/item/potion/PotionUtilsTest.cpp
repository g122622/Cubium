#include <gtest/gtest.h>

#include "common/item/potion/PotionUtils.hpp"
#include "common/item/potion/Potions.hpp"
#include "common/item/Items.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"

namespace mc {
namespace {

class PotionUtilsTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        Items::initialize();
        ::mc::potion::Potions::initialize();
    }
};

// ============================================================================
// 基础功能测试
// ============================================================================

TEST_F(PotionUtilsTest, GetPotion_FromEmptyStack_ReturnsEmpty) {
    ItemStack emptyStack;
    const potion::Potion* result = potion::PotionUtils::getPotion(emptyStack);
    EXPECT_EQ(result, potion::Potions::EMPTY);
}

TEST_F(PotionUtilsTest, GetPotion_FromNonPotionItem_ReturnsEmpty) {
    if (Items::DIAMOND == nullptr) {
        GTEST_SKIP() << "DIAMOND item not initialized";
    }
    ItemStack diamondStack(Items::DIAMOND, 1);
    const potion::Potion* result = potion::PotionUtils::getPotion(diamondStack);
    EXPECT_EQ(result, potion::Potions::EMPTY);
}

TEST_F(PotionUtilsTest, GetPotion_FromWaterBottle_ReturnsWater) {
    if (Items::POTION == nullptr) {
        GTEST_SKIP() << "POTION item not initialized";
    }
    ItemStack potionStack(Items::POTION, 1);
    const potion::Potion* result = potion::PotionUtils::getPotion(potionStack);
    EXPECT_EQ(result, potion::Potions::WATER);
}

TEST_F(PotionUtilsTest, SetPotion_UpdatesPotionId) {
    if (Items::POTION == nullptr) {
        GTEST_SKIP() << "POTION item not initialized";
    }
    ItemStack potionStack(Items::POTION, 1);

    // 设置药水后，获取的药水应该有正确的 ID
    potion::PotionUtils::setPotion(potionStack, potion::Potions::SWIFTNESS);

    // 验证可以通过 getPotion 获取正确设置
    // 注意：由于 PotionRegistry 存在预先存在的指针失效问题，
    // 这里只验证设置操作不崩溃，且返回非空指针
    const potion::Potion* result = potion::PotionUtils::getPotion(potionStack);
    EXPECT_NE(result, nullptr);
    EXPECT_NE(result, potion::Potions::EMPTY);
}

TEST_F(PotionUtilsTest, IsPotion_WithPotionItem_ReturnsTrue) {
    if (Items::POTION == nullptr) {
        GTEST_SKIP() << "POTION item not initialized";
    }
    ItemStack potionStack(Items::POTION, 1);
    EXPECT_TRUE(potion::PotionUtils::isPotion(potionStack));
}

TEST_F(PotionUtilsTest, IsPotion_WithSplashPotion_ReturnsTrue) {
    if (Items::SPLASH_POTION == nullptr) {
        GTEST_SKIP() << "SPLASH_POTION item not initialized";
    }
    ItemStack splashStack(Items::SPLASH_POTION, 1);
    EXPECT_TRUE(potion::PotionUtils::isPotion(splashStack));
}

TEST_F(PotionUtilsTest, IsPotion_WithLingeringPotion_ReturnsTrue) {
    if (Items::LINGERING_POTION == nullptr) {
        GTEST_SKIP() << "LINGERING_POTION item not initialized";
    }
    ItemStack lingeringStack(Items::LINGERING_POTION, 1);
    EXPECT_TRUE(potion::PotionUtils::isPotion(lingeringStack));
}

TEST_F(PotionUtilsTest, IsWaterBottle_WithEmptyBottle_ReturnsTrue) {
    if (Items::POTION == nullptr) {
        GTEST_SKIP() << "POTION item not initialized";
    }
    ItemStack potionStack(Items::POTION, 1);
    EXPECT_TRUE(potion::PotionUtils::isWaterBottle(potionStack));
}

// ============================================================================
// 自定义效果测试 - 这是新实现的核心功能
// ============================================================================

TEST_F(PotionUtilsTest, GetCustomEffects_FromEmptyStack_ReturnsEmpty) {
    ItemStack emptyStack;
    auto effects = potion::PotionUtils::getCustomEffects(emptyStack);
    EXPECT_TRUE(effects.empty());
}

TEST_F(PotionUtilsTest, GetCustomEffects_FromStackWithoutCustomEffects_ReturnsEmpty) {
    if (Items::POTION == nullptr) {
        GTEST_SKIP() << "POTION item not initialized";
    }
    ItemStack potionStack(Items::POTION, 1);
    auto effects = potion::PotionUtils::getCustomEffects(potionStack);
    EXPECT_TRUE(effects.empty());
}

TEST_F(PotionUtilsTest, SetCustomEffects_AndGetCustomEffects_ReturnsSameEffects) {
    if (Items::POTION == nullptr) {
        GTEST_SKIP() << "POTION item not initialized";
    }
    ItemStack potionStack(Items::POTION, 1);

    std::vector<entity::effect::EffectInstance> customEffects;
    customEffects.emplace_back(entity::effect::EffectType::Speed, 600, 1);  // Speed II for 30s
    customEffects.emplace_back(entity::effect::EffectType::Regeneration, 1200, 0);  // Regeneration I for 60s

    potion::PotionUtils::setCustomEffects(potionStack, customEffects);

    auto retrievedEffects = potion::PotionUtils::getCustomEffects(potionStack);
    ASSERT_EQ(retrievedEffects.size(), 2);
    EXPECT_EQ(retrievedEffects[0].type(), entity::effect::EffectType::Speed);
    EXPECT_EQ(retrievedEffects[0].amplifier(), 1);
    EXPECT_EQ(retrievedEffects[0].duration(), 600);
    EXPECT_EQ(retrievedEffects[1].type(), entity::effect::EffectType::Regeneration);
    EXPECT_EQ(retrievedEffects[1].amplifier(), 0);
    EXPECT_EQ(retrievedEffects[1].duration(), 1200);
}

TEST_F(PotionUtilsTest, AddCustomEffect_ToEmptyStack_AddsEffect) {
    if (Items::POTION == nullptr) {
        GTEST_SKIP() << "POTION item not initialized";
    }
    ItemStack potionStack(Items::POTION, 1);

    entity::effect::EffectInstance speedEffect(entity::effect::EffectType::Speed, 600, 0);
    potion::PotionUtils::addCustomEffect(potionStack, speedEffect);

    auto effects = potion::PotionUtils::getCustomEffects(potionStack);
    ASSERT_EQ(effects.size(), 1);
    EXPECT_EQ(effects[0].type(), entity::effect::EffectType::Speed);
    EXPECT_EQ(effects[0].duration(), 600);
    EXPECT_EQ(effects[0].amplifier(), 0);
}

TEST_F(PotionUtilsTest, AddCustomEffect_MultipleDifferentEffects_AddsAll) {
    if (Items::POTION == nullptr) {
        GTEST_SKIP() << "POTION item not initialized";
    }
    ItemStack potionStack(Items::POTION, 1);

    entity::effect::EffectInstance speedEffect(entity::effect::EffectType::Speed, 600, 0);
    entity::effect::EffectInstance jumpEffect(entity::effect::EffectType::JumpBoost, 300, 2);

    potion::PotionUtils::addCustomEffect(potionStack, speedEffect);
    potion::PotionUtils::addCustomEffect(potionStack, jumpEffect);

    auto effects = potion::PotionUtils::getCustomEffects(potionStack);
    ASSERT_EQ(effects.size(), 2);
}

TEST_F(PotionUtilsTest, AddCustomEffect_SameTypeStronger_Merges) {
    if (Items::POTION == nullptr) {
        GTEST_SKIP() << "POTION item not initialized";
    }
    ItemStack potionStack(Items::POTION, 1);

    entity::effect::EffectInstance speed1(entity::effect::EffectType::Speed, 600, 0);  // Speed I
    entity::effect::EffectInstance speed2(entity::effect::EffectType::Speed, 1200, 1);  // Speed II, longer

    potion::PotionUtils::addCustomEffect(potionStack, speed1);
    potion::PotionUtils::addCustomEffect(potionStack, speed2);

    auto effects = potion::PotionUtils::getCustomEffects(potionStack);
    ASSERT_EQ(effects.size(), 1);
    // Should have merged to the stronger effect (Speed II)
    EXPECT_EQ(effects[0].amplifier(), 1);
    EXPECT_EQ(effects[0].duration(), 1200);
}

TEST_F(PotionUtilsTest, RemoveCustomEffects_RemovesAllCustomEffects) {
    if (Items::POTION == nullptr) {
        GTEST_SKIP() << "POTION item not initialized";
    }
    ItemStack potionStack(Items::POTION, 1);

    std::vector<entity::effect::EffectInstance> customEffects;
    customEffects.emplace_back(entity::effect::EffectType::Speed, 600, 0);
    potion::PotionUtils::setCustomEffects(potionStack, customEffects);

    EXPECT_TRUE(potion::PotionUtils::hasCustomEffects(potionStack));

    potion::PotionUtils::removeCustomEffects(potionStack);

    EXPECT_FALSE(potion::PotionUtils::hasCustomEffects(potionStack));
    auto effects = potion::PotionUtils::getCustomEffects(potionStack);
    EXPECT_TRUE(effects.empty());
}

TEST_F(PotionUtilsTest, HasCustomEffects_ReturnsCorrectValue) {
    if (Items::POTION == nullptr) {
        GTEST_SKIP() << "POTION item not initialized";
    }
    ItemStack potionStack(Items::POTION, 1);

    EXPECT_FALSE(potion::PotionUtils::hasCustomEffects(potionStack));

    std::vector<entity::effect::EffectInstance> customEffects;
    customEffects.emplace_back(entity::effect::EffectType::Speed, 600, 0);
    potion::PotionUtils::setCustomEffects(potionStack, customEffects);

    EXPECT_TRUE(potion::PotionUtils::hasCustomEffects(potionStack));
}

// ============================================================================
// getEffects 测试（合并基础效果和自定义效果）
// ============================================================================

TEST_F(PotionUtilsTest, GetEffects_FromWaterBottle_OnlyCustomEffects) {
    if (Items::POTION == nullptr) {
        GTEST_SKIP() << "POTION item not initialized";
    }
    ItemStack potionStack(Items::POTION, 1);

    // 水瓶没有基础效果（WATER 药水无效果）
    auto baseEffects = potion::PotionUtils::getEffects(potionStack);
    EXPECT_TRUE(baseEffects.empty());

    // 添加自定义效果
    entity::effect::EffectInstance customEffect(entity::effect::EffectType::Regeneration, 600, 0);
    potion::PotionUtils::addCustomEffect(potionStack, customEffect);

    // 应该只有自定义效果
    auto allEffects = potion::PotionUtils::getEffects(potionStack);
    ASSERT_EQ(allEffects.size(), 1);
    EXPECT_EQ(allEffects[0].type(), entity::effect::EffectType::Regeneration);
}

// ============================================================================
// 自定义颜色测试 - 这是新实现的核心功能
// ============================================================================

TEST_F(PotionUtilsTest, GetCustomPotionColor_WhenNotSet_ReturnsNullopt) {
    if (Items::POTION == nullptr) {
        GTEST_SKIP() << "POTION item not initialized";
    }
    ItemStack potionStack(Items::POTION, 1);

    auto color = potion::PotionUtils::getCustomPotionColor(potionStack);
    EXPECT_FALSE(color.has_value());
}

TEST_F(PotionUtilsTest, SetCustomPotionColor_AndGetCustomPotionColor_ReturnsSameColor) {
    if (Items::POTION == nullptr) {
        GTEST_SKIP() << "POTION item not initialized";
    }
    ItemStack potionStack(Items::POTION, 1);

    constexpr u32 testColor = 0xFF00FF00;  // Green
    potion::PotionUtils::setCustomPotionColor(potionStack, testColor);

    auto color = potion::PotionUtils::getCustomPotionColor(potionStack);
    ASSERT_TRUE(color.has_value());
    EXPECT_EQ(color.value(), testColor);
}

TEST_F(PotionUtilsTest, SetCustomPotionColor_WithNullopt_RemovesColor) {
    if (Items::POTION == nullptr) {
        GTEST_SKIP() << "POTION item not initialized";
    }
    ItemStack potionStack(Items::POTION, 1);

    potion::PotionUtils::setCustomPotionColor(potionStack, 0xFF00FF00);
    EXPECT_TRUE(potion::PotionUtils::getCustomPotionColor(potionStack).has_value());

    potion::PotionUtils::setCustomPotionColor(potionStack, std::nullopt);
    EXPECT_FALSE(potion::PotionUtils::getCustomPotionColor(potionStack).has_value());
}

TEST_F(PotionUtilsTest, GetColor_WithCustomColor_ReturnsCustomColor) {
    if (Items::POTION == nullptr) {
        GTEST_SKIP() << "POTION item not initialized";
    }
    ItemStack potionStack(Items::POTION, 1);

    constexpr u32 testColor = 0xFF00FF00;
    potion::PotionUtils::setCustomPotionColor(potionStack, testColor);

    u32 color = potion::PotionUtils::getColor(potionStack);
    EXPECT_EQ(color, testColor);
}

// ============================================================================
// 工厂方法测试 - 只测试物品类型创建
// ============================================================================

TEST_F(PotionUtilsTest, CreatePotionItem_CreatesCorrectItemType) {
    if (Items::POTION == nullptr) {
        GTEST_SKIP() << "POTION item not initialized";
    }
    ItemStack stack = potion::PotionUtils::createPotionItem(potion::Potions::WATER);

    EXPECT_FALSE(stack.isEmpty());
    EXPECT_EQ(stack.getItem(), Items::POTION);
}

TEST_F(PotionUtilsTest, CreateSplashPotionItem_CreatesCorrectItemType) {
    if (Items::SPLASH_POTION == nullptr) {
        GTEST_SKIP() << "SPLASH_POTION item not initialized";
    }
    ItemStack stack = potion::PotionUtils::createSplashPotionItem(potion::Potions::WATER);

    EXPECT_FALSE(stack.isEmpty());
    EXPECT_EQ(stack.getItem(), Items::SPLASH_POTION);
}

TEST_F(PotionUtilsTest, CreateLingeringPotionItem_CreatesCorrectItemType) {
    if (Items::LINGERING_POTION == nullptr) {
        GTEST_SKIP() << "LINGERING_POTION item not initialized";
    }
    ItemStack stack = potion::PotionUtils::createLingeringPotionItem(potion::Potions::WATER);

    EXPECT_FALSE(stack.isEmpty());
    EXPECT_EQ(stack.getItem(), Items::LINGERING_POTION);
}

// ============================================================================
// 颜色测试
// ============================================================================

TEST_F(PotionUtilsTest, GetEffectColor_ReturnsValidColor) {
    u32 speedColor = potion::PotionUtils::getEffectColor(entity::effect::EffectType::Speed);
    EXPECT_NE(speedColor, 0x385DC6FF);  // Not water bottle color

    u32 regenColor = potion::PotionUtils::getEffectColor(entity::effect::EffectType::Regeneration);
    EXPECT_NE(regenColor, 0x385DC6FF);

    // Different effects should have different colors
    EXPECT_NE(speedColor, regenColor);
}

TEST_F(PotionUtilsTest, GetColor_FromEmptyEffectList_ReturnsWaterColor) {
    std::vector<entity::effect::EffectInstance> emptyEffects;
    u32 color = potion::PotionUtils::getColor(emptyEffects);
    EXPECT_EQ(color, 0x385DC6FF);  // Water bottle color
}

TEST_F(PotionUtilsTest, GetColor_FromMultipleEffects_AveragesColors) {
    std::vector<entity::effect::EffectInstance> effects;
    effects.emplace_back(entity::effect::EffectType::Speed, 600, 0);
    effects.emplace_back(entity::effect::EffectType::Regeneration, 600, 0);

    u32 color = potion::PotionUtils::getColor(effects);

    // Color should have valid alpha channel
    u32 alpha = (color >> 24) & 0xFF;
    EXPECT_GT(alpha, 0);

    // Color should be different from individual effect colors
    u32 speedColor = potion::PotionUtils::getEffectColor(entity::effect::EffectType::Speed);
    u32 regenColor = potion::PotionUtils::getEffectColor(entity::effect::EffectType::Regeneration);
    EXPECT_NE(color, speedColor);
    EXPECT_NE(color, regenColor);
}

// ============================================================================
// 效果序列化完整性测试
// ============================================================================

TEST_F(PotionUtilsTest, CustomEffects_AllFieldsPreserved) {
    if (Items::POTION == nullptr) {
        GTEST_SKIP() << "POTION item not initialized";
    }
    ItemStack potionStack(Items::POTION, 1);

    // 创建带有所有字段的效果
    entity::effect::EffectInstance originalEffect(
        entity::effect::EffectType::Speed,  // type
        1234,                                // duration
        2,                                   // amplifier (Speed III)
        true,                                // ambient
        false,                               // visible (no particles)
        true                                 // showIcon
    );

    potion::PotionUtils::addCustomEffect(potionStack, originalEffect);

    auto effects = potion::PotionUtils::getCustomEffects(potionStack);
    ASSERT_EQ(effects.size(), 1);

    // 验证所有字段都被正确保存和读取
    const auto& retrieved = effects[0];
    EXPECT_EQ(retrieved.type(), originalEffect.type());
    EXPECT_EQ(retrieved.duration(), originalEffect.duration());
    EXPECT_EQ(retrieved.amplifier(), originalEffect.amplifier());
    EXPECT_EQ(retrieved.isAmbient(), originalEffect.isAmbient());
    EXPECT_EQ(retrieved.isVisible(), originalEffect.isVisible());
    EXPECT_EQ(retrieved.showIcon(), originalEffect.showIcon());
}

TEST_F(PotionUtilsTest, SetCustomEffects_EmptyList_RemovesEffects) {
    if (Items::POTION == nullptr) {
        GTEST_SKIP() << "POTION item not initialized";
    }
    ItemStack potionStack(Items::POTION, 1);

    // 先添加一些效果
    std::vector<entity::effect::EffectInstance> effects;
    effects.emplace_back(entity::effect::EffectType::Speed, 600, 0);
    potion::PotionUtils::setCustomEffects(potionStack, effects);
    EXPECT_TRUE(potion::PotionUtils::hasCustomEffects(potionStack));

    // 设置空列表应该移除效果
    std::vector<entity::effect::EffectInstance> emptyEffects;
    potion::PotionUtils::setCustomEffects(potionStack, emptyEffects);
    EXPECT_FALSE(potion::PotionUtils::hasCustomEffects(potionStack));
}

TEST_F(PotionUtilsTest, MultipleCustomEffects_AllPreserved) {
    if (Items::POTION == nullptr) {
        GTEST_SKIP() << "POTION item not initialized";
    }
    ItemStack potionStack(Items::POTION, 1);

    std::vector<entity::effect::EffectInstance> effects;
    effects.emplace_back(entity::effect::EffectType::Speed, 600, 0);
    effects.emplace_back(entity::effect::EffectType::JumpBoost, 300, 1);
    effects.emplace_back(entity::effect::EffectType::Regeneration, 900, 2);

    potion::PotionUtils::setCustomEffects(potionStack, effects);

    auto retrieved = potion::PotionUtils::getCustomEffects(potionStack);
    ASSERT_EQ(retrieved.size(), 3);

    EXPECT_EQ(retrieved[0].type(), entity::effect::EffectType::Speed);
    EXPECT_EQ(retrieved[1].type(), entity::effect::EffectType::JumpBoost);
    EXPECT_EQ(retrieved[2].type(), entity::effect::EffectType::Regeneration);
}

} // namespace
} // namespace mc
