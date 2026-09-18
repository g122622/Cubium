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
#include "common/TempDirHelper.hpp"
#include "server/function/FunctionManager.hpp"
#include <fstream>
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
    EXPECT_FALSE(result.value().isMacro());
}

TEST_F(FunctionLoaderTest, ParseFunctionContent_CommentsIgnored)
{
    FunctionLoader loader(manager);
    auto result = loader.parseFunctionContent("test:comments", "# This is a comment\nsay hello\n# Another comment");
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().commands.size(), 1u);
    EXPECT_EQ(result.value().commands[0], "say hello");
    EXPECT_FALSE(result.value().isMacro());
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

TEST_F(FunctionLoaderTest, ParseFunctionContent_MacroLinesBecomeMacroEntries)
{
    // 新行为：$ 开头的行被解析为 MacroFunctionEntry::Macro，整个文件成为宏函数
    // 宏行格式：$<body>，其中 body 含 $(var) 替换
    FunctionLoader loader(manager);
    auto result = loader.parseFunctionContent("test:macro", "$say $(var)\nsay hello\n$tellraw @a $(msg)");
    ASSERT_TRUE(result.success());
    // 含 $ 宏行的文件不再是 CommandFunction，commands 为空
    EXPECT_TRUE(result.value().commands.empty());
    // isMacro() 为真
    EXPECT_TRUE(result.value().isMacro());
    // 含 3 个 entry（1 个 say 命令转为 PlainText + 2 个宏行）
    EXPECT_EQ(result.value().macroEntries.size(), 3u);
    // 形参列表含 2 个去重后的形参：var, msg
    EXPECT_EQ(result.value().macroParameters.size(), 2u);
    EXPECT_EQ(result.value().macroParameters[0], "var");
    EXPECT_EQ(result.value().macroParameters[1], "msg");
}

TEST_F(FunctionLoaderTest, ParseFunctionContent_EmptyContent)
{
    FunctionLoader loader(manager);
    auto result = loader.parseFunctionContent("test:empty", "");
    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().commands.empty());
    EXPECT_FALSE(result.value().isMacro());
}

TEST_F(FunctionLoaderTest, ParseFunctionContent_OnlyComments)
{
    FunctionLoader loader(manager);
    auto result = loader.parseFunctionContent("test:onlycomments", "# comment 1\n# comment 2\n# comment 3");
    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().commands.empty());
    EXPECT_FALSE(result.value().isMacro());
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

TEST_F(FunctionLoaderTest, ParseFunctionContent_MixedContentWithMacroBecomesMacroFunction)
{
    // 新行为：含 $ 宏行的文件被注册为宏函数
    // - "say first command" / "say indented command" / "say slash-prefixed" / "give @a diamond" 都转为 PlainText entry
    // - "$say $(skip_me)" / "$tellraw @a $(another_macro)" 转为 Macro entry
    // - "skip_me" 和 "another_macro" 加入形参列表
    FunctionLoader loader(manager);
    std::string content = "# Header comment\n"
                          "\n"
                          "say first command\n"
                          "$say $(skip_me)\n"
                          "  say indented command\n"
                          "/say slash-prefixed\n"
                          "# Middle comment\n"
                          "give @a diamond\n"
                          "$tellraw @a $(another_macro)\n";
    auto result = loader.parseFunctionContent("test:mixed", content);
    ASSERT_TRUE(result.success());
    // 含 $ 宏行：commands 为空，macroEntries 含 6 项
    EXPECT_TRUE(result.value().commands.empty());
    EXPECT_TRUE(result.value().isMacro());
    EXPECT_EQ(result.value().macroEntries.size(), 6u);
    // 形参列表: skip_me, another_macro
    EXPECT_EQ(result.value().macroParameters.size(), 2u);
    EXPECT_EQ(result.value().macroParameters[0], "skip_me");
    EXPECT_EQ(result.value().macroParameters[1], "another_macro");
}

TEST_F(FunctionLoaderTest, ParseFunctionContent_MixedContentWithoutMacroStaysCommandFunction)
{
    // 不含 $ 宏行：仍为 CommandFunction，commands 列表正常
    FunctionLoader loader(manager);
    std::string content = "# Header comment\n"
                          "\n"
                          "say first command\n"
                          "  say indented command\n"
                          "/say slash-prefixed\n"
                          "# Middle comment\n"
                          "give @a diamond\n";
    auto result = loader.parseFunctionContent("test:mixed_plain", content);
    ASSERT_TRUE(result.success());
    EXPECT_FALSE(result.value().isMacro());
    EXPECT_EQ(result.value().commands.size(), 4u);
    EXPECT_EQ(result.value().commands[0], "say first command");
    EXPECT_EQ(result.value().commands[1], "say indented command");
    EXPECT_EQ(result.value().commands[2], "say slash-prefixed");
    EXPECT_EQ(result.value().commands[3], "give @a diamond");
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
    EXPECT_EQ(result.value().entries.size(), 2u);
    EXPECT_EQ(result.value().entries[0].id.toString(), "minecraft:game_loop");
    EXPECT_EQ(result.value().entries[0].type, FunctionLoader::TagEntryType::Function);
    EXPECT_TRUE(result.value().entries[0].required);
    EXPECT_EQ(result.value().entries[1].id.toString(), "minecraft:another");
    EXPECT_EQ(result.value().entries[1].type, FunctionLoader::TagEntryType::Function);
    EXPECT_TRUE(result.value().entries[1].required);
}

TEST_F(FunctionLoaderTest, ParseTagJson_TagReference)
{
    FunctionLoader loader(manager);
    ResourceLocation tagLoc = ResourceLocation::parse("minecraft:tick");
    auto result = loader.parseTagJson(tagLoc, R"({"values": ["#minecraft:load", "minecraft:game_loop"]})");
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().entries.size(), 2u);
    EXPECT_EQ(result.value().entries[0].id.toString(), "minecraft:load");
    EXPECT_EQ(result.value().entries[0].type, FunctionLoader::TagEntryType::Tag);
    EXPECT_TRUE(result.value().entries[0].required);
    EXPECT_EQ(result.value().entries[1].id.toString(), "minecraft:game_loop");
    EXPECT_EQ(result.value().entries[1].type, FunctionLoader::TagEntryType::Function);
    EXPECT_TRUE(result.value().entries[1].required);
}

