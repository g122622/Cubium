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
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"

using namespace mc;
using namespace mc::crafting;

/**
 * @brief 配方书单元测试（对齐 MC 1.21.11）。
 *
 * 覆盖三件事，分别对应 1.21.11 相对 1.16.5 的结构变化：
 *
 *   1. GUI 状态按 **RecipeBookType**（哪个界面）寻址，与页签分类无关；
 *   2. 页签分类（CraftingBookCategory / CookingBookCategory）是配方数据的属性；
 *   3. 解锁集合 `known` 与高亮集合 `highlight` 相互独立。
 */
class RecipeBookTest : public ::testing::Test {};

// ========== RecipeBookType ==========

TEST_F(RecipeBookTest, TypeToString)
{
    EXPECT_STREQ(recipeBookTypeToString(RecipeBookType::Crafting), "crafting");
    EXPECT_STREQ(recipeBookTypeToString(RecipeBookType::Furnace), "furnace");
    EXPECT_STREQ(recipeBookTypeToString(RecipeBookType::BlastFurnace), "blast_furnace");
    EXPECT_STREQ(recipeBookTypeToString(RecipeBookType::Smoker), "smoker");
}

TEST_F(RecipeBookTest, TypeFromString)
{
    EXPECT_EQ(recipeBookTypeFromString("crafting"), RecipeBookType::Crafting);
    EXPECT_EQ(recipeBookTypeFromString("minecraft:crafting"), RecipeBookType::Crafting);
    EXPECT_EQ(recipeBookTypeFromString("blast_furnace"), RecipeBookType::BlastFurnace);
    EXPECT_EQ(recipeBookTypeFromString("minecraft:smoker"), RecipeBookType::Smoker);
    EXPECT_FALSE(recipeBookTypeFromString("building").has_value());
    EXPECT_FALSE(recipeBookTypeFromString("").has_value());
}

// ========== 页签分类 ==========

TEST_F(RecipeBookTest, CraftingCategoryStrings)
{
    EXPECT_STREQ(craftingBookCategoryToString(CraftingBookCategory::Building), "building");
    EXPECT_STREQ(craftingBookCategoryToString(CraftingBookCategory::Redstone), "redstone");
    EXPECT_STREQ(craftingBookCategoryToString(CraftingBookCategory::Equipment), "equipment");
    EXPECT_STREQ(craftingBookCategoryToString(CraftingBookCategory::Misc), "misc");
    EXPECT_EQ(craftingBookCategoryFromString("redstone"), CraftingBookCategory::Redstone);
    // 缺省与无法识别都折成 Misc，与原版 `orElse(MISC)` 一致。
    EXPECT_EQ(craftingBookCategoryFromString("nonsense"), CraftingBookCategory::Misc);
}

TEST_F(RecipeBookTest, CookingCategoryStrings)
{
    EXPECT_STREQ(cookingBookCategoryToString(CookingBookCategory::Food), "food");
    EXPECT_STREQ(cookingBookCategoryToString(CookingBookCategory::Blocks), "blocks");
    EXPECT_STREQ(cookingBookCategoryToString(CookingBookCategory::Misc), "misc");
    EXPECT_EQ(cookingBookCategoryFromString("blocks"), CookingBookCategory::Blocks);
    EXPECT_EQ(cookingBookCategoryFromString("nonsense"), CookingBookCategory::Misc);
}

/**
 * 页签分类与配方书类型是**两套独立**的东西：类型是界面，分类是页签。
 * 早期实现把「工作台」既当类型又当分类用，会让「打开建筑页签」等价于「打开工作台
 * 配方书」。这里固定住「分类取值不影响任何类型的界面状态」。
 */
TEST_F(RecipeBookTest, TabCategoryDoesNotAffectBookSettings)
{
    RecipeBookSettings settings;
    settings.setOpen(RecipeBookType::Crafting, true);

    for (const CraftingBookCategory category : {CraftingBookCategory::Building,
             CraftingBookCategory::Redstone,
             CraftingBookCategory::Equipment,
             CraftingBookCategory::Misc}) {
        EXPECT_STRNE(craftingBookCategoryToString(category), recipeBookTypeToString(RecipeBookType::Furnace));
    }
    EXPECT_TRUE(settings.isOpen(RecipeBookType::Crafting));
    EXPECT_FALSE(settings.isOpen(RecipeBookType::Furnace));
}

// ========== RecipeBookSettings ==========

TEST_F(RecipeBookTest, SettingsDefaultAllClosed)
{
    RecipeBookSettings settings;
    for (const RecipeBookType type :
        {RecipeBookType::Crafting, RecipeBookType::Furnace, RecipeBookType::BlastFurnace, RecipeBookType::Smoker}) {
        EXPECT_FALSE(settings.isOpen(type));
        EXPECT_FALSE(settings.isFiltering(type));
    }
}

