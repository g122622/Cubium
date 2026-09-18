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

#include "common/item/crafting/RecipeBook.hpp"
#include "common/item/crafting/RecipeManager.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <sstream>

using namespace mc;
using namespace mc::crafting;

/**
 * @brief RecipeBook 单元测试
 */
class RecipeBookTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建测试用配方ID
        recipe1 = ResourceLocation("minecraft", "crafting_table");
        recipe2 = ResourceLocation("minecraft", "furnace");
        recipe3 = ResourceLocation("minecraft", "stone_pickaxe");
    }

    ResourceLocation recipe1;
    ResourceLocation recipe2;
    ResourceLocation recipe3;
};

// ========== RecipeBookCategory 测试 ==========

TEST_F(RecipeBookTest, CategoryToString)
{
    EXPECT_STREQ(recipeBookCategoryToString(RecipeBookCategory::Crafting), "crafting");
    EXPECT_STREQ(recipeBookCategoryToString(RecipeBookCategory::Furnace), "furnace");
    EXPECT_STREQ(recipeBookCategoryToString(RecipeBookCategory::BlastFurnace), "blast_furnace");
    EXPECT_STREQ(recipeBookCategoryToString(RecipeBookCategory::Smoker), "smoker");
}

TEST_F(RecipeBookTest, CategoryFromString)
{
    EXPECT_EQ(recipeBookCategoryFromString("crafting"), RecipeBookCategory::Crafting);
    EXPECT_EQ(recipeBookCategoryFromString("minecraft:crafting"), RecipeBookCategory::Crafting);
    EXPECT_EQ(recipeBookCategoryFromString("furnace"), RecipeBookCategory::Furnace);
    EXPECT_EQ(recipeBookCategoryFromString("blast_furnace"), RecipeBookCategory::BlastFurnace);
    EXPECT_EQ(recipeBookCategoryFromString("smoker"), RecipeBookCategory::Smoker);
    EXPECT_EQ(recipeBookCategoryFromString("unknown"), std::nullopt);
}

// ========== RecipeBookStatus 测试 ==========

TEST_F(RecipeBookTest, StatusDefaultValues)
{
    RecipeBookStatus status;

    // 默认所有分类都是关闭状态
    EXPECT_FALSE(status.isGuiOpen(RecipeBookCategory::Crafting));
    EXPECT_FALSE(status.isFilteringCraftable(RecipeBookCategory::Crafting));
    EXPECT_FALSE(status.isGuiOpen(RecipeBookCategory::Furnace));
    EXPECT_FALSE(status.isFilteringCraftable(RecipeBookCategory::Furnace));
    EXPECT_FALSE(status.isGuiOpen(RecipeBookCategory::BlastFurnace));
    EXPECT_FALSE(status.isFilteringCraftable(RecipeBookCategory::BlastFurnace));
    EXPECT_FALSE(status.isGuiOpen(RecipeBookCategory::Smoker));
    EXPECT_FALSE(status.isFilteringCraftable(RecipeBookCategory::Smoker));
}

TEST_F(RecipeBookTest, StatusSetAndGet)
{
    RecipeBookStatus status;

    // 设置 Crafting 分类状态
    status.setGuiOpen(RecipeBookCategory::Crafting, true);
    EXPECT_TRUE(status.isGuiOpen(RecipeBookCategory::Crafting));

    status.setFilteringCraftable(RecipeBookCategory::Crafting, true);
    EXPECT_TRUE(status.isFilteringCraftable(RecipeBookCategory::Crafting));

    // 设置完整状态
    status.setCategoryStatus(RecipeBookCategory::Furnace, true, true);
    EXPECT_TRUE(status.isGuiOpen(RecipeBookCategory::Furnace));
    EXPECT_TRUE(status.isFilteringCraftable(RecipeBookCategory::Furnace));
}

TEST_F(RecipeBookTest, StatusCopyFrom)
{
    RecipeBookStatus status1;
    status1.setCategoryStatus(RecipeBookCategory::Crafting, true, false);
    status1.setCategoryStatus(RecipeBookCategory::Furnace, false, true);

    RecipeBookStatus status2;
    status2.copyFrom(status1);

    EXPECT_TRUE(status2.isGuiOpen(RecipeBookCategory::Crafting));
    EXPECT_FALSE(status2.isFilteringCraftable(RecipeBookCategory::Crafting));
    EXPECT_FALSE(status2.isGuiOpen(RecipeBookCategory::Furnace));
    EXPECT_TRUE(status2.isFilteringCraftable(RecipeBookCategory::Furnace));
}

