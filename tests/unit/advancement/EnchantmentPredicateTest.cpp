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

#include "common/advancement/MinMaxBounds.hpp"
#include "common/advancement/trigger/conditions/EnchantmentPredicate.hpp"
#include "common/item/enchantment/EnchantmentContainer.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::advancement;
using namespace mc::item::enchant;

// ========== isAny 测试 ==========

class EnchantmentPredicateTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(EnchantmentPredicateTest, DefaultIsAny)
{
    EnchantmentPredicate predicate;
    EXPECT_TRUE(predicate.isAny());
    EXPECT_FALSE(predicate.getEnchantment().has_value());
    EXPECT_TRUE(predicate.getLevels().isUnbounded());
}

TEST_F(EnchantmentPredicateTest, WithEnchantmentOnlyIsNotAny)
{
    ResourceLocation enchId("minecraft", "sharpness");
    EnchantmentPredicate predicate(enchId, IntBounds{});
    EXPECT_FALSE(predicate.isAny());
    EXPECT_TRUE(predicate.getEnchantment().has_value());
    EXPECT_TRUE(predicate.getLevels().isUnbounded());
}

TEST_F(EnchantmentPredicateTest, WithLevelsOnlyIsNotAny)
{
    IntBounds levels = IntBounds::atLeast(1);
    EnchantmentPredicate predicate(std::nullopt, levels);
    EXPECT_FALSE(predicate.isAny());
    EXPECT_FALSE(predicate.getEnchantment().has_value());
    EXPECT_FALSE(predicate.getLevels().isUnbounded());
}

TEST_F(EnchantmentPredicateTest, WithBothIsNotAny)
{
    ResourceLocation enchId("minecraft", "sharpness");
    IntBounds levels = IntBounds::exactly(5);
    EnchantmentPredicate predicate(enchId, levels);
    EXPECT_FALSE(predicate.isAny());
}

// ========== test 测试 ==========

TEST_F(EnchantmentPredicateTest, AnyPredicateMatchesEmptyContainer)
{
    EnchantmentPredicate predicate;
    EnchantmentContainer empty;
    EXPECT_TRUE(predicate.test(empty));
}

TEST_F(EnchantmentPredicateTest, AnyPredicateMatchesNonEmptyContainer)
{
    EnchantmentPredicate predicate;
    EnchantmentContainer enchants;
    enchants.set("minecraft:sharpness", 5);
    EXPECT_TRUE(predicate.test(enchants));
}

TEST_F(EnchantmentPredicateTest, SpecificEnchantmentMatches)
{
    ResourceLocation enchId("minecraft", "sharpness");
    IntBounds levels; // unbounded - any level
    EnchantmentPredicate predicate(enchId, levels);

    EnchantmentContainer enchants;
    enchants.set("minecraft:sharpness", 5);
    EXPECT_TRUE(predicate.test(enchants));
}

TEST_F(EnchantmentPredicateTest, SpecificEnchantmentMismatch)
{
    ResourceLocation enchId("minecraft", "sharpness");
    IntBounds levels;
    EnchantmentPredicate predicate(enchId, levels);

    EnchantmentContainer enchants;
    enchants.set("minecraft:protection", 4);
    EXPECT_FALSE(predicate.test(enchants));
}

TEST_F(EnchantmentPredicateTest, SpecificEnchantmentWithLevelRange)
{
    ResourceLocation enchId("minecraft", "sharpness");
    IntBounds levels = IntBounds::between(3, 5);
    EnchantmentPredicate predicate(enchId, levels);

    // Level 5 - within range
    EnchantmentContainer enchants1;
    enchants1.set("minecraft:sharpness", 5);
    EXPECT_TRUE(predicate.test(enchants1));

    // Level 2 - below range
    EnchantmentContainer enchants2;
    enchants2.set("minecraft:sharpness", 2);
    EXPECT_FALSE(predicate.test(enchants2));

    // Level 3 - at lower bound
    EnchantmentContainer enchants3;
    enchants3.set("minecraft:sharpness", 3);
    EXPECT_TRUE(predicate.test(enchants3));

    // Level 6 - above range
    EnchantmentContainer enchants4;
    enchants4.set("minecraft:sharpness", 6);
    EXPECT_FALSE(predicate.test(enchants4));
}

TEST_F(EnchantmentPredicateTest, SpecificEnchantmentNotPresent)
{
    ResourceLocation enchId("minecraft", "sharpness");
    IntBounds levels = IntBounds::atLeast(1);
    EnchantmentPredicate predicate(enchId, levels);

    // Empty container - enchantment not present
    EnchantmentContainer empty;
    EXPECT_FALSE(predicate.test(empty));

    // Different enchantment
    EnchantmentContainer other;
    other.set("minecraft:efficiency", 3);
    EXPECT_FALSE(predicate.test(other));
}