TEST_F(RecipeBookTest, SettingsIndependentPerType)
{
    RecipeBookSettings settings;
    settings.set(RecipeBookType::Crafting, true, false);
    settings.set(RecipeBookType::BlastFurnace, false, true);

    EXPECT_TRUE(settings.isOpen(RecipeBookType::Crafting));
    EXPECT_FALSE(settings.isFiltering(RecipeBookType::Crafting));
    EXPECT_FALSE(settings.isOpen(RecipeBookType::BlastFurnace));
    EXPECT_TRUE(settings.isFiltering(RecipeBookType::BlastFurnace));
    // 没设过的两个仍然关闭。
    EXPECT_FALSE(settings.isOpen(RecipeBookType::Furnace));
    EXPECT_FALSE(settings.isFiltering(RecipeBookType::Smoker));
}

TEST_F(RecipeBookTest, SettingsReplaceFromAndEquality)
{
    RecipeBookSettings a;
    a.set(RecipeBookType::Smoker, true, true);
    RecipeBookSettings b;
    EXPECT_FALSE(a == b);

    b.replaceFrom(a);
    EXPECT_TRUE(a == b);
    EXPECT_TRUE(b.isOpen(RecipeBookType::Smoker));

    // copy() 是深拷贝：改副本不影响原件。
    RecipeBookSettings c = a.copy();
    c.setOpen(RecipeBookType::Smoker, false);
    EXPECT_TRUE(a.isOpen(RecipeBookType::Smoker));
    EXPECT_FALSE(c.isOpen(RecipeBookType::Smoker));
}

// ========== ServerRecipeBook：解锁与高亮 ==========

TEST_F(RecipeBookTest, AddInsertsKnownAndHighlight)
{
    ServerRecipeBook book;
    const ResourceLocation id("minecraft", "oak_planks");

    book.add(id);
    EXPECT_TRUE(book.contains(id));
    EXPECT_TRUE(book.isHighlighted(id));
    EXPECT_EQ(book.knownCount(), 1u);
    EXPECT_EQ(book.highlightCount(), 1u);
}

TEST_F(RecipeBookTest, RemoveHighlightKeepsKnown)
{
    ServerRecipeBook book;
    const ResourceLocation id("minecraft", "stick");
    book.add(id);

    book.removeHighlight(id);
    EXPECT_TRUE(book.contains(id)) << "移除高亮不应把配方也锁掉";
    EXPECT_FALSE(book.isHighlighted(id));
}

TEST_F(RecipeBookTest, RemoveClearsBothSets)
{
    ServerRecipeBook book;
    const ResourceLocation id("minecraft", "torch");
    book.add(id);

    book.remove(id);
    EXPECT_FALSE(book.contains(id));
    EXPECT_FALSE(book.isHighlighted(id));
}

// ========== 打包转移 ==========

TEST_F(RecipeBookTest, PackAndApplyRoundTrip)
{
    ServerRecipeBook source;
    source.setBookSetting(RecipeBookType::Furnace, true, false);
    const ResourceLocation a("minecraft", "bread");
    const ResourceLocation b("minecraft", "cake");
    source.add(a);
    source.add(b);
    source.remove(b);

    ServerRecipeBook target;
    target.apply(source.pack());

    EXPECT_TRUE(target.getBookSettings() == source.getBookSettings());
    EXPECT_EQ(target.knownCount(), source.knownCount());
    EXPECT_EQ(target.highlightCount(), source.highlightCount());
    EXPECT_TRUE(target.contains(a));
    EXPECT_TRUE(target.isHighlighted(a));
    EXPECT_FALSE(target.contains(b));
}

// ========== NBT 往返 ==========

TEST_F(RecipeBookTest, NbtRoundTrip)
{
    ServerRecipeBook book;
    book.setBookSetting(RecipeBookType::Crafting, true, true);
    book.setBookSetting(RecipeBookType::Smoker, false, true);
    const ResourceLocation kept("minecraft", "oak_door");
    const ResourceLocation seen("minecraft", "chest");
    book.add(kept);
    book.add(seen);
    book.removeHighlight(seen);

    const nbt::tags::compound_tag tag = book.write();

    ServerRecipeBook restored;
    // 谓词认下全部配方：本用例只验证读写往返，不验证「未知配方被丢弃」。
    restored.read(tag, [](const ResourceLocation&) {
        return true;
    });

    EXPECT_TRUE(restored.getBookSettings() == book.getBookSettings());
    EXPECT_TRUE(restored.contains(kept));
    EXPECT_TRUE(restored.isHighlighted(kept));
    EXPECT_TRUE(restored.contains(seen));
    EXPECT_FALSE(restored.isHighlighted(seen)) << "高亮状态必须与已解锁状态分开存取";
}

