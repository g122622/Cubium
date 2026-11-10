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
#include "common/advancement/trigger/conditions/ItemPredicate.hpp"
#include "common/item/enchantment/EnchantmentContainer.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::advancement;
using namespace mc::item::enchant;

// ========== 构造辅助 ==========

namespace {

/**
 * @brief 用「可选单个附魔 + 等级范围」构造谓词
 *
 * 谓词本身持有 HolderSet 语义的条目列表；测试里大量用例只关心单个附魔，
 * 这里收敛成一个辅助，避免每处都手写 vector。
 */
EnchantmentPredicate _makePredicate(const std::optional<ResourceLocation>& enchantment, const IntBounds& levels)
{
    std::vector<EnchantmentPredicate::Entry> entries;
    if (enchantment.has_value()) {
        entries.push_back(EnchantmentPredicate::Entry{enchantment.value(), false});
    }
    return EnchantmentPredicate(std::move(entries), levels);
}

} // namespace

// ========== isAny 测试 ==========

class EnchantmentPredicateTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(EnchantmentPredicateTest, DefaultIsAny)
{
    EnchantmentPredicate predicate;
    EXPECT_TRUE(predicate.isAny());
    EXPECT_TRUE(predicate.getEntries().empty());
    EXPECT_TRUE(predicate.getLevels().isUnbounded());
}

TEST_F(EnchantmentPredicateTest, WithEnchantmentOnlyIsNotAny)
{
    ResourceLocation enchId("minecraft", "sharpness");
    const EnchantmentPredicate predicate = _makePredicate(enchId, IntBounds{});
    EXPECT_FALSE(predicate.isAny());
    EXPECT_FALSE(predicate.getEntries().empty());
    EXPECT_TRUE(predicate.getLevels().isUnbounded());
}

TEST_F(EnchantmentPredicateTest, WithLevelsOnlyIsNotAny)
{
    IntBounds levels = IntBounds::atLeast(1);
    const EnchantmentPredicate predicate = _makePredicate(std::nullopt, levels);
    EXPECT_FALSE(predicate.isAny());
    EXPECT_TRUE(predicate.getEntries().empty());
    EXPECT_FALSE(predicate.getLevels().isUnbounded());
}

TEST_F(EnchantmentPredicateTest, WithBothIsNotAny)
{
    ResourceLocation enchId("minecraft", "sharpness");
    IntBounds levels = IntBounds::exactly(5);
    const EnchantmentPredicate predicate = _makePredicate(enchId, levels);
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
    const EnchantmentPredicate predicate = _makePredicate(enchId, levels);

    EnchantmentContainer enchants;
    enchants.set("minecraft:sharpness", 5);
    EXPECT_TRUE(predicate.test(enchants));
}

TEST_F(EnchantmentPredicateTest, SpecificEnchantmentMismatch)
{
    ResourceLocation enchId("minecraft", "sharpness");
    IntBounds levels;
    const EnchantmentPredicate predicate = _makePredicate(enchId, levels);

    EnchantmentContainer enchants;
    enchants.set("minecraft:protection", 4);
    EXPECT_FALSE(predicate.test(enchants));
}

TEST_F(EnchantmentPredicateTest, SpecificEnchantmentWithLevelRange)
{
    ResourceLocation enchId("minecraft", "sharpness");
    IntBounds levels = IntBounds::between(3, 5);
    const EnchantmentPredicate predicate = _makePredicate(enchId, levels);

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
    const EnchantmentPredicate predicate = _makePredicate(enchId, levels);

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
    const EnchantmentPredicate predicate = _makePredicate(std::nullopt, levels);

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
    const EnchantmentPredicate predicate = _makePredicate(std::nullopt, levels);

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
    const EnchantmentPredicate predicate = _makePredicate(enchId, levels);

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
    const EnchantmentPredicate predicate = _makePredicate(enchId, levels);

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
    const EnchantmentPredicate predicate = _makePredicate(enchId, levels);

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
    const EnchantmentPredicate predicate = _makePredicate(std::nullopt, levels);

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
    EXPECT_FALSE(result.value().getEntries().empty());
    EXPECT_EQ(result.value().getEntries().front().id.toString(), "minecraft:sharpness");
    EXPECT_TRUE(result.value().getLevels().isUnbounded());
}