TEST_F(EnchantmentPredicateTest, AnyEnchantmentWithLevels)
{
    // No specific enchantment, but levels must be at least 3
    IntBounds levels = IntBounds::atLeast(3);
    EnchantmentPredicate predicate(std::nullopt, levels);

    // Has enchantment at level 5 - should match
    EnchantmentContainer enchants1;
    enchants1.set("minecraft:sharpness", 5);
    EXPECT_TRUE(predicate.test(enchants1));

    // Has enchantment at level 2 - should not match
    EnchantmentContainer enchants2;
    enchants2.set("minecraft:protection", 2);
    EXPECT_FALSE(predicate.test(enchants2));

    // Empty container - should not match
    EnchantmentContainer empty;
    EXPECT_FALSE(predicate.test(empty));
}

TEST_F(EnchantmentPredicateTest, AnyEnchantmentWithUnboundedLevels)
{
    // No specific enchantment, levels unbounded - should match if any enchantment exists
    IntBounds levels; // unbounded
    EnchantmentPredicate predicate(std::nullopt, levels);

    // Has enchantment - should match
    EnchantmentContainer enchants;
    enchants.set("minecraft:sharpness", 1);
    EXPECT_TRUE(predicate.test(enchants));

    // Empty - should match (both enchantment and levels are effectively "any")
    // Note: isAny() would be true in this case since no enchantment and unbounded levels
}

TEST_F(EnchantmentPredicateTest, ExactLevelMatch)
{
    ResourceLocation enchId("minecraft", "sharpness");
    IntBounds levels = IntBounds::exactly(5);
    EnchantmentPredicate predicate(enchId, levels);

    EnchantmentContainer enchants1;
    enchants1.set("minecraft:sharpness", 5);
    EXPECT_TRUE(predicate.test(enchants1));

    EnchantmentContainer enchants2;
    enchants2.set("minecraft:sharpness", 4);
    EXPECT_FALSE(predicate.test(enchants2));

    EnchantmentContainer enchants3;
    enchants3.set("minecraft:sharpness", 6);
    EXPECT_FALSE(predicate.test(enchants3));
}

TEST_F(EnchantmentPredicateTest, AtMostLevel)
{
    ResourceLocation enchId("minecraft", "protection");
    IntBounds levels = IntBounds::atMost(3);
    EnchantmentPredicate predicate(enchId, levels);

    EnchantmentContainer enchants1;
    enchants1.set("minecraft:protection", 3);
    EXPECT_TRUE(predicate.test(enchants1));

    EnchantmentContainer enchants2;
    enchants2.set("minecraft:protection", 4);
    EXPECT_FALSE(predicate.test(enchants2));
}

TEST_F(EnchantmentPredicateTest, MultipleEnchantmentsInContainer)
{
    ResourceLocation enchId("minecraft", "sharpness");
    IntBounds levels = IntBounds::atLeast(3);
    EnchantmentPredicate predicate(enchId, levels);

    // Container has multiple enchantments, sharpness at level 5
    EnchantmentContainer enchants;
    enchants.set("minecraft:sharpness", 5);
    enchants.set("minecraft:fire_aspect", 2);
    EXPECT_TRUE(predicate.test(enchants));
}

TEST_F(EnchantmentPredicateTest, AnyEnchantmentMultipleInContainer)
{
    // Looking for any enchantment with level >= 3
    IntBounds levels = IntBounds::atLeast(3);
    EnchantmentPredicate predicate(std::nullopt, levels);

    // One at level 2, another at level 5 - should match because 5 >= 3
    EnchantmentContainer enchants;
    enchants.set("minecraft:protection", 2);
    enchants.set("minecraft:sharpness", 5);
    EXPECT_TRUE(predicate.test(enchants));
}

// ========== fromJson 测试 ==========

TEST_F(EnchantmentPredicateTest, FromJsonNullReturnsAny)
{
    auto result = EnchantmentPredicate::fromJson(nullptr);
    EXPECT_TRUE(result.success());
    EXPECT_TRUE(result.value().isAny());
}

TEST_F(EnchantmentPredicateTest, FromJsonEnchantmentOnly)
{
    nlohmann::json json = {{"enchantment", "minecraft:sharpness"}};
    auto result = EnchantmentPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());
    EXPECT_TRUE(result.value().getEnchantment().has_value());
    EXPECT_EQ(result.value().getEnchantment().value().toString(), "minecraft:sharpness");
    EXPECT_TRUE(result.value().getLevels().isUnbounded());
}

TEST_F(EnchantmentPredicateTest, FromJsonLevelsOnly)
{
    nlohmann::json json = {{"levels", {{"min", 3}}}};
    auto result = EnchantmentPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());
    EXPECT_FALSE(result.value().getEnchantment().has_value());
    EXPECT_FALSE(result.value().getLevels().isUnbounded());
    EXPECT_TRUE(result.value().getLevels().getMin().has_value());
    EXPECT_EQ(result.value().getLevels().getMin().value(), 3);
}

