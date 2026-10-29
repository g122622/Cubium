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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN AN EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "common/entity/attribute/AttributeRegistry.hpp"
#include "common/entity/attribute/AttributeMap.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity::attribute;

// ============================================================================
// AttributeRegistry::instance 测试
// ============================================================================

TEST(AttributeRegistryTest, SingletonInstanceIsValid)
{
    auto& registry = AttributeRegistry::instance();
    EXPECT_GT(registry.size(), 0u);
}

TEST(AttributeRegistryTest, AllBuiltinAttributesAreRegistered)
{
    auto& registry = AttributeRegistry::instance();

    // MC 原版属性
    EXPECT_TRUE(registry.isKnown(Attributes::MAX_HEALTH));
    EXPECT_TRUE(registry.isKnown(Attributes::FOLLOW_RANGE));
    EXPECT_TRUE(registry.isKnown(Attributes::KNOCKBACK_RESISTANCE));
    EXPECT_TRUE(registry.isKnown(Attributes::MOVEMENT_SPEED));
    EXPECT_TRUE(registry.isKnown(Attributes::FLYING_SPEED));
    EXPECT_TRUE(registry.isKnown(Attributes::ATTACK_DAMAGE));
    EXPECT_TRUE(registry.isKnown(Attributes::ATTACK_KNOCKBACK));
    EXPECT_TRUE(registry.isKnown(Attributes::ATTACK_SPEED));
    EXPECT_TRUE(registry.isKnown(Attributes::ARMOR));
    EXPECT_TRUE(registry.isKnown(Attributes::ARMOR_TOUGHNESS));
    EXPECT_TRUE(registry.isKnown(Attributes::LUCK));
    EXPECT_TRUE(registry.isKnown(Attributes::MAX_ABSORPTION));
    EXPECT_TRUE(registry.isKnown(Attributes::BREATH_MAX));
    EXPECT_TRUE(registry.isKnown(Attributes::JUMP_STRENGTH));
    EXPECT_TRUE(registry.isKnown(Attributes::HORSE_JUMP_STRENGTH));
    EXPECT_TRUE(registry.isKnown(Attributes::ZOMBIE_SPAWN_REINFORCEMENTS));

    // Forge 扩展属性
    // 1.21 起重力是原版属性 minecraft:gravity（旧的 forge.entity_gravity 已并入）。
    EXPECT_TRUE(registry.isKnown(Attributes::ENTITY_GRAVITY));
    // 1.21 新增、此前缺失的原版属性。
    EXPECT_TRUE(registry.isKnown(Attributes::BLOCK_BREAK_SPEED));
    EXPECT_TRUE(registry.isKnown(Attributes::STEP_HEIGHT));
    EXPECT_TRUE(registry.isKnown(Attributes::SCALE));
    EXPECT_TRUE(registry.isKnown(Attributes::TEMPT_RANGE));
    EXPECT_TRUE(registry.isKnown(Attributes::CAMERA_DISTANCE));
    EXPECT_TRUE(registry.isKnown(Attributes::MINING_EFFICIENCY));
    EXPECT_TRUE(registry.isKnown(Attributes::SUBMERGED_MINING_SPEED));
    EXPECT_TRUE(registry.isKnown(Attributes::WATER_MOVEMENT_EFFICIENCY));
    EXPECT_TRUE(registry.isKnown(Attributes::SNEAKING_SPEED));
    EXPECT_TRUE(registry.isKnown(Attributes::SWEEPING_DAMAGE_RATIO));

    // MC 1.21+ 新增属性
    EXPECT_TRUE(registry.isKnown(Attributes::MOVEMENT_EFFICIENCY));
    EXPECT_TRUE(registry.isKnown(Attributes::BLOCK_INTERACTION_RANGE));
    EXPECT_TRUE(registry.isKnown(Attributes::ENTITY_INTERACTION_RANGE));
    EXPECT_TRUE(registry.isKnown(Attributes::SAFE_FALL_DISTANCE));
    EXPECT_TRUE(registry.isKnown(Attributes::FALL_DAMAGE_MULTIPLIER));
}

TEST(AttributeRegistryTest, UnknownAttributeIsNotKnown)
{
    auto& registry = AttributeRegistry::instance();
    EXPECT_FALSE(registry.isKnown("generic.nonexistent"));
    EXPECT_FALSE(registry.isKnown("unknown_attr"));
    EXPECT_FALSE(registry.isKnown(""));
}

// ============================================================================
// AttributeRegistry::getRange 测试
// ============================================================================

TEST(AttributeRegistryTest, MaxHealthRangeMatchesFactoryDefinition)
{
    // 关键回归测试：原硬编码为 {0.0, 1024.0}，但 Attributes::maxHealth() 定义的 minValue 为 1.0
    auto& registry = AttributeRegistry::instance();
    auto [minVal, maxVal] = registry.getRange(Attributes::MAX_HEALTH);
    EXPECT_DOUBLE_EQ(minVal, 1.0);
    EXPECT_DOUBLE_EQ(maxVal, 1024.0);
}