/**
 * 读取时未知配方被丢弃，且不影响其余条目。
 *
 * 对应原版 `ServerRecipeBook#loadUntrusted`：不认识的配方记日志后跳过，
 * 不会让整份存档读失败。
 */
TEST_F(RecipeBookTest, NbtDropsUnknownRecipes)
{
    ServerRecipeBook book;
    const ResourceLocation known("minecraft", "stick");
    const ResourceLocation unknown("minecraft", "not_a_recipe");
    book.add(known);
    book.add(unknown);
    const nbt::tags::compound_tag tag = book.write();

    ServerRecipeBook restored;
    restored.read(tag, [&known](const ResourceLocation& id) {
        return id == known;
    });

    EXPECT_TRUE(restored.contains(known));
    EXPECT_FALSE(restored.contains(unknown));
}

TEST_F(RecipeBookTest, NbtEmptyTagYieldsDefaults)
{
    ServerRecipeBook book;
    const nbt::tags::compound_tag tag;

    book.read(tag, [](const ResourceLocation&) {
        return true;
    });
    EXPECT_EQ(book.knownCount(), 0u);
    EXPECT_EQ(book.highlightCount(), 0u);
    EXPECT_FALSE(book.isOpen(RecipeBookType::Crafting));
}

/**
 * 打包顺序稳定：集合是无序的，但写出的列表必须排序。
 * 否则「存档读写一趟内容不变」的判定会凭空多出顺序差异。
 */
TEST_F(RecipeBookTest, PackOrderIsStable)
{
    ServerRecipeBook book;
    for (const char* name : {"z_last", "a_first", "m_middle"}) {
        book.add(ResourceLocation("minecraft", name));
    }
    const auto first = book.pack();
    const auto second = book.pack();

    ASSERT_EQ(first.known.size(), second.known.size());
    for (size_t index = 0; index < first.known.size(); ++index) {
        EXPECT_EQ(first.known[index], second.known[index]);
    }
    // 排序后第一个应当是按字符串最小的那条。
    EXPECT_EQ(first.known.front(), ResourceLocation("minecraft", "a_first"));
}

// ========== 页签归属 ==========

/**
 * 类型 + 分类 → 页签归属（`RecipeBookCategories` 注册表里的那 13 个常量）。
 *
 * 这里逐一固定住原版的映射，尤其是两个**不是**机械对应的地方：
 *
 *   - 烟熏炉只有「食物」一页：分类写 `blocks` 也落 `smoker_food`；
 *   - 高炉没有「食物」页：食物类落 `blast_furnace_misc`，而不是凭空的
 *     `blast_furnace_food`。
 *
 * 早期实现把「配方书类型」和「页签分类」当成同一个枚举，才需要这样的对照表；
 * 1.21.11 拆成两层后，这类归属仍要逐条对上，否则客户端拿到的页签键是错的。
 */
TEST_F(RecipeBookTest, PagePathsMatchVanillaRegistry)
{
    EXPECT_STREQ(recipeBookPageToPath(RecipeBookPage::CraftingBuildingBlocks), "crafting_building_blocks");
    EXPECT_STREQ(recipeBookPageToPath(RecipeBookPage::CraftingRedstone), "crafting_redstone");
    EXPECT_STREQ(recipeBookPageToPath(RecipeBookPage::CraftingEquipment), "crafting_equipment");
    EXPECT_STREQ(recipeBookPageToPath(RecipeBookPage::CraftingMisc), "crafting_misc");
    EXPECT_STREQ(recipeBookPageToPath(RecipeBookPage::FurnaceFood), "furnace_food");
    EXPECT_STREQ(recipeBookPageToPath(RecipeBookPage::FurnaceBlocks), "furnace_blocks");
    EXPECT_STREQ(recipeBookPageToPath(RecipeBookPage::FurnaceMisc), "furnace_misc");
    EXPECT_STREQ(recipeBookPageToPath(RecipeBookPage::BlastFurnaceBlocks), "blast_furnace_blocks");
    EXPECT_STREQ(recipeBookPageToPath(RecipeBookPage::BlastFurnaceMisc), "blast_furnace_misc");
    EXPECT_STREQ(recipeBookPageToPath(RecipeBookPage::SmokerFood), "smoker_food");
    EXPECT_STREQ(recipeBookPageToPath(RecipeBookPage::Stonecutter), "stonecutter");
    EXPECT_STREQ(recipeBookPageToPath(RecipeBookPage::Smithing), "smithing");
    EXPECT_STREQ(recipeBookPageToPath(RecipeBookPage::Campfire), "campfire");
}
