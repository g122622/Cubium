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

#include "common/world/gen/valueprovider/IntProviderParser.hpp"

#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::world::gen::valueprovider;

// ============================================================================
// 裸整数简写测试
// ============================================================================

TEST(IntProviderParserTest, BareInteger_ReturnsConstantInt)
{
    nlohmann::json json = 5;
    auto result = IntProviderParser::parse(json);
    ASSERT_TRUE(result.success());
    auto provider = result.value();
    ASSERT_NE(provider, nullptr);
    EXPECT_EQ(provider->getMinValue(), 5);
    EXPECT_EQ(provider->getMaxValue(), 5);
    EXPECT_STREQ(provider->getTypeName(), "constant");
}

TEST(IntProviderParserTest, BareInteger_Zero)
{
    nlohmann::json json = 0;
    auto result = IntProviderParser::parse(json);
    ASSERT_TRUE(result.success());
    auto provider = result.value();
    EXPECT_EQ(provider->getMinValue(), 0);
    EXPECT_EQ(provider->getMaxValue(), 0);
}

TEST(IntProviderParserTest, BareInteger_Negative)
{
    nlohmann::json json = -3;
    auto result = IntProviderParser::parse(json);
    ASSERT_TRUE(result.success());
    auto provider = result.value();
    EXPECT_EQ(provider->getMinValue(), -3);
    EXPECT_EQ(provider->getMaxValue(), -3);
}

// ============================================================================
// ConstantInt 解析测试
// ============================================================================

TEST(IntProviderParserTest, ConstantInt_NestedValue)
{
    nlohmann::json json = {{"type", "minecraft:constant"}, {"value", 10}};
    auto result = IntProviderParser::parse(json);
    ASSERT_TRUE(result.success());
    auto provider = result.value();
    EXPECT_EQ(provider->getMinValue(), 10);
    EXPECT_EQ(provider->getMaxValue(), 10);
    EXPECT_STREQ(provider->getTypeName(), "constant");
}

TEST(IntProviderParserTest, ConstantInt_WithoutNamespace)
{
    nlohmann::json json = {{"type", "constant"}, {"value", 7}};
    auto result = IntProviderParser::parse(json);
    ASSERT_TRUE(result.success());
    auto provider = result.value();
    EXPECT_EQ(provider->getMinValue(), 7);
}

TEST(IntProviderParserTest, ConstantInt_MissingValue_Error)
{
    nlohmann::json json = {{"type", "constant"}};
    auto result = IntProviderParser::parse(json);
    EXPECT_FALSE(result.success());
}

// ============================================================================
// UniformInt 解析测试
// ============================================================================

TEST(IntProviderParserTest, UniformInt_Basic)
{
    nlohmann::json json = {{"type", "minecraft:uniform"}, {"min_inclusive", 2}, {"max_inclusive", 8}};
    auto result = IntProviderParser::parse(json);
    ASSERT_TRUE(result.success());
    auto provider = result.value();
    EXPECT_EQ(provider->getMinValue(), 2);
    EXPECT_EQ(provider->getMaxValue(), 8);
    EXPECT_STREQ(provider->getTypeName(), "uniform");
}

TEST(IntProviderParserTest, UniformInt_SameMinMax)
{
    nlohmann::json json = {{"type", "uniform"}, {"min_inclusive", 5}, {"max_inclusive", 5}};
    auto result = IntProviderParser::parse(json);
    ASSERT_TRUE(result.success());
    auto provider = result.value();
    EXPECT_EQ(provider->getMinValue(), 5);
    EXPECT_EQ(provider->getMaxValue(), 5);
}

TEST(IntProviderParserTest, UniformInt_MinGreaterThanMax_Error)
{
    nlohmann::json json = {{"type", "uniform"}, {"min_inclusive", 10}, {"max_inclusive", 5}};
    auto result = IntProviderParser::parse(json);
    EXPECT_FALSE(result.success());
}

TEST(IntProviderParserTest, UniformInt_MissingField_Error)
{
    nlohmann::json json = {{"type", "uniform"}, {"min_inclusive", 0}};
    auto result = IntProviderParser::parse(json);
    EXPECT_FALSE(result.success());
}

