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

#include "common/item/core/ItemRegistry.hpp"
#include "common/item/tag/ItemTagLoader.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/InMemoryResourcePack.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "item/Items.hpp"
#include "item/items/block/BlockItemRegistry.hpp"

using namespace mc;

class ItemTagLoaderTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 初始化顺序：方块 -> 物品 -> 方块物品 -> 物品标签
        VanillaBlocks::initialize();
        Items::ensureInitialized();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
        // 标签表从空开始：运行时数据包是标签的唯一来源，用例只验证自己写入的标签
        // （`minecraft:flowers` 在内置快照里，注册它只是为了让 #tag 引用有目标）。
        item::tag::ItemTags::initialize();
    }

    static const Item* item(const std::string& id)
    {
        return ItemRegistry::instance().getItem(ResourceLocation("minecraft", id));
    }
};

// ============================================================================
// loadFromJson - 基本 JSON 解析
// ============================================================================

TEST_F(ItemTagLoaderTest, LoadFromJsonBasicDirectItems)
{
    const std::string json = R"({
        "values": [
            "minecraft:diamond",
            "minecraft:emerald",
            "minecraft:iron_ingot"
        ]
    })";

    auto result = item::tag::ItemTagLoader::loadFromJson(json, ResourceLocation("minecraft", "test_tag"));
    ASSERT_TRUE(result.success());

    const auto& tag = result.value();
    EXPECT_EQ(tag.getId(), ResourceLocation("minecraft", "test_tag"));
    EXPECT_FALSE(tag.isReplace());

    ASSERT_NE(item("diamond"), nullptr);
    ASSERT_NE(item("emerald"), nullptr);
    ASSERT_NE(item("iron_ingot"), nullptr);
    EXPECT_TRUE(tag.contains(item("diamond")));
    EXPECT_TRUE(tag.contains(item("emerald")));
    EXPECT_TRUE(tag.contains(item("iron_ingot")));
    EXPECT_EQ(tag.getItems().size(), 3u);
}

TEST_F(ItemTagLoaderTest, LoadFromJsonMissingValuesArray)
{
    const std::string json = R"({ "replace": false })";

    auto result = item::tag::ItemTagLoader::loadFromJson(json, ResourceLocation("minecraft", "bad_tag"));
    EXPECT_FALSE(result.success());
}

TEST_F(ItemTagLoaderTest, LoadFromJsonEmptyValuesArray)
{
    const std::string json = R"({ "values": [] })";

    auto result = item::tag::ItemTagLoader::loadFromJson(json, ResourceLocation("minecraft", "empty_tag"));
    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().getItems().empty());
}

TEST_F(ItemTagLoaderTest, LoadFromJsonInvalidJson)
{
    auto result = item::tag::ItemTagLoader::loadFromJson("not valid json", ResourceLocation("minecraft", "invalid"));
    EXPECT_FALSE(result.success());
}

// ============================================================================
// loadFromJson - replace 语义
// ============================================================================

TEST_F(ItemTagLoaderTest, LoadFromJsonReplaceTrue)
{
    const std::string json = R"({
        "replace": true,
        "values": ["minecraft:diamond"]
    })";

    auto result = item::tag::ItemTagLoader::loadFromJson(json, ResourceLocation("minecraft", "replace_tag"));
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().getItems().size(), 1u);
}

TEST_F(ItemTagLoaderTest, LoadFromJsonReplaceDefault)
{
    auto result = item::tag::ItemTagLoader::loadFromJson(
        R"({ "values": ["minecraft:diamond"] })", ResourceLocation("minecraft", "default_tag"));
    ASSERT_TRUE(result.success());
    EXPECT_FALSE(result.value().isReplace());
}

