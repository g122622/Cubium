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

#include "common/item/tag/ItemTagLoader.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "item/Items.hpp"
#include "item/core/ItemStack.hpp"
#include "item/items/block/BlockItemRegistry.hpp"

using namespace mc;

class ItemTagLoaderTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 初始化顺序：方块 -> 物品 -> 方块物品 -> 物品标签
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
        item::tag::ItemTags::initialize();
    }
};

// ============================================================================
// loadFromJson - 基本 JSON 解析
// ============================================================================

TEST_F(ItemTagLoaderTest, LoadFromJsonBasicDirectItems)
{
    // 基本标签：直接物品列表
    const std::string json = R"({
        "values": [
            "minecraft:diamond",
            "minecraft:emerald",
            "minecraft:iron_ingot"
        ]
    })";

    auto result = item::tag::ItemTagLoader::loadFromJson(json, ResourceLocation("minecraft", "test_tag"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    EXPECT_EQ(tag->getId(), ResourceLocation("minecraft", "test_tag"));
    EXPECT_FALSE(tag->isReplace());

    // 检查物品是否存在
    const auto& items = tag->getItems();
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    Item* emerald = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "emerald"));
    Item* ironIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "iron_ingot"));
    ASSERT_NE(diamond, nullptr);
    ASSERT_NE(emerald, nullptr);
    ASSERT_NE(ironIngot, nullptr);

    EXPECT_TRUE(tag->contains(diamond));
    EXPECT_TRUE(tag->contains(emerald));
    EXPECT_TRUE(tag->contains(ironIngot));
    EXPECT_EQ(items.size(), 3u);
}

TEST_F(ItemTagLoaderTest, LoadFromJsonMissingValuesArray)
{
    // 缺少 values 数组应返回错误
    const std::string json = R"({
        "replace": false
    })";

    auto result = item::tag::ItemTagLoader::loadFromJson(json, ResourceLocation("minecraft", "bad_tag"));
    EXPECT_FALSE(result.success());
}

TEST_F(ItemTagLoaderTest, LoadFromJsonEmptyValuesArray)
{
    // 空 values 数组应成功但标签无物品
    const std::string json = R"({
        "values": []
    })";

    auto result = item::tag::ItemTagLoader::loadFromJson(json, ResourceLocation("minecraft", "empty_tag"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    EXPECT_EQ(tag->getItems().size(), 0u);
}

TEST_F(ItemTagLoaderTest, LoadFromJsonInvalidJson)
{
    // 无效 JSON 应返回错误
    const std::string json = "not valid json";

    auto result = item::tag::ItemTagLoader::loadFromJson(json, ResourceLocation("minecraft", "invalid"));
    EXPECT_FALSE(result.success());
}

// ============================================================================
// loadFromJson - replace 语义
// ============================================================================

TEST_F(ItemTagLoaderTest, LoadFromJsonReplaceTrue)
{
    // replace=true 标签
    const std::string json = R"({
        "replace": true,
        "values": [
            "minecraft:diamond"
        ]
    })";

    auto result = item::tag::ItemTagLoader::loadFromJson(json, ResourceLocation("minecraft", "replace_tag"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    EXPECT_TRUE(tag->isReplace());
    EXPECT_EQ(tag->getItems().size(), 1u);
}

TEST_F(ItemTagLoaderTest, LoadFromJsonReplaceFalse)
{
    // replace=false 标签（默认行为）
    const std::string json = R"({
        "replace": false,
        "values": [
            "minecraft:diamond"
        ]
    })";

    auto result = item::tag::ItemTagLoader::loadFromJson(json, ResourceLocation("minecraft", "append_tag"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    EXPECT_FALSE(tag->isReplace());
}

TEST_F(ItemTagLoaderTest, LoadFromJsonReplaceDefault)
{
    // 默认 replace 为 false
    const std::string json = R"({
        "values": [
            "minecraft:diamond"
        ]
    })";

    auto result = item::tag::ItemTagLoader::loadFromJson(json, ResourceLocation("minecraft", "default_tag"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    EXPECT_FALSE(tag->isReplace());
}

// ============================================================================
// loadFromJson - 标签引用 (# 语法)
// ============================================================================

TEST_F(ItemTagLoaderTest, LoadFromJsonTagReference)
{
    // 标签引用: #minecraft:flowers 引用已有的 FLOWERS 标签
    const std::string json = R"({
        "values": [
            "#minecraft:flowers"
        ]
    })";

    auto result = item::tag::ItemTagLoader::loadFromJson(json, ResourceLocation("minecraft", "flowers_reference"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    // FLOWERS 标签已在 initialize() 中注册，应包含引用的物品
    Item* dandelion = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dandelion"));
    ASSERT_NE(dandelion, nullptr);
    EXPECT_TRUE(tag->contains(dandelion));
}

TEST_F(ItemTagLoaderTest, LoadFromJsonMissingTagReference)
{
    // 引用不存在的标签应输出警告但不应失败
    const std::string json = R"({
        "values": [
            "#minecraft:nonexistent_tag"
        ]
    })";

    auto result = item::tag::ItemTagLoader::loadFromJson(json, ResourceLocation("minecraft", "missing_ref"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    // 不存在的标签引用不会添加任何物品
    EXPECT_EQ(tag->getItems().size(), 0u);
}

// ============================================================================
// loadFromJson - 对象格式 (required 语义)
// ============================================================================

TEST_F(ItemTagLoaderTest, LoadFromJsonObjectFormatRequiredTrue)
{
    // 对象格式，required=true（默认）
    const std::string json = R"({
        "values": [
            {"id": "minecraft:diamond", "required": true}
        ]
    })";

    auto result = item::tag::ItemTagLoader::loadFromJson(json, ResourceLocation("minecraft", "obj_required"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);
    EXPECT_TRUE(tag->contains(diamond));
}

