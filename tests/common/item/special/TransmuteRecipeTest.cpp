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

#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/RecipeManager.hpp"
#include "common/item/crafting/RecipeSerializers.hpp"
#include "common/item/crafting/TransmuteRecipe.hpp"
#include "common/item/items/special/bundle/BundleContents.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::crafting;
using namespace mc::item::items;
using namespace mc::item::tag;

// ============================================================================
// 测试夹具
// ============================================================================

class TransmuteRecipeTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        Items::initialize();
        ItemTags::initialize();
    }

    void SetUp() override { RecipeManager::instance().clear(); }

    void TearDown() override { RecipeManager::instance().clear(); }
};

// ============================================================================
// 构造函数测试
// ============================================================================

TEST_F(TransmuteRecipeTest, Constructor_BasicProperties)
{
    Ingredient input = Ingredient::fromItem(*Items::BUNDLE);
    Ingredient material = Ingredient::fromItem(*Items::WHITE_DYE);
    ItemStack result(*Items::WHITE_BUNDLE, 1);

    TransmuteRecipe recipe(ResourceLocation("minecraft", "white_bundle"), input, material, result, "bundle_dye");

    EXPECT_EQ(recipe.getId(), ResourceLocation("minecraft", "white_bundle"));
    EXPECT_EQ(recipe.getType(), RecipeType::Transmute);
    EXPECT_EQ(recipe.getGroup(), "bundle_dye");
    EXPECT_TRUE(recipe.isSpecial());
    EXPECT_TRUE(recipe.isDynamic());
    EXPECT_EQ(recipe.getResultItem().getItem(), Items::WHITE_BUNDLE);
    EXPECT_EQ(recipe.getIngredients().size(), 2u);
}

// ============================================================================
// canFitIn 测试
// ============================================================================

TEST_F(TransmuteRecipeTest, CanFitIn_2x2OrLarger_ReturnsTrue)
{
    Ingredient input = Ingredient::fromItem(*Items::BUNDLE);
    Ingredient material = Ingredient::fromItem(*Items::WHITE_DYE);
    ItemStack result(*Items::WHITE_BUNDLE, 1);

    TransmuteRecipe recipe(ResourceLocation("test", "transmute"), input, material, result);

    // 2 个原料需要至少 2 个槽位
    EXPECT_FALSE(recipe.canFitIn(1, 1));
    EXPECT_TRUE(recipe.canFitIn(2, 1));
    EXPECT_TRUE(recipe.canFitIn(2, 2));
    EXPECT_TRUE(recipe.canFitIn(3, 3));
}

// ============================================================================
// 匹配测试
// ============================================================================

TEST_F(TransmuteRecipeTest, Matches_BundleAndDye_ReturnsTrue)
{
    Ingredient input = Ingredient::fromItem(*Items::BUNDLE);
    Ingredient material = Ingredient::fromItem(*Items::WHITE_DYE);
    ItemStack result(*Items::WHITE_BUNDLE, 1);

    TransmuteRecipe recipe(ResourceLocation("test", "transmute"), input, material, result);

    CraftingInventory inventory(3, 3);
    inventory.setItem(0, ItemStack(*Items::BUNDLE, 1));
    inventory.setItem(1, ItemStack(*Items::WHITE_DYE, 1));

    EXPECT_TRUE(recipe.matches(inventory));
}

TEST_F(TransmuteRecipeTest, Matches_PositionIndependent)
{
    Ingredient input = Ingredient::fromItem(*Items::BUNDLE);
    Ingredient material = Ingredient::fromItem(*Items::WHITE_DYE);
    ItemStack result(*Items::WHITE_BUNDLE, 1);

    TransmuteRecipe recipe(ResourceLocation("test", "transmute"), input, material, result);

    // 任意位置都应该匹配
    CraftingInventory inventory(3, 3);
    inventory.setItem(5, ItemStack(*Items::WHITE_DYE, 1));
    inventory.setItem(8, ItemStack(*Items::BUNDLE, 1));

    EXPECT_TRUE(recipe.matches(inventory));
}

TEST_F(TransmuteRecipeTest, Matches_MissingMaterial_ReturnsFalse)
{
    Ingredient input = Ingredient::fromItem(*Items::BUNDLE);
    Ingredient material = Ingredient::fromItem(*Items::WHITE_DYE);
    ItemStack result(*Items::WHITE_BUNDLE, 1);

    TransmuteRecipe recipe(ResourceLocation("test", "transmute"), input, material, result);

    // 只有 bundle，没有染料
    CraftingInventory inventory(3, 3);
    inventory.setItem(0, ItemStack(*Items::BUNDLE, 1));

    EXPECT_FALSE(recipe.matches(inventory));
}