TEST_F(ItemTagLoaderTest, LoadFromJsonAppendsToBaseTag)
{
    // 数据包合并语义：replace 为假时追加到已有标签，为真时丢弃已有内容。
    item::tag::ItemTag base(ResourceLocation("minecraft", "merge_tag"), false);
    ASSERT_NE(item("diamond"), nullptr);
    base.add(item("diamond"));

    auto appended = item::tag::ItemTagLoader::loadFromJson(
        R"({ "values": ["minecraft:emerald"] })", ResourceLocation("minecraft", "merge_tag"), base);
    ASSERT_TRUE(appended.success());
    EXPECT_TRUE(appended.value().contains(item("diamond")));
    EXPECT_TRUE(appended.value().contains(item("emerald")));

    auto replaced = item::tag::ItemTagLoader::loadFromJson(
        R"({ "replace": true, "values": ["minecraft:emerald"] })", ResourceLocation("minecraft", "merge_tag"), base);
    ASSERT_TRUE(replaced.success());
    EXPECT_FALSE(replaced.value().contains(item("diamond")));
    EXPECT_TRUE(replaced.value().contains(item("emerald")));
}

// ============================================================================
// ItemTags::mergeTagVersions - 多数据包合并
// ============================================================================

TEST_F(ItemTagLoaderTest, MergeTagVersionsAppendsHighPriorityLast)
{
    // versions 按优先级从高到低给；合并结果应按「低 → 高」书写。
    const std::vector<std::string> versions = {
        R"({ "values": ["minecraft:emerald"] })",
        R"({ "values": ["minecraft:diamond"] })",
    };
    auto merged = item::tag::ItemTags::mergeTagVersions(versions, ResourceLocation("minecraft", "merge_versions"));
    ASSERT_TRUE(merged.success());
    ASSERT_EQ(merged.value()["values"].size(), 2u);
    EXPECT_EQ(merged.value()["values"][0].get<std::string>(), "minecraft:diamond");
    EXPECT_EQ(merged.value()["values"][1].get<std::string>(), "minecraft:emerald");
}

TEST_F(ItemTagLoaderTest, MergeTagVersionsReplaceDropsLowerPriority)
{
    item::tag::ItemTag base(ResourceLocation("minecraft", "merge_replace"), false);
    ASSERT_NE(item("iron_ingot"), nullptr);
    base.add(item("iron_ingot"));

    const std::vector<std::string> versions = {
        R"({ "replace": true, "values": ["minecraft:emerald"] })",
        R"({ "values": ["minecraft:diamond"] })",
    };
    auto merged = item::tag::ItemTags::mergeTagVersions(versions, ResourceLocation("minecraft", "merge_replace"));
    ASSERT_TRUE(merged.success());

    auto applied = item::tag::ItemTags::applyTagJson(merged.value(), base);
    ASSERT_TRUE(applied.success());
    EXPECT_FALSE(base.contains(item("iron_ingot")));
    EXPECT_FALSE(base.contains(item("diamond")));
    EXPECT_TRUE(base.contains(item("emerald")));
}

// ============================================================================
// 标签引用
// ============================================================================

