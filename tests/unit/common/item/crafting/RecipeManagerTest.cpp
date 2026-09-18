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

#include "item/crafting/RecipeManager.hpp"
#include "entity/inventory/CraftingInventory.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/crafting/IRecipe.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::crafting;

// 简单的测试配方类
class TestRecipe : public CraftingRecipe {
public:
    TestRecipe(const ResourceLocation& id,
        RecipeType type,
        std::vector<Ingredient> ingredients,
        ItemStack result,
        i32 width = 0,
        i32 height = 0)
        : m_id(id)
        , m_type(type)
        , m_ingredients(std::move(ingredients))
        , m_result(std::move(result))
        , m_width(width)
        , m_height(height)
    {}

    bool matches(const CraftingInventory& inventory) const override
    {
        // 简单的测试匹配逻辑
        // 检查每个原料是否在网格中
        if (m_ingredients.empty()) {
            return inventory.isEmpty();
        }

        // 简化版：检查是否有足够匹配的槽位
        std::vector<bool> used(inventory.getContainerSize(), false);
        for (const Ingredient& ing : m_ingredients) {
            bool found = false;
            for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
                if (!used[i] && ing.test(inventory.getItem(i))) {
                    used[i] = true;
                    found = true;
                    break;
                }
            }
            if (!found) {
                return false;
            }
        }
        return true;
    }

    ItemStack assemble(const CraftingInventory& inventory) const override
    {
        (void)inventory;
        return m_result.copy();
    }

    ItemStack getResultItem() const override { return m_result; }

    std::vector<ItemStack> getRemainingItems(const CraftingInventory& inventory) const override
    {
        return RecipeUtils::getDefaultRemainingItems(inventory);
    }

    const std::vector<Ingredient>& getIngredients() const override { return m_ingredients; }

    i32 getRecipeWidth() const override { return m_width; }
    i32 getRecipeHeight() const override { return m_height; }

    ResourceLocation getId() const override { return m_id; }
    RecipeType getType() const override { return m_type; }

private:
    ResourceLocation m_id;
    RecipeType m_type;
    std::vector<Ingredient> m_ingredients;
    ItemStack m_result;
    i32 m_width;
    i32 m_height;
};

class RecipeManagerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 每个测试开始前清空注册表
        RecipeManager::instance().clear();
    }

    void TearDown() override
    {
        // 每个测试结束后清空注册表
        RecipeManager::instance().clear();
    }
};

// ========== 基础测试 ==========

TEST_F(RecipeManagerTest, IsSingleton)
{
    RecipeManager& instance1 = RecipeManager::instance();
    RecipeManager& instance2 = RecipeManager::instance();
    EXPECT_EQ(&instance1, &instance2);
}

TEST_F(RecipeManagerTest, StartsEmpty)
{
    EXPECT_EQ(RecipeManager::instance().getRecipeCount(), 0u);
}

TEST_F(RecipeManagerTest, ClearRemovesAllRecipes)
{
    auto recipe = std::make_unique<TestRecipe>(
        ResourceLocation("test", "recipe1"), RecipeType::ShapelessCrafting, std::vector<Ingredient>(), ItemStack());

    RecipeManager::instance().registerRecipe(std::move(recipe));
    EXPECT_EQ(RecipeManager::instance().getRecipeCount(), 1u);

    RecipeManager::instance().clear();
    EXPECT_EQ(RecipeManager::instance().getRecipeCount(), 0u);
}

// ========== 注册测试 ==========

TEST_F(RecipeManagerTest, RegisterRecipe_Success)
{
    auto recipe = std::make_unique<TestRecipe>(
        ResourceLocation("test", "recipe1"), RecipeType::ShapelessCrafting, std::vector<Ingredient>(), ItemStack());

    bool result = RecipeManager::instance().registerRecipe(std::move(recipe));
    EXPECT_TRUE(result);
    EXPECT_EQ(RecipeManager::instance().getRecipeCount(), 1u);
}