TEST_F(TransmuteRecipeTest, Matches_TooManyItems_ReturnsFalse)
{
    Ingredient input = Ingredient::fromItem(*Items::BUNDLE);
    Ingredient material = Ingredient::fromItem(*Items::WHITE_DYE);
    ItemStack result(*Items::WHITE_BUNDLE, 1);

    TransmuteRecipe recipe(ResourceLocation("test", "transmute"), input, material, result);

    // 3 个物品不匹配（需要恰好 2 个）
    CraftingInventory inventory(3, 3);
    inventory.setItem(0, ItemStack(*Items::BUNDLE, 1));
    inventory.setItem(1, ItemStack(*Items::WHITE_DYE, 1));
    inventory.setItem(2, ItemStack(*Items::STONE, 1));

    EXPECT_FALSE(recipe.matches(inventory));
}

TEST_F(TransmuteRecipeTest, Matches_SameItemAsResult_ReturnsFalse)
{
    // 已是白色收纳袋，再用白色染料转化无意义
    Ingredient input = Ingredient::fromTag("minecraft:bundles");
    Ingredient material = Ingredient::fromItem(*Items::WHITE_DYE);
    ItemStack result(*Items::WHITE_BUNDLE, 1);

    TransmuteRecipe recipe(ResourceLocation("test", "transmute"), input, material, result);

    CraftingInventory inventory(3, 3);
    inventory.setItem(0, ItemStack(*Items::WHITE_BUNDLE, 1));
    inventory.setItem(1, ItemStack(*Items::WHITE_DYE, 1));

    // 由于 _isResultUnchanged 返回 true，匹配应失败
    EXPECT_FALSE(recipe.matches(inventory));
}

TEST_F(TransmuteRecipeTest, Matches_WrongMaterial_ReturnsFalse)
{
    Ingredient input = Ingredient::fromItem(*Items::BUNDLE);
    Ingredient material = Ingredient::fromItem(*Items::WHITE_DYE);
    ItemStack result(*Items::WHITE_BUNDLE, 1);

    TransmuteRecipe recipe(ResourceLocation("test", "transmute"), input, material, result);

    // 用石头代替染料
    CraftingInventory inventory(3, 3);
    inventory.setItem(0, ItemStack(*Items::BUNDLE, 1));
    inventory.setItem(1, ItemStack(*Items::STONE, 1));

    EXPECT_FALSE(recipe.matches(inventory));
}

// ============================================================================
// assemble 测试
// ============================================================================

TEST_F(TransmuteRecipeTest, Assemble_PreservesBundleContents)
{
    Ingredient input = Ingredient::fromItem(*Items::BUNDLE);
    Ingredient material = Ingredient::fromItem(*Items::WHITE_DYE);
    ItemStack result(*Items::WHITE_BUNDLE, 1);

    TransmuteRecipe recipe(ResourceLocation("test", "transmute"), input, material, result);

    // 创建带内容物的收纳袋
    ItemStack bundle(*Items::BUNDLE, 1);
    BundleContents::Mutable mutableContents(BundleContents::EMPTY);
    ItemStack stone(*Items::STONE, 5);
    mutableContents.tryInsert(stone);
    bundle.getOrCreateTag()["BundleContents"] = mutableContents.toImmutable().toJson();

    CraftingInventory inventory(3, 3);
    inventory.setItem(0, bundle);
    inventory.setItem(1, ItemStack(*Items::WHITE_DYE, 1));

    ItemStack assembled = recipe.assemble(inventory);
    EXPECT_EQ(assembled.getItem(), Items::WHITE_BUNDLE);
    EXPECT_EQ(assembled.getCount(), 1);

    // 验证 BundleContents 被保留
    BundleContents contents = BundleContents::fromItemStack(assembled);
    EXPECT_EQ(contents.size(), 1u);
    EXPECT_EQ(contents.weight(), 5);
    EXPECT_EQ(contents.getItemUnsafe(0).getItem(), Items::STONE);
    EXPECT_EQ(contents.getItemUnsafe(0).getCount(), 5);
}

TEST_F(TransmuteRecipeTest, Assemble_EmptyBundle_ProducesEmptyResult)
{
    Ingredient input = Ingredient::fromItem(*Items::BUNDLE);
    Ingredient material = Ingredient::fromItem(*Items::WHITE_DYE);
    ItemStack result(*Items::WHITE_BUNDLE, 1);

    TransmuteRecipe recipe(ResourceLocation("test", "transmute"), input, material, result);

    CraftingInventory inventory(3, 3);
    inventory.setItem(0, ItemStack(*Items::BUNDLE, 1));
    inventory.setItem(1, ItemStack(*Items::WHITE_DYE, 1));

    ItemStack assembled = recipe.assemble(inventory);
    EXPECT_EQ(assembled.getItem(), Items::WHITE_BUNDLE);
    EXPECT_TRUE(BundleContents::fromItemStack(assembled).isEmpty());
}

// ============================================================================
// JSON 解析测试
// ============================================================================