TEST_F(ItemTagLoaderTest, LoadFromJsonTagReference)
{
    // 引用已注册的标签：引用目标的内容并入结果。
    auto result = item::tag::ItemTagLoader::loadFromJson(
        R"({ "values": ["#minecraft:flowers"] })", ResourceLocation("minecraft", "flowers_reference"));
    ASSERT_TRUE(result.success());
    ASSERT_NE(item("dandelion"), nullptr);
    EXPECT_TRUE(result.value().contains(item("dandelion")));
}

TEST_F(ItemTagLoaderTest, LoadFromJsonMissingTagReferenceFails)
{
    // 引用不存在的标签：与数据包重载同一口径，结构错误直接失败。
    auto result = item::tag::ItemTagLoader::loadFromJson(
        R"({ "values": ["#minecraft:nonexistent_tag"] })", ResourceLocation("minecraft", "missing_ref"));
    EXPECT_FALSE(result.success());
}

TEST_F(ItemTagLoaderTest, LinkAndExpandResolvesNestedReferences)
{
    // 嵌套引用在 linkAndExpand 里展开成传递闭包（引用自己的标签是合法的空展开）。
    auto& tag = item::tag::ItemTags::registerTag(ResourceLocation("minecraft", "nested_ref_test"));
    auto& other = item::tag::ItemTags::registerTag(ResourceLocation("minecraft", "nested_ref_target_test"));
    tag.clear();
    other.clear();
    ASSERT_NE(item("diamond"), nullptr);
    ASSERT_NE(item("emerald"), nullptr);
    tag.add(item("diamond"));
    tag.addNestedTag(ResourceLocation("minecraft", "flowers"));
    tag.addNestedTag(ResourceLocation("minecraft", "nested_ref_target_test"));
    tag.addNestedTag(ResourceLocation("minecraft", "nested_ref_test"));
    other.add(item("emerald"));

    auto linked = item::tag::ItemTags::linkAndExpand();
    ASSERT_TRUE(linked.success());
    EXPECT_TRUE(tag.contains(item("diamond")));
    EXPECT_TRUE(tag.contains(item("emerald"))); // 来自 nested_ref_target_test
    EXPECT_TRUE(tag.contains(item("dandelion"))); // 来自 flowers
    // 引用已并入成员集合，不保留待展开的引用
    EXPECT_TRUE(tag.getNestedTags().empty());
}

// ============================================================================
// 对象格式（required 语义）
// ============================================================================

TEST_F(ItemTagLoaderTest, LoadFromJsonObjectFormatRequiredTrue)
{
    auto result = item::tag::ItemTagLoader::loadFromJson(
        R"({ "values": [{"id": "minecraft:diamond", "required": true}] })",
        ResourceLocation("minecraft", "obj_required"));
    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().contains(item("diamond")));
}

TEST_F(ItemTagLoaderTest, LoadFromJsonObjectFormatRequiredFalse)
{
    auto result = item::tag::ItemTagLoader::loadFromJson(
        R"({ "values": [
            {"id": "minecraft:diamond", "required": false},
            {"id": "minecraft:not_yet_implemented_item", "required": false}
        ] })",
        ResourceLocation("minecraft", "obj_optional"));
    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().contains(item("diamond")));
    EXPECT_EQ(result.value().getItems().size(), 1u);
}

TEST_F(ItemTagLoaderTest, LoadFromJsonObjectFormatMissingId)
{
    auto result = item::tag::ItemTagLoader::loadFromJson(
        R"({ "values": [{"required": false}] })", ResourceLocation("minecraft", "missing_id"));
    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().getItems().empty());
}

TEST_F(ItemTagLoaderTest, LoadFromJsonObjectFormatEmptyId)
{
    auto result = item::tag::ItemTagLoader::loadFromJson(
        R"({ "values": [{"id": "", "required": false}] })", ResourceLocation("minecraft", "empty_id"));
    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().getItems().empty());
}

TEST_F(ItemTagLoaderTest, LoadFromJsonUnknownItemRequiredFails)
{
    // 必需的未知物品：原版在那里整次重载失败，这里返回错误（不做静默降级）。
    auto result = item::tag::ItemTagLoader::loadFromJson(
        R"({ "values": ["minecraft:diamond", "minecraft:not_yet_implemented_item"] })",
        ResourceLocation("minecraft", "unknown_item_tag"));
    EXPECT_FALSE(result.success());
}

TEST_F(ItemTagLoaderTest, LoadFromJsonMixedFormat)
{
    auto result = item::tag::ItemTagLoader::loadFromJson(
        R"({ "values": [
            "minecraft:diamond",
            "#minecraft:flowers",
            {"id": "minecraft:emerald", "required": true},
            {"id": "minecraft:future_item", "required": false}
        ] })",
        ResourceLocation("minecraft", "mixed_tag"));
    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().contains(item("diamond")));
    EXPECT_TRUE(result.value().contains(item("emerald")));
    EXPECT_TRUE(result.value().contains(item("dandelion")));
}

TEST_F(ItemTagLoaderTest, LoadFromJsonDuplicateItems)
{
    auto result = item::tag::ItemTagLoader::loadFromJson(
        R"({ "values": ["minecraft:diamond", "minecraft:diamond"] })", ResourceLocation("minecraft", "dup_tag"));
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().getItems().size(), 1u);
}

TEST_F(ItemTagLoaderTest, LoadFromJsonCustomNamespace)
{
    auto result = item::tag::ItemTagLoader::loadFromJson(
        R"({ "values": ["minecraft:diamond"] })", ResourceLocation("mymod", "custom_tag"));
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().getId(), ResourceLocation("mymod", "custom_tag"));
}