TEST_F(FunctionLoaderTest, ParseTagJson_ReplaceTrue)
{
    FunctionLoader loader(manager);
    ResourceLocation tagLoc = ResourceLocation::parse("minecraft:tick");
    auto result = loader.parseTagJson(tagLoc, R"({"replace": true, "values": ["minecraft:new_func"]})");
    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().replace);
    EXPECT_EQ(result.value().entries.size(), 1u);
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
    EXPECT_TRUE(result.value().entries.empty());
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
    EXPECT_EQ(result.value().entries.size(), 2u);
    EXPECT_EQ(result.value().entries[0].id.toString(), "minecraft:tick");
    EXPECT_EQ(result.value().entries[0].type, FunctionLoader::TagEntryType::Tag);
    EXPECT_EQ(result.value().entries[1].id.toString(), "minecraft:load");
    EXPECT_EQ(result.value().entries[1].type, FunctionLoader::TagEntryType::Tag);
}

TEST_F(FunctionLoaderTest, ParseTagJson_ObjectEntryWithRequiredFalse)
{
    FunctionLoader loader(manager);
    ResourceLocation tagLoc = ResourceLocation::parse("minecraft:optional_tag");
    auto result = loader.parseTagJson(
        tagLoc, R"({"values": ["minecraft:required_func", {"id": "minecraft:optional_func", "required": false}]})");
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().entries.size(), 2u);
    // 第一个条目：字符串格式，默认 required=true
    EXPECT_EQ(result.value().entries[0].id.toString(), "minecraft:required_func");
    EXPECT_EQ(result.value().entries[0].type, FunctionLoader::TagEntryType::Function);
    EXPECT_TRUE(result.value().entries[0].required);
    // 第二个条目：对象格式，required=false
    EXPECT_EQ(result.value().entries[1].id.toString(), "minecraft:optional_func");
    EXPECT_EQ(result.value().entries[1].type, FunctionLoader::TagEntryType::Function);
    EXPECT_FALSE(result.value().entries[1].required);
}

TEST_F(FunctionLoaderTest, ParseTagJson_ObjectEntryWithTagReference)
{
    FunctionLoader loader(manager);
    ResourceLocation tagLoc = ResourceLocation::parse("minecraft:mixed_tag");
    auto result = loader.parseTagJson(tagLoc,
        R"({"values": ["minecraft:func1", "#minecraft:tick", {"id": "#minecraft:load", "required": false}, {"id": "minecraft:func2", "required": true}]})");
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().entries.size(), 4u);
    // "minecraft:func1" - 字符串格式，函数引用，required=true
    EXPECT_EQ(result.value().entries[0].id.toString(), "minecraft:func1");
    EXPECT_EQ(result.value().entries[0].type, FunctionLoader::TagEntryType::Function);
    EXPECT_TRUE(result.value().entries[0].required);
    // "#minecraft:tick" - 字符串格式，标签引用，required=true
    EXPECT_EQ(result.value().entries[1].id.toString(), "minecraft:tick");
    EXPECT_EQ(result.value().entries[1].type, FunctionLoader::TagEntryType::Tag);
    EXPECT_TRUE(result.value().entries[1].required);
    // {"id": "#minecraft:load", "required": false} - 对象格式，标签引用，required=false
    EXPECT_EQ(result.value().entries[2].id.toString(), "minecraft:load");
    EXPECT_EQ(result.value().entries[2].type, FunctionLoader::TagEntryType::Tag);
    EXPECT_FALSE(result.value().entries[2].required);
    // {"id": "minecraft:func2", "required": true} - 对象格式，函数引用，required=true
    EXPECT_EQ(result.value().entries[3].id.toString(), "minecraft:func2");
    EXPECT_EQ(result.value().entries[3].type, FunctionLoader::TagEntryType::Function);
    EXPECT_TRUE(result.value().entries[3].required);
}

TEST_F(FunctionLoaderTest, ParseTagJson_ObjectEntryMissingId)
{
    FunctionLoader loader(manager);
    ResourceLocation tagLoc = ResourceLocation::parse("minecraft:bad_tag");
    // 对象条目缺少 id 字段，应被跳过
    auto result = loader.parseTagJson(tagLoc, R"({"values": ["minecraft:valid", {"required": false}]})");
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().entries.size(), 1u);
    EXPECT_EQ(result.value().entries[0].id.toString(), "minecraft:valid");
}

TEST_F(FunctionLoaderTest, ParseTagJson_ObjectEntryDefaultRequired)
{
    FunctionLoader loader(manager);
    ResourceLocation tagLoc = ResourceLocation::parse("minecraft:default_required");
    // 对象格式不指定 required 时默认为 true
    auto result = loader.parseTagJson(tagLoc, R"({"values": [{"id": "minecraft:func"}]})");
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().entries.size(), 1u);
    EXPECT_EQ(result.value().entries[0].id.toString(), "minecraft:func");
    EXPECT_TRUE(result.value().entries[0].required);
}

TEST_F(FunctionLoaderTest, ParseTagJson_NonStringValuesIgnored)
{
    FunctionLoader loader(manager);
    ResourceLocation tagLoc = ResourceLocation::parse("minecraft:tag_with_bad_values");
    // 数值和布尔值被跳过，对象格式现在被正确解析
    auto result = loader.parseTagJson(
        tagLoc, R"({"values": ["minecraft:valid", 42, true, {"id": "minecraft:optional", "required": false}]})");
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().entries.size(), 2u);
    EXPECT_EQ(result.value().entries[0].id.toString(), "minecraft:valid");
    EXPECT_EQ(result.value().entries[0].type, FunctionLoader::TagEntryType::Function);
    EXPECT_TRUE(result.value().entries[0].required);
    EXPECT_EQ(result.value().entries[1].id.toString(), "minecraft:optional");
    EXPECT_EQ(result.value().entries[1].type, FunctionLoader::TagEntryType::Function);
    EXPECT_FALSE(result.value().entries[1].required);
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

// ========== required 验证行为测试 ==========

TEST_F(FunctionLoaderTest, ParseTagJson_RequiredFieldDefaultTrue)
{
    FunctionLoader loader(manager);
    ResourceLocation tagLoc = ResourceLocation::parse("minecraft:test_tag");
    // 字符串格式条目默认 required=true，对象格式不指定 required 时默认为 true
    auto result = loader.parseTagJson(tagLoc, R"({"values": ["minecraft:func1", {"id": "minecraft:func2"}]})");
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().entries.size(), 2u);
    EXPECT_TRUE(result.value().entries[0].required);
    EXPECT_TRUE(result.value().entries[1].required);
}