TEST(AttributeRegistryTest, FollowRangeRangeIsCorrect)
{
    auto& registry = AttributeRegistry::instance();
    auto [minVal, maxVal] = registry.getRange(Attributes::FOLLOW_RANGE);
    EXPECT_DOUBLE_EQ(minVal, 0.0);
    EXPECT_DOUBLE_EQ(maxVal, 2048.0);
}

TEST(AttributeRegistryTest, KnockbackResistanceRangeIsCorrect)
{
    auto& registry = AttributeRegistry::instance();
    auto [minVal, maxVal] = registry.getRange(Attributes::KNOCKBACK_RESISTANCE);
    EXPECT_DOUBLE_EQ(minVal, 0.0);
    EXPECT_DOUBLE_EQ(maxVal, 1.0);
}

TEST(AttributeRegistryTest, LuckRangeSupportsNegativeValues)
{
    auto& registry = AttributeRegistry::instance();
    auto [minVal, maxVal] = registry.getRange(Attributes::LUCK);
    EXPECT_DOUBLE_EQ(minVal, -1024.0);
    EXPECT_DOUBLE_EQ(maxVal, 1024.0);
}

TEST(AttributeRegistryTest, ArmorRangeIsCorrect)
{
    auto& registry = AttributeRegistry::instance();
    auto [minVal, maxVal] = registry.getRange(Attributes::ARMOR);
    EXPECT_DOUBLE_EQ(minVal, 0.0);
    EXPECT_DOUBLE_EQ(maxVal, 30.0);
}

TEST(AttributeRegistryTest, EntityGravityRangeMatchesVanilla)
{
    // vanilla Attributes.java:45-47：gravity 默认 0.08，范围 [-1.0, 1.0]
    // （旧实现沿用 Forge 的 ±8.0，与原版不一致）。
    auto& registry = AttributeRegistry::instance();
    auto [minVal, maxVal] = registry.getRange(Attributes::ENTITY_GRAVITY);
    EXPECT_DOUBLE_EQ(minVal, -1.0);
    EXPECT_DOUBLE_EQ(maxVal, 1.0);
    EXPECT_DOUBLE_EQ(registry.getDefaultValue(Attributes::ENTITY_GRAVITY, -1.0), 0.08);
}

TEST(AttributeRegistryTest, BlockInteractionRangeDefinitionMatchesMC)
{
    // MC 1.21.11: minecraft:block_interaction_range 默认 4.5，范围 [0, 64]
    auto& registry = AttributeRegistry::instance();
    const auto* attr = registry.getAttribute(Attributes::BLOCK_INTERACTION_RANGE);
    ASSERT_NE(attr, nullptr);
    EXPECT_EQ(attr->registryName(), "minecraft:block_interaction_range");
    EXPECT_DOUBLE_EQ(attr->defaultValue(), 4.5);
    EXPECT_DOUBLE_EQ(attr->minValue(), 0.0);
    EXPECT_DOUBLE_EQ(attr->maxValue(), 64.0);
}

TEST(AttributeRegistryTest, EntityInteractionRangeDefinitionMatchesMC)
{
    // MC 1.21.11: minecraft:entity_interaction_range 默认 3.0，范围 [0, 64]
    auto& registry = AttributeRegistry::instance();
    const auto* attr = registry.getAttribute(Attributes::ENTITY_INTERACTION_RANGE);
    ASSERT_NE(attr, nullptr);
    EXPECT_EQ(attr->registryName(), "minecraft:entity_interaction_range");
    EXPECT_DOUBLE_EQ(attr->defaultValue(), 3.0);
    EXPECT_DOUBLE_EQ(attr->minValue(), 0.0);
    EXPECT_DOUBLE_EQ(attr->maxValue(), 64.0);
}

TEST(AttributeRegistryTest, SafeFallDistanceDefinitionMatchesMC)
{
    // minecraft:safe_fall_distance 默认 3.0，范围 [-1024, 1024]（允许负值，供修饰符使用）
    auto& registry = AttributeRegistry::instance();
    const auto* attr = registry.getAttribute(Attributes::SAFE_FALL_DISTANCE);
    ASSERT_NE(attr, nullptr);
    EXPECT_EQ(attr->registryName(), "minecraft:safe_fall_distance");
    EXPECT_DOUBLE_EQ(attr->defaultValue(), 3.0);
    EXPECT_DOUBLE_EQ(attr->minValue(), -1024.0);
    EXPECT_DOUBLE_EQ(attr->maxValue(), 1024.0);
}

