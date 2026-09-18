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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/crafting/RecipeLoader.hpp"
#include "item/crafting/RecipeManager.hpp"
#include "item/crafting/RecipeSerializers.hpp"
#include "item/items/block/BlockItemRegistry.hpp"
#include "resource/ResourceLocation.hpp"
#include "world/block/registry/VanillaBlocks.hpp"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace mc;

class RecipeFormatTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
        crafting::RecipeManager::instance().clear();
    }

    void TearDown() override { crafting::RecipeManager::instance().clear(); }
};

// ============================================================================
// MC 1.21+ 字符串格式 ingredient 测试
// ============================================================================

TEST_F(RecipeFormatTest, ParseIngredient_StringFormat)
{
    // MC 1.21+ 格式：ingredient 为字符串而非对象
    nlohmann::json ingredientJson = "minecraft:raw_iron";
    auto result = crafting::RecipeSerializers::parseIngredient(ingredientJson);
    ASSERT_TRUE(result.success()) << "String ingredient should parse successfully";
    EXPECT_FALSE(result.value().hasNoMatchingItems());
}

TEST_F(RecipeFormatTest, ParseIngredient_ObjectFormat)
{
    // 旧格式：ingredient 为对象
    nlohmann::json ingredientJson = {{"item", "minecraft:raw_iron"}};
    auto result = crafting::RecipeSerializers::parseIngredient(ingredientJson);
    ASSERT_TRUE(result.success()) << "Object ingredient should parse successfully";
    EXPECT_FALSE(result.value().hasNoMatchingItems());
}

TEST_F(RecipeFormatTest, ParseIngredient_ArrayFormat)
{
    // 数组格式：多选项
    nlohmann::json ingredientJson = nlohmann::json::array({"minecraft:raw_iron", "minecraft:raw_gold"});
    auto result = crafting::RecipeSerializers::parseIngredient(ingredientJson);
    ASSERT_TRUE(result.success()) << "Array ingredient should parse successfully";
    EXPECT_FALSE(result.value().hasNoMatchingItems());
}

TEST_F(RecipeFormatTest, ParseIngredient_StringFormatUnregisteredItem)
{
    // 未注册物品的字符串 ingredient 应返回空 Ingredient
    nlohmann::json ingredientJson = "minecraft:nonexistent_item";
    auto result = crafting::RecipeSerializers::parseIngredient(ingredientJson);
    ASSERT_TRUE(result.success()) << "Unregistered string ingredient should not error";
    EXPECT_TRUE(result.value().hasNoMatchingItems());
}

TEST_F(RecipeFormatTest, ParseIngredient_TagFormat)
{
    // 标签格式
    nlohmann::json ingredientJson = {{"tag", "minecraft:planks"}};
    auto result = crafting::RecipeSerializers::parseIngredient(ingredientJson);
    ASSERT_TRUE(result.success()) << "Tag ingredient should parse successfully";
}

// ============================================================================
// MC 1.21+ "id" 字段 result 测试
// ============================================================================

TEST_F(RecipeFormatTest, ParseResult_IdField)
{
    // MC 1.21+ 格式：result 使用 "id" 而非 "item"
    nlohmann::json resultJson = {{"id", "minecraft:iron_ingot"}};
    auto result = crafting::RecipeSerializers::parseResult(resultJson);
    ASSERT_TRUE(result.success()) << "Result with 'id' field should parse successfully";
    EXPECT_EQ(result.value().getItem()->itemLocation(), ResourceLocation("minecraft", "iron_ingot"));
    EXPECT_EQ(result.value().getCount(), 1);
}

TEST_F(RecipeFormatTest, ParseResult_ItemField)
{
    // 旧格式：result 使用 "item"
    nlohmann::json resultJson = {{"item", "minecraft:iron_ingot"}};
    auto result = crafting::RecipeSerializers::parseResult(resultJson);
    ASSERT_TRUE(result.success()) << "Result with 'item' field should parse successfully";
    EXPECT_EQ(result.value().getItem()->itemLocation(), ResourceLocation("minecraft", "iron_ingot"));
}

TEST_F(RecipeFormatTest, ParseResult_IdFieldWithCount)
{
    // MC 1.21+ 格式带 count
    nlohmann::json resultJson = {{"id", "minecraft:raw_iron"}, {"count", 9}};
    auto result = crafting::RecipeSerializers::parseResult(resultJson);
    ASSERT_TRUE(result.success()) << "Result with 'id' and 'count' should parse successfully";
    EXPECT_EQ(result.value().getCount(), 9);
}

TEST_F(RecipeFormatTest, ParseResult_IdFieldPreferredOverItem)
{
    // 同时有 id 和 item 时，item 优先（向后兼容）
    nlohmann::json resultJson = {{"item", "minecraft:iron_ingot"}, {"id", "minecraft:gold_ingot"}};
    auto result = crafting::RecipeSerializers::parseResult(resultJson);
    ASSERT_TRUE(result.success()) << "Result with both 'item' and 'id' should parse successfully";
    EXPECT_EQ(result.value().getItem()->itemLocation(), ResourceLocation("minecraft", "iron_ingot"))
        << "item field should take priority over id field";
}

