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

#include "common/mod/bedrock/addon/content/molang/MolangExpression.hpp"
#include "common/util/math/random/Random.hpp"

#include <cmath>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include <gtest/gtest.h>

using namespace mc::mod::bedrock::addon;

namespace {
class MolangExpressionTest : public testing::Test {
protected:
    mc::Result<MolangValue> evaluate(const std::string& source)
    {
        auto expression = MolangExpression::compile(source);
        if (expression.failed()) {
            return expression.error();
        }
        return expression.value().evaluate(context);
    }

    void number(const std::string& source, mc::f32 expected)
    {
        auto result = evaluate(source);
        ASSERT_TRUE(result.success()) << source << ": " << result.error().message();
        ASSERT_TRUE(std::holds_alternative<mc::f32>(result.value())) << source;
        EXPECT_NEAR(std::get<mc::f32>(result.value()), expected, 0.0001f) << source;
    }

    mc::math::Random random{123};
    std::map<std::string, MolangValue> properties;
    MolangEvaluationContext context{random, [this](std::string_view name) -> std::optional<MolangValue> {
                                        const auto found = properties.find(std::string(name));
                                        return found == properties.end() ? std::nullopt
                                                                         : std::optional<MolangValue>(found->second);
                                    }};
};
} // namespace

TEST_F(MolangExpressionTest, ArithmeticPrecedenceAssociativityAndUnaryOperators)
{
    number("2 + 3 * 4", 14);
    number("(2 + 3) * 4", 20);
    number("20 / 5 / 2", 2);
    number("10 - 3 - 2", 5);
    number("-2 * -3 + !0", 7);
    number("+4 % 3", 1);
}

TEST_F(MolangExpressionTest, WhitespaceScientificNumbersReturnAndCaseInsensitiveNames)
{
    properties["test:count"] = 4.0f;
    number(" \t return .5e2 + 1.5; \n", 51.5f);
    number("Q.Property('test:count') + MATH.FLOOR(2.9)", 6);
    number("TRUE && !FALSE", 1);
}

TEST_F(MolangExpressionTest, LogicalAndConditionalBranchesShortCircuit)
{
    number("0 && math.sqrt(-1)", 0);
    number("1 || math.ln(-1)", 1);
    number("1 ? 11 : math.sqrt(-1)", 11);
    number("0 ? math.ln(-1) : 12", 12);
    number("0 ? 42", 0);
    number("1 ? 42", 42);
    number("0 ? 1 : 1 ? 2 : 3", 2);
}

TEST_F(MolangExpressionTest, PropertyQueriesAndMissingPropertySemantics)
{
    properties["test:count"] = 7.0f;
    number("q.property('test:count') + 1", 8);
    number("query.has_property('test:count')", 1);
    number("q.has_property('test:missing')", 0);
    number("q.property('test:missing')", 0);
    number("q.has_property('test:missing') ? q.property('test:missing') : 5", 5);
}

TEST_F(MolangExpressionTest, StringsPreserveCaseAndCanBeConditionalEnumResults)
{
    properties["test:mode"] = std::string("Maid");
    number("q.property('test:mode') == 'Maid'", 1);
    number("q.property('test:mode') == 'maid'", 0);
    number("'first' != 'second'", 1);
    auto result = evaluate("q.property('test:mode') == 'Maid' ? 'working' : 'idle'");
    ASSERT_TRUE(result.success());
    ASSERT_TRUE(std::holds_alternative<std::string>(result.value()));
    EXPECT_EQ(std::get<std::string>(result.value()), "working");
}

