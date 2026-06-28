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

// ============================================================================
// 两阶段加载：跨标签引用解析
// ============================================================================

TEST_F(ItemTagLoaderTest, TwoPhaseCrossTagReferenceResolution)
{
    // 模拟两阶段加载：先注册空标签占位符，再解析引用
    // 这确保了当标签 A 引用标签 B 时，即使 B 尚未填充内容，
    // 只要 B 已注册到 ItemTags 中，引用就能正确解析。

    // 第一阶段：注册一个空标签占位符（模拟尚未加载的标签）
    auto& smallFlowersTag = item::tag::ItemTags::registerTag(ResourceLocation("minecraft", "small_flowers_test"));

    // 给这个标签添加内容（模拟另一个数据包已经加载了它）
    Item* dandelion = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dandelion"));
    Item* poppy = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "poppy"));
    ASSERT_NE(dandelion, nullptr);
    ASSERT_NE(poppy, nullptr);
    smallFlowersTag.add(dandelion);
    smallFlowersTag.add(poppy);

    // 第二阶段：解析引用 #minecraft:small_flowers_test 的标签
    // 由于 small_flowers_test 已注册到 ItemTags，引用应能解析成功
    const std::string json = R"({
        "values": [
            "#minecraft:small_flowers_test",
            "minecraft:iron_ingot"
        ]
    })";

    auto result = item::tag::ItemTagLoader::loadFromJson(json, ResourceLocation("minecraft", "all_flowers_test"));
    ASSERT_TRUE(result.success());

    auto tag = result.value();
    Item* ironIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "iron_ingot"));
    ASSERT_NE(ironIngot, nullptr);

    // 应包含来自 #minecraft:small_flowers_test 引用的物品
    EXPECT_TRUE(tag->contains(dandelion));
    EXPECT_TRUE(tag->contains(poppy));
    // 应包含直接指定的物品
    EXPECT_TRUE(tag->contains(ironIngot));
    EXPECT_EQ(tag->getItems().size(), 3u);
}

TEST_F(ItemTagLoaderTest, TwoPhaseForwardReferenceResolution)
{
    // 模拟两阶段加载场景：
    // 假设有标签 A 引用标签 B，但 B 尚未注册。
    // 在两阶段加载中，第一阶段会先注册空标签 B，第二阶段再解析引用。

    // 第一阶段：注册空标签占位符
    auto& tagB = item::tag::ItemTags::registerTag(ResourceLocation("minecraft", "forward_ref_target_test"));

    // 第二阶段：先解析标签 A（引用 B）
    const std::string jsonA = R"({
        "values": [
            "#minecraft:forward_ref_target_test"
        ]
    })";
    auto resultA =
        item::tag::ItemTagLoader::loadFromJson(jsonA, ResourceLocation("minecraft", "forward_ref_source_test"));
    ASSERT_TRUE(resultA.success());

    // 此时 B 还是空标签，所以 A 的引用解析后应该没有物品
    auto tagA = resultA.value();
    EXPECT_EQ(tagA->getItems().size(), 0u);

    // 现在给 B 添加内容（模拟 B 的数据包在之后被加载）
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);
    tagB.add(diamond);

    // 再次解析标签 A，这次应该能解析到 B 的内容
    auto resultA2 =
        item::tag::ItemTagLoader::loadFromJson(jsonA, ResourceLocation("minecraft", "forward_ref_source_test_2"));
    ASSERT_TRUE(resultA2.success());
    auto tagA2 = resultA2.value();
    EXPECT_TRUE(tagA2->contains(diamond));
    EXPECT_EQ(tagA2->getItems().size(), 1u);
}

TEST_F(ItemTagLoaderTest, TwoPhaseChainedTagReference)
{
    // 测试链式标签引用：A 引用 B，B 引用 C
    // 在两阶段加载中，所有标签都先注册为空占位符，
    // 然后按依赖顺序填充内容，引用链应能正确解析。

    // 注册链式标签
    auto& tagC = item::tag::ItemTags::registerTag(ResourceLocation("minecraft", "chain_c_test"));
    auto& tagB = item::tag::ItemTags::registerTag(ResourceLocation("minecraft", "chain_b_test"));

    // 填充 C 的内容
    Item* emerald = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "emerald"));
    ASSERT_NE(emerald, nullptr);
    tagC.add(emerald);

    // 填充 B 的内容（B 引用 C + 直接物品）
    const std::string jsonB = R"({
        "values": [
            "#minecraft:chain_c_test",
            "minecraft:diamond"
        ]
    })";
    auto resultB = item::tag::ItemTagLoader::loadFromJson(jsonB, ResourceLocation("minecraft", "chain_b_fill_test"));
    ASSERT_TRUE(resultB.success());
    auto parsedB = resultB.value();
    for (const auto* item : parsedB->getItems()) {
        tagB.add(item);
    }

    // 验证 B 包含 C 的内容 + 直接物品
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);
    EXPECT_TRUE(tagB.contains(emerald));
    EXPECT_TRUE(tagB.contains(diamond));

    // 解析 A（引用 B）
    const std::string jsonA = R"({
        "values": [
            "#minecraft:chain_b_test"
        ]
    })";
    auto resultA = item::tag::ItemTagLoader::loadFromJson(jsonA, ResourceLocation("minecraft", "chain_a_test"));
    ASSERT_TRUE(resultA.success());
    auto tagA = resultA.value();

    // A 应包含 B 的所有内容（即 emerald + diamond）
    EXPECT_TRUE(tagA->contains(emerald));
    EXPECT_TRUE(tagA->contains(diamond));
    EXPECT_EQ(tagA->getItems().size(), 2u);
}