// ============================================================================
// BiasedToBottomInt 解析测试
// ============================================================================

TEST(IntProviderParserTest, BiasedToBottom_Basic)
{
    nlohmann::json json = {{"type", "biased_to_bottom"}, {"min_inclusive", 1}, {"max_inclusive", 10}};
    auto result = IntProviderParser::parse(json);
    ASSERT_TRUE(result.success());
    auto provider = result.value();
    EXPECT_EQ(provider->getMinValue(), 1);
    EXPECT_EQ(provider->getMaxValue(), 10);
    EXPECT_STREQ(provider->getTypeName(), "biased_to_bottom");
}

TEST(IntProviderParserTest, BiasedToBottom_MinGreaterThanMax_Error)
{
    nlohmann::json json = {{"type", "biased_to_bottom"}, {"min_inclusive", 5}, {"max_inclusive", 2}};
    auto result = IntProviderParser::parse(json);
    EXPECT_FALSE(result.success());
}

// ============================================================================
// ClampedInt 解析测试
// ============================================================================

TEST(IntProviderParserTest, ClampedInt_Basic)
{
    nlohmann::json json = {{"type", "clamped"},
        {"source", {{"type", "uniform"}, {"min_inclusive", 0}, {"max_inclusive", 20}}},
        {"min_inclusive", 5},
        {"max_inclusive", 15}};
    auto result = IntProviderParser::parse(json);
    ASSERT_TRUE(result.success());
    auto provider = result.value();
    EXPECT_EQ(provider->getMinValue(), 5);
    EXPECT_EQ(provider->getMaxValue(), 15);
    EXPECT_STREQ(provider->getTypeName(), "clamped");
}

TEST(IntProviderParserTest, ClampedInt_NestedBareIntegerSource)
{
    nlohmann::json json = {{"type", "clamped"}, {"source", 10}, {"min_inclusive", 3}, {"max_inclusive", 7}};
    auto result = IntProviderParser::parse(json);
    ASSERT_TRUE(result.success());
    auto provider = result.value();
    EXPECT_EQ(provider->getMinValue(), 3);
    EXPECT_EQ(provider->getMaxValue(), 7);
}

TEST(IntProviderParserTest, ClampedInt_MissingSource_Error)
{
    nlohmann::json json = {{"type", "clamped"}, {"min_inclusive", 5}, {"max_inclusive", 15}};
    auto result = IntProviderParser::parse(json);
    EXPECT_FALSE(result.success());
}

// ============================================================================
// ClampedNormalInt 解析测试
// ============================================================================

TEST(IntProviderParserTest, ClampedNormal_Basic)
{
    nlohmann::json json = {
        {"type", "clamped_normal"}, {"mean", 5.0}, {"deviation", 2.0}, {"min_inclusive", 0}, {"max_inclusive", 10}};
    auto result = IntProviderParser::parse(json);
    ASSERT_TRUE(result.success());
    auto provider = result.value();
    EXPECT_EQ(provider->getMinValue(), 0);
    EXPECT_EQ(provider->getMaxValue(), 10);
    EXPECT_STREQ(provider->getTypeName(), "clamped_normal");
}

TEST(IntProviderParserTest, ClampedNormal_MissingField_Error)
{
    nlohmann::json json = {{"type", "clamped_normal"}, {"mean", 5.0}, {"min_inclusive", 0}, {"max_inclusive", 10}};
    auto result = IntProviderParser::parse(json);
    EXPECT_FALSE(result.success());
}

// ============================================================================
// WeightedListInt 解析测试
// ============================================================================

TEST(IntProviderParserTest, WeightedList_Basic)
{
    nlohmann::json json = {{"type", "weighted_list"},
        {"distribution",
            nlohmann::json::array({{{"data", 5}, {"weight", 3}},
                {{"data", {{"type", "uniform"}, {"min_inclusive", 0}, {"max_inclusive", 10}}}, {"weight", 2}}})}};
    auto result = IntProviderParser::parse(json);
    ASSERT_TRUE(result.success());
    auto provider = result.value();
    EXPECT_EQ(provider->getMinValue(), 0);
    EXPECT_EQ(provider->getMaxValue(), 10);
    EXPECT_STREQ(provider->getTypeName(), "weighted_list");
}