TEST_F(RecipeManagerTest, RegisterRecipe_DuplicateId_Fails)
{
    auto recipe1 = std::make_unique<TestRecipe>(
        ResourceLocation("test", "recipe1"), RecipeType::ShapelessCrafting, std::vector<Ingredient>(), ItemStack());

    auto recipe2 = std::make_unique<TestRecipe>(ResourceLocation("test", "recipe1"), // 相同ID
        RecipeType::ShapedCrafting,
        std::vector<Ingredient>(),
        ItemStack());

    bool result1 = RecipeManager::instance().registerRecipe(std::move(recipe1));
    EXPECT_TRUE(result1);

    bool result2 = RecipeManager::instance().registerRecipe(std::move(recipe2));
    EXPECT_FALSE(result2); // 应该失败

    EXPECT_EQ(RecipeManager::instance().getRecipeCount(), 1u);
}

TEST_F(RecipeManagerTest, RegisterRecipe_NullRecipe_Fails)
{
    bool result = RecipeManager::instance().registerRecipe(nullptr);
    EXPECT_FALSE(result);
}

TEST_F(RecipeManagerTest, RegisterMultipleRecipes)
{
    auto recipe1 = std::make_unique<TestRecipe>(
        ResourceLocation("test", "recipe1"), RecipeType::ShapelessCrafting, std::vector<Ingredient>(), ItemStack());

    auto recipe2 = std::make_unique<TestRecipe>(
        ResourceLocation("test", "recipe2"), RecipeType::ShapedCrafting, std::vector<Ingredient>(), ItemStack());

    RecipeManager::instance().registerRecipe(std::move(recipe1));
    RecipeManager::instance().registerRecipe(std::move(recipe2));

    EXPECT_EQ(RecipeManager::instance().getRecipeCount(), 2u);
}

// ========== 查询测试 ==========

TEST_F(RecipeManagerTest, GetRecipe_ById_Found)
{
    ResourceLocation id("test", "recipe1");

    auto recipe =
        std::make_unique<TestRecipe>(id, RecipeType::ShapelessCrafting, std::vector<Ingredient>(), ItemStack());

    RecipeManager::instance().registerRecipe(std::move(recipe));

    const CraftingRecipe* found = RecipeManager::instance().getRecipe(id);
    EXPECT_NE(found, nullptr);
    EXPECT_EQ(found->getId(), id);
}

TEST_F(RecipeManagerTest, GetRecipe_ById_NotFound)
{
    ResourceLocation id("test", "nonexistent");

    const CraftingRecipe* found = RecipeManager::instance().getRecipe(id);
    EXPECT_EQ(found, nullptr);
}

TEST_F(RecipeManagerTest, HasRecipe_Existing_ReturnsTrue)
{
    ResourceLocation id("test", "recipe1");

    auto recipe =
        std::make_unique<TestRecipe>(id, RecipeType::ShapelessCrafting, std::vector<Ingredient>(), ItemStack());

    RecipeManager::instance().registerRecipe(std::move(recipe));

    EXPECT_TRUE(RecipeManager::instance().hasRecipe(id));
}

TEST_F(RecipeManagerTest, HasRecipe_NonExisting_ReturnsFalse)
{
    ResourceLocation id("test", "nonexistent");
    EXPECT_FALSE(RecipeManager::instance().hasRecipe(id));
}

// ========== 类型查询测试 ==========