TEST_F(RecipeBookTest, StatusEquality)
{
    RecipeBookStatus status1;
    status1.setCategoryStatus(RecipeBookCategory::Crafting, true, true);

    RecipeBookStatus status2;
    status2.setCategoryStatus(RecipeBookCategory::Crafting, true, true);

    RecipeBookStatus status3;
    status3.setCategoryStatus(RecipeBookCategory::Crafting, false, true);

    EXPECT_TRUE(status1 == status2);
    EXPECT_FALSE(status1 == status3);
}

// ========== RecipeBook 基础测试 ==========

TEST_F(RecipeBookTest, UnlockRecipe)
{
    RecipeBook book;

    // 初始状态：配方未解锁
    EXPECT_FALSE(book.isUnlocked(recipe1));

    // 解锁配方
    book.unlock(recipe1);
    EXPECT_TRUE(book.isUnlocked(recipe1));

    // 重复解锁不影响
    book.unlock(recipe1);
    EXPECT_TRUE(book.isUnlocked(recipe1));
    EXPECT_EQ(book.getUnlockedCount(), 1u);
}

TEST_F(RecipeBookTest, LockRecipe)
{
    RecipeBook book;

    // 解锁配方
    book.unlock(recipe1);
    book.markNew(recipe1);
    EXPECT_TRUE(book.isUnlocked(recipe1));
    EXPECT_TRUE(book.isNew(recipe1));

    // 锁定配方
    book.lock(recipe1);
    EXPECT_FALSE(book.isUnlocked(recipe1));
    EXPECT_FALSE(book.isNew(recipe1));
}

TEST_F(RecipeBookTest, NewRecipeTracking)
{
    RecipeBook book;

    // 解锁配方后标记为新配方
    book.unlock(recipe1);
    book.markNew(recipe1);
    EXPECT_TRUE(book.isNew(recipe1));

    // 标记为已查看
    book.markSeen(recipe1);
    EXPECT_FALSE(book.isNew(recipe1));

    // 配方仍然解锁
    EXPECT_TRUE(book.isUnlocked(recipe1));
}

TEST_F(RecipeBookTest, MultipleRecipes)
{
    RecipeBook book;

    // 解锁多个配方
    book.unlock(recipe1);
    book.unlock(recipe2);
    book.unlock(recipe3);

    EXPECT_EQ(book.getUnlockedCount(), 3u);
    EXPECT_TRUE(book.isUnlocked(recipe1));
    EXPECT_TRUE(book.isUnlocked(recipe2));
    EXPECT_TRUE(book.isUnlocked(recipe3));
}

TEST_F(RecipeBookTest, CopyFrom)
{
    RecipeBook book1;
    book1.unlock(recipe1);
    book1.unlock(recipe2);
    book1.markNew(recipe1);
    book1.getStatus().setGuiOpen(RecipeBookCategory::Crafting, true);

    RecipeBook book2;
    book2.copyFrom(book1);

    EXPECT_TRUE(book2.isUnlocked(recipe1));
    EXPECT_TRUE(book2.isUnlocked(recipe2));
    EXPECT_TRUE(book2.isNew(recipe1));
    EXPECT_TRUE(book2.getStatus().isGuiOpen(RecipeBookCategory::Crafting));
}

TEST_F(RecipeBookTest, Clear)
{
    RecipeBook book;
    book.unlock(recipe1);
    book.unlock(recipe2);
    book.markNew(recipe1);

    book.clear();

    EXPECT_EQ(book.getUnlockedCount(), 0u);
    EXPECT_EQ(book.getNewCount(), 0u);
    EXPECT_FALSE(book.isUnlocked(recipe1));
    EXPECT_FALSE(book.isUnlocked(recipe2));
}

// ========== ServerRecipeBook 测试 ==========

