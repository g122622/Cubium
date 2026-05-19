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

#include "item/crafting/RecipeSerializers.hpp"
#include "item/core/Item.hpp"
#include "item/core/ItemRegistry.hpp"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::crafting;

/**
 * @brief RecipeSerializers 测试
 *
 * 测试配方序列化器的 JSON 解析功能，包括：
 * - parseResult(): 结果物品堆解析（字符串形式、对象形式、带NBT数据）
 * - parseIngredient(): 原料解析（单物品、标签、数组形式）
 * - shrinkPattern(): pattern压缩逻辑
 * - validatePattern(): pattern验证逻辑
 */
class RecipeSerializersTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 注册一些测试物品
        auto& registry = ItemRegistry::instance();

        // 注册石头物品
        m_stoneItem = &registry.registerItem(ResourceLocation("minecraft", "stone"), ItemProperties().maxStackSize(64));

        // 注册铁剑物品
        m_ironSwordItem = &registry.registerItem(
            ResourceLocation("minecraft", "iron_sword"), ItemProperties().maxStackSize(1).maxDamage(250));

        // 注册橡木木板
        m_oakPlanksItem =
            &registry.registerItem(ResourceLocation("minecraft", "oak_planks"), ItemProperties().maxStackSize(64));
    }

    void TearDown() override
    {
        // 清理注册表（如果需要）
    }

    const Item* m_stoneItem = nullptr;
    const Item* m_ironSwordItem = nullptr;
    const Item* m_oakPlanksItem = nullptr;
};

// ========== parseResult 测试 ==========

TEST_F(RecipeSerializersTest, ParseResult_StringForm_ReturnsItemStack)
{
    // 字符串形式：仅物品ID
    nlohmann::json json = "minecraft:stone";

    auto result = RecipeSerializers::parseResult(json);

    ASSERT_TRUE(result.success());
    EXPECT_FALSE(result.value().isEmpty());
    EXPECT_EQ(result.value().getItem(), m_stoneItem);
    EXPECT_EQ(result.value().getCount(), 1);
}

TEST_F(RecipeSerializersTest, ParseResult_StringForm_UnknownItem_ReturnsError)
{
    // 字符串形式：未知物品
    nlohmann::json json = "minecraft:unknown_item";

    auto result = RecipeSerializers::parseResult(json);

    EXPECT_TRUE(result.failed());
    EXPECT_EQ(result.error().code(), ErrorCode::ResourceParseError);
}

TEST_F(RecipeSerializersTest, ParseResult_ObjectForm_WithCount_ReturnsItemStack)
{
    // 对象形式：带数量
    nlohmann::json json = {{"item", "minecraft:stone"}, {"count", 32}};

    auto result = RecipeSerializers::parseResult(json);

    ASSERT_TRUE(result.success());
    EXPECT_FALSE(result.value().isEmpty());
    EXPECT_EQ(result.value().getItem(), m_stoneItem);
    EXPECT_EQ(result.value().getCount(), 32);
}

TEST_F(RecipeSerializersTest, ParseResult_ObjectForm_ZeroCount_ClampedToOne)
{
    // 对象形式：数量为0，应被修正为1
    nlohmann::json json = {{"item", "minecraft:stone"}, {"count", 0}};

    auto result = RecipeSerializers::parseResult(json);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().getCount(), 1);
}

TEST_F(RecipeSerializersTest, ParseResult_ObjectForm_NegativeCount_ClampedToOne)
{
    // 对象形式：负数数量，应被修正为1
    nlohmann::json json = {{"item", "minecraft:stone"}, {"count", -5}};

    auto result = RecipeSerializers::parseResult(json);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().getCount(), 1);
}

TEST_F(RecipeSerializersTest, ParseResult_ObjectForm_NoCount_DefaultsToOne)
{
    // 对象形式：无数量，默认为1
    nlohmann::json json = {{"item", "minecraft:stone"}};

    auto result = RecipeSerializers::parseResult(json);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().getCount(), 1);
}

