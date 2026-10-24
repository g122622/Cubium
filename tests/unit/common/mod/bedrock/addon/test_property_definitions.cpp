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

#include "common/mod/bedrock/addon/content/entity/properties/EntityPropertyDefinitions.hpp"
#include "common/mod/bedrock/addon/content/molang/MolangExpression.hpp"
#include "common/util/math/random/Random.hpp"
#include <set>
#include <gtest/gtest.h>

using namespace mc::mod::bedrock::addon;

TEST(EntityPropertyDefinitionsTest, DefaultExpressionsProduceTypedValuesAndClampNumbers)
{
    const nlohmann::json source = {
        {"test:count", {{"type", "int"}, {"range", {0, 100}}, {"default", "math.pow(10, 3)"}}},
        {"test:speed", {{"type", "float"}, {"range", {0, 10}}, {"default", "math.sqrt(6.25)"}}},
        {"test:flag", {{"type", "bool"}, {"default", "-2"}}},
        {"test:mode", {{"type", "enum"}, {"values", {"idle", "work"}}, {"default", "'work'"}}}};
    auto schema = EntityPropertyDefinitions::compile(source);
    ASSERT_TRUE(schema.success()) << schema.error().message();
    mc::math::Random random(123);
    auto actual = schema.value().instantiate(random);
    ASSERT_TRUE(actual.success()) << actual.error().message();
    EXPECT_EQ(actual.value()["test:count"]["default"], 100);
    EXPECT_EQ(actual.value()["test:speed"]["default"], 2.5);
    EXPECT_EQ(actual.value()["test:flag"]["default"], true);
    EXPECT_EQ(actual.value()["test:mode"]["default"], "work");
}

TEST(EntityPropertyDefinitionsTest, RandomDefaultsAreEvaluatedForEachInstantiation)
{
    auto schema = EntityPropertyDefinitions::compile(
        {{"test:random", {{"type", "int"}, {"range", {1, 9}}, {"default", "math.random_integer(1, 9)"}}}});
    ASSERT_TRUE(schema.success());
    mc::math::Random random(123);
    std::set<int> seen;
    for (int index = 0; index < 40; ++index) {
        auto actual = schema.value().instantiate(random);
        ASSERT_TRUE(actual.success());
        const int value = actual.value()["test:random"]["default"].get<int>();
        EXPECT_GE(value, 1);
        EXPECT_LE(value, 9);
        seen.insert(value);
    }
    EXPECT_GT(seen.size(), 1u);
}

TEST(EntityPropertyDefinitionsTest, DefaultPropertyQueriesAreRejectedEvenInUnselectedBranches)
{
    for (const auto* expression :
        {"q.property('test:other')", "q.has_property('test:other')", "0 ? q.property('test:other') : 2"}) {
        const auto result = EntityPropertyDefinitions::compile(
            {{"test:value", {{"type", "int"}, {"range", {0, 100}}, {"default", expression}}}});
        ASSERT_TRUE(result.failed());
        EXPECT_EQ(result.error().code(), mc::ErrorCode::Unsupported);
    }
}

TEST(EntityPropertyDefinitionsTest, LiteralEnumDefaultsAreNotTreatedAsExpressions)
{
    auto schema = EntityPropertyDefinitions::compile(
        {{"test:mode", {{"type", "enum"}, {"values", {"true", "work"}}, {"default", "true"}}}});
    ASSERT_TRUE(schema.success());
    mc::math::Random random(123);
    auto actual = schema.value().instantiate(random);
    ASSERT_TRUE(actual.success());
    EXPECT_EQ(actual.value()["test:mode"]["default"], "true");
}

TEST(EntityPropertyDefinitionsTest, InvalidRuntimeResultsFailWithoutChangingTheCompiledSchema)
{
    auto schema = EntityPropertyDefinitions::compile(
        {{"test:value", {{"type", "int"}, {"range", {0, 100}}, {"default", "math.sqrt(-1)"}}}});
    ASSERT_TRUE(schema.success());
    const auto before = schema.value().literalSchema();
    mc::math::Random random(123);
    EXPECT_TRUE(schema.value().instantiate(random).failed());
    EXPECT_EQ(schema.value().literalSchema(), before);
}