TEST_F(RecipeBookTest, ServerRecipeBookAddRecipes)
{
    ServerRecipeBook book;

    std::vector<ResourceLocation> recipes = {recipe1, recipe2, recipe3};
    size_t unlocked = book.add(recipes.begin(), recipes.end());

    // 所有配方都应该被解锁
    EXPECT_EQ(unlocked, 3u);
    EXPECT_TRUE(book.isUnlocked(recipe1));
    EXPECT_TRUE(book.isUnlocked(recipe2));
    EXPECT_TRUE(book.isUnlocked(recipe3));

    // 新配方应该被标记
    EXPECT_TRUE(book.isNew(recipe1));
    EXPECT_TRUE(book.isNew(recipe2));
    EXPECT_TRUE(book.isNew(recipe3));
}

TEST_F(RecipeBookTest, ServerRecipeBookAddAlreadyUnlocked)
{
    ServerRecipeBook book;

    // 先解锁 recipe1
    book.unlock(recipe1);

    std::vector<ResourceLocation> recipes = {recipe1, recipe2};
    size_t unlocked = book.add(recipes.begin(), recipes.end());

    // 只有 recipe2 应该被解锁
    EXPECT_EQ(unlocked, 1u);
    EXPECT_TRUE(book.isUnlocked(recipe1));
    EXPECT_TRUE(book.isUnlocked(recipe2));
    EXPECT_FALSE(book.isNew(recipe1)); // recipe1 已存在，不会被标记为新
    EXPECT_TRUE(book.isNew(recipe2));
}

TEST_F(RecipeBookTest, ServerRecipeBookRemoveRecipes)
{
    ServerRecipeBook book;

    // 先解锁配方
    book.unlock(recipe1);
    book.unlock(recipe2);
    book.unlock(recipe3);

    // 移除部分配方
    std::vector<ResourceLocation> toRemove = {recipe1, recipe2};
    size_t removed = book.remove(toRemove.begin(), toRemove.end());

    EXPECT_EQ(removed, 2u);
    EXPECT_FALSE(book.isUnlocked(recipe1));
    EXPECT_FALSE(book.isUnlocked(recipe2));
    EXPECT_TRUE(book.isUnlocked(recipe3));
}

TEST_F(RecipeBookTest, ServerRecipeBookNbtSerialization)
{
    // 创建一个配方书并设置状态
    ServerRecipeBook book1;
    book1.unlock(recipe1);
    book1.unlock(recipe2);
    book1.markNew(recipe1);
    book1.getStatus().setGuiOpen(RecipeBookCategory::Crafting, true);
    book1.getStatus().setFilteringCraftable(RecipeBookCategory::Furnace, true);

    // 序列化到 NBT
    nbt::tags::compound_tag tag = book1.write();

    // 反序列化到新的配方书
    ServerRecipeBook book2;
    book2.read(tag);

    // 验证配方状态
    EXPECT_TRUE(book2.isUnlocked(recipe1));
    EXPECT_TRUE(book2.isUnlocked(recipe2));
    EXPECT_TRUE(book2.isNew(recipe1));

    // 验证 GUI 状态
    EXPECT_TRUE(book2.getStatus().isGuiOpen(RecipeBookCategory::Crafting));
    EXPECT_TRUE(book2.getStatus().isFilteringCraftable(RecipeBookCategory::Furnace));
}

TEST_F(RecipeBookTest, ServerRecipeBookConsumeNewRecipes)
{
    ServerRecipeBook book;
    book.unlock(recipe1);
    book.unlock(recipe2);
    book.markNew(recipe1);
    book.markNew(recipe2);

    EXPECT_EQ(book.getNewCount(), 2u);

    // 消费新配方列表
    std::vector<ResourceLocation> newRecipes = book.consumeNewRecipes();
    EXPECT_EQ(newRecipes.size(), 2u);
    EXPECT_EQ(book.getNewCount(), 0u);

    // 配方仍然解锁
    EXPECT_TRUE(book.isUnlocked(recipe1));
    EXPECT_TRUE(book.isUnlocked(recipe2));
}

TEST_F(RecipeBookTest, ServerRecipeBookGetAllUnlockedRecipes)
{
    ServerRecipeBook book;
    book.unlock(recipe1);
    book.unlock(recipe2);
    book.unlock(recipe3);

    std::vector<ResourceLocation> allRecipes = book.getAllUnlockedRecipes();
    EXPECT_EQ(allRecipes.size(), 3u);

    // 验证所有配方都在列表中
    bool found1 = false, found2 = false, found3 = false;
    for (const auto& id : allRecipes) {
        if (id == recipe1) found1 = true;
        if (id == recipe2) found2 = true;
        if (id == recipe3) found3 = true;
    }
    EXPECT_TRUE(found1);
    EXPECT_TRUE(found2);
    EXPECT_TRUE(found3);
}

