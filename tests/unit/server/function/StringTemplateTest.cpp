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
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::function;

/**
 * @brief StringTemplate 单元测试
 *
 * 对应 MC 1.21.11 net.minecraft.commands.functions.StringTemplate 的行为。
 */
class StringTemplateTest : public ::testing::Test {};

namespace {

/// 测试辅助：解析成功返回模板，失败返回 nullopt（失败原因由生产代码承担）
std::optional<StringTemplate> _parseTemplate(const std::string& text)
{
    auto result = StringTemplate::fromString(text);
    if (result.failed()) {
        return std::nullopt;
    }
    return std::move(result).value();
}

/// 测试辅助：替换成功返回字符串，失败返回 nullopt
std::optional<std::string> _substitute(const StringTemplate& tmpl, const std::vector<std::string>& values)
{
    auto result = tmpl.substitute(values);
    if (result.failed()) {
        return std::nullopt;
    }
    return std::move(result).value();
}

} // namespace

TEST_F(StringTemplateTest, FromString_SingleVariable)
{
    auto tmpl = _parseTemplate("say $(name)");
    ASSERT_TRUE(tmpl.has_value());
    ASSERT_EQ(tmpl->variables().size(), 1u);
    EXPECT_EQ(tmpl->variables()[0], "name");
    // segments 数量 = variables + 1
    ASSERT_EQ(tmpl->segments().size(), 2u);
    EXPECT_EQ(tmpl->segments()[0], "say ");
    EXPECT_EQ(tmpl->segments()[1], "");
}

TEST_F(StringTemplateTest, FromString_MultipleVariables)
{
    auto tmpl = _parseTemplate("say $(a) and $(b)");
    ASSERT_TRUE(tmpl.has_value());
    ASSERT_EQ(tmpl->variables().size(), 2u);
    EXPECT_EQ(tmpl->variables()[0], "a");
    EXPECT_EQ(tmpl->variables()[1], "b");
    ASSERT_EQ(tmpl->segments().size(), 3u);
    EXPECT_EQ(tmpl->segments()[0], "say ");
    EXPECT_EQ(tmpl->segments()[1], " and ");
    EXPECT_EQ(tmpl->segments()[2], "");
}

TEST_F(StringTemplateTest, FromString_VariableAtStart)
{
    auto tmpl = _parseTemplate("$(greeting) world");
    ASSERT_TRUE(tmpl.has_value());
    ASSERT_EQ(tmpl->variables().size(), 1u);
    EXPECT_EQ(tmpl->variables()[0], "greeting");
    ASSERT_EQ(tmpl->segments().size(), 2u);
    EXPECT_EQ(tmpl->segments()[0], "");
    EXPECT_EQ(tmpl->segments()[1], " world");
}

TEST_F(StringTemplateTest, FromString_VariableAtEnd)
{
    auto tmpl = _parseTemplate("hello $(name)");
    ASSERT_TRUE(tmpl.has_value());
    ASSERT_EQ(tmpl->variables().size(), 1u);
    EXPECT_EQ(tmpl->variables()[0], "name");
    ASSERT_EQ(tmpl->segments().size(), 2u);
    EXPECT_EQ(tmpl->segments()[0], "hello ");
    // 末尾的空 segment 也保留
    EXPECT_EQ(tmpl->segments()[1], "");
}

TEST_F(StringTemplateTest, FromString_NoVariables_ReturnsError)
{
    // 无变量属于数据包内容错误，返回错误而不是抛异常
    EXPECT_FALSE(StringTemplate::fromString("say hello").success());
}

TEST_F(StringTemplateTest, FromString_UnterminatedVariable_ReturnsError)
{
    EXPECT_FALSE(StringTemplate::fromString("say $(name").success());
}

TEST_F(StringTemplateTest, FromString_InvalidVariableName_ReturnsError)
{
    // 含非法字符（-）
    EXPECT_FALSE(StringTemplate::fromString("say $(na-me)").success());
}

TEST_F(StringTemplateTest, FromString_DollarNotFollowedByParen)
{
    // '$' 不跟 '('，不视为变量起始；若整行没有变量则返回错误
    EXPECT_FALSE(StringTemplate::fromString("say $5").success());
    // 混合：先有 $5（普通字符）后有 $(var)
    auto tmpl = _parseTemplate("say $5 and $(var)");
    ASSERT_TRUE(tmpl.has_value());
    ASSERT_EQ(tmpl->variables().size(), 1u);
    EXPECT_EQ(tmpl->variables()[0], "var");
    ASSERT_EQ(tmpl->segments().size(), 2u);
    EXPECT_EQ(tmpl->segments()[0], "say $5 and ");
    EXPECT_EQ(tmpl->segments()[1], "");
}

TEST_F(StringTemplateTest, FromString_VariableNameWithUnderscoreAndDigits)
{
    auto tmpl = _parseTemplate("$(var_1)_$(var_2)");
    ASSERT_TRUE(tmpl.has_value());
    ASSERT_EQ(tmpl->variables().size(), 2u);
    EXPECT_EQ(tmpl->variables()[0], "var_1");
    EXPECT_EQ(tmpl->variables()[1], "var_2");
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
    auto tmpl = _parseTemplate("say $(name)");
    ASSERT_TRUE(tmpl.has_value());
    EXPECT_EQ(_substitute(*tmpl, {"Steve"}), "say Steve");
}

TEST_F(StringTemplateTest, Substitute_MultipleVariables)
{
    auto tmpl = _parseTemplate("say $(a) and $(b)");
    ASSERT_TRUE(tmpl.has_value());
    EXPECT_EQ(_substitute(*tmpl, {"Alex", "Bob"}), "say Alex and Bob");
}

TEST_F(StringTemplateTest, Substitute_VariableInMiddle)
{
    auto tmpl = _parseTemplate("tellraw @a {\"text\":\"$(msg)\"}");
    ASSERT_TRUE(tmpl.has_value());
    EXPECT_EQ(_substitute(*tmpl, {"hello world"}), "tellraw @a {\"text\":\"hello world\"}");
}

TEST_F(StringTemplateTest, Substitute_CountMismatch_Asserts)
{
    auto tmpl = _parseTemplate("say $(a) and $(b)");
    ASSERT_TRUE(tmpl.has_value());

    // 实参个数与形参不一致属于调用方违约，默认断言终止进程；用 EXPECT_DEATH 验证子进程终止。
    EXPECT_DEATH(MC_UNUSED(tmpl->substitute({"only_one"})), "");
    EXPECT_DEATH(MC_UNUSED(tmpl->substitute({"a", "b", "c"})), "");
}

TEST_F(StringTemplateTest, Substitute_RepeatedVariable)
{
    // 同一变量多次出现：StringTemplate.variables() 会重复，substitute 期望 values 与之一一对应
    auto tmpl = _parseTemplate("$(a) $(a) $(a)");
    ASSERT_TRUE(tmpl.has_value());
    ASSERT_EQ(tmpl->variables().size(), 3u);
    EXPECT_EQ(_substitute(*tmpl, {"X", "X", "X"}), "X X X");
}