TEST_F(RecipeManagerTest, GetRecipesByType)
{
    auto shaped1 = std::make_unique<TestRecipe>(
        ResourceLocation("test", "shaped1"), RecipeType::ShapedCrafting, std::vector<Ingredient>(), ItemStack());

    auto shaped2 = std::make_unique<TestRecipe>(
        ResourceLocation("test", "shaped2"), RecipeType::ShapedCrafting, std::vector<Ingredient>(), ItemStack());

    auto shapeless = std::make_unique<TestRecipe>(
        ResourceLocation("test", "shapeless"), RecipeType::ShapelessCrafting, std::vector<Ingredient>(), ItemStack());

    RecipeManager::instance().registerRecipe(std::move(shaped1));
    RecipeManager::instance().registerRecipe(std::move(shaped2));
    RecipeManager::instance().registerRecipe(std::move(shapeless));

    auto shapedRecipes = RecipeManager::instance().getRecipesByType(RecipeType::ShapedCrafting);
    EXPECT_EQ(shapedRecipes.size(), 2u);

    auto shapelessRecipes = RecipeManager::instance().getRecipesByType(RecipeType::ShapelessCrafting);
    EXPECT_EQ(shapelessRecipes.size(), 1u);

    auto smeltingRecipes = RecipeManager::instance().getRecipesByType(RecipeType::Smelting);
    EXPECT_EQ(smeltingRecipes.size(), 0u);
}

// ========== 匹配测试 ==========

TEST_F(RecipeManagerTest, FindMatchingRecipe_NoRecipes_ReturnsNull)
{
    CraftingInventory inventory(3, 3);
    const CraftingRecipe* found = RecipeManager::instance().findMatchingRecipe(inventory);
    EXPECT_EQ(found, nullptr);
}

TEST_F(RecipeManagerTest, FindMatchingRecipe_MatchingRecipe_ReturnsRecipe)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered, skipping test";
    }

    // 创建一个需要石头原料的配方
    Ingredient stoneIngredient = Ingredient::fromItem(*stone);
    ItemStack result(*stone, 2); // 结果：2个石头

    auto recipe = std::make_unique<TestRecipe>(ResourceLocation("test", "stone_doubling"),
        RecipeType::ShapelessCrafting,
        std::vector<Ingredient>{stoneIngredient},
        result);

    RecipeManager::instance().registerRecipe(std::move(recipe));

    // 创建一个有石头的网格
    CraftingInventory inventory(3, 3);
    inventory.setItem(0, ItemStack(*stone, 1));

    const CraftingRecipe* found = RecipeManager::instance().findMatchingRecipe(inventory);
    EXPECT_NE(found, nullptr);
    EXPECT_EQ(found->getId(), ResourceLocation("test", "stone_doubling"));
}

TEST_F(RecipeManagerTest, FindMatchingRecipe_NoMatch_ReturnsNull)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    Item* dirt = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dirt"));

    if (!stone || !dirt) {
        GTEST_SKIP() << "Required items not registered, skipping test";
    }

    // 创建一个需要石头的配方
    Ingredient stoneIngredient = Ingredient::fromItem(*stone);
    ItemStack result(*stone, 2);

    auto recipe = std::make_unique<TestRecipe>(ResourceLocation("test", "stone_doubling"),
        RecipeType::ShapelessCrafting,
        std::vector<Ingredient>{stoneIngredient},
        result);

    RecipeManager::instance().registerRecipe(std::move(recipe));

    // 创建一个只有泥土的网格
    CraftingInventory inventory(3, 3);
    inventory.setItem(0, ItemStack(*dirt, 1));

    const CraftingRecipe* found = RecipeManager::instance().findMatchingRecipe(inventory);
    EXPECT_EQ(found, nullptr);
}

// ========== 结果查询测试 ==========