TEST(AttributeRegistryTest, FallDamageMultiplierDefinitionMatchesMC)
{
    // minecraft:fall_damage_multiplier 默认 1.0，范围 [0, 100]（马类覆盖为 0.5）
    auto& registry = AttributeRegistry::instance();
    const auto* attr = registry.getAttribute(Attributes::FALL_DAMAGE_MULTIPLIER);
    ASSERT_NE(attr, nullptr);
    EXPECT_EQ(attr->registryName(), "minecraft:fall_damage_multiplier");
    EXPECT_DOUBLE_EQ(attr->defaultValue(), 1.0);
    EXPECT_DOUBLE_EQ(attr->minValue(), 0.0);
    EXPECT_DOUBLE_EQ(attr->maxValue(), 100.0);
}

TEST(AttributeRegistryTest, UnknownAttributeReturnsFallbackRange)
{
    auto& registry = AttributeRegistry::instance();
    auto [minVal, maxVal] = registry.getRange("generic.nonexistent");
    EXPECT_DOUBLE_EQ(minVal, 0.0);
    EXPECT_DOUBLE_EQ(maxVal, 1024.0);
}

// ============================================================================
// AttributeRegistry::getAttribute 测试
// ============================================================================

TEST(AttributeRegistryTest, GetAttributeReturnsCorrectDefinition)
{
    auto& registry = AttributeRegistry::instance();
    const auto* attr = registry.getAttribute(Attributes::MAX_HEALTH);
    ASSERT_NE(attr, nullptr);
    EXPECT_EQ(attr->registryName(), "minecraft:max_health");
    EXPECT_DOUBLE_EQ(attr->defaultValue(), 20.0);
    EXPECT_DOUBLE_EQ(attr->minValue(), 1.0);
    EXPECT_DOUBLE_EQ(attr->maxValue(), 1024.0);
}

TEST(AttributeRegistryTest, GetAttributeReturnsNullForUnknown)
{
    auto& registry = AttributeRegistry::instance();
    const auto* attr = registry.getAttribute("generic.nonexistent");
    EXPECT_EQ(attr, nullptr);
}

// ============================================================================
// AttributeRegistry::getDefaultValue 测试
// ============================================================================

TEST(AttributeRegistryTest, GetDefaultValueReturnsCorrectValue)
{
    auto& registry = AttributeRegistry::instance();
    EXPECT_DOUBLE_EQ(registry.getDefaultValue(Attributes::MAX_HEALTH), 20.0);
    EXPECT_DOUBLE_EQ(registry.getDefaultValue(Attributes::MOVEMENT_SPEED), 0.7);
    EXPECT_DOUBLE_EQ(registry.getDefaultValue(Attributes::ATTACK_DAMAGE), 2.0);
    EXPECT_DOUBLE_EQ(registry.getDefaultValue(Attributes::FLYING_SPEED), 0.4);
}

TEST(AttributeRegistryTest, GetDefaultValueReturnsFallbackForUnknown)
{
    auto& registry = AttributeRegistry::instance();
    EXPECT_DOUBLE_EQ(registry.getDefaultValue("generic.nonexistent"), 0.0);
    EXPECT_DOUBLE_EQ(registry.getDefaultValue("generic.nonexistent", 42.0), 42.0);
}

// ============================================================================
// AttributeRegistry::normalizeName 测试
// ============================================================================

TEST(AttributeRegistryTest, NormalizeNameAddsGenericPrefix)
{
    auto& registry = AttributeRegistry::instance();
    EXPECT_EQ(registry.normalizeName("max_health"), "minecraft:max_health");
    EXPECT_EQ(registry.normalizeName("movement_speed"), "minecraft:movement_speed");
    EXPECT_EQ(registry.normalizeName("attack_damage"), "minecraft:attack_damage");
    // jump_strength 短名现在匹配通用 minecraft:jump_strength（LivingEntity 跳跃力），
    // 而非马族专用的 horse.jump_strength（待重构移除）。generic 命名空间优先于 horse。
    EXPECT_EQ(registry.normalizeName("jump_strength"), "minecraft:jump_strength");
}

TEST(AttributeRegistryTest, NormalizeNamePreservesExistingPrefix)
{
    auto& registry = AttributeRegistry::instance();
    EXPECT_EQ(registry.normalizeName("minecraft:max_health"), "minecraft:max_health");
    EXPECT_EQ(registry.normalizeName("horse.jump_strength"), "horse.jump_strength");
}

TEST(AttributeRegistryTest, NormalizeNameAcceptsLegacyGenericPrefix)
{
    // 1.16.5 的 generic. 写法仍然可用，统一归到原版名上。
    auto& registry = AttributeRegistry::instance();
    EXPECT_EQ(registry.normalizeName("generic.max_health"), "minecraft:max_health");
    EXPECT_EQ(registry.normalizeName("generic.attack_damage"), "minecraft:attack_damage");
    // 没有同名原版属性的 generic.* 原样返回，由调用方报「未知属性」。
    EXPECT_EQ(registry.normalizeName("generic.nonexistent"), "generic.nonexistent");
}