TEST(IntProviderParserTest, WeightedList_EmptyDistribution_Error)
{
    nlohmann::json json = {{"type", "weighted_list"}, {"distribution", nlohmann::json::array()}};
    auto result = IntProviderParser::parse(json);
    EXPECT_FALSE(result.success());
}

TEST(IntProviderParserTest, WeightedList_MissingDistribution_Error)
{
    nlohmann::json json = {{"type", "weighted_list"}};
    auto result = IntProviderParser::parse(json);
    EXPECT_FALSE(result.success());
}

TEST(IntProviderParserTest, WeightedList_NegativeWeight_Error)
{
    nlohmann::json json = {
        {"type", "weighted_list"}, {"distribution", nlohmann::json::array({{{"data", 5}, {"weight", -1}}})}};
    auto result = IntProviderParser::parse(json);
    EXPECT_FALSE(result.success());
}

// ============================================================================
// 未知类型和错误输入测试
// ============================================================================

TEST(IntProviderParserTest, UnknownType_Error)
{
    nlohmann::json json = {{"type", "unknown_type"}, {"value", 5}};
    auto result = IntProviderParser::parse(json);
    EXPECT_FALSE(result.success());
}

TEST(IntProviderParserTest, MissingType_Error)
{
    nlohmann::json json = {{"value", 5}};
    auto result = IntProviderParser::parse(json);
    EXPECT_FALSE(result.success());
}

TEST(IntProviderParserTest, NullInput_Error)
{
    nlohmann::json json = nullptr;
    auto result = IntProviderParser::parse(json);
    EXPECT_FALSE(result.success());
}

TEST(IntProviderParserTest, ArrayInput_Error)
{
    nlohmann::json json = nlohmann::json::array({1, 2, 3});
    auto result = IntProviderParser::parse(json);
    EXPECT_FALSE(result.success());
}

TEST(IntProviderParserTest, StringInput_Error)
{
    nlohmann::json json = "hello";
    auto result = IntProviderParser::parse(json);
    EXPECT_FALSE(result.success());
}

TEST(IntProviderParserTest, FloatInput_Error)
{
    nlohmann::json json = 3.14;
    auto result = IntProviderParser::parse(json);
    EXPECT_FALSE(result.success());
}

// ============================================================================
// 范围校验测试
// ============================================================================

TEST(IntProviderParserTest, PositiveValidation_BareInteger_Pass)
{
    nlohmann::json json = 5;
    auto result = IntProviderParser::parse(json, 1); // minValue >= 1
    ASSERT_TRUE(result.success());
    auto provider = result.value();
    EXPECT_EQ(provider->getMinValue(), 5);
}

TEST(IntProviderParserTest, PositiveValidation_BareInteger_Fail)
{
    nlohmann::json json = 0;
    auto result = IntProviderParser::parse(json, 1); // minValue >= 1, but 0 < 1
    EXPECT_FALSE(result.success());
}

TEST(IntProviderParserTest, PositiveValidation_Uniform_Pass)
{
    nlohmann::json json = {{"type", "uniform"}, {"min_inclusive", 2}, {"max_inclusive", 8}};
    auto result = IntProviderParser::parse(json, 1); // minValue >= 1
    ASSERT_TRUE(result.success());
    auto provider = result.value();
    EXPECT_EQ(provider->getMinValue(), 2);
}

TEST(IntProviderParserTest, PositiveValidation_Uniform_Fail)
{
    nlohmann::json json = {{"type", "uniform"}, {"min_inclusive", 0}, {"max_inclusive", 5}};
    auto result = IntProviderParser::parse(json, 1); // minValue >= 1, but 0 < 1
    EXPECT_FALSE(result.success());
}

TEST(IntProviderParserTest, MaxValidation_Uniform_Fail)
{
    nlohmann::json json = {{"type", "uniform"}, {"min_inclusive", 1}, {"max_inclusive", 20}};
    auto result = IntProviderParser::parse(json, std::nullopt, 10); // maxValue <= 10, but 20 > 10
    EXPECT_FALSE(result.success());
}