TEST_F(EnchantmentPredicateTest, FromJsonLevelsOnly)
{
    nlohmann::json json = {{"levels", {{"min", 3}}}};
    auto result = EnchantmentPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());
    EXPECT_TRUE(result.value().getEntries().empty());
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
    EXPECT_FALSE(result.value().getEntries().empty());
    EXPECT_EQ(result.value().getEntries().front().id.toString(), "minecraft:protection");
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
    const EnchantmentPredicate predicate = _makePredicate(enchId, IntBounds{});
    auto json = predicate.toJson();
    ASSERT_TRUE(json.is_object());
    EXPECT_EQ(json["enchantment"].get<std::string>(), "minecraft:sharpness");
    EXPECT_FALSE(json.contains("levels"));
}

TEST_F(EnchantmentPredicateTest, ToJsonLevelsOnly)
{
    IntBounds levels = IntBounds::atLeast(3);
    const EnchantmentPredicate predicate = _makePredicate(std::nullopt, levels);
    auto json = predicate.toJson();
    ASSERT_TRUE(json.is_object());
    EXPECT_FALSE(json.contains("enchantment"));
    EXPECT_TRUE(json.contains("levels"));
}

TEST_F(EnchantmentPredicateTest, ToJsonBothFields)
{
    ResourceLocation enchId("minecraft", "protection");
    IntBounds levels = IntBounds::between(1, 4);
    const EnchantmentPredicate predicate = _makePredicate(enchId, levels);
    auto json = predicate.toJson();
    ASSERT_TRUE(json.is_object());
    EXPECT_EQ(json["enchantment"].get<std::string>(), "minecraft:protection");
    EXPECT_TRUE(json.contains("levels"));
}

// ========== Round-trip 测试 ==========

TEST_F(EnchantmentPredicateTest, RoundTripEnchantmentOnly)
{
    ResourceLocation enchId("minecraft", "sharpness");
    const EnchantmentPredicate original = _makePredicate(enchId, IntBounds{});

    auto json = original.toJson();
    auto result = EnchantmentPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().getEntries().empty());
    EXPECT_EQ(result.value().getEntries().front().id.toString(), "minecraft:sharpness");
    EXPECT_TRUE(result.value().getLevels().isUnbounded());
}

TEST_F(EnchantmentPredicateTest, RoundTripBothFields)
{
    ResourceLocation enchId("minecraft", "protection");
    IntBounds levels = IntBounds::between(2, 5);
    const EnchantmentPredicate original = _makePredicate(enchId, levels);

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
    const EnchantmentPredicate original = _makePredicate(std::nullopt, levels);

    auto json = original.toJson();
    auto result = EnchantmentPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_TRUE(result.value().getEntries().empty());
    EXPECT_FALSE(result.value().getLevels().isUnbounded());

    // Test the restored predicate
    EnchantmentContainer enchants;
    enchants.set("minecraft:sharpness", 5);
    EXPECT_TRUE(result.value().test(enchants));
}

// ============================================================================
// ItemPredicate 的 vanilla DataComponentMatchers 写法
// （装备谓词 {"mainhand": {"predicates": {"minecraft:enchantments": [...]}}}）
// ============================================================================

TEST_F(EnchantmentPredicateTest, ItemPredicate_PredicatesWrapperParsesEnchantments)
{
    // 对齐数据包"熔炼掉落"条件：直接攻击者主手物品带 #smelts_loot 附魔
    const nlohmann::json json = {{"predicates",
        {{"minecraft:enchantments", nlohmann::json::array({{{"enchantments", "#minecraft:smelts_loot"}}})}}}};

    auto result = ItemPredicate::fromJson(json);
    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().isWrappedPredicates());
    EXPECT_FALSE(result.value().isAny());
    const auto& enchantments = result.value().getEnchantments();
    ASSERT_EQ(enchantments.size(), 1u);
}