TEST_F(MolangExpressionTest, DegreeBasedTrigonometryAndCoreMathFunctions)
{
    number("math.sin(90)", 1);
    number("math.cos(180)", -1);
    number("math.asin(1)", 90);
    number("math.acos(0)", 90);
    number("math.atan(1)", 45);
    number("math.atan2(1, 0)", 90);
    number("math.abs(-3) + math.ceil(1.2) + math.floor(2.9)", 7);
    number("math.clamp(20, 0, 10)", 10);
    number("math.max(2, 5) + math.min(2, 5)", 7);
    number("math.pow(2, 3) + math.sqrt(9)", 11);
    number("math.trunc(-2.9) + math.round(2.2)", 0);
    number("math.sign(-5) + math.copy_sign(3, -1)", -4);
    number("math.exp(0) + math.ln(1)", 1);
    number("math.hermite_blend(0.5)", 0.5f);
    number("math.inverse_lerp(10, 20, 15)", 0.5f);
    number("math.lerp(10, 20, 0.5)", 15);
    number("math.lerprotate(350, 10, 0.5)", 360);
    number("math.min_angle(540)", -180);
    number("math.mod(10, 3)", 1);
    number("math.pi > 3.14 && math.pi < 3.15", 1);
}

TEST_F(MolangExpressionTest, NumericOperationsUseSinglePrecision)
{
    number("(16777216 + 1) - 16777216", 0);
    number("1 / 0", 0);
    number("math.mod(1, 0)", 0);
    number("math.inverse_lerp(1, 1, 5)", 0);
}

TEST_F(MolangExpressionTest, RandomIntervalsIncludeIntegerBoundsAndUseHostRandomSource)
{
    auto expression = MolangExpression::compile("math.random_integer(-3, 4)");
    ASSERT_TRUE(expression.success());
    bool sawLow = false;
    bool sawHigh = false;
    for (int index = 0; index < 100; ++index) {
        auto value = expression.value().evaluate(context);
        ASSERT_TRUE(value.success());
        const auto number = std::get<mc::f32>(value.value());
        EXPECT_GE(number, -3);
        EXPECT_LE(number, 4);
        EXPECT_EQ(std::floor(number), number);
        sawLow = sawLow || number == -3;
        sawHigh = sawHigh || number == 4;
    }
    EXPECT_TRUE(sawLow);
    EXPECT_TRUE(sawHigh);
    number("math.random(2, 2)", 2);
    number("math.random_integer(3, 3)", 3);
}

TEST_F(MolangExpressionTest, MalformedAndUnsupportedSyntaxFailsDuringCompilation)
{
    const std::vector<std::string> invalid = {"",
        "1 +",
        "(1 + 2",
        "'unclosed",
        "q.property()",
        "math.sin(1, 2)",
        "math.unknown(1)",
        "variable.counter + 1",
        "1e100",
        "1 2",
        "q.property('name', 2)",
        "return;"};
    for (const auto& source : invalid) {
        EXPECT_TRUE(MolangExpression::compile(source).failed()) << source;
    }
}

TEST_F(MolangExpressionTest, RuntimeTypeAndDomainErrorsAreReturned)
{
    for (const auto& source : {"math.sqrt('bad')",
             "q.property(1)",
             "math.clamp(1, 5, 2)",
             "math.random_integer(4, 2)",
             "math.sqrt(-1)",
             "math.ln(-1)"}) {
        auto expression = MolangExpression::compile(source);
        ASSERT_TRUE(expression.success()) << source;
        EXPECT_TRUE(expression.value().evaluate(context).failed()) << source;
    }
}

TEST_F(MolangExpressionTest, CompiledProgramsReadEachEvaluationContext)
{
    auto expression = MolangExpression::compile("q.property('test:value') * 2");
    ASSERT_TRUE(expression.success());
    properties["test:value"] = 3.0f;
    auto first = expression.value().evaluate(context);
    ASSERT_TRUE(first.success());
    EXPECT_EQ(std::get<mc::f32>(first.value()), 6);
    properties["test:value"] = 8.0f;
    auto second = expression.value().evaluate(context);
    ASSERT_TRUE(second.success());
    EXPECT_EQ(std::get<mc::f32>(second.value()), 16);
}

TEST_F(MolangExpressionTest, ExpressionSizeAndNestingAreBounded)
{
    EXPECT_TRUE(MolangExpression::compile(std::string(200, '(') + "1" + std::string(200, ')')).failed());
    std::string source = "1";
    for (int index = 0; index < 200; ++index) {
        source += "+1";
    }
    EXPECT_TRUE(MolangExpression::compile(source).failed());
}