TEST_F(ItemTagLoaderTest, LoadFromJsonObjectFormatRequiredFalse)
{
    // 对象格式，required=false - 未知物品应静默跳过
    const std::string json = R"({
        "values": [
            {"id": "minecraft:diamond", "required": false},
            {"id": "minecraft:not_yet_implemented_item", "required": false}
        ]
    })";

    auto result = item::tag::ItemTagLoader::loadFromJson(json, ResourceLocation("minecraft", "obj_optional"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);
    EXPECT_TRUE(tag->contains(diamond));
    // 不存在的物品 required=false 应静默跳过，不报错
    EXPECT_EQ(tag->getItems().size(), 1u);
}

TEST_F(ItemTagLoaderTest, LoadFromJsonObjectFormatMissingId)
{
    // 对象格式缺少 id 字段应跳过该条目
    const std::string json = R"({
        "values": [
            {"required": false}
        ]
    })";

    auto result = item::tag::ItemTagLoader::loadFromJson(json, ResourceLocation("minecraft", "missing_id"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    EXPECT_EQ(tag->getItems().size(), 0u);
}

TEST_F(ItemTagLoaderTest, LoadFromJsonObjectFormatEmptyId)
{
    // 对象格式 id 为空字符串应跳过
    const std::string json = R"({
        "values": [
            {"id": "", "required": false}
        ]
    })";

    auto result = item::tag::ItemTagLoader::loadFromJson(json, ResourceLocation("minecraft", "empty_id"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    EXPECT_EQ(tag->getItems().size(), 0u);
}

// ============================================================================
// loadFromJson - 循环引用检测
// ============================================================================

TEST_F(ItemTagLoaderTest, LoadFromJsonCircularTagReference)
{
    // 标签引用自身（循环引用）应跳过
    const std::string json = R"({
        "values": [
            "#minecraft:circular_tag"
        ]
    })";

    auto result = item::tag::ItemTagLoader::loadFromJson(json, ResourceLocation("minecraft", "circular_tag"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    // 自引用不应添加任何物品（循环检测）
    EXPECT_EQ(tag->getItems().size(), 0u);
}

// ============================================================================
// loadFromJson - 未知物品处理
// ============================================================================

TEST_F(ItemTagLoaderTest, LoadFromJsonUnknownItemRequired)
{
    // required=true（默认字符串格式）的未知物品应输出警告
    const std::string json = R"({
        "values": [
            "minecraft:diamond",
            "minecraft:not_yet_implemented_item"
        ]
    })";

    auto result = item::tag::ItemTagLoader::loadFromJson(json, ResourceLocation("minecraft", "unknown_item_tag"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);
    EXPECT_TRUE(tag->contains(diamond));
    // 未知物品不会被添加，但仍能解析成功
    EXPECT_EQ(tag->getItems().size(), 1u);
}

// ============================================================================
// loadFromJson - 混合格式
// ============================================================================

TEST_F(ItemTagLoaderTest, LoadFromJsonMixedFormat)
{
    // 混合字符串格式和对象格式
    const std::string json = R"({
        "values": [
            "minecraft:diamond",
            "#minecraft:flowers",
            {"id": "minecraft:emerald", "required": true},
            {"id": "minecraft:future_item", "required": false}
        ]
    })";

    auto result = item::tag::ItemTagLoader::loadFromJson(json, ResourceLocation("minecraft", "mixed_tag"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    Item* emerald = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "emerald"));
    Item* dandelion = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dandelion"));

    ASSERT_NE(diamond, nullptr);
    ASSERT_NE(emerald, nullptr);
    ASSERT_NE(dandelion, nullptr);

    EXPECT_TRUE(tag->contains(diamond));
    EXPECT_TRUE(tag->contains(emerald));
    EXPECT_TRUE(tag->contains(dandelion)); // 来自 #minecraft:flowers 引用
}

// ============================================================================
// loadFromJson - ItemTag::clear 和 replace 语义集成
// ============================================================================