// ========== HolderSet 多元素语义 ==========

// 原版 `enchantments` 是 HolderSet，数组里每个元素都参与"任意一个"匹配。
// 此前只取第一个元素，多元素写法会漏掉其余附魔。
TEST_F(EnchantmentPredicateTest, MultiElementSetMatchesAnyListedEnchantment)
{
    const nlohmann::json json = {
        {"enchantments", {"minecraft:sharpness", "minecraft:smite"}}, {"levels", {{"min", 1}}}};

    auto result = EnchantmentPredicate::fromJson(json);
    ASSERT_TRUE(result.success());
    ASSERT_EQ(result.value().getEntries().size(), 2u);

    EnchantmentContainer smite;
    smite.set("minecraft:smite", 3);
    EXPECT_TRUE(result.value().test(smite)) << "second element must also be matchable";

    EnchantmentContainer sharpness;
    sharpness.set("minecraft:sharpness", 2);
    EXPECT_TRUE(result.value().test(sharpness));

    EnchantmentContainer unrelated;
    unrelated.set("minecraft:protection", 4);
    EXPECT_FALSE(result.value().test(unrelated));
}

TEST_F(EnchantmentPredicateTest, MultiElementSetHonoursLevelBounds)
{
    const nlohmann::json json = {
        {"enchantments", {"minecraft:sharpness", "minecraft:smite"}}, {"levels", {{"min", 3}}}};

    auto result = EnchantmentPredicate::fromJson(json);
    ASSERT_TRUE(result.success());

    // 命中的是列表第二项且等级达标 → 匹配。
    EnchantmentContainer smite;
    smite.set("minecraft:smite", 3);
    EXPECT_TRUE(result.value().test(smite));

    // 命中的是列表第二项但等级不足 → 不匹配。
    EnchantmentContainer weakSmite;
    weakSmite.set("minecraft:smite", 1);
    EXPECT_FALSE(result.value().test(weakSmite));
}

TEST_F(EnchantmentPredicateTest, MultiElementSetKeepsTagAndIdMixed)
{
    const nlohmann::json json = {{"enchantments", {"#minecraft:smelts_loot", "minecraft:fire_aspect"}}};

    auto result = EnchantmentPredicate::fromJson(json);
    ASSERT_TRUE(result.success());
    ASSERT_EQ(result.value().getEntries().size(), 2u);
    EXPECT_TRUE(result.value().getEntries()[0].isTag);
    EXPECT_EQ(result.value().getEntries()[0].id.toString(), "minecraft:smelts_loot");
    EXPECT_FALSE(result.value().getEntries()[1].isTag);

    // 标签解析器给出标签成员，第一项即可命中。
    const auto tagResolver = [](const ResourceLocation&) {
        return std::vector<std::string>{"minecraft:silk_touch"};
    };
    EnchantmentContainer silkTouch;
    silkTouch.set("minecraft:silk_touch", 1);
    EXPECT_TRUE(result.value().test(silkTouch, tagResolver));

    // 混合写法里的第二项（具体 ID）同样可命中。
    EnchantmentContainer fireAspect;
    fireAspect.set("minecraft:fire_aspect", 2);
    EXPECT_TRUE(result.value().test(fireAspect, tagResolver));
}

TEST_F(EnchantmentPredicateTest, MultiElementSetRoundTripsThroughJson)
{
    const nlohmann::json json = {{"enchantments", {"minecraft:sharpness", "minecraft:smite"}}};

    auto parsed = EnchantmentPredicate::fromJson(json);
    ASSERT_TRUE(parsed.success());

    const nlohmann::json written = parsed.value().toJson();
    auto roundTripped = EnchantmentPredicate::fromJson(written);
    ASSERT_TRUE(roundTripped.success());
    ASSERT_EQ(roundTripped.value().getEntries().size(), 2u);

    EnchantmentContainer smite;
    smite.set("minecraft:smite", 1);
    EXPECT_TRUE(roundTripped.value().test(smite));
}
