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

#include "common/entity/entities/projectile/OtherProjectiles.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/Items.hpp"

#include <nlohmann/json.hpp>
#include <cmath>

namespace mc {
namespace {

// ============================================================================
// 基础属性测试
// ============================================================================

/**
 * @brief 测试烟花火箭尺寸
 *
 * MC 1.16.5: 烟花火箭尺寸为 0.25 x 0.25
 */
TEST(FireworkRocketBasicTest, DimensionsCorrect)
{
    entity::FireworkRocketEntity firework(LegacyEntityType::FireworkRocket, EntityId(1));

    EXPECT_FLOAT_EQ(firework.width(), 0.25f);
    EXPECT_FLOAT_EQ(firework.height(), 0.25f);
}

/**
 * @brief 测试默认飞行时间
 */
TEST(FireworkRocketBasicTest, DefaultFlightTime)
{
    entity::FireworkRocketEntity firework(LegacyEntityType::FireworkRocket, EntityId(1));

    // 默认飞行时间为 1
    EXPECT_EQ(firework.flightTime(), 1);
}

/**
 * @brief 测试设置飞行时间
 */
TEST(FireworkRocketBasicTest, SetFlightTime)
{
    entity::FireworkRocketEntity firework(LegacyEntityType::FireworkRocket, EntityId(1));

    firework.setFlightTime(3);
    EXPECT_EQ(firework.flightTime(), 3);
}

/**
 * @brief 测试从弩射出标记
 */
TEST(FireworkRocketBasicTest, ShotFromCrossbowFlag)
{
    entity::FireworkRocketEntity firework(LegacyEntityType::FireworkRocket, EntityId(1));

    EXPECT_FALSE(firework.shotFromCrossbow());

    firework.setShotFromCrossbow(true);
    EXPECT_TRUE(firework.shotFromCrossbow());
}

// ============================================================================
// 物品数据测试
// ============================================================================

/**
 * @brief 测试从物品获取爆炸效果数量 - 无爆炸效果（空物品）
 */
TEST(FireworkRocketItemTest, GetExplosionCountEmpty)
{
    entity::FireworkRocketEntity firework(LegacyEntityType::FireworkRocket, EntityId(1));

    // 空物品
    ItemStack emptyStack(Items::AIR, 0);
    firework.setFireworkItem(emptyStack);

    EXPECT_EQ(firework.getExplosionCount(), 0);
}

/**
 * @brief 测试从物品获取爆炸效果数量 - 无爆炸数据
 */
TEST(FireworkRocketItemTest, GetExplosionCountNoExplosions)
{
    entity::FireworkRocketEntity firework(LegacyEntityType::FireworkRocket, EntityId(1));

    // 创建烟花火箭物品，只设置飞行时间
    ItemStack stack(Items::FIREWORK_ROCKET, 1);
    nlohmann::json& tag = stack.getOrCreateTag();
    tag["Fireworks"] = nlohmann::json::object();
    tag["Fireworks"]["Flight"] = 1;

    firework.setFireworkItem(stack);

    EXPECT_EQ(firework.getExplosionCount(), 0);
}

/**
 * @brief 测试从物品获取爆炸效果数量 - 单个爆炸效果
 */
TEST(FireworkRocketItemTest, GetExplosionCountSingle)
{
    entity::FireworkRocketEntity firework(LegacyEntityType::FireworkRocket, EntityId(1));

    ItemStack stack(Items::FIREWORK_ROCKET, 1);
    nlohmann::json& tag = stack.getOrCreateTag();
    tag["Fireworks"] = nlohmann::json::object();
    tag["Fireworks"]["Flight"] = 1;
    tag["Fireworks"]["Explosions"] = nlohmann::json::array();
    tag["Fireworks"]["Explosions"].push_back(nlohmann::json::object());

    firework.setFireworkItem(stack);

    EXPECT_EQ(firework.getExplosionCount(), 1);
}

/**
 * @brief 测试从物品获取爆炸效果数量 - 多个爆炸效果
 */
TEST(FireworkRocketItemTest, GetExplosionCountMultiple)
{
    entity::FireworkRocketEntity firework(LegacyEntityType::FireworkRocket, EntityId(1));

    // 7 个爆炸效果（最大）
    ItemStack stack(Items::FIREWORK_ROCKET, 1);
    nlohmann::json& tag = stack.getOrCreateTag();
    tag["Fireworks"] = nlohmann::json::object();
    tag["Fireworks"]["Flight"] = 1;
    tag["Fireworks"]["Explosions"] = nlohmann::json::array();
    for (int i = 0; i < 7; ++i) {
        tag["Fireworks"]["Explosions"].push_back(nlohmann::json::object());
    }

    firework.setFireworkItem(stack);

    EXPECT_EQ(firework.getExplosionCount(), 7);
}

/**
 * @brief 测试从物品读取飞行时间
 */
TEST(FireworkRocketItemTest, ReadsFlightTimeFromItem)
{
    entity::FireworkRocketEntity firework(LegacyEntityType::FireworkRocket, EntityId(1));

    ItemStack stack(Items::FIREWORK_ROCKET, 1);
    nlohmann::json& tag = stack.getOrCreateTag();
    tag["Fireworks"] = nlohmann::json::object();
    tag["Fireworks"]["Flight"] = 3;

    firework.setFireworkItem(stack);

    EXPECT_EQ(firework.flightTime(), 3);
}

/**
 * @brief 测试飞行时间不能小于 1
 */
TEST(FireworkRocketItemTest, FlightTimeMinimumOne)
{
    entity::FireworkRocketEntity firework(LegacyEntityType::FireworkRocket, EntityId(1));

    ItemStack stack(Items::FIREWORK_ROCKET, 1);
    nlohmann::json& tag = stack.getOrCreateTag();
    tag["Fireworks"] = nlohmann::json::object();
    tag["Fireworks"]["Flight"] = 0;  // 设置为 0

    firework.setFireworkItem(stack);

    // 飞行时间最小为 1
    EXPECT_EQ(firework.flightTime(), 1);
}

/**
 * @brief 测试烟花火箭物品存储
 */
TEST(FireworkRocketItemTest, FireworkItemStorage)
{
    entity::FireworkRocketEntity firework(LegacyEntityType::FireworkRocket, EntityId(1));

    ItemStack stack(Items::FIREWORK_ROCKET, 1);
    nlohmann::json& tag = stack.getOrCreateTag();
    tag["Fireworks"] = nlohmann::json::object();
    tag["Fireworks"]["Flight"] = 2;
    tag["Fireworks"]["Explosions"] = nlohmann::json::array();
    tag["Fireworks"]["Explosions"].push_back(nlohmann::json::object());

    firework.setFireworkItem(stack);

    // 验证物品已存储
    EXPECT_EQ(firework.fireworkItem().getItem(), Items::FIREWORK_ROCKET);
    EXPECT_EQ(firework.flightTime(), 2);
    EXPECT_EQ(firework.getExplosionCount(), 1);
}

// ============================================================================
// 爆炸伤害计算公式测试
// ============================================================================

/**
 * @brief 测试爆炸伤害计算公式
 *
 * MC 1.16.5 伤害公式：
 * 基础伤害 = 5 + 爆炸效果数量 * 2
 * 实际伤害 = 基础伤害 * sqrt((5 - 距离) / 5)
 */
TEST(FireworkRocketDamageFormulaTest, BaseDamageCalculation)
{
    // 基础伤害计算
    // 0 个爆炸效果：基础伤害 = 5 + 0 * 2 = 5（但不会造成伤害，因为没有爆炸效果）
    // 1 个爆炸效果：基础伤害 = 5 + 1 * 2 = 7
    // 3 个爆炸效果：基础伤害 = 5 + 3 * 2 = 11
    // 7 个爆炸效果：基础伤害 = 5 + 7 * 2 = 19

    auto calculateBaseDamage = [](i32 explosionCount) -> f32 {
        return 5.0f + static_cast<f32>(explosionCount * 2);
    };

    EXPECT_FLOAT_EQ(calculateBaseDamage(1), 7.0f);
    EXPECT_FLOAT_EQ(calculateBaseDamage(3), 11.0f);
    EXPECT_FLOAT_EQ(calculateBaseDamage(7), 19.0f);
}

/**
 * @brief 测试距离衰减公式
 */
TEST(FireworkRocketDamageFormulaTest, DistanceAttenuation)
{
    constexpr f32 EXPLOSION_RADIUS = 5.0f;

    auto calculateDamage = [EXPLOSION_RADIUS](i32 explosionCount, f64 distance) -> f32 {
        f32 baseDamage = 5.0f + static_cast<f32>(explosionCount * 2);
        if (baseDamage <= 0.0f || distance >= EXPLOSION_RADIUS) {
            return 0.0f;
        }
        return baseDamage * static_cast<f32>(std::sqrt((EXPLOSION_RADIUS - distance) / EXPLOSION_RADIUS));
    };

    // 距离 0，完整伤害
    EXPECT_FLOAT_EQ(calculateDamage(1, 0.0), 7.0f);
    EXPECT_FLOAT_EQ(calculateDamage(3, 0.0), 11.0f);
    EXPECT_FLOAT_EQ(calculateDamage(7, 0.0), 19.0f);

    // 距离 2.5，约 70.7% 伤害
    f32 damageAtHalf = calculateDamage(1, 2.5);
    EXPECT_NEAR(damageAtHalf, 7.0f * 0.707f, 0.1f);

    // 距离 5，无伤害
    EXPECT_FLOAT_EQ(calculateDamage(1, 5.0), 0.0f);
    EXPECT_FLOAT_EQ(calculateDamage(7, 5.0), 0.0f);

    // 超出范围，无伤害
    EXPECT_FLOAT_EQ(calculateDamage(7, 6.0), 0.0f);

    // 距离 4，约 44.7% 伤害
    f32 damageAt4 = calculateDamage(3, 4.0);
    f32 expectedAt4 = 11.0f * static_cast<f32>(std::sqrt(0.2));
    EXPECT_NEAR(damageAt4, expectedAt4, 0.1f);
}

/**
 * @brief 测试爆炸半径常量
 */
TEST(FireworkRocketDamageFormulaTest, ExplosionRadius)
{
    // MC 1.16.5: 烟花火箭爆炸半径为 5 格
    constexpr f32 EXPLOSION_RADIUS = 5.0f;
    EXPECT_FLOAT_EQ(EXPLOSION_RADIUS, 5.0f);
}

/**
 * @brief 测试最大爆炸效果数量
 */
TEST(FireworkRocketDamageFormulaTest, MaxExplosions)
{
    // MC 1.16.5: 烟花火箭最多可以有 7 个爆炸效果
    // 最高伤害 = 5 + 7 * 2 = 19 点（9.5 颗心）
    constexpr i32 MAX_EXPLOSIONS = 7;
    constexpr f32 MAX_BASE_DAMAGE = 5.0f + static_cast<f32>(MAX_EXPLOSIONS * 2);

    EXPECT_EQ(MAX_EXPLOSIONS, 7);
    EXPECT_FLOAT_EQ(MAX_BASE_DAMAGE, 19.0f);
}

} // namespace
} // namespace mc
