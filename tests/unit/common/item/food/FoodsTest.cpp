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

/**
 * @file FoodsTest.cpp
 * @brief Food 属性对齐 MC Java 1.21.11 单元测试
 *
 * 验证 Foods 注册的食物属性（nutrition/saturationModifier/effects/alwaysEdible）
 * 与 vanilla 1.21.11 精确对齐。vanilla 数据源：
 *   - FoodProperties: net/minecraft/world/food/Foods.java
 *   - Consume effects: net/minecraft/world/item/component/Consumables.java
 *
 * Food.addEffect(type, duration, amplifier, probability) 中 amplifier 0=I, 1=II...
 * 与 vanilla MobEffectInstance(level) 语义一致（amplifier+1 为显示等级）。
 */

#include <gtest/gtest.h>

#include "common/entity/effect/EffectType.hpp"
#include "common/item/food/Foods.hpp"

namespace mc {
namespace item::food {
namespace {

// 在指定食物的效果列表中查找第一个匹配类型的 effect，找不到返回 nullptr。
[[nodiscard]] const FoodEffect* findEffect(const Food& food, entity::effect::EffectType type)
{
    for (const auto& e : food.getEffects()) {
        if (e.type == type) {
            return &e;
        }
    }
    return nullptr;
}

// ============================================================================
// PUFFERFISH — vanilla Consumables.java:48-58
//   Poison  1200, amplifier 1 (Poison II)
//   Hunger  300,  amplifier 2 (Hunger III)
//   Nausea  300,  amplifier 0 (Nausea I)
// 此前 Cubium 误把 Poison amplifier 写成 3（Poison IV），中毒伤害约翻倍。
// ============================================================================

TEST(FoodsTest, Pufferfish_NutritionAndSaturation)
{
    // vanilla Foods.java:33 PUFFERFISH nutrition=1, saturationModifier=0.1
    EXPECT_EQ(Foods::PUFFERFISH.getHunger(), 1);
    EXPECT_FLOAT_EQ(Foods::PUFFERFISH.getSaturationModifier(), 0.1f);
}

TEST(FoodsTest, Pufferfish_PoisonIsLevel2NotLevel4)
{
    // 对齐 vanilla Consumables.java:52 MobEffects.POISON, 1200, 1 → Poison II
    const FoodEffect* poison = findEffect(Foods::PUFFERFISH, entity::effect::EffectType::Poison);
    ASSERT_NE(poison, nullptr) << "Pufferfish should apply Poison";
    EXPECT_EQ(poison->duration, 1200) << "Poison duration 1200 ticks (60s)";
    EXPECT_EQ(poison->amplifier, 1) << "Poison amplifier must be 1 (Poison II), not 3 (Poison IV)";
    EXPECT_FLOAT_EQ(poison->probability, 1.0f);
}

TEST(FoodsTest, Pufferfish_HungerAndNausea)
{
    // vanilla Consumables.java:53-54
    const FoodEffect* hunger = findEffect(Foods::PUFFERFISH, entity::effect::EffectType::Hunger);
    ASSERT_NE(hunger, nullptr);
    EXPECT_EQ(hunger->duration, 300);
    EXPECT_EQ(hunger->amplifier, 2) << "Hunger III";

    const FoodEffect* nausea = findEffect(Foods::PUFFERFISH, entity::effect::EffectType::Nausea);
    ASSERT_NE(nausea, nullptr);
    EXPECT_EQ(nausea->duration, 300);
    EXPECT_EQ(nausea->amplifier, 0) << "Nausea I";
}

// ============================================================================
// HONEY_BOTTLE — vanilla Foods.java:26
//   nutrition=6, saturationModifier=0.1, alwaysEdible()
// vanilla Consumables.java:16-20 HONEY_BOTTLE consumable: 清除 Poison。
// 此前 Cubium 缺 setAlwaysEdible()，致玩家饱食时无法饮用蜂蜜瓶解毒。
// ============================================================================

TEST(FoodsTest, HoneyBottle_IsAlwaysEdible)
{
    // 对齐 vanilla Foods.java:26 alwaysEdible() —— 饱食时可饮用（用于解毒）
    EXPECT_TRUE(Foods::HONEY_BOTTLE.canAlwaysEat())
        << "Honey bottle must be alwaysEdible (vanilla allows drinking when full to cure poison)";
    EXPECT_EQ(Foods::HONEY_BOTTLE.getHunger(), 6);
    EXPECT_FLOAT_EQ(Foods::HONEY_BOTTLE.getSaturationModifier(), 0.1f);
}

// ============================================================================
// 对照组：金苹果 alwaysEdible（已对齐，防止回归）
// ============================================================================

TEST(FoodsTest, GoldenApple_IsAlwaysEdible)
{
    EXPECT_TRUE(Foods::GOLDEN_APPLE.canAlwaysEat());
    EXPECT_TRUE(Foods::ENCHANTED_GOLDEN_APPLE.canAlwaysEat());
}

} // namespace
} // namespace item::food
} // namespace mc
