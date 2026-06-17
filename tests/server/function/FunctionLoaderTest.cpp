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

#include "server/function/FunctionLoader.hpp"
#include "server/function/FunctionManager.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::function;

/**
 * @brief FunctionLoader 单元测试
 */
class FunctionLoaderTest : public ::testing::Test {
protected:
    FunctionManager manager;
};

// ========== parseFunctionContent 测试 ==========

TEST_F(FunctionLoaderTest, ParseFunctionContent_SimpleCommands)
{
    FunctionLoader loader(manager);
    auto result = loader.parseFunctionContent("test:simple", "say hello\ngive @a diamond");
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().commands.size(), 2u);
    EXPECT_EQ(result.value().commands[0], "say hello");
    EXPECT_EQ(result.value().commands[1], "give @a diamond");
    EXPECT_EQ(result.value().skippedMacroCount, 0u);
}

TEST_F(FunctionLoaderTest, ParseFunctionContent_CommentsIgnored)
{
    FunctionLoader loader(manager);
    auto result = loader.parseFunctionContent("test:comments", "# This is a comment\nsay hello\n# Another comment");
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().commands.size(), 1u);
    EXPECT_EQ(result.value().commands[0], "say hello");
    EXPECT_EQ(result.value().skippedMacroCount, 0u);
}

TEST_F(FunctionLoaderTest, ParseFunctionContent_EmptyLinesIgnored)
{
    FunctionLoader loader(manager);
    auto result = loader.parseFunctionContent("test:empty", "\n\nsay hello\n\n\ngive @a diamond\n\n");
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().commands.size(), 2u);
}

TEST_F(FunctionLoaderTest, ParseFunctionContent_WhitespaceOnlyLinesIgnored)
{
    FunctionLoader loader(manager);
    auto result = loader.parseFunctionContent("test:whitespace", "   \n\tsay hello\n   \t  \n");
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().commands.size(), 1u);
    EXPECT_EQ(result.value().commands[0], "say hello");
}

TEST_F(FunctionLoaderTest, ParseFunctionContent_SlashPrefixStripped)
{
    FunctionLoader loader(manager);
    auto result = loader.parseFunctionContent("test:slash", "/say hello");
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().commands.size(), 1u);
    EXPECT_EQ(result.value().commands[0], "say hello");
}

TEST_F(FunctionLoaderTest, ParseFunctionContent_DoubleSlashIsError)
{
    FunctionLoader loader(manager);
    auto result = loader.parseFunctionContent("test:doubleslash", "// this is not a valid comment");
    EXPECT_FALSE(result.success());
}

TEST_F(FunctionLoaderTest, ParseFunctionContent_LineContinuation)
{
    FunctionLoader loader(manager);
    auto result = loader.parseFunctionContent("test:continuation", "say hello \\\nworld");
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().commands.size(), 1u);
    EXPECT_EQ(result.value().commands[0], "say hello world");
}

TEST_F(FunctionLoaderTest, ParseFunctionContent_MacroLinesSkipped)
{
    FunctionLoader loader(manager);
    auto result = loader.parseFunctionContent("test:macro", "$(var)\nsay hello\n$macro_line");
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().commands.size(), 1u);
    EXPECT_EQ(result.value().commands[0], "say hello");
    EXPECT_EQ(result.value().skippedMacroCount, 2u);
}

TEST_F(FunctionLoaderTest, ParseFunctionContent_EmptyContent)
{
    FunctionLoader loader(manager);
    auto result = loader.parseFunctionContent("test:empty", "");
    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().commands.empty());
    EXPECT_EQ(result.value().skippedMacroCount, 0u);
}

TEST_F(FunctionLoaderTest, ParseFunctionContent_OnlyComments)
{
    FunctionLoader loader(manager);
    auto result = loader.parseFunctionContent("test:onlycomments", "# comment 1\n# comment 2\n# comment 3");
    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().commands.empty());
    EXPECT_EQ(result.value().skippedMacroCount, 0u);
}

TEST_F(FunctionLoaderTest, ParseFunctionContent_WindowsLineEndings)
{
    FunctionLoader loader(manager);
    auto result = loader.parseFunctionContent("test:crlf", "say hello\r\ngive @a diamond\r\n");
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().commands.size(), 2u);
    EXPECT_EQ(result.value().commands[0], "say hello");
    EXPECT_EQ(result.value().commands[1], "give @a diamond");
}

TEST_F(FunctionLoaderTest, ParseFunctionContent_CommandTooLong)
{
    FunctionLoader loader(manager);
    // 生成超长命令 (> 2,000,000 chars)
    std::string longCommand(2000001, 'a');
    auto result = loader.parseFunctionContent("test:toolong", longCommand);
    EXPECT_FALSE(result.success());
}

TEST_F(FunctionLoaderTest, ParseFunctionContent_MixedContent)
{
    FunctionLoader loader(manager);
    std::string content = "# Header comment\n"
                          "\n"
                          "say first command\n"
                          "$(skip_me)\n"
                          "  say indented command\n"
                          "/say slash-prefixed\n"
                          "# Middle comment\n"
                          "give @a diamond\n"
                          "$another_macro\n";
    auto result = loader.parseFunctionContent("test:mixed", content);
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().commands.size(), 4u);
    EXPECT_EQ(result.value().commands[0], "say first command");
    EXPECT_EQ(result.value().commands[1], "say indented command");
    EXPECT_EQ(result.value().commands[2], "say slash-prefixed");
    EXPECT_EQ(result.value().commands[3], "give @a diamond");
    EXPECT_EQ(result.value().skippedMacroCount, 2u);
}

// ========== pathToFunctionId 测试 ==========

TEST_F(FunctionLoaderTest, PathToFunctionId_StandardPath)
{
    FunctionLoader loader(manager);
    EXPECT_EQ("minecraft:test", loader.pathToFunctionId("data/minecraft/functions/test.mcfunction"));
    EXPECT_EQ("minecraft:foo/bar", loader.pathToFunctionId("data/minecraft/functions/foo/bar.mcfunction"));
    EXPECT_EQ("mod_id:custom/func", loader.pathToFunctionId("data/mod_id/functions/custom/func.mcfunction"));
}

TEST_F(FunctionLoaderTest, PathToFunctionId_DeepNestedPath)
{
    FunctionLoader loader(manager);
    EXPECT_EQ("minecraft:a/b/c/d", loader.pathToFunctionId("data/minecraft/functions/a/b/c/d.mcfunction"));
}

TEST_F(FunctionLoaderTest, PathToFunctionId_NoDataPrefix)
{
    FunctionLoader loader(manager);
    // 相对路径格式（不含 data/ 前缀）
    EXPECT_EQ("minecraft:test", loader.pathToFunctionId("minecraft/functions/test.mcfunction"));
}
