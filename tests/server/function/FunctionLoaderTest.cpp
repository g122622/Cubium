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

// ========== pathToTagId 测试 ==========

TEST_F(FunctionLoaderTest, PathToTagId_StandardPath)
{
    FunctionLoader loader(manager);
    EXPECT_EQ("minecraft:tick", loader.pathToTagId("minecraft/tags/functions/tick.json"));
    EXPECT_EQ("minecraft:load", loader.pathToTagId("minecraft/tags/functions/load.json"));
    EXPECT_EQ("minecraft:foo/bar", loader.pathToTagId("minecraft/tags/functions/foo/bar.json"));
    EXPECT_EQ("mod_id:custom/tag", loader.pathToTagId("mod_id/tags/functions/custom/tag.json"));
}

TEST_F(FunctionLoaderTest, PathToTagId_WithDataPrefix)
{
    FunctionLoader loader(manager);
    EXPECT_EQ("minecraft:tick", loader.pathToTagId("data/minecraft/tags/functions/tick.json"));
    EXPECT_EQ("minecraft:game_loop", loader.pathToTagId("data/minecraft/tags/functions/game_loop.json"));
    EXPECT_EQ("mod_id:custom/tag", loader.pathToTagId("data/mod_id/tags/functions/custom/tag.json"));
}

TEST_F(FunctionLoaderTest, PathToTagId_DeepNestedPath)
{
    FunctionLoader loader(manager);
    EXPECT_EQ("minecraft:a/b/c", loader.pathToTagId("minecraft/tags/functions/a/b/c.json"));
}

// ========== parseTagJson 测试 ==========

TEST_F(FunctionLoaderTest, ParseTagJson_SimpleValues)
{
    FunctionLoader loader(manager);
    ResourceLocation tagLoc = ResourceLocation::parse("minecraft:tick");
    auto result = loader.parseTagJson(tagLoc, R"({"values": ["minecraft:game_loop", "minecraft:another"]})");
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().id.toString(), "minecraft:tick");
    EXPECT_FALSE(result.value().replace);
    EXPECT_EQ(result.value().functionIds.size(), 2u);
    EXPECT_EQ(result.value().functionIds[0].toString(), "minecraft:game_loop");
    EXPECT_EQ(result.value().functionIds[1].toString(), "minecraft:another");
    EXPECT_TRUE(result.value().tagReferences.empty());
}

TEST_F(FunctionLoaderTest, ParseTagJson_TagReference)
{
    FunctionLoader loader(manager);
    ResourceLocation tagLoc = ResourceLocation::parse("minecraft:tick");
    auto result = loader.parseTagJson(tagLoc, R"({"values": ["#minecraft:load", "minecraft:game_loop"]})");
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().functionIds.size(), 1u);
    EXPECT_EQ(result.value().functionIds[0].toString(), "minecraft:game_loop");
    EXPECT_EQ(result.value().tagReferences.size(), 1u);
    EXPECT_EQ(result.value().tagReferences[0].toString(), "minecraft:load");
}

TEST_F(FunctionLoaderTest, ParseTagJson_ReplaceTrue)
{
    FunctionLoader loader(manager);
    ResourceLocation tagLoc = ResourceLocation::parse("minecraft:tick");
    auto result = loader.parseTagJson(tagLoc, R"({"replace": true, "values": ["minecraft:new_func"]})");
    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().replace);
    EXPECT_EQ(result.value().functionIds.size(), 1u);
}

TEST_F(FunctionLoaderTest, ParseTagJson_ReplaceFalse)
{
    FunctionLoader loader(manager);
    ResourceLocation tagLoc = ResourceLocation::parse("minecraft:tick");
    auto result = loader.parseTagJson(tagLoc, R"({"replace": false, "values": ["minecraft:func"]})");
    ASSERT_TRUE(result.success());
    EXPECT_FALSE(result.value().replace);
}

TEST_F(FunctionLoaderTest, ParseTagJson_EmptyValues)
{
    FunctionLoader loader(manager);
    ResourceLocation tagLoc = ResourceLocation::parse("minecraft:empty_tag");
    auto result = loader.parseTagJson(tagLoc, R"({"values": []})");
    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().functionIds.empty());
    EXPECT_TRUE(result.value().tagReferences.empty());
}