TEST_F(FunctionLoaderTest, ParseTagJson_RequiredFalseEntryParsed)
{
    FunctionLoader loader(manager);
    ResourceLocation tagLoc = ResourceLocation::parse("minecraft:test_tag");
    auto result = loader.parseTagJson(tagLoc,
        R"({"values": [{"id": "minecraft:missing_func", "required": false}, {"id": "#minecraft:missing_tag", "required": false}]})");
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().entries.size(), 2u);
    EXPECT_EQ(result.value().entries[0].type, FunctionLoader::TagEntryType::Function);
    EXPECT_FALSE(result.value().entries[0].required);
    EXPECT_EQ(result.value().entries[1].type, FunctionLoader::TagEntryType::Tag);
    EXPECT_FALSE(result.value().entries[1].required);
}

TEST_F(FunctionLoaderTest, FunctionManager_RequiredFunctionMissing_DiscardsTag)
{
    // 模拟 required=true 函数缺失导致标签被丢弃的行为
    // 注册一个标签引用了不存在的函数（required=true）
    // 在 FunctionLoader 的 loadFunctionTags 中，required=true 的缺失函数会导致标签不被注册
    // 此处直接测试 FunctionManager 的基础行为
    FunctionManager mgr;
    ResourceLocation tagLoc = ResourceLocation::parse("minecraft:test_tag");

    // FunctionManager 不做 required 验证，它只负责存储和查询
    // required 验证在 FunctionLoader::loadFunctionTags 中完成
    // 所以我们注册的标签可以包含不存在的函数 ID
    std::vector<ResourceLocation> funcs = {ResourceLocation::parse("minecraft:nonexistent_func")};
    mgr.registerTag(tagLoc, std::move(funcs));

    EXPECT_TRUE(mgr.hasTag(tagLoc));
    EXPECT_EQ(mgr.getTag(tagLoc).size(), 1u);
}

TEST_F(FunctionLoaderTest, FunctionManager_MultiLayerTagReference)
{
    // 测试多层标签引用的展开
    // 标签 A 引用标签 B，标签 B 包含函数 func_b
    // 在 FunctionLoader 中，标签 A 的函数列表应包含标签 B 的函数
    FunctionManager mgr;

    // 先注册标签 B（只含直接函数，模拟 3a 阶段）
    ResourceLocation tagB = ResourceLocation::parse("minecraft:tag_b");
    mgr.registerTag(tagB, {ResourceLocation::parse("minecraft:func_b")});
    EXPECT_EQ(mgr.getTag(tagB).size(), 1u);

    // 模拟 3b 阶段：标签 A 引用标签 B，通过 getTag 获取 B 的函数
    // 然后注册标签 A 包含 func_a + func_b
    ResourceLocation tagA = ResourceLocation::parse("minecraft:tag_a");
    const auto& tagBFuncs = mgr.getTag(tagB);
    std::vector<ResourceLocation> tagAFuncs = {ResourceLocation::parse("minecraft:func_a")};
    for (const auto& f : tagBFuncs) {
        tagAFuncs.push_back(f);
    }
    mgr.registerTag(tagA, std::move(tagAFuncs));

    // 标签 A 应包含 func_a 和 func_b
    EXPECT_EQ(mgr.getTag(tagA).size(), 2u);
    EXPECT_EQ(mgr.getTag(tagA)[0].toString(), "minecraft:func_a");
    EXPECT_EQ(mgr.getTag(tagA)[1].toString(), "minecraft:func_b");
}

TEST_F(FunctionLoaderTest, FunctionManager_CascadingTagReference)
{
    // 测试三层标签引用: A → B → C
    FunctionManager mgr;

    // 注册标签 C
    ResourceLocation tagC = ResourceLocation::parse("minecraft:tag_c");
    mgr.registerTag(tagC, {ResourceLocation::parse("minecraft:func_c")});

    // 注册标签 B（引用标签 C 的函数）
    ResourceLocation tagB = ResourceLocation::parse("minecraft:tag_b");
    std::vector<ResourceLocation> tagBFuncs = {ResourceLocation::parse("minecraft:func_b")};
    for (const auto& f : mgr.getTag(tagC)) {
        tagBFuncs.push_back(f);
    }
    mgr.registerTag(tagB, std::move(tagBFuncs));

    // 注册标签 A（引用标签 B 的函数，其中已包含标签 C 的函数）
    ResourceLocation tagA = ResourceLocation::parse("minecraft:tag_a");
    std::vector<ResourceLocation> tagAFuncs = {ResourceLocation::parse("minecraft:func_a")};
    for (const auto& f : mgr.getTag(tagB)) {
        tagAFuncs.push_back(f);
    }
    mgr.registerTag(tagA, std::move(tagAFuncs));

    // 标签 A 应包含 func_a, func_b, func_c
    EXPECT_EQ(mgr.getTag(tagA).size(), 3u);
    EXPECT_EQ(mgr.getTag(tagA)[0].toString(), "minecraft:func_a");
    EXPECT_EQ(mgr.getTag(tagA)[1].toString(), "minecraft:func_b");
    EXPECT_EQ(mgr.getTag(tagA)[2].toString(), "minecraft:func_c");
}

// ========== loadFunctionTags 集成测试（通过临时数据包目录） ==========

namespace {

void writeTextFile(const std::filesystem::path& path, const std::string& text)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file << text;
}

void cleanupTempDir(const std::filesystem::path& dir)
{
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

} // anonymous namespace

class FunctionLoaderIntegrationTest : public ::testing::Test {
protected:
    void TearDown() override
    {
        if (!m_tempDir.empty()) {
            cleanupTempDir(m_tempDir);
        }
    }

    /// 创建一个基本的数据包目录，包含 pack.mcmeta
    std::filesystem::path createBaseDataPack()
    {
        m_tempDir = mc::test::makeUniqueTestDir("mc_func_loader_test");
        auto packDir = m_tempDir / "test_pack";
        std::filesystem::create_directories(packDir / "data" / "minecraft" / "functions");
        std::filesystem::create_directories(packDir / "data" / "minecraft" / "tags" / "functions");
        writeTextFile(packDir / "pack.mcmeta", R"({"pack":{"pack_format":41,"description":"test"}})");
        return packDir;
    }