// ============================================================================
// ItemTag 基本操作
// ============================================================================

TEST_F(ItemTagLoaderTest, ItemTagClearAndReplace)
{
    item::tag::ItemTag tag(ResourceLocation("minecraft", "test"), false);
    tag.add(item("diamond"));
    EXPECT_EQ(tag.getItems().size(), 1u);

    tag.clear();
    EXPECT_TRUE(tag.getItems().empty());

    tag.setReplace(true);
    EXPECT_TRUE(tag.isReplace());
    tag.add(item("emerald"));
    EXPECT_EQ(tag.getItems().size(), 1u);
    EXPECT_TRUE(tag.contains(item("emerald")));
}

TEST_F(ItemTagLoaderTest, ItemTagAddAll)
{
    item::tag::ItemTag tag(ResourceLocation("minecraft", "test"), false);
    tag.addAll({item("diamond"), item("emerald"), item("iron_ingot")});
    EXPECT_EQ(tag.getItems().size(), 3u);
}

TEST_F(ItemTagLoaderTest, ItemTagAddNullSkipped)
{
    item::tag::ItemTag tag(ResourceLocation("minecraft", "test"), false);
    tag.add(nullptr);
    EXPECT_TRUE(tag.getItems().empty());

    tag.addAll({nullptr, item("diamond"), nullptr});
    EXPECT_EQ(tag.getItems().size(), 1u);
}

TEST_F(ItemTagLoaderTest, ItemTagClearDropsNestedReferences)
{
    item::tag::ItemTag tag(ResourceLocation("minecraft", "test"), false);
    tag.addNestedTag(ResourceLocation("minecraft", "flowers"));
    EXPECT_EQ(tag.getNestedTags().size(), 1u);
    tag.clear();
    EXPECT_TRUE(tag.getNestedTags().empty());
}

// ============================================================================
// 数据包加载：loadFromResourcePack
// ============================================================================

