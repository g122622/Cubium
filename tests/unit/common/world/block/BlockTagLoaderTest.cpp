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

#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/InMemoryResourcePack.hpp"
#include "common/world/block/BlockTagLoader.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;

class BlockTagLoaderTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 方块注册 + 内置标签初始化（BlockTags::initialize 随 VanillaBlocks 一并完成）
        VanillaBlocks::initialize();
    }
};

// ============================================================================
// loadFromResourcePack - 基本加载
// ============================================================================

TEST_F(BlockTagLoaderTest, LoadFromResourcePackBasicTag)
{
    auto pack = std::make_unique<mc::InMemoryResourcePack>("block_tag_pack");
    pack->addServerDataResource("minecraft/tags/block/test_basic_blocks.json",
        R"({
            "values": [
                "minecraft:stone",
                "minecraft:dirt",
                "minecraft:sand"
            ]
        })");

    auto result = BlockTagLoader::loadFromResourcePack(*pack);
    ASSERT_TRUE(result.success());
    EXPECT_GE(result.value(), 1u);

    auto* tag = BlockTags::getTag(ResourceLocation("minecraft", "test_basic_blocks"));
    ASSERT_NE(tag, nullptr);
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft", "stone")));
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft", "dirt")));
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft", "sand")));
    EXPECT_EQ(tag->getBlockIds().size(), 3u);
}

TEST_F(BlockTagLoaderTest, UnknownBlockMemberIsSkipped)
{
    // 未注册的方块名（required 默认 true）应被跳过，其余成员仍生效
    auto pack = std::make_unique<mc::InMemoryResourcePack>("unknown_block_pack");
    pack->addServerDataResource("minecraft/tags/block/test_unknown_block.json",
        R"({
            "values": [
                "minecraft:stone",
                "minecraft:this_block_does_not_exist"
            ]
        })");

    auto result = BlockTagLoader::loadFromResourcePack(*pack);
    ASSERT_TRUE(result.success());

    auto* tag = BlockTags::getTag(ResourceLocation("minecraft", "test_unknown_block"));
    ASSERT_NE(tag, nullptr);
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft", "stone")));
    EXPECT_EQ(tag->getBlockIds().size(), 1u);
}

TEST_F(BlockTagLoaderTest, TagReferenceResolvesMembers)
{
    // 标签引用：test_all 引用 test_sub_a / test_sub_b，应展开全部成员
    auto pack = std::make_unique<mc::InMemoryResourcePack>("ref_block_pack");
    pack->addServerDataResource(
        "minecraft/tags/block/test_sub_a.json", R"({"values": ["minecraft:stone", "minecraft:dirt"]})");
    pack->addServerDataResource("minecraft/tags/block/test_sub_b.json", R"({"values": ["minecraft:sand"]})");
    pack->addServerDataResource(
        "minecraft/tags/block/test_all.json", R"({"values": ["#minecraft:test_sub_a", "#minecraft:test_sub_b"]})");

    auto result = BlockTagLoader::loadFromResourcePack(*pack);
    ASSERT_TRUE(result.success());

    auto* all = BlockTags::getTag(ResourceLocation("minecraft", "test_all"));
    ASSERT_NE(all, nullptr);
    EXPECT_TRUE(all->contains(ResourceLocation("minecraft", "stone")));
    EXPECT_TRUE(all->contains(ResourceLocation("minecraft", "dirt")));
    EXPECT_TRUE(all->contains(ResourceLocation("minecraft", "sand")));
    EXPECT_EQ(all->getBlockIds().size(), 3u);
}

TEST_F(BlockTagLoaderTest, ReplaceSemanticsClearsBuiltinTag)
{
    // 数据包 replace=true 应清空内置标签内容后再写入
    auto& builtin = BlockTags::registerTag(ResourceLocation("minecraft", "test_replace_builtin"));
    builtin.add(ResourceLocation("minecraft", "bedrock"));

    auto pack = std::make_unique<mc::InMemoryResourcePack>("replace_block_pack");
    pack->addServerDataResource("minecraft/tags/block/test_replace_builtin.json",
        R"({
            "replace": true,
            "values": ["minecraft:stone"]
        })");

    auto result = BlockTagLoader::loadFromResourcePack(*pack);
    ASSERT_TRUE(result.success());

    auto* tag = BlockTags::getTag(ResourceLocation("minecraft", "test_replace_builtin"));
    ASSERT_NE(tag, nullptr);
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft", "stone")));
    EXPECT_FALSE(tag->contains(ResourceLocation("minecraft", "bedrock")));
}