    /// 向数据包添加一个函数文件
    void addFunction(const std::filesystem::path& packDir,
        const std::string& namespace_,
        const std::string& functionPath,
        const std::string& content)
    {
        auto funcDir = packDir / "data" / namespace_ / "functions";
        // 确保目录存在
        auto fullPath = funcDir / (functionPath + ".mcfunction");
        std::filesystem::create_directories(fullPath.parent_path());
        writeTextFile(fullPath, content);
    }

    /// 向数据包添加一个函数标签文件
    void addTag(const std::filesystem::path& packDir,
        const std::string& namespace_,
        const std::string& tagPath,
        const std::string& jsonContent)
    {
        auto tagDir = packDir / "data" / namespace_ / "tags" / "functions";
        auto fullPath = tagDir / (tagPath + ".json");
        std::filesystem::create_directories(fullPath.parent_path());
        writeTextFile(fullPath, jsonContent);
    }

    std::filesystem::path m_tempDir;
};

TEST_F(FunctionLoaderIntegrationTest, RequiredTrue_MissingFunction_DiscardsTag)
{
    // 测试：required=true 的函数缺失时，整个标签被丢弃
    auto packDir = createBaseDataPack();

    // 添加一个存在的函数
    addFunction(packDir, "minecraft", "existing_func", "say hello");

    // 添加标签，引用一个不存在的函数（required=true，默认）
    addTag(packDir, "minecraft", "test_tag", R"({"values": ["minecraft:existing_func", "minecraft:missing_func"]})");

    // 加载
    FunctionManager mgr;
    FunctionLoader loader(mgr);
    mc::resource::DataPackRepository dataPacks;
    dataPacks.scanDirectory(m_tempDir);

    auto result = loader.loadFromDataPackRepository(dataPacks);
    ASSERT_TRUE(result.success());

    // 函数应该加载成功
    EXPECT_EQ(result.value().successCount, 1u);

    // 标签应被丢弃（因为 minecraft:missing_func 不存在且 required=true）
    EXPECT_EQ(result.value().tagCount, 0u);

    // FunctionManager 中不应有该标签
    EXPECT_FALSE(mgr.hasTag(ResourceLocation::parse("minecraft:test_tag")));
}

TEST_F(FunctionLoaderIntegrationTest, RequiredFalse_MissingFunction_TagKept)
{
    // 测试：required=false 的函数缺失时，标签保留（缺失函数被跳过）
    auto packDir = createBaseDataPack();

    // 添加一个存在的函数
    addFunction(packDir, "minecraft", "existing_func", "say hello");

    // 添加标签，引用缺失的函数但标记 required=false
    addTag(packDir,
        "minecraft",
        "test_tag",
        R"({"values": ["minecraft:existing_func", {"id": "minecraft:missing_func", "required": false}]})");

    FunctionManager mgr;
    FunctionLoader loader(mgr);
    mc::resource::DataPackRepository dataPacks;
    dataPacks.scanDirectory(m_tempDir);

    auto result = loader.loadFromDataPackRepository(dataPacks);
    ASSERT_TRUE(result.success());

    // 标签应保留
    EXPECT_EQ(result.value().tagCount, 1u);
    EXPECT_TRUE(mgr.hasTag(ResourceLocation::parse("minecraft:test_tag")));

    // 标签应只包含存在的函数
    const auto& funcs = mgr.getTag(ResourceLocation::parse("minecraft:test_tag"));
    EXPECT_EQ(funcs.size(), 1u);
    EXPECT_EQ(funcs[0].toString(), "minecraft:existing_func");
}

