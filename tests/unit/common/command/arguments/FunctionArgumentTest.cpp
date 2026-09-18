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

/**
 * @file FunctionArgumentTest.cpp
 * @brief FunctionArgumentType 和 FunctionArgumentResult 单元测试
 *
 * 测试函数参数解析，包括：
 * - 普通函数名解析（简单名称、带命名空间、带路径）
 * - 标签引用解析（# 前缀）
 * - 空输入和非法输入的错误处理
 * - FunctionArgumentResult 的基本属性
 * - getTypeName 和 getExamples
 * - 贪婪读取：在非法字符处停止、不截断命名空间
 */

#include <gtest/gtest.h>

#include "common/command/StringReader.hpp"
#include "common/command/arguments/FunctionArgument.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/resource/ResourceLocation.hpp"

using mc::ResourceLocation;
using mc::command::CommandErrorType;
using mc::command::CommandException;
using mc::command::FunctionArgumentResult;
using mc::command::FunctionArgumentType;
using mc::command::StringReader;

// ========== FunctionArgumentResult 测试 ==========

class FunctionArgumentResultTest : public ::testing::Test {};

TEST_F(FunctionArgumentResultTest, DefaultConstructor)
{
    FunctionArgumentResult result;
    EXPECT_FALSE(result.isTag());
    EXPECT_EQ(result.id().toString(), "minecraft:empty");
}

TEST_F(FunctionArgumentResultTest, FunctionReference)
{
    ResourceLocation id("minecraft", "test/foo");
    FunctionArgumentResult result(id);
    EXPECT_FALSE(result.isTag());
    EXPECT_EQ(result.id().namespace_(), "minecraft");
    EXPECT_EQ(result.id().path(), "test/foo");
    EXPECT_EQ(result.displayName(), "minecraft:test/foo");
}

TEST_F(FunctionArgumentResultTest, TagReference)
{
    ResourceLocation id("minecraft", "tick");
    FunctionArgumentResult result(id, true);
    EXPECT_TRUE(result.isTag());
    EXPECT_EQ(result.id().namespace_(), "minecraft");
    EXPECT_EQ(result.id().path(), "tick");
    EXPECT_EQ(result.displayName(), "#minecraft:tick");
}

TEST_F(FunctionArgumentResultTest, DisplayNameWithoutNamespace)
{
    ResourceLocation id = ResourceLocation::parse("my_function");
    FunctionArgumentResult result(id);
    EXPECT_FALSE(result.isTag());
    // ResourceLocation::parse 默认命名空间为 minecraft
    EXPECT_EQ(result.displayName(), "minecraft:my_function");
}

// ========== FunctionArgumentType 解析测试 ==========

class FunctionArgumentTypeTest : public ::testing::Test {
protected:
    FunctionArgumentType parser;
};

TEST_F(FunctionArgumentTypeTest, ParseSimpleFunctionName)
{
    // "foo" — 简单函数名，默认命名空间为 minecraft
    StringReader reader("foo");
    auto result = parser.parse(reader);
    EXPECT_FALSE(result.isTag());
    EXPECT_EQ(result.id().path(), "foo");
    EXPECT_EQ(reader.getCursor(), 3); // 消费了 "foo"
}

TEST_F(FunctionArgumentTypeTest, ParseNamespacedFunctionName)
{
    // "minecraft:foo/bar" — 带命名空间和路径的函数名（17 字符）
    StringReader reader("minecraft:foo/bar");
    auto result = parser.parse(reader);
    EXPECT_FALSE(result.isTag());
    EXPECT_EQ(result.id().namespace_(), "minecraft");
    EXPECT_EQ(result.id().path(), "foo/bar");
    EXPECT_EQ(reader.getCursor(), 17); // 消费了全部 17 字符
}

TEST_F(FunctionArgumentTypeTest, ParseCustomNamespaceFunction)
{
    // "mydatapack:custom/function" — 自定义命名空间（26 字符）
    StringReader reader("mydatapack:custom/function");
    auto result = parser.parse(reader);
    EXPECT_FALSE(result.isTag());
    EXPECT_EQ(result.id().namespace_(), "mydatapack");
    EXPECT_EQ(result.id().path(), "custom/function");
    EXPECT_EQ(reader.getCursor(), 26); // 消费了全部 26 字符
}