TEST_F(FunctionLoaderTest, ParseTagJson_MissingValuesArray)
{
    FunctionLoader loader(manager);
    ResourceLocation tagLoc = ResourceLocation::parse("minecraft:bad_tag");
    auto result = loader.parseTagJson(tagLoc, R"({"replace": false})");
    EXPECT_FALSE(result.success());
}

TEST_F(FunctionLoaderTest, ParseTagJson_InvalidJson)
{
    FunctionLoader loader(manager);
    ResourceLocation tagLoc = ResourceLocation::parse("minecraft:bad_tag");
    auto result = loader.parseTagJson(tagLoc, "not valid json");
    EXPECT_FALSE(result.success());
}

TEST_F(FunctionLoaderTest, ParseTagJson_OnlyTagReferences)
{
    FunctionLoader loader(manager);
    ResourceLocation tagLoc = ResourceLocation::parse("minecraft:combined_tag");
    auto result = loader.parseTagJson(tagLoc, R"({"values": ["#minecraft:tick", "#minecraft:load"]})");
    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().functionIds.empty());
    EXPECT_EQ(result.value().tagReferences.size(), 2u);
    EXPECT_EQ(result.value().tagReferences[0].toString(), "minecraft:tick");
    EXPECT_EQ(result.value().tagReferences[1].toString(), "minecraft:load");
}

TEST_F(FunctionLoaderTest, ParseTagJson_NonStringValuesIgnored)
{
    FunctionLoader loader(manager);
    ResourceLocation tagLoc = ResourceLocation::parse("minecraft:tag_with_bad_values");
    auto result = loader.parseTagJson(tagLoc, R"({"values": ["minecraft:valid", 42, true, {"id": "test"}]})");
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().functionIds.size(), 1u);
    EXPECT_EQ(result.value().functionIds[0].toString(), "minecraft:valid");
}

// ========== FunctionManager 标签测试 ==========

TEST_F(FunctionLoaderTest, FunctionManager_RegisterAndQueryTag)
{
    FunctionManager mgr;
    ResourceLocation tickTag = ResourceLocation::parse("minecraft:tick");
    std::vector<ResourceLocation> funcs = {
        ResourceLocation::parse("minecraft:game_loop"), ResourceLocation::parse("minecraft:another_tick")};
    mgr.registerTag(tickTag, std::move(funcs));

    EXPECT_TRUE(mgr.hasTag(tickTag));
    EXPECT_EQ(mgr.getTag(tickTag).size(), 2u);
    EXPECT_EQ(mgr.getTag(tickTag)[0].toString(), "minecraft:game_loop");
    EXPECT_EQ(mgr.getTag(tickTag)[1].toString(), "minecraft:another_tick");
}

TEST_F(FunctionLoaderTest, FunctionManager_TagNotFound)
{
    FunctionManager mgr;
    ResourceLocation nonexistentTag = ResourceLocation::parse("minecraft:nonexistent");
    EXPECT_FALSE(mgr.hasTag(nonexistentTag));
    EXPECT_TRUE(mgr.getTag(nonexistentTag).empty());
}

TEST_F(FunctionLoaderTest, FunctionManager_GetAllTagIds)
{
    FunctionManager mgr;
    mgr.registerTag(ResourceLocation::parse("minecraft:tick"), {ResourceLocation::parse("minecraft:a")});
    mgr.registerTag(ResourceLocation::parse("minecraft:load"), {ResourceLocation::parse("minecraft:b")});
    auto ids = mgr.getAllTagIds();
    EXPECT_EQ(ids.size(), 2u);
}

TEST_F(FunctionLoaderTest, FunctionManager_ClearAlsoClearsTags)
{
    FunctionManager mgr;
    mgr.registerTag(ResourceLocation::parse("minecraft:tick"), {ResourceLocation::parse("minecraft:a")});
    EXPECT_EQ(mgr.tagCount(), 1u);
    mgr.clear();
    EXPECT_EQ(mgr.tagCount(), 0u);
}