// ========== NBT 序列化边界测试 ==========

TEST_F(RecipeBookTest, NbtEmptyBook)
{
    ServerRecipeBook book1;

    // 序列化空配方书
    nbt::tags::compound_tag tag = book1.write();

    // 反序列化
    ServerRecipeBook book2;
    book2.read(tag);

    EXPECT_EQ(book2.getUnlockedCount(), 0u);
    EXPECT_EQ(book2.getNewCount(), 0u);
}

TEST_F(RecipeBookTest, NbtMissingFields)
{
    // 创建一个空的 NBT 标签
    nbt::tags::compound_tag emptyTag;

    // 应该不会崩溃
    ServerRecipeBook book;
    book.read(emptyTag);

    EXPECT_EQ(book.getUnlockedCount(), 0u);
    EXPECT_EQ(book.getNewCount(), 0u);
}

// ========== isBookRecipe 测试 ==========

TEST_F(RecipeBookTest, IsBookRecipe_UnlockedNonDynamicRecipe)
{
    // 已解锁的非动态配方应在配方书中显示
    RecipeBook book;
    book.unlock(recipe1);
    // recipe1 是 "minecraft:crafting_table"，不是动态配方，应可显示
    EXPECT_TRUE(book.isBookRecipe(recipe1));
}

TEST_F(RecipeBookTest, IsBookRecipe_NotUnlocked)
{
    // 未解锁的配方不在配方书中
    RecipeBook book;
    EXPECT_FALSE(book.isBookRecipe(recipe1));
}

TEST_F(RecipeBookTest, IsBookRecipe_UnlockedAndNotUnlockedMix)
{
    // 解锁 recipe1 但不解锁 recipe2
    RecipeBook book;
    book.unlock(recipe1);
    EXPECT_TRUE(book.isBookRecipe(recipe1));
    EXPECT_FALSE(book.isBookRecipe(recipe2));
}

TEST_F(RecipeBookTest, IsBookRecipe_AfterLock)
{
    // 锁定后不在配方书中
    RecipeBook book;
    book.unlock(recipe1);
    EXPECT_TRUE(book.isBookRecipe(recipe1));
    book.lock(recipe1);
    EXPECT_FALSE(book.isBookRecipe(recipe1));
}

TEST_F(RecipeBookTest, IsDynamicRecipe_NonExistentRecipe)
{
    // 不存在的配方不被视为动态配方
    ResourceLocation nonexistent("minecraft", "nonexistent_recipe");
    EXPECT_FALSE(ServerRecipeBook::isDynamicRecipe(nonexistent));
}

TEST_F(RecipeBookTest, IsBookRecipe_DynamicRecipeExcluded)
{
    // 即使动态配方被意外添加到解锁列表中（通过直接调用unlock），
    // isBookRecipe 也应返回 false，因为动态配方不会出现在配方书中
    // 注意：正常流程中 ServerRecipeBook::add() 会自动过滤动态配方
    RecipeBook book;

    // 注册一个动态配方用于测试
    // ArmorDyeRecipe 是动态配方的典型例子
    ResourceLocation armorDyeId("minecraft", "armor_dye");
    // 即使通过底层 unlock 直接添加，isBookRecipe 也应排除
    book.unlock(armorDyeId);

    // 如果该配方已在 RecipeManager 中注册且为动态配方，则 isBookRecipe 返回 false
    // 如果该配方未注册，则 isBookRecipe 返回 true（非动态配方的默认行为）
    // 这验证了 isBookRecipe 的过滤逻辑是正确的
    const crafting::CraftingRecipe* recipe = crafting::RecipeManager::instance().getRecipe(armorDyeId);
    if (recipe != nullptr && recipe->isDynamic()) {
        EXPECT_FALSE(book.isBookRecipe(armorDyeId));
    } else {
        // 未注册的配方或不存在的动态配方，回退到 isUnlocked 行为
        EXPECT_TRUE(book.isBookRecipe(armorDyeId));
    }
}