TEST_F(BlockTagLoaderTest, AppendSemanticsKeepsBuiltinMembers)
{
    // 默认追加语义应保留内置成员并追加数据包成员
    auto& builtin = BlockTags::registerTag(ResourceLocation("minecraft", "test_append_builtin"));
    builtin.add(ResourceLocation("minecraft", "bedrock"));

    auto pack = std::make_unique<mc::InMemoryResourcePack>("append_block_pack");
    pack->addServerDataResource("minecraft/tags/block/test_append_builtin.json", R"({"values": ["minecraft:stone"]})");

    auto result = BlockTagLoader::loadFromResourcePack(*pack);
    ASSERT_TRUE(result.success());

    auto* tag = BlockTags::getTag(ResourceLocation("minecraft", "test_append_builtin"));
    ASSERT_NE(tag, nullptr);
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft", "stone")));
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft", "bedrock")));
}

TEST_F(BlockTagLoaderTest, RequiredFalseMissingReferenceIsSilent)
{
    // required=false 的缺失引用应静默跳过，不影响其它成员
    auto pack = std::make_unique<mc::InMemoryResourcePack>("required_false_pack");
    pack->addServerDataResource("minecraft/tags/block/test_required_false.json",
        R"({
            "values": [
                "minecraft:stone",
                {"id": "#minecraft:no_such_tag", "required": false}
            ]
        })");

    auto result = BlockTagLoader::loadFromResourcePack(*pack);
    ASSERT_TRUE(result.success());

    auto* tag = BlockTags::getTag(ResourceLocation("minecraft", "test_required_false"));
    ASSERT_NE(tag, nullptr);
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft", "stone")));
    EXPECT_EQ(tag->getBlockIds().size(), 1u);
}

TEST_F(BlockTagLoaderTest, InvalidJsonIsReportedButDoesNotCrash)
{
    auto pack = std::make_unique<mc::InMemoryResourcePack>("invalid_json_pack");
    pack->addServerDataResource("minecraft/tags/block/test_invalid.json", "not valid json");

    auto result = BlockTagLoader::loadFromResourcePack(*pack);
    ASSERT_TRUE(result.success());
    // 无效文件不计入成功加载数量，且不产生标签
    EXPECT_EQ(BlockTags::getTag(ResourceLocation("minecraft", "test_invalid")), nullptr);
}

TEST_F(BlockTagLoaderTest, DataPackAppendsToBuiltinTag)
{
    // 数据包可对已注册的（内置/自定义）标签追加成员，且保留原有成员
    auto& builtin = BlockTags::registerTag(ResourceLocation("minecraft", "test_builtin_append"));
    builtin.add(ResourceLocation("minecraft", "stone"));

    auto pack = std::make_unique<mc::InMemoryResourcePack>("append_builtin_pack");
    pack->addServerDataResource(
        "minecraft/tags/block/test_builtin_append.json", R"({"values": ["minecraft:bedrock"]})");

    auto result = BlockTagLoader::loadFromResourcePack(*pack);
    ASSERT_TRUE(result.success());

    auto* after = BlockTags::getTag(ResourceLocation("minecraft", "test_builtin_append"));
    ASSERT_NE(after, nullptr);
    EXPECT_TRUE(after->contains(ResourceLocation("minecraft", "bedrock")));
    EXPECT_TRUE(after->contains(ResourceLocation("minecraft", "stone")));
}