TEST_F(EnchantmentPredicateTest, FromJsonEnchantmentAndLevels)
{
    nlohmann::json json = {{"enchantment", "minecraft:protection"}, {"levels", {{"min", 1}, {"max", 4}}}};
    auto result = EnchantmentPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());
    EXPECT_TRUE(result.value().getEnchantment().has_value());
    EXPECT_EQ(result.value().getEnchantment().value().toString(), "minecraft:protection");
    EXPECT_FALSE(result.value().getLevels().isUnbounded());
    EXPECT_EQ(result.value().getLevels().getMin().value(), 1);
    EXPECT_EQ(result.value().getLevels().getMax().value(), 4);
}

TEST_F(EnchantmentPredicateTest, FromJsonExactLevel)
{
    nlohmann::json json = {{"enchantment", "minecraft:sharpness"}, {"levels", 5}};
    auto result = EnchantmentPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().getLevels().isUnbounded());
    EXPECT_TRUE(result.value().getLevels().getMin().has_value());
    EXPECT_TRUE(result.value().getLevels().getMax().has_value());
    EXPECT_EQ(result.value().getLevels().getMin().value(), 5);
    EXPECT_EQ(result.value().getLevels().getMax().value(), 5);
}

TEST_F(EnchantmentPredicateTest, FromJsonEmptyObjectReturnsAny)
{
    nlohmann::json json = nlohmann::json::object();
    auto result = EnchantmentPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_TRUE(result.value().isAny());
}

// ========== toJson 测试 ==========

TEST_F(EnchantmentPredicateTest, ToJsonAnyReturnsNull)
{
    EnchantmentPredicate predicate;
    auto json = predicate.toJson();
    EXPECT_TRUE(json.is_null());
}

TEST_F(EnchantmentPredicateTest, ToJsonEnchantmentOnly)
{
    ResourceLocation enchId("minecraft", "sharpness");
    EnchantmentPredicate predicate(enchId, IntBounds{});
    auto json = predicate.toJson();
    ASSERT_TRUE(json.is_object());
    EXPECT_EQ(json["enchantment"].get<std::string>(), "minecraft:sharpness");
    EXPECT_FALSE(json.contains("levels"));
}

TEST_F(EnchantmentPredicateTest, ToJsonLevelsOnly)
{
    IntBounds levels = IntBounds::atLeast(3);
    EnchantmentPredicate predicate(std::nullopt, levels);
    auto json = predicate.toJson();
    ASSERT_TRUE(json.is_object());
    EXPECT_FALSE(json.contains("enchantment"));
    EXPECT_TRUE(json.contains("levels"));
}

TEST_F(EnchantmentPredicateTest, ToJsonBothFields)
{
    ResourceLocation enchId("minecraft", "protection");
    IntBounds levels = IntBounds::between(1, 4);
    EnchantmentPredicate predicate(enchId, levels);
    auto json = predicate.toJson();
    ASSERT_TRUE(json.is_object());
    EXPECT_EQ(json["enchantment"].get<std::string>(), "minecraft:protection");
    EXPECT_TRUE(json.contains("levels"));
}

// ========== Round-trip 测试 ==========

TEST_F(EnchantmentPredicateTest, RoundTripEnchantmentOnly)
{
    ResourceLocation enchId("minecraft", "sharpness");
    EnchantmentPredicate original(enchId, IntBounds{});

    auto json = original.toJson();
    auto result = EnchantmentPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_TRUE(result.value().getEnchantment().has_value());
    EXPECT_EQ(result.value().getEnchantment().value().toString(), "minecraft:sharpness");
    EXPECT_TRUE(result.value().getLevels().isUnbounded());
}

TEST_F(EnchantmentPredicateTest, RoundTripBothFields)
{
    ResourceLocation enchId("minecraft", "protection");
    IntBounds levels = IntBounds::between(2, 5);
    EnchantmentPredicate original(enchId, levels);

    auto json = original.toJson();
    auto result = EnchantmentPredicate::fromJson(json);
    EXPECT_TRUE(result.success());

    // Test the restored predicate against the same conditions
    EnchantmentContainer enchants;
    enchants.set("minecraft:protection", 3);
    EXPECT_TRUE(result.value().test(enchants));

    EnchantmentContainer enchants2;
    enchants2.set("minecraft:protection", 1);
    EXPECT_FALSE(result.value().test(enchants2));
}

TEST_F(EnchantmentPredicateTest, RoundTripLevelsOnly)
{
    IntBounds levels = IntBounds::atLeast(5);
    EnchantmentPredicate original(std::nullopt, levels);

    auto json = original.toJson();
    auto result = EnchantmentPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().getEnchantment().has_value());
    EXPECT_FALSE(result.value().getLevels().isUnbounded());

    // Test the restored predicate
    EnchantmentContainer enchants;
    enchants.set("minecraft:sharpness", 5);
    EXPECT_TRUE(result.value().test(enchants));
}