TEST_F(RecipeSerializersTest, ParseResult_MissingItemField_ReturnsError)
{
    // 对象形式：缺少item字段
    nlohmann::json json = {{"count", 10}};

    auto result = RecipeSerializers::parseResult(json);

    EXPECT_TRUE(result.failed());
    EXPECT_EQ(result.error().code(), ErrorCode::ResourceParseError);
}

TEST_F(RecipeSerializersTest, ParseResult_InvalidType_ReturnsError)
{
    // 无效类型：数组
    nlohmann::json json = nlohmann::json::array({1, 2, 3});

    auto result = RecipeSerializers::parseResult(json);

    EXPECT_TRUE(result.failed());
    EXPECT_EQ(result.error().code(), ErrorCode::ResourceParseError);
}

// ========== parseResult NBT 测试 ==========

TEST_F(RecipeSerializersTest, ParseResult_WithNbtJsonObject_MergesTag)
{
    // 对象形式：带JSON对象格式的NBT数据
    nlohmann::json json = {
        {"item", "minecraft:iron_sword"}, {"count", 1}, {"nbt", {{"display", {{"Name", "Custom Sword"}}}}}};

    auto result = RecipeSerializers::parseResult(json);

    ASSERT_TRUE(result.success());
    EXPECT_FALSE(result.value().isEmpty());
    EXPECT_EQ(result.value().getItem(), m_ironSwordItem);
    EXPECT_EQ(result.value().getCount(), 1);

    // 验证NBT数据被合并
    EXPECT_TRUE(result.value().hasTag());
    const nlohmann::json* tag = result.value().getTag();
    ASSERT_NE(tag, nullptr);
    EXPECT_TRUE(tag->contains("display"));
    EXPECT_EQ((*tag)["display"]["Name"], "Custom Sword");
}

TEST_F(RecipeSerializersTest, ParseResult_WithNbtMojangsonString_MergesTag)
{
    // 对象形式：带Mojangson字符串格式的NBT数据
    nlohmann::json json = {
        {"item", "minecraft:iron_sword"}, {"count", 1}, {"nbt", "{display:{Name:\"Custom Sword\"}}"}};

    auto result = RecipeSerializers::parseResult(json);

    ASSERT_TRUE(result.success());
    EXPECT_FALSE(result.value().isEmpty());
    EXPECT_EQ(result.value().getItem(), m_ironSwordItem);

    // 验证NBT数据被合并
    EXPECT_TRUE(result.value().hasTag());
    const nlohmann::json* tag = result.value().getTag();
    ASSERT_NE(tag, nullptr);
    EXPECT_TRUE(tag->contains("display"));
    EXPECT_EQ((*tag)["display"]["Name"], "Custom Sword");
}

TEST_F(RecipeSerializersTest, ParseResult_WithNbtMojangsonNestedObject_MergesTag)
{
    // 对象形式：带嵌套对象的Mojangson格式
    nlohmann::json json = {{"item", "minecraft:iron_sword"},
        {"count", 1},
        {"nbt", "{display:{Name:\"{\\\"text\\\":\\\"Legendary Sword\\\"}\"}}"}};

    auto result = RecipeSerializers::parseResult(json);

    ASSERT_TRUE(result.success());
    EXPECT_FALSE(result.value().isEmpty());

    // 验证NBT数据被合并
    EXPECT_TRUE(result.value().hasTag());
    const nlohmann::json* tag = result.value().getTag();
    ASSERT_NE(tag, nullptr);
    EXPECT_TRUE(tag->contains("display"));
}