TEST(IntProviderParserTest, NoValidation_Pass)
{
    // 默认不校验范围
    nlohmann::json json = {{"type", "uniform"}, {"min_inclusive", -100}, {"max_inclusive", 100}};
    auto result = IntProviderParser::parse(json);
    ASSERT_TRUE(result.success());
    auto provider = result.value();
    EXPECT_EQ(provider->getMinValue(), -100);
    EXPECT_EQ(provider->getMaxValue(), 100);
}

TEST(IntProviderParserTest, NegativeMinInclusive_BareInteger_Fail)
{
    // 负下界校验：random_offset 的 xz_spread/y_spread 范围 [-16,16]。
    // -100 < -16 应被拒绝（旧实现 minInclusive>=0 守卫会把 -16 当哨兵跳过，此为回归测试）。
    nlohmann::json json = -100;
    auto result = IntProviderParser::parse(json, -16, 16);
    EXPECT_FALSE(result.success());
}

TEST(IntProviderParserTest, NegativeMinInclusive_BareInteger_Pass)
{
    // -16 恰好等于下界，应通过。
    nlohmann::json json = -16;
    auto result = IntProviderParser::parse(json, -16, 16);
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value()->getMinValue(), -16);
}

TEST(IntProviderParserTest, NegativeMinInclusive_Uniform_Fail)
{
    // uniform(-100, -50) 的 min=-100 < -16，应被拒绝。
    nlohmann::json json = {{"type", "uniform"}, {"min_inclusive", -100}, {"max_inclusive", -50}};
    auto result = IntProviderParser::parse(json, -16, 16);
    EXPECT_FALSE(result.success());
}

TEST(IntProviderParserTest, NegativeMinInclusive_Uniform_Pass)
{
    // uniform(-16, 16) 恰好落在 [-16,16]，应通过。
    nlohmann::json json = {{"type", "uniform"}, {"min_inclusive", -16}, {"max_inclusive", 16}};
    auto result = IntProviderParser::parse(json, -16, 16);
    ASSERT_TRUE(result.success());
    auto provider = result.value();
    EXPECT_EQ(provider->getMinValue(), -16);
    EXPECT_EQ(provider->getMaxValue(), 16);
}

// ============================================================================
// clone() 测试
// ============================================================================

TEST(IntProviderParserTest, Clone_ConstantInt)
{
    auto original = std::make_unique<ConstantInt>(42);
    auto cloned = original->clone();
    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->getMinValue(), 42);
    EXPECT_EQ(cloned->getMaxValue(), 42);
    EXPECT_STREQ(cloned->getTypeName(), "constant");
}

TEST(IntProviderParserTest, Clone_UniformInt)
{
    auto original = std::make_unique<UniformInt>(3, 7);
    auto cloned = original->clone();
    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->getMinValue(), 3);
    EXPECT_EQ(cloned->getMaxValue(), 7);
    EXPECT_STREQ(cloned->getTypeName(), "uniform");
}

TEST(IntProviderParserTest, Clone_ClampedInt_DeepCopy)
{
    auto source = std::make_unique<UniformInt>(0, 20);
    auto original = std::make_unique<ClampedInt>(std::move(source), 5, 15);
    auto cloned = original->clone();
    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->getMinValue(), 5);
    EXPECT_EQ(cloned->getMaxValue(), 15);
    EXPECT_STREQ(cloned->getTypeName(), "clamped");
}

TEST(IntProviderParserTest, Clone_WeightedListInt_DeepCopy)
{
    std::vector<WeightedListInt::WeightedEntry> entries;
    WeightedListInt::WeightedEntry entry1;
    entry1.provider = std::make_unique<ConstantInt>(3);
    entry1.weight = 2;
    entries.push_back(std::move(entry1));
    WeightedListInt::WeightedEntry entry2;
    entry2.provider = std::make_unique<UniformInt>(5, 10);
    entry2.weight = 3;
    entries.push_back(std::move(entry2));

    auto original = std::make_unique<WeightedListInt>(std::move(entries));
    auto cloned = original->clone();
    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->getMinValue(), 3);
    EXPECT_EQ(cloned->getMaxValue(), 10);
    EXPECT_STREQ(cloned->getTypeName(), "weighted_list");
}