TEST_F(RecipeManagerTest, GetRecipesForResult)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    Item* dirt = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dirt"));

    if (!stone || !dirt) {
        GTEST_SKIP() << "Required items not registered, skipping test";
    }

    // 创建两个产出石头的配方
    auto recipe1 = std::make_unique<TestRecipe>(ResourceLocation("test", "recipe1"),
        RecipeType::ShapelessCrafting,
        std::vector<Ingredient>(),
        ItemStack(*stone, 1));

    auto recipe2 = std::make_unique<TestRecipe>(ResourceLocation("test", "recipe2"),
        RecipeType::ShapelessCrafting,
        std::vector<Ingredient>(),
        ItemStack(*stone, 2));

    auto recipe3 = std::make_unique<TestRecipe>(ResourceLocation("test", "recipe3"),
        RecipeType::ShapelessCrafting,
        std::vector<Ingredient>(),
        ItemStack(*dirt, 1));

    RecipeManager::instance().registerRecipe(std::move(recipe1));
    RecipeManager::instance().registerRecipe(std::move(recipe2));
    RecipeManager::instance().registerRecipe(std::move(recipe3));

    // 查询产出石头的配方
    auto stoneRecipes = RecipeManager::instance().getRecipesForResult(*stone);
    EXPECT_EQ(stoneRecipes.size(), 2u);

    // 查询产出泥土的配方
    auto dirtRecipes = RecipeManager::instance().getRecipesForResult(*dirt);
    EXPECT_EQ(dirtRecipes.size(), 1u);
}

// ========== 遍历测试 ==========

TEST_F(RecipeManagerTest, ForEachRecipe)
{
    auto recipe1 = std::make_unique<TestRecipe>(
        ResourceLocation("test", "recipe1"), RecipeType::ShapelessCrafting, std::vector<Ingredient>(), ItemStack());

    auto recipe2 = std::make_unique<TestRecipe>(
        ResourceLocation("test", "recipe2"), RecipeType::ShapedCrafting, std::vector<Ingredient>(), ItemStack());

    RecipeManager::instance().registerRecipe(std::move(recipe1));
    RecipeManager::instance().registerRecipe(std::move(recipe2));

    std::vector<ResourceLocation> ids;
    RecipeManager::instance().forEachRecipe([&ids](const CraftingRecipe& recipe) { ids.push_back(recipe.getId()); });

    EXPECT_EQ(ids.size(), 2u);
    EXPECT_NE(std::find(ids.begin(), ids.end(), ResourceLocation("test", "recipe1")), ids.end());
    EXPECT_NE(std::find(ids.begin(), ids.end(), ResourceLocation("test", "recipe2")), ids.end());
}

// ========== GetAllRecipes测试 ==========

TEST_F(RecipeManagerTest, GetAllRecipes)
{
    auto recipe1 = std::make_unique<TestRecipe>(
        ResourceLocation("test", "recipe1"), RecipeType::ShapelessCrafting, std::vector<Ingredient>(), ItemStack());

    auto recipe2 = std::make_unique<TestRecipe>(
        ResourceLocation("test", "recipe2"), RecipeType::ShapedCrafting, std::vector<Ingredient>(), ItemStack());

    RecipeManager::instance().registerRecipe(std::move(recipe1));
    RecipeManager::instance().registerRecipe(std::move(recipe2));

    auto allRecipes = RecipeManager::instance().getAllRecipes();
    EXPECT_EQ(allRecipes.size(), 2u);
}

// ========== FindAllMatchingRecipes测试 ==========

TEST_F(RecipeManagerTest, FindAllMatchingRecipes)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered, skipping test";
    }

    Ingredient stoneIngredient = Ingredient::fromItem(*stone);

    // 创建多个可能匹配的配方
    auto recipe1 = std::make_unique<TestRecipe>(ResourceLocation("test", "recipe1"),
        RecipeType::ShapelessCrafting,
        std::vector<Ingredient>{stoneIngredient},
        ItemStack(*stone, 1));

    auto recipe2 = std::make_unique<TestRecipe>(ResourceLocation("test", "recipe2"),
        RecipeType::ShapelessCrafting,
        std::vector<Ingredient>{stoneIngredient},
        ItemStack(*stone, 2));

    RecipeManager::instance().registerRecipe(std::move(recipe1));
    RecipeManager::instance().registerRecipe(std::move(recipe2));

    CraftingInventory inventory(3, 3);
    inventory.setItem(0, ItemStack(*stone, 1));

    auto matching = RecipeManager::instance().findAllMatchingRecipes(inventory);
    EXPECT_EQ(matching.size(), 2u);
}
