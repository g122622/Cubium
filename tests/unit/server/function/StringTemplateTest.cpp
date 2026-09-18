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

#include "server/function/StringTemplate.hpp"
#include <stdexcept>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::function;

/**
 * @brief StringTemplate 单元测试
 *
 * 对应 MC 1.21.11 net.minecraft.commands.functions.StringTemplate 的行为。
 */
class StringTemplateTest : public ::testing::Test {};

TEST_F(StringTemplateTest, FromString_SingleVariable)
{
    auto tmpl = StringTemplate::fromString("say $(name)");
    ASSERT_EQ(tmpl.variables().size(), 1u);
    EXPECT_EQ(tmpl.variables()[0], "name");
    // segments 数量 = variables + 1
    ASSERT_EQ(tmpl.segments().size(), 2u);
    EXPECT_EQ(tmpl.segments()[0], "say ");
    EXPECT_EQ(tmpl.segments()[1], "");
}

TEST_F(StringTemplateTest, FromString_MultipleVariables)
{
    auto tmpl = StringTemplate::fromString("say $(a) and $(b)");
    ASSERT_EQ(tmpl.variables().size(), 2u);
    EXPECT_EQ(tmpl.variables()[0], "a");
    EXPECT_EQ(tmpl.variables()[1], "b");
    ASSERT_EQ(tmpl.segments().size(), 3u);
    EXPECT_EQ(tmpl.segments()[0], "say ");
    EXPECT_EQ(tmpl.segments()[1], " and ");
    EXPECT_EQ(tmpl.segments()[2], "");
}

TEST_F(StringTemplateTest, FromString_VariableAtStart)
{
    auto tmpl = StringTemplate::fromString("$(greeting) world");
    ASSERT_EQ(tmpl.variables().size(), 1u);
    EXPECT_EQ(tmpl.variables()[0], "greeting");
    ASSERT_EQ(tmpl.segments().size(), 2u);
    EXPECT_EQ(tmpl.segments()[0], "");
    EXPECT_EQ(tmpl.segments()[1], " world");
}

TEST_F(StringTemplateTest, FromString_VariableAtEnd)
{
    auto tmpl = StringTemplate::fromString("hello $(name)");
    ASSERT_EQ(tmpl.variables().size(), 1u);
    EXPECT_EQ(tmpl.variables()[0], "name");
    ASSERT_EQ(tmpl.segments().size(), 2u);
    EXPECT_EQ(tmpl.segments()[0], "hello ");
    // 末尾的空 segment 也保留
    EXPECT_EQ(tmpl.segments()[1], "");
}

TEST_F(StringTemplateTest, FromString_NoVariables_Throws)
{
    // 无变量应抛异常
    EXPECT_THROW(StringTemplate::fromString("say hello"), std::invalid_argument);
}

TEST_F(StringTemplateTest, FromString_UnterminatedVariable_Throws)
{
    EXPECT_THROW(StringTemplate::fromString("say $(name"), std::invalid_argument);
}

TEST_F(StringTemplateTest, FromString_InvalidVariableName_Throws)
{
    // 含非法字符（-）
    EXPECT_THROW(StringTemplate::fromString("say $(na-me)"), std::invalid_argument);
}

TEST_F(StringTemplateTest, FromString_DollarNotFollowedByParen)
{
    // '$' 不跟 '('，不视为变量起始，但若无其他变量则抛 "No variables"
    EXPECT_THROW(StringTemplate::fromString("say $5"), std::invalid_argument);
    // 混合：先有 $5（普通字符）后有 $(var)
    auto tmpl = StringTemplate::fromString("say $5 and $(var)");
    ASSERT_EQ(tmpl.variables().size(), 1u);
    EXPECT_EQ(tmpl.variables()[0], "var");
    ASSERT_EQ(tmpl.segments().size(), 2u);
    EXPECT_EQ(tmpl.segments()[0], "say $5 and ");
    EXPECT_EQ(tmpl.segments()[1], "");
}

TEST_F(StringTemplateTest, FromString_VariableNameWithUnderscoreAndDigits)
{
    auto tmpl = StringTemplate::fromString("$(var_1)_$(var_2)");
    ASSERT_EQ(tmpl.variables().size(), 2u);
    EXPECT_EQ(tmpl.variables()[0], "var_1");
    EXPECT_EQ(tmpl.variables()[1], "var_2");
}

TEST_F(StringTemplateTest, IsValidVariableName)
{
    EXPECT_TRUE(StringTemplate::isValidVariableName("abc"));
    EXPECT_TRUE(StringTemplate::isValidVariableName("a1b2"));
    EXPECT_TRUE(StringTemplate::isValidVariableName("_under"));
    EXPECT_TRUE(StringTemplate::isValidVariableName("CamelCase"));
    EXPECT_FALSE(StringTemplate::isValidVariableName(""));
    EXPECT_FALSE(StringTemplate::isValidVariableName("has-dash"));
    EXPECT_FALSE(StringTemplate::isValidVariableName("has space"));
    EXPECT_FALSE(StringTemplate::isValidVariableName("has.dot"));
    EXPECT_FALSE(StringTemplate::isValidVariableName("has$sign"));
}

TEST_F(StringTemplateTest, Substitute_SingleVariable)
{
    auto tmpl = StringTemplate::fromString("say $(name)");
    auto result = tmpl.substitute({"Steve"});
    EXPECT_EQ(result, "say Steve");
}

TEST_F(StringTemplateTest, Substitute_MultipleVariables)
{
    auto tmpl = StringTemplate::fromString("say $(a) and $(b)");
    auto result = tmpl.substitute({"Alex", "Bob"});
    EXPECT_EQ(result, "say Alex and Bob");
}

TEST_F(StringTemplateTest, Substitute_VariableInMiddle)
{
    auto tmpl = StringTemplate::fromString("tellraw @a {\"text\":\"$(msg)\"}");
    auto result = tmpl.substitute({"hello world"});
    EXPECT_EQ(result, "tellraw @a {\"text\":\"hello world\"}");
}

TEST_F(StringTemplateTest, Substitute_CountMismatch_Throws)
{
    auto tmpl = StringTemplate::fromString("say $(a) and $(b)");
    // values 数量与 variables 不匹配
    EXPECT_THROW(tmpl.substitute({"only_one"}), std::invalid_argument);
    EXPECT_THROW(tmpl.substitute({"a", "b", "c"}), std::invalid_argument);
}

TEST_F(StringTemplateTest, Substitute_RepeatedVariable)
{
    // 同一变量多次出现：StringTemplate.variables() 会重复，substitute 期望 values 与之一一对应
    auto tmpl = StringTemplate::fromString("$(a) $(a) $(a)");
    ASSERT_EQ(tmpl.variables().size(), 3u);
    auto result = tmpl.substitute({"X", "X", "X"});
    EXPECT_EQ(result, "X X X");
}
