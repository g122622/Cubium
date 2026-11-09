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

#include "common/item/loot/base/LevelBasedValue.hpp"
#include "common/item/loot/LootSerializers.hpp"

#include <memory>
#include <vector>

using namespace mc;
using namespace mc::loot;

namespace {

/// @brief 浮点比较的默认容差，避免逐位比较导致脆弱测试。
constexpr f32 kEps = 1e-5f;

} // namespace

// ============================================================================
// 求值语义
// ============================================================================

TEST(LevelBasedValueTest, ConstantIgnoresLevel)
{
    const auto value = LevelBasedValue::constant(0.25f);
    EXPECT_NEAR(value->calculate(0), 0.25f, kEps);
    EXPECT_NEAR(value->calculate(1), 0.25f, kEps);
    EXPECT_NEAR(value->calculate(99), 0.25f, kEps);
}

// 线性：base + perLevelAboveFirst * (level - 1)。等级 1 时等于 base。
TEST(LevelBasedValueTest, LinearStartsAtBaseForLevelOne)
{
    const auto value = LevelBasedValue::linear(0.1f, 0.05f);
    EXPECT_NEAR(value->calculate(1), 0.1f, kEps);
    EXPECT_NEAR(value->calculate(2), 0.15f, kEps);
    EXPECT_NEAR(value->calculate(5), 0.3f, kEps);
}

// 等级平方：level² + added。数据包 efficiency 用的是 added = 1.0。
TEST(LevelBasedValueTest, LevelsSquaredAddsOffset)
{
    const auto value = LevelBasedValue::levelsSquared(1.0f);
    EXPECT_NEAR(value->calculate(1), 2.0f, kEps);
    EXPECT_NEAR(value->calculate(2), 5.0f, kEps);
    EXPECT_NEAR(value->calculate(3), 10.0f, kEps);
}

// 钳制：把内部结果压进 [min, max]。
TEST(LevelBasedValueTest, ClampedLimitsInnerValue)
{
    // 内部线性在等级 1..5 上取 0.1..0.5，钳制到 [0.2, 0.4]。
    auto inner = LevelBasedValue::linear(0.0f, 0.1f);
    const auto value = LevelBasedValue::clamped(std::move(inner), 0.2f, 0.4f);

    EXPECT_NEAR(value->calculate(1), 0.2f, kEps) << "below min must clamp up";
    EXPECT_NEAR(value->calculate(3), 0.2f, kEps) << "inner 0.2 is exactly min";
    EXPECT_NEAR(value->calculate(4), 0.3f, kEps) << "inside range passes through";
    EXPECT_NEAR(value->calculate(9), 0.4f, kEps) << "above max must clamp down";
}

// 分数：分子 / 分母。数据包 unbreaking 的分母是 level + 1。
TEST(LevelBasedValueTest, FractionDividesNumeratorByDenominator)
{
    const auto numerator = LevelBasedValue::linear(1.0f, 1.0f); // = level
    const auto denominator = LevelBasedValue::linear(1.0f, 1.0f);
    const auto value = LevelBasedValue::fraction(numerator->clone(), denominator->clone());

    EXPECT_NEAR(value->calculate(1), 1.0f, kEps);
    EXPECT_NEAR(value->calculate(3), 1.0f, kEps) << "numerator == denominator must always be 1";
}

// 分母为 0 时必须返回 0，而不是 inf/NaN。
TEST(LevelBasedValueTest, FractionWithZeroDenominatorReturnsZero)
{
    const auto value = LevelBasedValue::fraction(
        LevelBasedValue::constant(5.0f), LevelBasedValue::constant(0.0f));
    const f32 result = value->calculate(1);
    EXPECT_FLOAT_EQ(result, 0.0f);
    EXPECT_FALSE(std::isnan(result));
    EXPECT_FALSE(std::isinf(result));
}

// 指数：pow(base, power)。
TEST(LevelBasedValueTest, ExponentRaisesBaseToPower)
{
    const auto value = LevelBasedValue::exponent(
        LevelBasedValue::constant(2.0f), LevelBasedValue::constant(3.0f));
    EXPECT_NEAR(value->calculate(1), 8.0f, kEps);
}

// 查表：等级在表长内取对应项，超出后走回退。
TEST(LevelBasedValueTest, LookupUsesTableThenFallback)
{
    const auto value = LevelBasedValue::lookup({0.1f, 0.2f, 0.3f}, LevelBasedValue::constant(0.9f));

    EXPECT_NEAR(value->calculate(1), 0.1f, kEps) << "index 0 corresponds to level 1";
    EXPECT_NEAR(value->calculate(3), 0.3f, kEps);
    EXPECT_NEAR(value->calculate(4), 0.9f, kEps) << "beyond table length uses fallback";
}