TEST_F(FunctionArgumentTypeTest, ParseTagReference)
{
    // "#minecraft:tick" — 标签引用（15 字符）
    StringReader reader("#minecraft:tick");
    auto result = parser.parse(reader);
    EXPECT_TRUE(result.isTag());
    EXPECT_EQ(result.id().namespace_(), "minecraft");
    EXPECT_EQ(result.id().path(), "tick");
    EXPECT_EQ(reader.getCursor(), 15); // 消费了全部 15 字符
}

TEST_F(FunctionArgumentTypeTest, ParseTagReferenceNoNamespace)
{
    // "#my_tag" — 无命名空间的标签引用，默认为 minecraft
    StringReader reader("#my_tag");
    auto result = parser.parse(reader);
    EXPECT_TRUE(result.isTag());
    EXPECT_EQ(result.id().namespace_(), "minecraft");
    EXPECT_EQ(result.id().path(), "my_tag");
}

TEST_F(FunctionArgumentTypeTest, ParseTagReferenceWithNamespace)
{
    // "#mymod:tags/test" — 带命名空间和路径的标签引用（16 字符）
    StringReader reader("#mymod:tags/test");
    auto result = parser.parse(reader);
    EXPECT_TRUE(result.isTag());
    EXPECT_EQ(result.id().namespace_(), "mymod");
    EXPECT_EQ(result.id().path(), "tags/test");
    EXPECT_EQ(reader.getCursor(), 16);
}

TEST_F(FunctionArgumentTypeTest, ParseFunctionWithTrailingSpace)
{
    // "minecraft:foo 123" — 后面有空格时，只在空格前停止读取
    // "minecraft:foo" = 13 字符
    StringReader reader("minecraft:foo 123");
    auto result = parser.parse(reader);
    EXPECT_FALSE(result.isTag());
    EXPECT_EQ(result.id().namespace_(), "minecraft");
    EXPECT_EQ(result.id().path(), "foo");
    EXPECT_EQ(reader.getCursor(), 13); // "minecraft:foo" = 13 字符
}

TEST_F(FunctionArgumentTypeTest, ParseTagWithTrailingSpace)
{
    // "#minecraft:load extra" — 标签引用后有空格
    // "#minecraft:load" = 15 字符
    StringReader reader("#minecraft:load extra");
    auto result = parser.parse(reader);
    EXPECT_TRUE(result.isTag());
    EXPECT_EQ(result.id().namespace_(), "minecraft");
    EXPECT_EQ(result.id().path(), "load");
    EXPECT_EQ(reader.getCursor(), 15); // "#minecraft:load" = 15 字符
}

TEST_F(FunctionArgumentTypeTest, ParseEmptyInputThrows)
{
    // 空输入应抛出异常
    StringReader reader("");
    EXPECT_THROW({ parser.parse(reader); }, CommandException);
}

TEST_F(FunctionArgumentTypeTest, ParseHashOnlyThrows)
{
    // 只有 "#" 后面没有标识符应抛出异常
    StringReader reader("#");
    EXPECT_THROW({ parser.parse(reader); }, CommandException);
}

TEST_F(FunctionArgumentTypeTest, ParseHashFollowedBySpaceThrows)
{
    // "# " 后面只有空格应抛出异常
    StringReader reader("# ");
    EXPECT_THROW({ parser.parse(reader); }, CommandException);
}

TEST_F(FunctionArgumentTypeTest, ParseFunctionWithDotsAndDashes)
{
    // "minecraft:foo.bar-baz" — 包含点和短横线的函数名
    StringReader reader("minecraft:foo.bar-baz");
    auto result = parser.parse(reader);
    EXPECT_FALSE(result.isTag());
    EXPECT_EQ(result.id().namespace_(), "minecraft");
    EXPECT_EQ(result.id().path(), "foo.bar-baz");
}