TEST_F(TransmuteRecipeTest, JsonParse_ValidTransmuteRecipe)
{
    nlohmann::json json = nlohmann::json::parse(R"({
        "type": "minecraft:crafting_transmute",
        "category": "equipment",
        "group": "bundle_dye",
        "input": "#minecraft:bundles",
        "material": "minecraft:white_dye",
        "result": {
            "id": "minecraft:white_bundle"
        }
    })");

    auto result = RecipeSerializers::fromJson(ResourceLocation("minecraft", "white_bundle"), json);
    ASSERT_TRUE(result.success()) << result.error().message();

    // 注意：Result<std::unique_ptr<T>>::value() 会转移所有权，所以只能调用一次
    std::unique_ptr<CraftingRecipe> recipe = std::move(result).value();
    ASSERT_NE(recipe, nullptr);

    EXPECT_EQ(recipe->getType(), RecipeType::Transmute);
    EXPECT_EQ(recipe->getId(), ResourceLocation("minecraft", "white_bundle"));
    EXPECT_EQ(recipe->getGroup(), "bundle_dye");
}

TEST_F(TransmuteRecipeTest, JsonParse_MissingInput_ReturnsError)
{
    nlohmann::json json = nlohmann::json::parse(R"({
        "type": "minecraft:crafting_transmute",
        "material": "minecraft:white_dye",
        "result": {
            "id": "minecraft:white_bundle"
        }
    })");

    auto result = RecipeSerializers::fromJson(ResourceLocation("test", "bad"), json);
    EXPECT_FALSE(result.success());
}

TEST_F(TransmuteRecipeTest, JsonParse_MissingMaterial_ReturnsError)
{
    nlohmann::json json = nlohmann::json::parse(R"({
        "type": "minecraft:crafting_transmute",
        "input": "#minecraft:bundles",
        "result": {
            "id": "minecraft:white_bundle"
        }
    })");

    auto result = RecipeSerializers::fromJson(ResourceLocation("test", "bad"), json);
    EXPECT_FALSE(result.success());
}

TEST_F(TransmuteRecipeTest, JsonParse_MissingResult_ReturnsError)
{
    nlohmann::json json = nlohmann::json::parse(R"({
        "type": "minecraft:crafting_transmute",
        "input": "#minecraft:bundles",
        "material": "minecraft:white_dye"
    })");

    auto result = RecipeSerializers::fromJson(ResourceLocation("test", "bad"), json);
    EXPECT_FALSE(result.success());
}

// ============================================================================
// 注册与查询测试
// ============================================================================

TEST_F(TransmuteRecipeTest, RegisterAndFind_TransmuteRecipe)
{
    Ingredient input = Ingredient::fromItem(*Items::BUNDLE);
    Ingredient material = Ingredient::fromItem(*Items::WHITE_DYE);
    ItemStack result(*Items::WHITE_BUNDLE, 1);

    auto recipe = std::make_unique<TransmuteRecipe>(
        ResourceLocation("test", "bundle_to_white"), input, material, result, "bundle_dye");

    ASSERT_TRUE(RecipeManager::instance().registerRecipe(std::move(recipe)));

    CraftingInventory inventory(3, 3);
    inventory.setItem(0, ItemStack(*Items::BUNDLE, 1));
    inventory.setItem(1, ItemStack(*Items::WHITE_DYE, 1));

    const CraftingRecipe* found = RecipeManager::instance().findMatchingRecipe(inventory);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->getId(), ResourceLocation("test", "bundle_to_white"));
    EXPECT_EQ(found->getType(), RecipeType::Transmute);
}

// ============================================================================
// transmuteCopy 测试
// ============================================================================

TEST_F(TransmuteRecipeTest, TransmuteCopy_PreservesNBT)
{
    ItemStack bundle(*Items::BUNDLE, 1);
    BundleContents::Mutable mutableContents(BundleContents::EMPTY);
    ItemStack stone(*Items::STONE, 7);
    mutableContents.tryInsert(stone);
    bundle.getOrCreateTag()["BundleContents"] = mutableContents.toImmutable().toJson();

    // transmuteCopy 应保留 NBT
    ItemStack whiteBundle = bundle.transmuteCopy(*Items::WHITE_BUNDLE, 1);
    EXPECT_EQ(whiteBundle.getItem(), Items::WHITE_BUNDLE);
    EXPECT_EQ(whiteBundle.getCount(), 1);

    BundleContents contents = BundleContents::fromItemStack(whiteBundle);
    EXPECT_EQ(contents.size(), 1u);
    EXPECT_EQ(contents.weight(), 7);
}

TEST_F(TransmuteRecipeTest, TransmuteCopy_PreservesCustomName)
{
    ItemStack bundle(*Items::BUNDLE, 1);
    bundle.setCustomName("My Bundle");

    ItemStack redBundle = bundle.transmuteCopy(*Items::RED_BUNDLE, 1);
    EXPECT_EQ(redBundle.getItem(), Items::RED_BUNDLE);
    EXPECT_TRUE(redBundle.hasCustomName());
    EXPECT_EQ(redBundle.getCustomName(), "My Bundle");
}