// 等级 0（无附魔）不应读到表的负下标。
TEST(LevelBasedValueTest, LookupHandlesLevelZeroViaFallback)
{
    const auto value = LevelBasedValue::lookup({0.1f, 0.2f}, LevelBasedValue::constant(0.5f));
    EXPECT_NEAR(value->calculate(0), 0.5f, kEps) << "level 0 must not index values[-1]";
}

// clone 必须是深拷贝：改内部不影响原对象（表达式树不能共享可变状态）。
TEST(LevelBasedValueTest, CloneIsIndependent)
{
    const auto value = LevelBasedValue::linear(1.0f, 2.0f);
    const auto copy = value->clone();
    ASSERT_NE(copy, nullptr);
    EXPECT_NEAR(copy->calculate(3), value->calculate(3), kEps);
    EXPECT_NE(copy.get(), value.get());
}

namespace mc {
namespace loot {

/// @brief 直接验证 LevelBasedValue 的 JSON 解析。
///
/// 类名必须是 mc::loot::LevelBasedValueParseTest：LootSerializers 以
/// `friend class ::mc::loot::LevelBasedValueParseTest` 授予私有静态方法访问权。
class LevelBasedValueParseTest : public ::testing::Test {};
// ============================================================================
// JSON 解析
// ============================================================================

TEST_F(LevelBasedValueParseTest, ParsesBareNumberAsConstant)
{
    auto result = LootSerializers::parseLevelBasedValue(nlohmann::json(0.35));
    ASSERT_TRUE(result.success());
    const auto constant = result.value();
    ASSERT_NE(constant, nullptr);
    EXPECT_NEAR(constant->calculate(7), 0.35f, kEps);
}

TEST_F(LevelBasedValueParseTest, ParsesNestedClampedFraction)
{
    // 模拟数据包 frost_walker 的嵌套写法：clamped 包住线性。
    const auto json = nlohmann::json::parse(R"({
        "type": "minecraft:clamped",
        "min": 0.0,
        "max": 1.0,
        "value": {
            "type": "minecraft:linear",
            "base": 0.0,
            "per_level_above_first": 0.25
        }
    })");

    auto result = LootSerializers::parseLevelBasedValue(json);
    ASSERT_TRUE(result.success());
    // Result<std::unique_ptr<T>> 的 value() 转移所有权，只能取一次。
    const auto value = result.value();
    ASSERT_NE(value, nullptr);
    EXPECT_NEAR(value->calculate(2), 0.25f, kEps);
    EXPECT_NEAR(value->calculate(9), 1.0f, kEps) << "clamped at max";
}

TEST_F(LevelBasedValueParseTest, ParsesLookupWithFallback)
{
    // 模拟 wind_burst 的 lookup 写法。
    const auto json = nlohmann::json::parse(R"({
        "type": "minecraft:lookup",
        "values": [1.0, 2.0],
        "fallback": { "type": "minecraft:linear", "base": 3.0, "per_level_above_first": 1.0 }
    })");

    auto result = LootSerializers::parseLevelBasedValue(json);
    ASSERT_TRUE(result.success());
    const auto value = result.value();
    ASSERT_NE(value, nullptr);
    EXPECT_NEAR(value->calculate(1), 1.0f, kEps);
    EXPECT_NEAR(value->calculate(5), 7.0f, kEps) << "fallback linear: 3 + 1*(5-1)";
}

TEST_F(LevelBasedValueParseTest, AcceptsShortTypeName)
{
    const auto json = nlohmann::json::parse(R"({ "type": "levels_squared", "added": 1.0 })");
    auto result = LootSerializers::parseLevelBasedValue(json);
    ASSERT_TRUE(result.success());
    const auto squared = result.value();
    ASSERT_NE(squared, nullptr);
    EXPECT_NEAR(squared->calculate(3), 10.0f, kEps);
}

TEST_F(LevelBasedValueParseTest, RejectsUnknownType)
{
    const auto json = nlohmann::json::parse(R"({ "type": "minecraft:bogus" })");
    auto result = LootSerializers::parseLevelBasedValue(json);
    EXPECT_FALSE(result.success());
}

// 缺少 field 必须报错，不能静默取 0——那会让附魔加成悄悄变成无用数值。
TEST_F(LevelBasedValueParseTest, RejectsMissingNumericField)
{
    const auto json = nlohmann::json::parse(R"({ "type": "minecraft:levels_squared" })");
    auto result = LootSerializers::parseLevelBasedValue(json);
    EXPECT_FALSE(result.success());
}

TEST_F(LevelBasedValueParseTest, RejectsEmptyLookupValues)
{
    const auto json = nlohmann::json::parse(R"({
        "type": "minecraft:lookup",
        "values": [],
        "fallback": 1.0
    })");
    auto result = LootSerializers::parseLevelBasedValue(json);
    EXPECT_FALSE(result.success());
}

} // namespace loot
} // namespace mc