TEST_F(FunctionArgumentTypeTest, GetType)
{
    EXPECT_EQ(parser.getTypeName(), "function");
}

TEST_F(FunctionArgumentTypeTest, GetExamples)
{
    auto examples = parser.getExamples();
    EXPECT_EQ(examples.size(), 3u);
    EXPECT_EQ(examples[0], "foo");
    EXPECT_EQ(examples[1], "foo:bar");
    EXPECT_EQ(examples[2], "#foo");
}

TEST_F(FunctionArgumentTypeTest, FactoryMethod)
{
    auto ptr = FunctionArgumentType::functions();
    EXPECT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->getTypeName(), "function");
}

// ========== 贪婪读取边界测试 ==========

class FunctionArgumentGreedyReadTest : public ::testing::Test {
protected:
    FunctionArgumentType parser;
};

TEST_F(FunctionArgumentGreedyReadTest, StopsAtInvalidCharacters)
{
    // "minecraft:foo!bar" — '!' 不是合法标识符字符，应在 '!' 处停止读取
    // "minecraft:foo" = 13 字符
    StringReader reader("minecraft:foo!bar");
    auto result = parser.parse(reader);
    EXPECT_FALSE(result.isTag());
    EXPECT_EQ(result.id().namespace_(), "minecraft");
    EXPECT_EQ(result.id().path(), "foo");
    EXPECT_EQ(reader.getCursor(), 13); // "minecraft:foo" = 13 字符
}

TEST_F(FunctionArgumentGreedyReadTest, StopsAtSpaceInFunction)
{
    // "foo bar" — 空格不是合法标识符字符
    StringReader reader("foo bar");
    auto result = parser.parse(reader);
    EXPECT_FALSE(result.isTag());
    EXPECT_EQ(result.id().path(), "foo");
    EXPECT_EQ(reader.getCursor(), 3);
}

TEST_F(FunctionArgumentGreedyReadTest, StopsAtInvalidCharInTag)
{
    // "#minecraft:ti ck" — 标签中有空格
    // "#minecraft:ti" = 13 字符
    StringReader reader("#minecraft:ti ck");
    auto result = parser.parse(reader);
    EXPECT_TRUE(result.isTag());
    EXPECT_EQ(result.id().namespace_(), "minecraft");
    EXPECT_EQ(result.id().path(), "ti");
    EXPECT_EQ(reader.getCursor(), 13); // "#minecraft:ti" = 13 字符
}

TEST_F(FunctionArgumentGreedyReadTest, UppercaseNotConsumed)
{
    // MC Java 的 Identifier 仅允许小写字母。贪婪读取器不消费大写字母。
    // "minecraft:FOO" — 'F' 不是合法标识符字符
    // "minecraft:" = 10 字符
    StringReader reader("minecraft:FOO");
    auto result = parser.parse(reader);
    EXPECT_FALSE(result.isTag());
    // 贪婪读取在 ':' 后的 'F' 处停止，ResourceLocation::parse("minecraft:") 会解析为
    // namespace="minecraft", path=""（空路径）
    EXPECT_EQ(reader.getCursor(), 10); // "minecraft:" = 10 字符
}

TEST_F(FunctionArgumentGreedyReadTest, UnderscoreAndDigitsAllowed)
{
    // "minecraft:func_01" — 下划线和数字是合法的
    StringReader reader("minecraft:func_01");
    auto result = parser.parse(reader);
    EXPECT_FALSE(result.isTag());
    EXPECT_EQ(result.id().namespace_(), "minecraft");
    EXPECT_EQ(result.id().path(), "func_01");
}

TEST_F(FunctionArgumentGreedyReadTest, TagStopsAtExclamationMark)
{
    // "#minecraft:tick!" — '!' 不是合法标识符字符
    // "#minecraft:tick" = 15 字符
    StringReader reader("#minecraft:tick!");
    auto result = parser.parse(reader);
    EXPECT_TRUE(result.isTag());
    EXPECT_EQ(result.id().namespace_(), "minecraft");
    EXPECT_EQ(result.id().path(), "tick");
    EXPECT_EQ(reader.getCursor(), 15);
}