TEST(AttributeRegistryTest, NormalizeNameStripsMinecraftPrefix)
{
    auto& registry = AttributeRegistry::instance();
    EXPECT_EQ(registry.normalizeName("minecraft:max_health"), "minecraft:max_health");
    // 旧的「再补一次 minecraft:」写法只要看起来像已知属性也一样归一。
    EXPECT_EQ(registry.normalizeName("minecraft:minecraft:max_health"), "minecraft:max_health");
}

TEST(AttributeRegistryTest, NormalizeNameAddsMinecraftPrefix)
{
    // 原版的 spawn_reinforcements 在 minecraft 命名空间下；1.16.5 的 zombie. 写法由
    // 「去掉前缀再查」的规则落在 minecraft:spawn_reinforcements 上（见下一条）。
    auto& registry = AttributeRegistry::instance();
    EXPECT_EQ(registry.normalizeName("spawn_reinforcements"), "minecraft:spawn_reinforcements");
    EXPECT_EQ(registry.normalizeName("zombie.spawn_reinforcements"), "minecraft:spawn_reinforcements");
}

TEST(AttributeRegistryTest, NormalizeNameAddsForgePrefix)
{
    auto& registry = AttributeRegistry::instance();
    // 1.21 起重力是原版属性；短名与旧的 Forge 名都解析到 minecraft:gravity。
    EXPECT_EQ(registry.normalizeName("entity_gravity"), "minecraft:gravity");
    EXPECT_EQ(registry.normalizeName("forge.entity_gravity"), "minecraft:gravity");
}

TEST(AttributeRegistryTest, NormalizeNameReturnsUnknownAsIs)
{
    auto& registry = AttributeRegistry::instance();
    EXPECT_EQ(registry.normalizeName("totally_unknown"), "totally_unknown");
}

// ============================================================================
// AttributeRegistry::getAllNames 测试
// ============================================================================

TEST(AttributeRegistryTest, GetAllNamesReturnsAllRegisteredAttributes)
{
    auto& registry = AttributeRegistry::instance();
    auto names = registry.getAllNames();
    EXPECT_EQ(names.size(), registry.size());

    // 检查列表中包含几个关键属性
    EXPECT_NE(std::find(names.begin(), names.end(), "minecraft:max_health"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "minecraft:movement_speed"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "horse.jump_strength"), names.end());
}

TEST(AttributeRegistryTest, GetAllNamesIsSorted)
{
    auto& registry = AttributeRegistry::instance();
    auto names = registry.getAllNames();
    auto sorted = names;
    std::sort(sorted.begin(), sorted.end());
    EXPECT_EQ(names, sorted);
}

// ============================================================================
// AttributeRegistry::registerAttribute 测试
// ============================================================================

TEST(AttributeRegistryTest, RegisterCustomAttribute)
{
    auto& registry = AttributeRegistry::instance();
    size_t originalSize = registry.size();

    // 注册一个自定义属性
    Attribute customAttr("custom.test_attribute", 100.0, 0.0, 500.0);
    EXPECT_TRUE(registry.registerAttribute(customAttr));

    // 验证已注册
    EXPECT_TRUE(registry.isKnown("custom.test_attribute"));
    EXPECT_EQ(registry.size(), originalSize + 1);

    // 验证范围正确
    auto [minVal, maxVal] = registry.getRange("custom.test_attribute");
    EXPECT_DOUBLE_EQ(minVal, 0.0);
    EXPECT_DOUBLE_EQ(maxVal, 500.0);

    // 验证默认值
    EXPECT_DOUBLE_EQ(registry.getDefaultValue("custom.test_attribute"), 100.0);

    // 重复注册应失败
    EXPECT_FALSE(registry.registerAttribute(customAttr));
}

// ============================================================================
// AttributeRegistry 与 AttributeMap 集成测试
// ============================================================================

TEST(AttributeRegistryIntegrationTest, RegistryRangeMatchesAttributeInstance)
{
    // 验证通过 AttributeRegistry 获取的范围与通过 AttributeMap 注册后
    // 从 AttributeInstance 获取的范围一致
    auto& registry = AttributeRegistry::instance();

    AttributeMap map;
    map.registerAttribute(*Attributes::maxHealth());

    const auto* instance = map.getInstance(Attributes::MAX_HEALTH);
    ASSERT_NE(instance, nullptr);

    auto [registryMin, registryMax] = registry.getRange(Attributes::MAX_HEALTH);
    EXPECT_DOUBLE_EQ(registryMin, instance->attribute().minValue());
    EXPECT_DOUBLE_EQ(registryMax, instance->attribute().maxValue());
}