TEST_F(RecipeFormatTest, ParseResult_MissingBothFields)
{
    // 既没有 item 也没有 id
    nlohmann::json resultJson = {{"count", 1}};
    auto result = crafting::RecipeSerializers::parseResult(resultJson);
    EXPECT_FALSE(result.success()) << "Result without 'item' or 'id' should fail";
}

// ============================================================================
// MC 1.21+ 完整配方格式测试 - Smelting
// ============================================================================

TEST_F(RecipeFormatTest, SmeltingRecipe_MC121Format)
{
    // MC 1.21+ 格式：ingredient 为字符串，result 使用 id
    nlohmann::json recipeJson = {{"type", "minecraft:smelting"},
        {"category", "misc"},
        {"cookingtime", 200},
        {"experience", 0.7},
        {"group", "iron_ingot"},
        {"ingredient", "minecraft:raw_iron"},
        {"result", {{"id", "minecraft:iron_ingot"}}}};

    auto result = crafting::RecipeSerializers::fromSmeltingJson(
        ResourceLocation("minecraft", "iron_ingot_from_smelting_raw_iron"), recipeJson);
    ASSERT_TRUE(result.success()) << "MC 1.21+ format smelting recipe should parse: " << result.error().message();
    auto recipe = result.value();
    ASSERT_NE(recipe, nullptr);
    EXPECT_EQ(recipe->getId(), ResourceLocation("minecraft", "iron_ingot_from_smelting_raw_iron"));
    EXPECT_EQ(recipe->getCookTime(), 200);
}

TEST_F(RecipeFormatTest, BlastingRecipe_MC121Format)
{
    nlohmann::json recipeJson = {{"type", "minecraft:blasting"},
        {"category", "misc"},
        {"cookingtime", 100},
        {"experience", 0.7},
        {"group", "iron_ingot"},
        {"ingredient", "minecraft:raw_iron"},
        {"result", {{"id", "minecraft:iron_ingot"}}}};

    auto result = crafting::RecipeSerializers::fromSmeltingJson(
        ResourceLocation("minecraft", "iron_ingot_from_blasting_raw_iron"), recipeJson);
    ASSERT_TRUE(result.success()) << "MC 1.21+ format blasting recipe should parse: " << result.error().message();
    auto recipe = result.value();
    EXPECT_EQ(recipe->getCookTime(), 100);
}

TEST_F(RecipeFormatTest, SmeltingRecipe_ObjectIngredientFormat)
{
    // 旧格式：ingredient 为对象
    nlohmann::json recipeJson = {{"type", "minecraft:smelting"},
        {"category", "misc"},
        {"cookingtime", 200},
        {"experience", 1.0},
        {"ingredient", {{"item", "minecraft:raw_gold"}}},
        {"result", {{"item", "minecraft:gold_ingot"}}}};

    auto result = crafting::RecipeSerializers::fromSmeltingJson(
        ResourceLocation("minecraft", "gold_ingot_from_smelting_raw_gold"), recipeJson);
    ASSERT_TRUE(result.success()) << "Object format ingredient should parse: " << result.error().message();
}

// ============================================================================
// MC 1.21+ Shaped Recipe 格式测试
// ============================================================================

TEST_F(RecipeFormatTest, ShapedRecipe_MC121Format_StringKeyIngredient)
{
    // MC 1.21+ 格式：key 中的 ingredient 为字符串
    nlohmann::json recipeJson = {{"type", "minecraft:crafting_shaped"},
        {"category", "building"},
        {"key", {{"#", "minecraft:raw_iron"}}},
        {"pattern", {"###", "###", "###"}},
        {"result", {{"count", 1}, {"id", "minecraft:raw_iron_block"}}}};

    auto result = crafting::RecipeSerializers::fromJson(ResourceLocation("minecraft", "raw_iron_block"), recipeJson);
    ASSERT_TRUE(result.success()) << "MC 1.21+ shaped recipe with string key ingredient should parse: "
                                  << result.error().message();
}

// ============================================================================
// MC 1.21+ Shapeless Recipe 格式测试
// ============================================================================

TEST_F(RecipeFormatTest, ShapelessRecipe_MC121Format_StringIngredients)
{
    // MC 1.21+ 格式：ingredients 中的元素为字符串
    nlohmann::json recipeJson = {{"type", "minecraft:crafting_shapeless"},
        {"category", "misc"},
        {"ingredients", nlohmann::json::array({"minecraft:raw_iron_block"})},
        {"result", {{"count", 9}, {"id", "minecraft:raw_iron"}}}};

    auto result = crafting::RecipeSerializers::fromJson(ResourceLocation("minecraft", "raw_iron"), recipeJson);
    ASSERT_TRUE(result.success()) << "MC 1.21+ shapeless recipe with string ingredients should parse: "
                                  << result.error().message();
}