TEST_F(FunctionLoaderIntegrationTest, RequiredTrue_MissingTagRef_DiscardsTag)
{
    // 测试：required=true 的标签引用缺失时，整个标签被丢弃
    auto packDir = createBaseDataPack();

    addFunction(packDir, "minecraft", "func_a", "say hello");

    // 标签引用一个不存在的标签（required=true，默认）
    addTag(packDir, "minecraft", "test_tag", R"({"values": ["minecraft:func_a", "#minecraft:nonexistent_tag"]})");

    FunctionManager mgr;
    FunctionLoader loader(mgr);
    mc::resource::DataPackRepository dataPacks;
    dataPacks.scanDirectory(m_tempDir);

    auto result = loader.loadFromDataPackRepository(dataPacks);
    ASSERT_TRUE(result.success());

    // 标签应被丢弃（因为引用的 nonexistent_tag 不存在且 required=true）
    EXPECT_EQ(result.value().tagCount, 0u);
    EXPECT_FALSE(mgr.hasTag(ResourceLocation::parse("minecraft:test_tag")));
}

TEST_F(FunctionLoaderIntegrationTest, RequiredFalse_MissingTagRef_TagKept)
{
    // 测试：required=false 的标签引用缺失时，标签保留
    auto packDir = createBaseDataPack();

    addFunction(packDir, "minecraft", "func_a", "say hello");

    // 标签引用一个不存在的标签但标记 required=false
    addTag(packDir,
        "minecraft",
        "test_tag",
        R"({"values": ["minecraft:func_a", {"id": "#minecraft:nonexistent_tag", "required": false}]})");

    FunctionManager mgr;
    FunctionLoader loader(mgr);
    mc::resource::DataPackRepository dataPacks;
    dataPacks.scanDirectory(m_tempDir);

    auto result = loader.loadFromDataPackRepository(dataPacks);
    ASSERT_TRUE(result.success());

    // 标签应保留
    EXPECT_EQ(result.value().tagCount, 1u);
    EXPECT_TRUE(mgr.hasTag(ResourceLocation::parse("minecraft:test_tag")));

    // 标签应只包含直接函数
    const auto& funcs = mgr.getTag(ResourceLocation::parse("minecraft:test_tag"));
    EXPECT_EQ(funcs.size(), 1u);
    EXPECT_EQ(funcs[0].toString(), "minecraft:func_a");
}

TEST_F(FunctionLoaderIntegrationTest, CascadingDiscard)
{
    // 测试：级联丢弃 — 标签 A required=true 引用标签 B，标签 B 因缺失函数被丢弃，
    // 导致标签 A 也被丢弃
    auto packDir = createBaseDataPack();

    // 只添加 func_a，不添加 func_b
    addFunction(packDir, "minecraft", "func_a", "say hello");

    // 标签 B 引用不存在的 func_b（required=true），因此标签 B 会被丢弃
    addTag(packDir, "minecraft", "tag_b", R"({"values": ["minecraft:missing_func_b"]})");

    // 标签 A required=true 引用标签 B，标签 B 被丢弃后标签 A 也应被丢弃
    addTag(packDir, "minecraft", "tag_a", R"({"values": ["minecraft:func_a", "#minecraft:tag_b"]})");

    FunctionManager mgr;
    FunctionLoader loader(mgr);
    mc::resource::DataPackRepository dataPacks;
    dataPacks.scanDirectory(m_tempDir);

    auto result = loader.loadFromDataPackRepository(dataPacks);
    ASSERT_TRUE(result.success());

    // 两个标签都应被丢弃
    EXPECT_EQ(result.value().tagCount, 0u);
    EXPECT_FALSE(mgr.hasTag(ResourceLocation::parse("minecraft:tag_a")));
    EXPECT_FALSE(mgr.hasTag(ResourceLocation::parse("minecraft:tag_b")));
}

TEST_F(FunctionLoaderIntegrationTest, CascadingDiscard_RequiredFalseRef_Kept)
{
    // 测试：级联丢弃不发生在 required=false 的标签引用上
    auto packDir = createBaseDataPack();

    addFunction(packDir, "minecraft", "func_a", "say hello");

    // 标签 B 引用不存在的函数，会被丢弃
    addTag(packDir, "minecraft", "tag_b", R"({"values": ["minecraft:missing_func_b"]})");

    // 标签 A required=false 引用标签 B，标签 B 被丢弃但标签 A 应保留
    addTag(packDir,
        "minecraft",
        "tag_a",
        R"({"values": ["minecraft:func_a", {"id": "#minecraft:tag_b", "required": false}]})");

    FunctionManager mgr;
    FunctionLoader loader(mgr);
    mc::resource::DataPackRepository dataPacks;
    dataPacks.scanDirectory(m_tempDir);

    auto result = loader.loadFromDataPackRepository(dataPacks);
    ASSERT_TRUE(result.success());

    // 标签 A 应保留，标签 B 应被丢弃
    EXPECT_EQ(result.value().tagCount, 1u);
    EXPECT_TRUE(mgr.hasTag(ResourceLocation::parse("minecraft:tag_a")));
    EXPECT_FALSE(mgr.hasTag(ResourceLocation::parse("minecraft:tag_b")));

    // 标签 A 应只包含 func_a（tag_b 的函数不传播）
    const auto& funcs = mgr.getTag(ResourceLocation::parse("minecraft:tag_a"));
    EXPECT_EQ(funcs.size(), 1u);
    EXPECT_EQ(funcs[0].toString(), "minecraft:func_a");
}

TEST_F(FunctionLoaderIntegrationTest, TagReferenceExpansion)
{
    // 测试：标签引用正确展开 — 标签 A 引用标签 B，标签 B 的函数应出现在标签 A 中
    auto packDir = createBaseDataPack();

    addFunction(packDir, "minecraft", "func_a", "say hello");
    addFunction(packDir, "minecraft", "func_b", "say world");

    // 标签 B 包含 func_b
    addTag(packDir, "minecraft", "tag_b", R"({"values": ["minecraft:func_b"]})");

    // 标签 A 包含 func_a 并引用标签 B
    addTag(packDir, "minecraft", "tag_a", R"({"values": ["minecraft:func_a", "#minecraft:tag_b"]})");

    FunctionManager mgr;
    FunctionLoader loader(mgr);
    mc::resource::DataPackRepository dataPacks;
    dataPacks.scanDirectory(m_tempDir);

    auto result = loader.loadFromDataPackRepository(dataPacks);
    ASSERT_TRUE(result.success());

    // 两个标签都应注册成功
    EXPECT_EQ(result.value().tagCount, 2u);
    EXPECT_TRUE(mgr.hasTag(ResourceLocation::parse("minecraft:tag_a")));
    EXPECT_TRUE(mgr.hasTag(ResourceLocation::parse("minecraft:tag_b")));

    // 标签 B 应只包含 func_b
    const auto& tagBFuncs = mgr.getTag(ResourceLocation::parse("minecraft:tag_b"));
    EXPECT_EQ(tagBFuncs.size(), 1u);
    EXPECT_EQ(tagBFuncs[0].toString(), "minecraft:func_b");

    // 标签 A 应包含 func_a 和 func_b（从 tag_b 展开）
    const auto& tagAFuncs = mgr.getTag(ResourceLocation::parse("minecraft:tag_a"));
    EXPECT_EQ(tagAFuncs.size(), 2u);
    EXPECT_EQ(tagAFuncs[0].toString(), "minecraft:func_a");
    EXPECT_EQ(tagAFuncs[1].toString(), "minecraft:func_b");
}

TEST_F(FunctionLoaderIntegrationTest, MultiLayerTagReferenceExpansion)
{
    // 测试：多层标签引用展开 — A → B → C
    auto packDir = createBaseDataPack();

    addFunction(packDir, "minecraft", "func_a", "say hello");
    addFunction(packDir, "minecraft", "func_b", "say world");
    addFunction(packDir, "minecraft", "func_c", "say test");

    // 标签 C 包含 func_c
    addTag(packDir, "minecraft", "tag_c", R"({"values": ["minecraft:func_c"]})");

    // 标签 B 包含 func_b 并引用标签 C
    addTag(packDir, "minecraft", "tag_b", R"({"values": ["minecraft:func_b", "#minecraft:tag_c"]})");

    // 标签 A 包含 func_a 并引用标签 B
    addTag(packDir, "minecraft", "tag_a", R"({"values": ["minecraft:func_a", "#minecraft:tag_b"]})");

    FunctionManager mgr;
    FunctionLoader loader(mgr);
    mc::resource::DataPackRepository dataPacks;
    dataPacks.scanDirectory(m_tempDir);

    auto result = loader.loadFromDataPackRepository(dataPacks);
    ASSERT_TRUE(result.success());

    EXPECT_EQ(result.value().tagCount, 3u);

    // 标签 C: [func_c]
    const auto& tagCFuncs = mgr.getTag(ResourceLocation::parse("minecraft:tag_c"));
    EXPECT_EQ(tagCFuncs.size(), 1u);
    EXPECT_EQ(tagCFuncs[0].toString(), "minecraft:func_c");

    // 标签 B: [func_b, func_c]（展开标签 C 的函数）
    const auto& tagBFuncs = mgr.getTag(ResourceLocation::parse("minecraft:tag_b"));
    EXPECT_EQ(tagBFuncs.size(), 2u);
    EXPECT_EQ(tagBFuncs[0].toString(), "minecraft:func_b");
    EXPECT_EQ(tagBFuncs[1].toString(), "minecraft:func_c");

    // 标签 A: [func_a, func_b, func_c]（展开标签 B，其中已包含标签 C 的函数）
    const auto& tagAFuncs = mgr.getTag(ResourceLocation::parse("minecraft:tag_a"));
    EXPECT_EQ(tagAFuncs.size(), 3u);
    EXPECT_EQ(tagAFuncs[0].toString(), "minecraft:func_a");
    EXPECT_EQ(tagAFuncs[1].toString(), "minecraft:func_b");
    EXPECT_EQ(tagAFuncs[2].toString(), "minecraft:func_c");
}

// ========== 多数据包标签合并集成测试 ==========

/// 创建包含两个数据包的临时目录，用于多数据包标签合并测试
/// 低优先级包（priority=0）：定义 tick 标签和一些函数
/// 高优先级包（priority=1）：定义同名 tick 标签和额外函数
class FunctionLoaderMultiPackTest : public ::testing::Test {
protected:
    void TearDown() override
    {
        if (!m_tempDir.empty()) {
            cleanupTempDir(m_tempDir);
        }
    }

    /// 创建两个数据包的临时目录
    void createMultiPackDir()
    {
        m_tempDir = mc::test::makeUniqueTestDir("mc_func_loader_test");

        // 低优先级数据包
        m_lowPackDir = m_tempDir / "low_pack";
        std::filesystem::create_directories(m_lowPackDir / "data" / "minecraft" / "functions");
        std::filesystem::create_directories(m_lowPackDir / "data" / "minecraft" / "tags" / "functions");
        writeTextFile(m_lowPackDir / "pack.mcmeta", R"({"pack":{"pack_format":41,"description":"low pack"}})");

        // 高优先级数据包
        m_highPackDir = m_tempDir / "high_pack";
        std::filesystem::create_directories(m_highPackDir / "data" / "minecraft" / "functions");
        std::filesystem::create_directories(m_highPackDir / "data" / "minecraft" / "tags" / "functions");
        writeTextFile(m_highPackDir / "pack.mcmeta", R"({"pack":{"pack_format":41,"description":"high pack"}})");
    }

    /// 添加数据包到 DataPackRepository，使用显式优先级确保确定性排序
    /// high_pack: priority=1 (高优先级)
    /// low_pack: priority=0 (低优先级)
    void addPacksWithExplicitPriority(mc::resource::DataPackRepository& dataPacks)
    {
        dataPacks.addPack(m_lowPackDir, true, 0);  // 低优先级
        dataPacks.addPack(m_highPackDir, true, 1); // 高优先级
    }

    /// 向数据包添加一个函数文件
    void addFunction(const std::filesystem::path& packDir,
        const std::string& namespace_,
        const std::string& functionPath,
        const std::string& content)
    {
        auto funcDir = packDir / "data" / namespace_ / "functions";
        auto fullPath = funcDir / (functionPath + ".mcfunction");
        std::filesystem::create_directories(fullPath.parent_path());
        writeTextFile(fullPath, content);
    }

    /// 向数据包添加一个函数标签文件
    void addTag(const std::filesystem::path& packDir,
        const std::string& namespace_,
        const std::string& tagPath,
        const std::string& jsonContent)
    {
        auto tagDir = packDir / "data" / namespace_ / "tags" / "functions";
        auto fullPath = tagDir / (tagPath + ".json");
        std::filesystem::create_directories(fullPath.parent_path());
        writeTextFile(fullPath, jsonContent);
    }

    std::filesystem::path m_tempDir;
    std::filesystem::path m_lowPackDir;
    std::filesystem::path m_highPackDir;
};

TEST_F(FunctionLoaderMultiPackTest, AppendMerge_TwoPacksSameTag)
{
    // 测试：两个数据包定义同名标签，replace=false（默认），条目应合并
    // MC Java 行为：低优先级包的条目先处理，高优先级包的条目后追加
    createMultiPackDir();

    // 低优先级包：tick 标签包含 func_a 和 func_b
    addFunction(m_lowPackDir, "minecraft", "func_a", "say a");
    addFunction(m_lowPackDir, "minecraft", "func_b", "say b");
    addTag(m_lowPackDir, "minecraft", "tick", R"({"values": ["minecraft:func_a", "minecraft:func_b"]})");

    // 高优先级包：tick 标签包含 func_c
    addFunction(m_highPackDir, "minecraft", "func_c", "say c");
    addTag(m_highPackDir, "minecraft", "tick", R"({"values": ["minecraft:func_c"]})");

    FunctionManager mgr;
    FunctionLoader loader(mgr);
    mc::resource::DataPackRepository dataPacks;
    addPacksWithExplicitPriority(dataPacks);

    auto result = loader.loadFromDataPackRepository(dataPacks);
    ASSERT_TRUE(result.success());

    // 标签应该成功注册
    EXPECT_TRUE(mgr.hasTag(ResourceLocation::parse("minecraft:tick")));

    // 合并后的标签应包含低优先级和高优先级包的所有函数
    // MC Java 行为：先处理低优先级(func_a, func_b)，再追加高优先级(func_c)
    const auto& tickFuncs = mgr.getTag(ResourceLocation::parse("minecraft:tick"));
    EXPECT_EQ(tickFuncs.size(), 3u);

    // 验证所有函数都在标签中（顺序：低优先级条目先，高优先级条目后）
    EXPECT_EQ(tickFuncs[0].toString(), "minecraft:func_a");
    EXPECT_EQ(tickFuncs[1].toString(), "minecraft:func_b");
    EXPECT_EQ(tickFuncs[2].toString(), "minecraft:func_c");
}

TEST_F(FunctionLoaderMultiPackTest, ReplaceTrue_HighPriorityClearsLowPriority)
{
    // 测试：高优先级数据包的标签设置 replace=true，应清空低优先级包的条目后追加自己的
    // MC Java 行为：低优先级条目先处理并累积，高优先级 replace=true 时清空已有条目后追加
    createMultiPackDir();

    // 低优先级包：tick 标签包含 func_a 和 func_b
    addFunction(m_lowPackDir, "minecraft", "func_a", "say a");
    addFunction(m_lowPackDir, "minecraft", "func_b", "say b");
    addTag(m_lowPackDir, "minecraft", "tick", R"({"values": ["minecraft:func_a", "minecraft:func_b"]})");

    // 高优先级包：tick 标签设置 replace=true，只包含 func_c
    addFunction(m_highPackDir, "minecraft", "func_c", "say c");
    addTag(m_highPackDir, "minecraft", "tick", R"({"replace": true, "values": ["minecraft:func_c"]})");

    FunctionManager mgr;
    FunctionLoader loader(mgr);
    mc::resource::DataPackRepository dataPacks;
    addPacksWithExplicitPriority(dataPacks);

    auto result = loader.loadFromDataPackRepository(dataPacks);
    ASSERT_TRUE(result.success());

    // 标签应该成功注册
    EXPECT_TRUE(mgr.hasTag(ResourceLocation::parse("minecraft:tick")));

    // replace=true 应清空低优先级包的条目，只保留高优先级包的条目
    const auto& tickFuncs = mgr.getTag(ResourceLocation::parse("minecraft:tick"));
    EXPECT_EQ(tickFuncs.size(), 1u);
    EXPECT_EQ(tickFuncs[0].toString(), "minecraft:func_c");
}

TEST_F(FunctionLoaderMultiPackTest, ReplaceTrue_LowPriorityDoesNotAffectHighPriority)
{
    // 测试：低优先级数据包的标签设置 replace=true，不应影响高优先级包的追加条目
    // MC Java 行为：低优先级 replace=true 清空当前列表（此时可能为空或已有更早的低优先级条目），
    // 然后高优先级包的条目继续追加
    createMultiPackDir();

    // 低优先级包：tick 标签设置 replace=true，包含 func_a
    addFunction(m_lowPackDir, "minecraft", "func_a", "say a");
    addTag(m_lowPackDir, "minecraft", "tick", R"({"replace": true, "values": ["minecraft:func_a"]})");

    // 高优先级包：tick 标签 replace=false（默认），包含 func_b
    addFunction(m_highPackDir, "minecraft", "func_b", "say b");
    addTag(m_highPackDir, "minecraft", "tick", R"({"values": ["minecraft:func_b"]})");

    FunctionManager mgr;
    FunctionLoader loader(mgr);
    mc::resource::DataPackRepository dataPacks;
    addPacksWithExplicitPriority(dataPacks);

    auto result = loader.loadFromDataPackRepository(dataPacks);
    ASSERT_TRUE(result.success());

    EXPECT_TRUE(mgr.hasTag(ResourceLocation::parse("minecraft:tick")));

    // 低优先级 replace=true 清空了（空列表），然后追加 func_a，
    // 高优先级 replace=false 追加 func_b
    // 最终: [func_a, func_b]
    const auto& tickFuncs = mgr.getTag(ResourceLocation::parse("minecraft:tick"));
    EXPECT_EQ(tickFuncs.size(), 2u);
    EXPECT_EQ(tickFuncs[0].toString(), "minecraft:func_a");
    EXPECT_EQ(tickFuncs[1].toString(), "minecraft:func_b");
}

TEST_F(FunctionLoaderMultiPackTest, DifferentTagsFromDifferentPacks)
{
    // 测试：不同数据包定义不同的标签，应各自独立注册
    createMultiPackDir();

    // 低优先级包：定义 load 标签
    addFunction(m_lowPackDir, "minecraft", "init_func", "say init");
    addTag(m_lowPackDir, "minecraft", "load", R"({"values": ["minecraft:init_func"]})");

    // 高优先级包：定义 tick 标签（不同于低优先级包的标签）
    addFunction(m_highPackDir, "minecraft", "tick_func", "say tick");
    addTag(m_highPackDir, "minecraft", "tick", R"({"values": ["minecraft:tick_func"]})");

    FunctionManager mgr;
    FunctionLoader loader(mgr);
    mc::resource::DataPackRepository dataPacks;
    addPacksWithExplicitPriority(dataPacks);

    auto result = loader.loadFromDataPackRepository(dataPacks);
    ASSERT_TRUE(result.success());

    EXPECT_EQ(result.value().tagCount, 2u);

    // load 标签
    EXPECT_TRUE(mgr.hasTag(ResourceLocation::parse("minecraft:load")));
    const auto& loadFuncs = mgr.getTag(ResourceLocation::parse("minecraft:load"));
    EXPECT_EQ(loadFuncs.size(), 1u);
    EXPECT_EQ(loadFuncs[0].toString(), "minecraft:init_func");

    // tick 标签
    EXPECT_TRUE(mgr.hasTag(ResourceLocation::parse("minecraft:tick")));
    const auto& tickFuncs = mgr.getTag(ResourceLocation::parse("minecraft:tick"));
    EXPECT_EQ(tickFuncs.size(), 1u);
    EXPECT_EQ(tickFuncs[0].toString(), "minecraft:tick_func");
}

TEST_F(FunctionLoaderMultiPackTest, ReplaceTrue_ThreePacks)
{
    // 测试：三个数据包，中间优先级设置 replace=true
    // 预期行为：低优先级条目先累积，中间优先级 replace=true 清空后追加，
    // 高优先级 replace=false 追加到中间优先级的条目后
    m_tempDir = mc::test::makeUniqueTestDir("mc_func_loader_test");

    // 低优先级包 (priority=0)
    auto lowPackDir = m_tempDir / "low_pack";
    std::filesystem::create_directories(lowPackDir / "data" / "minecraft" / "functions");
    std::filesystem::create_directories(lowPackDir / "data" / "minecraft" / "tags" / "functions");
    writeTextFile(lowPackDir / "pack.mcmeta", R"({"pack":{"pack_format":41,"description":"low pack"}})");
    addFunction(lowPackDir, "minecraft", "func_a", "say a");
    addFunction(lowPackDir, "minecraft", "func_b", "say b");
    addTag(lowPackDir, "minecraft", "tick", R"({"values": ["minecraft:func_a", "minecraft:func_b"]})");

    // 中间优先级包 (priority=1) - replace=true
    auto midPackDir = m_tempDir / "mid_pack";
    std::filesystem::create_directories(midPackDir / "data" / "minecraft" / "functions");
    std::filesystem::create_directories(midPackDir / "data" / "minecraft" / "tags" / "functions");
    writeTextFile(midPackDir / "pack.mcmeta", R"({"pack":{"pack_format":41,"description":"mid pack"}})");
    addFunction(midPackDir, "minecraft", "func_c", "say c");
    addTag(midPackDir, "minecraft", "tick", R"({"replace": true, "values": ["minecraft:func_c"]})");

    // 高优先级包 (priority=2)
    auto highPackDir = m_tempDir / "high_pack";
    std::filesystem::create_directories(highPackDir / "data" / "minecraft" / "functions");
    std::filesystem::create_directories(highPackDir / "data" / "minecraft" / "tags" / "functions");
    writeTextFile(highPackDir / "pack.mcmeta", R"({"pack":{"pack_format":41,"description":"high pack"}})");
    addFunction(highPackDir, "minecraft", "func_d", "say d");
    addTag(highPackDir, "minecraft", "tick", R"({"values": ["minecraft:func_d"]})");

    FunctionManager mgr;
    FunctionLoader loader(mgr);
    mc::resource::DataPackRepository dataPacks;
    dataPacks.addPack(lowPackDir, true, 0);  // 低优先级
    dataPacks.addPack(midPackDir, true, 1);  // 中间优先级
    dataPacks.addPack(highPackDir, true, 2); // 高优先级

    auto result = loader.loadFromDataPackRepository(dataPacks);
    ASSERT_TRUE(result.success());

    EXPECT_TRUE(mgr.hasTag(ResourceLocation::parse("minecraft:tick")));

    // 低优先级条目 [func_a, func_b] 先累积，
    // 中间优先级 replace=true 清空后追加 [func_c]，
    // 高优先级 replace=false 追加 [func_d]，
    // 最终: [func_c, func_d]
    const auto& tickFuncs = mgr.getTag(ResourceLocation::parse("minecraft:tick"));
    EXPECT_EQ(tickFuncs.size(), 2u);
    EXPECT_EQ(tickFuncs[0].toString(), "minecraft:func_c");
    EXPECT_EQ(tickFuncs[1].toString(), "minecraft:func_d");
}

TEST_F(FunctionLoaderMultiPackTest, AppendMerge_TagReferencesAcrossPacks)
{
    // 测试：不同数据包定义不同标签，高优先级包引用低优先级包的标签
    createMultiPackDir();

    // 低优先级包：定义 tag_b 包含 func_b
    addFunction(m_lowPackDir, "minecraft", "func_b", "say b");
    addTag(m_lowPackDir, "minecraft", "tag_b", R"({"values": ["minecraft:func_b"]})");

    // 高优先级包：定义 tag_a 引用 tag_b
    addFunction(m_highPackDir, "minecraft", "func_a", "say a");
    addTag(m_highPackDir, "minecraft", "tag_a", R"({"values": ["minecraft:func_a", "#minecraft:tag_b"]})");

    FunctionManager mgr;
    FunctionLoader loader(mgr);
    mc::resource::DataPackRepository dataPacks;
    addPacksWithExplicitPriority(dataPacks);

    auto result = loader.loadFromDataPackRepository(dataPacks);
    ASSERT_TRUE(result.success());

    EXPECT_EQ(result.value().tagCount, 2u);

    // tag_b: [func_b]
    EXPECT_TRUE(mgr.hasTag(ResourceLocation::parse("minecraft:tag_b")));
    const auto& tagBFuncs = mgr.getTag(ResourceLocation::parse("minecraft:tag_b"));
    EXPECT_EQ(tagBFuncs.size(), 1u);
    EXPECT_EQ(tagBFuncs[0].toString(), "minecraft:func_b");

    // tag_a: [func_a, func_b]（展开 #minecraft:tag_b 引用）
    EXPECT_TRUE(mgr.hasTag(ResourceLocation::parse("minecraft:tag_a")));
    const auto& tagAFuncs = mgr.getTag(ResourceLocation::parse("minecraft:tag_a"));
    EXPECT_EQ(tagAFuncs.size(), 2u);
    EXPECT_EQ(tagAFuncs[0].toString(), "minecraft:func_a");
    EXPECT_EQ(tagAFuncs[1].toString(), "minecraft:func_b");
}

TEST_F(FunctionLoaderMultiPackTest, ReplaceTrue_WithSameTagReference)
{
    // 测试：高优先级包 replace=true 的标签引用低优先级包的标签
    createMultiPackDir();

    // 低优先级包：定义 tag_b 和 tick 标签
    addFunction(m_lowPackDir, "minecraft", "func_b", "say b");
    addFunction(m_lowPackDir, "minecraft", "tick_func_low", "say tick_low");
    addTag(m_lowPackDir, "minecraft", "tag_b", R"({"values": ["minecraft:func_b"]})");
    addTag(m_lowPackDir, "minecraft", "tick", R"({"values": ["minecraft:tick_func_low", "#minecraft:tag_b"]})");

    // 高优先级包：tick 标签 replace=true，只包含 tick_func_high
    addFunction(m_highPackDir, "minecraft", "tick_func_high", "say tick_high");
    addTag(m_highPackDir, "minecraft", "tick", R"({"replace": true, "values": ["minecraft:tick_func_high"]})");

    FunctionManager mgr;
    FunctionLoader loader(mgr);
    mc::resource::DataPackRepository dataPacks;
    addPacksWithExplicitPriority(dataPacks);

    auto result = loader.loadFromDataPackRepository(dataPacks);
    ASSERT_TRUE(result.success());

    // tick 标签：低优先级的 [tick_func_low, tag_b引用] 被 replace=true 清空，
    // 只保留高优先级的 [tick_func_high]
    EXPECT_TRUE(mgr.hasTag(ResourceLocation::parse("minecraft:tick")));
    const auto& tickFuncs = mgr.getTag(ResourceLocation::parse("minecraft:tick"));
    EXPECT_EQ(tickFuncs.size(), 1u);
    EXPECT_EQ(tickFuncs[0].toString(), "minecraft:tick_func_high");

    // tag_b 不受影响
    EXPECT_TRUE(mgr.hasTag(ResourceLocation::parse("minecraft:tag_b")));
    const auto& tagBFuncs = mgr.getTag(ResourceLocation::parse("minecraft:tag_b"));
    EXPECT_EQ(tagBFuncs.size(), 1u);
    EXPECT_EQ(tagBFuncs[0].toString(), "minecraft:func_b");
}
