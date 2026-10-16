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
    nlohmann::json json = {{"id", "minecraft:stone"}, {"count", 32}};

    auto result = RecipeSerializers::parseResult(json);

    ASSERT_TRUE(result.success());
    EXPECT_FALSE(result.value().isEmpty());
    EXPECT_EQ(result.value().getItem(), m_stoneItem);
    EXPECT_EQ(result.value().getCount(), 32);
}

TEST_F(RecipeSerializersTest, ParseResult_ObjectForm_ZeroCount_ClampedToOne)
{
    // 对象形式：数量为0，应被修正为1
    nlohmann::json json = {{"id", "minecraft:stone"}, {"count", 0}};

    auto result = RecipeSerializers::parseResult(json);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().getCount(), 1);
}

TEST_F(RecipeSerializersTest, ParseResult_ObjectForm_NegativeCount_ClampedToOne)
{
    // 对象形式：负数数量，应被修正为1
    nlohmann::json json = {{"id", "minecraft:stone"}, {"count", -5}};

    auto result = RecipeSerializers::parseResult(json);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().getCount(), 1);
}

TEST_F(RecipeSerializersTest, ParseResult_ObjectForm_NoCount_DefaultsToOne)
{
    // 对象形式：无数量，默认为1
    nlohmann::json json = {{"id", "minecraft:stone"}};

    auto result = RecipeSerializers::parseResult(json);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().getCount(), 1);
}

TEST_F(RecipeSerializersTest, ParseResult_MissingIdField_ReturnsError)
{
    // 对象形式：缺少 id 字段
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

// ========== parseIngredient 测试 ==========

TEST_F(RecipeSerializersTest, ParseIngredient_SingleItem_ReturnsIngredient)
{
    nlohmann::json json = "minecraft:stone";

    auto result = RecipeSerializers::parseIngredient(json);

    ASSERT_TRUE(result.success());
    EXPECT_FALSE(result.value().isEmpty());
}

TEST_F(RecipeSerializersTest, ParseIngredient_ItemArray_ReturnsMergedIngredient)
{
    nlohmann::json json = nlohmann::json::array({"minecraft:stone", "minecraft:oak_planks"});

    auto result = RecipeSerializers::parseIngredient(json);

    ASSERT_TRUE(result.success());
    EXPECT_FALSE(result.value().isEmpty());
}

TEST_F(RecipeSerializersTest, ParseIngredient_UnknownItem_FailsRecipeLoad)
{
    // 未知物品必须让配方加载失败，不能降级成空原料：空原料的匹配语义是「该槽位必须为空」，
    // 配料全部未注册的配方因此会匹配完全空的合成网格并产出结果（凭空造物）。
    nlohmann::json json = "minecraft:unknown_item";

    auto result = RecipeSerializers::parseIngredient(json);

    ASSERT_TRUE(result.failed());
    EXPECT_EQ(result.error().code(), ErrorCode::ResourceParseError);
}

TEST_F(RecipeSerializersTest, ParseIngredient_NonStringNonArray_ReturnsError)
{
    nlohmann::json json = {{"count", 1}};

    auto result = RecipeSerializers::parseIngredient(json);

    EXPECT_TRUE(result.failed());
    EXPECT_EQ(result.error().code(), ErrorCode::ResourceParseError);
}