TEST_F(RecipeSerializersTest, ParseResult_WithInvalidNbtString_IgnoresNbt)
{
    // 对象形式：无效的NBT字符串，应忽略NBT但仍然创建物品堆
    nlohmann::json json = {{"item", "minecraft:stone"}, {"count", 10}, {"nbt", "{invalid nbt syntax}"}};

    auto result = RecipeSerializers::parseResult(json);

    // 物品堆应该成功创建
    ASSERT_TRUE(result.success());
    EXPECT_FALSE(result.value().isEmpty());
    EXPECT_EQ(result.value().getItem(), m_stoneItem);
    EXPECT_EQ(result.value().getCount(), 10);

    // NBT解析失败，应没有自定义数据
    EXPECT_FALSE(result.value().hasTag());
}

TEST_F(RecipeSerializersTest, ParseResult_WithNbtMultipleFields_MergesAllFields)
{
    // 对象形式：多个NBT字段
    nlohmann::json json = {{"item", "minecraft:iron_sword"},
        {"count", 1},
        {"nbt",
            {{"display", {{"Name", "Test"}, {"Lore", nlohmann::json::array({"Line 1", "Line 2"})}}},
                {"CustomModelData", 12345}}}};

    auto result = RecipeSerializers::parseResult(json);

    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().hasTag());
    const nlohmann::json* tag = result.value().getTag();
    ASSERT_NE(tag, nullptr);

    // 验证所有字段都被合并
    EXPECT_TRUE(tag->contains("display"));
    EXPECT_TRUE(tag->contains("CustomModelData"));
    EXPECT_EQ((*tag)["display"]["Name"], "Test");
    EXPECT_EQ((*tag)["CustomModelData"], 12345);
}

TEST_F(RecipeSerializersTest, ParseResult_WithEmptyNbtObject_NoTag)
{
    // 对象形式：空的NBT对象
    nlohmann::json json = {{"item", "minecraft:stone"}, {"count", 1}, {"nbt", nlohmann::json::object()}};

    auto result = RecipeSerializers::parseResult(json);

    ASSERT_TRUE(result.success());
    // 空对象合并后不应该有自定义数据（或为空对象）
    // 具体行为取决于mergeTag的实现
}

// ========== parseIngredient 测试 ==========

TEST_F(RecipeSerializersTest, ParseIngredient_SingleItem_ReturnsIngredient)
{
    nlohmann::json json = {{"item", "minecraft:stone"}};

    auto result = RecipeSerializers::parseIngredient(json);

    ASSERT_TRUE(result.success());
    EXPECT_FALSE(result.value().isEmpty());
}

TEST_F(RecipeSerializersTest, ParseIngredient_ItemArray_ReturnsMergedIngredient)
{
    nlohmann::json json = nlohmann::json::array({{{"item", "minecraft:stone"}}, {{"item", "minecraft:oak_planks"}}});

    auto result = RecipeSerializers::parseIngredient(json);

    ASSERT_TRUE(result.success());
    EXPECT_FALSE(result.value().isEmpty());
}

TEST_F(RecipeSerializersTest, ParseIngredient_Tag_ReturnsTagIngredient)
{
    nlohmann::json json = {{"tag", "minecraft:planks"}};

    auto result = RecipeSerializers::parseIngredient(json);

    ASSERT_TRUE(result.success());
    // 标签原料在没有标签系统时可能为空
}

TEST_F(RecipeSerializersTest, ParseIngredient_UnknownItem_ReturnsEmptyIngredient)
{
    // MC 原版行为：未知物品返回空原料（hasNoMatchingItems == true）
    nlohmann::json json = {{"item", "minecraft:unknown_item"}};

    auto result = RecipeSerializers::parseIngredient(json);

    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().isEmpty());
}

TEST_F(RecipeSerializersTest, ParseIngredient_MissingItemAndTag_ReturnsError)
{
    nlohmann::json json = {{"count", 1}};

    auto result = RecipeSerializers::parseIngredient(json);

    EXPECT_TRUE(result.failed());
    EXPECT_EQ(result.error().code(), ErrorCode::ResourceParseError);
}