TEST_F(ItemTagLoaderTest, LoadFromResourcePackBasicTags)
{
    auto pack = std::make_unique<InMemoryResourcePack>("test_pack");
    pack->addServerDataResource(
        "minecraft/tags/item/test_small_flowers.json", R"({ "values": ["minecraft:dandelion", "minecraft:poppy"] })");
    pack->addServerDataResource(
        "minecraft/tags/item/test_tall_flowers.json", R"({ "values": ["minecraft:sunflower", "minecraft:lilac"] })");
    // 引用方与目标同批加载：文件顺序不影响展开。
    pack->addServerDataResource("minecraft/tags/item/test_all_flowers.json",
        R"({ "values": ["#minecraft:test_small_flowers", "#minecraft:test_tall_flowers"] })");

    auto result = item::tag::ItemTagLoader::loadFromResourcePack(*pack);
    ASSERT_TRUE(result.success());
    EXPECT_GE(result.value(), 3u);

    const auto* small = item::tag::ItemTags::getTag(ResourceLocation("minecraft", "test_small_flowers"));
    ASSERT_NE(small, nullptr);
    EXPECT_TRUE(small->contains(item("dandelion")));
    EXPECT_TRUE(small->contains(item("poppy")));

    const auto* all = item::tag::ItemTags::getTag(ResourceLocation("minecraft", "test_all_flowers"));
    ASSERT_NE(all, nullptr);
    EXPECT_TRUE(all->contains(item("dandelion")));
    EXPECT_TRUE(all->contains(item("poppy")));
    EXPECT_TRUE(all->contains(item("sunflower")));
    EXPECT_TRUE(all->contains(item("lilac")));
    EXPECT_EQ(all->getItems().size(), 4u);
}

TEST_F(ItemTagLoaderTest, LoadFromResourcePackReplaceSemantics)
{
    auto& builtinTag = item::tag::ItemTags::registerTag(ResourceLocation("minecraft", "test_replace_pack_tag"));
    builtinTag.clear();
    builtinTag.add(item("diamond"));

    auto pack = std::make_unique<InMemoryResourcePack>("replace_pack");
    pack->addServerDataResource(
        "minecraft/tags/item/test_replace_pack_tag.json", R"({ "replace": true, "values": ["minecraft:emerald"] })");

    auto result = item::tag::ItemTagLoader::loadFromResourcePack(*pack);
    ASSERT_TRUE(result.success());

    const auto* tag = item::tag::ItemTags::getTag(ResourceLocation("minecraft", "test_replace_pack_tag"));
    ASSERT_NE(tag, nullptr);
    EXPECT_TRUE(tag->contains(item("emerald")));
    EXPECT_FALSE(tag->contains(item("diamond")));
}

TEST_F(ItemTagLoaderTest, LoadFromResourcePackAppendSemantics)
{
    auto& builtinTag = item::tag::ItemTags::registerTag(ResourceLocation("minecraft", "test_append_pack_tag"));
    builtinTag.clear();
    builtinTag.add(item("diamond"));

    auto pack = std::make_unique<InMemoryResourcePack>("append_pack");
    pack->addServerDataResource(
        "minecraft/tags/item/test_append_pack_tag.json", R"({ "values": ["minecraft:emerald"] })");

    auto result = item::tag::ItemTagLoader::loadFromResourcePack(*pack);
    ASSERT_TRUE(result.success());

    const auto* tag = item::tag::ItemTags::getTag(ResourceLocation("minecraft", "test_append_pack_tag"));
    ASSERT_NE(tag, nullptr);
    EXPECT_TRUE(tag->contains(item("diamond")));
    EXPECT_TRUE(tag->contains(item("emerald")));
}

TEST_F(ItemTagLoaderTest, LoadFromResourcePackUnknownRequiredItemFails)
{
    // 原版 TagLoader.loadTagsForExistingRegistries：标签里有未注册的必需物品时整次重载失败。
    auto pack = std::make_unique<InMemoryResourcePack>("unknown_item_pack");
    pack->addServerDataResource(
        "minecraft/tags/item/test_unknown_required_tag.json", R"({ "values": ["minecraft:not_yet_implemented_item"] })");

    auto result = item::tag::ItemTagLoader::loadFromResourcePack(*pack);
    EXPECT_FALSE(result.success());
    // 带洞的标签整条不写入：表里只剩一个空占位（与原版「该标签的成员全部未知」一致）。
    const auto* tag = item::tag::ItemTags::getTag(ResourceLocation("minecraft", "test_unknown_required_tag"));
    ASSERT_NE(tag, nullptr);
    EXPECT_TRUE(tag->getItems().empty());
}

TEST_F(ItemTagLoaderTest, LoadFromResourcePackUnknownTagReferenceFails)
{
    auto pack = std::make_unique<InMemoryResourcePack>("unknown_tag_pack");
    pack->addServerDataResource(
        "minecraft/tags/item/test_dangling_ref.json", R"({ "values": ["#minecraft:does_not_exist"] })");

    auto result = item::tag::ItemTagLoader::loadFromResourcePack(*pack);
    EXPECT_FALSE(result.success());

    // 悬空引用留在表里会让之后的每次载入都失败：清掉这个夹具标签，不污染后续用例。
    auto& tag = item::tag::ItemTags::registerTag(ResourceLocation("minecraft", "test_dangling_ref"));
    tag.clear();
}

TEST_F(ItemTagLoaderTest, LoadFromResourcePackRequiredFalse)
{
    auto pack = std::make_unique<InMemoryResourcePack>("required_pack");
    pack->addServerDataResource("minecraft/tags/item/test_required_tag.json",
        R"({ "values": [
            "minecraft:diamond",
            {"id": "minecraft:future_item", "required": false}
        ] })");

    auto result = item::tag::ItemTagLoader::loadFromResourcePack(*pack);
    ASSERT_TRUE(result.success());

    const auto* tag = item::tag::ItemTags::getTag(ResourceLocation("minecraft", "test_required_tag"));
    ASSERT_NE(tag, nullptr);
    EXPECT_TRUE(tag->contains(item("diamond")));
    EXPECT_EQ(tag->getItems().size(), 1u);
}