TEST_F(ItemTagLoaderTest, ItemTagClearAndReplace)
{
    // 测试 ItemTag 的 clear 和 replace 功能
    auto tag = std::make_unique<item::tag::ItemTag>(ResourceLocation("minecraft", "test"), false);

    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    Item* emerald = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "emerald"));
    ASSERT_NE(diamond, nullptr);
    ASSERT_NE(emerald, nullptr);

    tag->add(diamond);
    EXPECT_EQ(tag->getItems().size(), 1u);
    EXPECT_TRUE(tag->contains(diamond));

    // clear 后物品清空
    tag->clear();
    EXPECT_EQ(tag->getItems().size(), 0u);
    EXPECT_FALSE(tag->contains(diamond));

    // replace 语义：设置 replace=true 后清空再追加
    tag->setReplace(true);
    EXPECT_TRUE(tag->isReplace());

    tag->add(emerald);
    EXPECT_EQ(tag->getItems().size(), 1u);
    EXPECT_TRUE(tag->contains(emerald));
    EXPECT_FALSE(tag->contains(diamond));
}

TEST_F(ItemTagLoaderTest, ItemTagAddAll)
{
    // 测试 ItemTag 的 addAll 功能
    auto tag = std::make_unique<item::tag::ItemTag>(ResourceLocation("minecraft", "test"), false);

    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    Item* emerald = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "emerald"));
    Item* ironIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "iron_ingot"));
    ASSERT_NE(diamond, nullptr);
    ASSERT_NE(emerald, nullptr);
    ASSERT_NE(ironIngot, nullptr);

    std::vector<const Item*> items = {diamond, emerald, ironIngot};
    tag->addAll(items);

    EXPECT_EQ(tag->getItems().size(), 3u);
    EXPECT_TRUE(tag->contains(diamond));
    EXPECT_TRUE(tag->contains(emerald));
    EXPECT_TRUE(tag->contains(ironIngot));
}

TEST_F(ItemTagLoaderTest, ItemTagAddNullSkipped)
{
    // 测试添加 null 物品指针时跳过而不崩溃
    auto tag = std::make_unique<item::tag::ItemTag>(ResourceLocation("minecraft", "test"), false);

    tag->add(nullptr);
    EXPECT_EQ(tag->getItems().size(), 0u);

    // addAll 中的 null 也应跳过
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);
    std::vector<const Item*> items = {nullptr, diamond, nullptr};
    tag->addAll(items);
    EXPECT_EQ(tag->getItems().size(), 1u);
    EXPECT_TRUE(tag->contains(diamond));
}

// ============================================================================
// loadFromJson - 重复物品去重
// ============================================================================

TEST_F(ItemTagLoaderTest, LoadFromJsonDuplicateItems)
{
    // 重复物品应被去重（unordered_set 特性）
    const std::string json = R"({
        "values": [
            "minecraft:diamond",
            "minecraft:diamond",
            "minecraft:diamond"
        ]
    })";

    auto result = item::tag::ItemTagLoader::loadFromJson(json, ResourceLocation("minecraft", "dup_tag"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);
    EXPECT_TRUE(tag->contains(diamond));
    EXPECT_EQ(tag->getItems().size(), 1u); // unordered_set 自动去重
}

// ============================================================================
// loadFromJson - 命名空间处理
// ============================================================================

TEST_F(ItemTagLoaderTest, LoadFromJsonCustomNamespace)
{
    // 自定义命名空间的标签
    const std::string json = R"({
        "values": [
            "minecraft:diamond"
        ]
    })";

    auto result = item::tag::ItemTagLoader::loadFromJson(json, ResourceLocation("mymod", "custom_tag"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    EXPECT_EQ(tag->getId(), ResourceLocation("mymod", "custom_tag"));
}

// ============================================================================
// 集成测试：数据包标签追加到内置标签
// ============================================================================

TEST_F(ItemTagLoaderTest, DatapackAppendToBuiltinTag)
{
    // 模拟数据包追加物品到已有的 FLOWERS 标签
    const std::string json = R"({
        "values": [
            "minecraft:diamond"
        ]
    })";

    auto result = item::tag::ItemTagLoader::loadFromJson(json, ResourceLocation("minecraft", "flowers"));
    ASSERT_TRUE(result.success());

    // 获取已有的 FLOWERS 标签
    auto* flowersTag = item::tag::ItemTags::getTag(ResourceLocation("minecraft", "flowers"));
    ASSERT_NE(flowersTag, nullptr);

    // 模拟追加操作
    auto parsedTag = result.value();
    for (const auto* item : parsedTag->getItems()) {
        flowersTag->add(item);
    }

    // FLOWERS 标签应仍包含原有物品
    Item* dandelion = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dandelion"));
    ASSERT_NE(dandelion, nullptr);
    EXPECT_TRUE(flowersTag->contains(dandelion));

    // 同时应包含新追加的物品
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);
    EXPECT_TRUE(flowersTag->contains(diamond));
}
