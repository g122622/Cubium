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

#include "item/crafting/Ingredient.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/core/ItemStack.hpp"
#include "item/items/block/BlockItemRegistry.hpp"
#include "item/tag/ItemTags.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::crafting;

// ============================================================================
// 基础 Ingredient 测试（不需要标签系统初始化）
// ============================================================================

class IngredientBasicTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(IngredientBasicTest, EmptyIngredient_IsEmpty)
{
    Ingredient ing;
    EXPECT_TRUE(ing.isEmpty());
    EXPECT_TRUE(ing.getMatchingStacks().empty());
    EXPECT_TRUE(ing.isSimple());
    EXPECT_FALSE(ing.hasTag());
}

TEST_F(IngredientBasicTest, EmptyIngredient_MatchesEmptyStack)
{
    Ingredient ing;

    // 空 Ingredient 匹配空物品堆
    ItemStack emptyStack;
    EXPECT_TRUE(ing.test(emptyStack));

    // 空 Ingredient 不匹配非空物品堆
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    if (stone) {
        ItemStack nonEmptyStack(*stone, 1);
        EXPECT_FALSE(ing.test(nonEmptyStack));
    }
}

TEST_F(IngredientBasicTest, EmptyStaticConst_IsEmpty)
{
    EXPECT_TRUE(Ingredient::EMPTY.isEmpty());
    EXPECT_TRUE(Ingredient::EMPTY.getMatchingStacks().empty());
}

TEST_F(IngredientBasicTest, NullItemPointer_ReturnsEmptyIngredient)
{
    Ingredient ing = Ingredient::fromItem(static_cast<const Item*>(nullptr));
    EXPECT_TRUE(ing.isEmpty());
}

// ============================================================================
// 需要物品注册表的 Ingredient 测试
// ============================================================================

class IngredientTest : public ::testing::Test {
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

// ========== 单物品 Ingredient 测试 ==========

TEST_F(IngredientTest, FromSingleItem_HasMatchingStacks)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);

    Ingredient ing = Ingredient::fromItem(*stone);

    EXPECT_FALSE(ing.isEmpty());
    EXPECT_EQ(ing.getMatchingStacks().size(), 1u);
    EXPECT_TRUE(ing.isSimple()); // 石头不可损坏
}

TEST_F(IngredientTest, FromSingleItem_MatchesCorrectItem)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);

    Ingredient ing = Ingredient::fromItem(*stone);

    // 匹配正确的物品
    ItemStack stack(*stone, 1);
    EXPECT_TRUE(ing.test(stack));

    // 匹配任意数量
    ItemStack stack64(*stone, 64);
    EXPECT_TRUE(ing.test(stack64));

    // 匹配物品指针
    EXPECT_TRUE(ing.test(stone));
}

TEST_F(IngredientTest, FromSingleItem_DoesNotMatchOtherItem)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    Item* dirt = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dirt"));
    ASSERT_NE(stone, nullptr);
    ASSERT_NE(dirt, nullptr);

    Ingredient ing = Ingredient::fromItem(*stone);

    ItemStack dirtStack(*dirt, 1);
    EXPECT_FALSE(ing.test(dirtStack));
    EXPECT_FALSE(ing.test(dirt));
}

TEST_F(IngredientTest, FromSingleItem_DoesNotMatchEmptyStack)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);

    Ingredient ing = Ingredient::fromItem(*stone);

    ItemStack emptyStack;
    EXPECT_FALSE(ing.test(emptyStack));
}

TEST_F(IngredientTest, FromSingleItem_NullPointer_ReturnsEmpty)
{
    Ingredient ing = Ingredient::fromItem(static_cast<const Item*>(nullptr));
    EXPECT_TRUE(ing.isEmpty());
}

// ========== 多物品 Ingredient 测试 ==========

TEST_F(IngredientTest, FromMultipleItems_MatchesAllItems)
{
    Item* oakPlanks = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "oak_planks"));
    Item* sprucePlanks = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "spruce_planks"));
    Item* birchPlanks = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "birch_planks"));

    ASSERT_NE(oakPlanks, nullptr);
    ASSERT_NE(sprucePlanks, nullptr);
    ASSERT_NE(birchPlanks, nullptr);

    Ingredient ing = Ingredient::fromItems({oakPlanks, sprucePlanks, birchPlanks});

    EXPECT_FALSE(ing.isEmpty());
    EXPECT_EQ(ing.getMatchingStacks().size(), 3u);
    EXPECT_TRUE(ing.test(*oakPlanks));
    EXPECT_TRUE(ing.test(*sprucePlanks));
    EXPECT_TRUE(ing.test(*birchPlanks));
}

TEST_F(IngredientTest, FromMultipleItems_DoesNotMatchOtherItems)
{
    Item* oakPlanks = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "oak_planks"));
    Item* sprucePlanks = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "spruce_planks"));
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));

    ASSERT_NE(oakPlanks, nullptr);
    ASSERT_NE(sprucePlanks, nullptr);
    ASSERT_NE(stone, nullptr);

    Ingredient ing = Ingredient::fromItems({oakPlanks, sprucePlanks});

    EXPECT_FALSE(ing.test(*stone));
}

TEST_F(IngredientTest, FromEmptyItemList_ReturnsEmptyIngredient)
{
    std::vector<const Item*> emptyItems;
    Ingredient ing = Ingredient::fromItems(emptyItems);

    EXPECT_TRUE(ing.isEmpty());
}

// ========== 物品堆 Ingredient 测试 ==========

TEST_F(IngredientTest, FromStacks_MatchesAllStacks)
{
    Item* oakPlanks = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "oak_planks"));
    Item* sprucePlanks = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "spruce_planks"));

    ASSERT_NE(oakPlanks, nullptr);
    ASSERT_NE(sprucePlanks, nullptr);

    std::vector<ItemStack> stacks = {ItemStack(*oakPlanks, 1), ItemStack(*sprucePlanks, 1)};

    Ingredient ing = Ingredient::fromStacks(stacks);

    EXPECT_FALSE(ing.isEmpty());
    EXPECT_EQ(ing.getMatchingStacks().size(), 2u);
    EXPECT_TRUE(ing.test(*oakPlanks));
    EXPECT_TRUE(ing.test(*sprucePlanks));
}

// ========== 标签 Ingredient 测试 ==========

TEST_F(IngredientTest, FromTag_HasTagFlag)
{
    Ingredient ing = Ingredient::fromTag("minecraft:flowers");

    EXPECT_TRUE(ing.hasTag());
    EXPECT_EQ(ing.getTag(), "minecraft:flowers");
}

TEST_F(IngredientTest, FromTag_ResolvedTag_MatchesTagItems)
{
    // FLOWERS 标签已通过 ItemTags::initialize() 注册
    Ingredient ing = Ingredient::fromTag("minecraft:flowers");

    Item* dandelion = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dandelion"));
    ASSERT_NE(dandelion, nullptr);

    // 标签Ingredient应该匹配标签中的物品
    EXPECT_TRUE(ing.test(*dandelion));
}

TEST_F(IngredientTest, FromTag_ResolvedTag_DoesNotMatchNonTagItems)
{
    Ingredient ing = Ingredient::fromTag("minecraft:flowers");

    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);

    EXPECT_FALSE(ing.test(*stone));
}

TEST_F(IngredientTest, FromTag_UnresolvedTag_HasNoMatchingItems)
{
    // 使用不存在的标签
    Ingredient ing = Ingredient::fromTag("minecraft:nonexistent_tag");

    EXPECT_TRUE(ing.hasTag());
    EXPECT_TRUE(ing.hasNoMatchingItems());
    // 未解析的标签Ingredient应视为非简单
    EXPECT_FALSE(ing.isSimple());
}

TEST_F(IngredientTest, FromTag_ResolvedSimpleTag_IsSimple)
{
    // FLOWERS 标签中都是不可损坏的花朵，所以应该isSimple为true
    Ingredient ing = Ingredient::fromTag("minecraft:flowers");

    // 标签已解析且所有物品都不可损坏
    EXPECT_TRUE(ing.isSimple());
}

TEST_F(IngredientTest, FromTag_ResolvedTag_GetAllMatchingItems)
{
    Ingredient ing = Ingredient::fromTag("minecraft:flowers");

    auto items = ing.getAllMatchingItems();
    // FLOWERS 标签应该包含多个花朵
    EXPECT_GE(items.size(), 10u);
}

TEST_F(IngredientTest, FromTag_CarpetsTag)
{
    Ingredient ing = Ingredient::fromTag("minecraft:carpets");

    EXPECT_TRUE(ing.hasTag());
    EXPECT_EQ(ing.getTag(), "minecraft:carpets");

    Item* whiteCarpet = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "white_carpet"));
    ASSERT_NE(whiteCarpet, nullptr);

    EXPECT_TRUE(ing.test(*whiteCarpet));

    // 地毯都不可损坏
    EXPECT_TRUE(ing.isSimple());
}

// ========== isSimple 测试 ==========

TEST_F(IngredientTest, IsSimple_WithNonDamageableItem)
{
    // 石头不可损坏
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);

    Ingredient ing = Ingredient::fromItem(*stone);
    EXPECT_TRUE(ing.isSimple());
}

TEST_F(IngredientTest, IsSimple_WithDamageableItem)
{
    // 钻石镐可损坏（TieredItem 会设置 maxDamage）
    Item* diamondPickaxe = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond_pickaxe"));
    if (!diamondPickaxe || !diamondPickaxe->isDamageable()) {
        GTEST_SKIP() << "Damageable item not available, skipping test";
    }

    Ingredient ing = Ingredient::fromItem(*diamondPickaxe);
    EXPECT_FALSE(ing.isSimple());
}

TEST_F(IngredientTest, IsSimple_WithMixedItems)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    Item* diamondPickaxe = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond_pickaxe"));

    ASSERT_NE(stone, nullptr);
    if (!diamondPickaxe || !diamondPickaxe->isDamageable()) {
        GTEST_SKIP() << "Diamond pickaxe not registered, skipping test";
    }

    // 混合可损坏和不可损坏物品
    Ingredient ing = Ingredient::fromItems({stone, diamondPickaxe});
    EXPECT_FALSE(ing.isSimple());
}

TEST_F(IngredientTest, IsSimple_EmptyIngredient)
{
    Ingredient ing;
    EXPECT_TRUE(ing.isSimple());
}

// ========== hasNoMatchingItems 测试 ==========

TEST_F(IngredientTest, HasNoMatchingItems_EmptyIngredient)
{
    Ingredient ing;
    EXPECT_TRUE(ing.hasNoMatchingItems());
}

TEST_F(IngredientTest, HasNoMatchingItems_WithItem)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);

    Ingredient ing = Ingredient::fromItem(*stone);
    EXPECT_FALSE(ing.hasNoMatchingItems());
}

TEST_F(IngredientTest, HasNoMatchingItems_WithResolvedTag)
{
    Ingredient ing = Ingredient::fromTag("minecraft:flowers");
    EXPECT_FALSE(ing.hasNoMatchingItems());
}

TEST_F(IngredientTest, HasNoMatchingItems_WithUnresolvedTag)
{
    Ingredient ing = Ingredient::fromTag("minecraft:nonexistent_tag");
    EXPECT_TRUE(ing.hasNoMatchingItems());
}

// ========== merge 测试 ==========

TEST_F(IngredientTest, Merge_TwoItemIngredients)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    Item* dirt = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dirt"));

    ASSERT_NE(stone, nullptr);
    ASSERT_NE(dirt, nullptr);

    Ingredient ing1 = Ingredient::fromItem(*stone);
    Ingredient ing2 = Ingredient::fromItem(*dirt);

    Ingredient merged = Ingredient::merge({ing1, ing2});

    EXPECT_FALSE(merged.isEmpty());
    EXPECT_TRUE(merged.test(*stone));
    EXPECT_TRUE(merged.test(*dirt));
    EXPECT_TRUE(merged.isSimple()); // 两者都不可损坏
}

TEST_F(IngredientTest, Merge_WithDuplicateItems)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));

    ASSERT_NE(stone, nullptr);

    Ingredient ing1 = Ingredient::fromItem(*stone);
    Ingredient ing2 = Ingredient::fromItem(*stone);

    Ingredient merged = Ingredient::merge({ing1, ing2});

    // 去重后应该只有一个物品
    EXPECT_EQ(merged.getMatchingStacks().size(), 1u);
}

TEST_F(IngredientTest, Merge_ItemWithTagIngredient)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);

    Ingredient itemIng = Ingredient::fromItem(*stone);
    Ingredient tagIng = Ingredient::fromTag("minecraft:flowers");

    Ingredient merged = Ingredient::merge({itemIng, tagIng});

    // 应该匹配石头
    EXPECT_TRUE(merged.test(*stone));
    // 应该匹配花朵标签中的物品
    Item* dandelion = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dandelion"));
    ASSERT_NE(dandelion, nullptr);
    EXPECT_TRUE(merged.test(*dandelion));
}

TEST_F(IngredientTest, Merge_EmptyIngredients)
{
    Ingredient empty;
    Ingredient empty2;

    Ingredient merged = Ingredient::merge({empty, empty2});
    EXPECT_TRUE(merged.isEmpty());
}

TEST_F(IngredientTest, Merge_SingleIngredient)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);

    Ingredient ing = Ingredient::fromItem(*stone);
    Ingredient merged = Ingredient::merge({ing});

    EXPECT_TRUE(merged.test(*stone));
}

TEST_F(IngredientTest, Merge_TwoTagIngredients)
{
    Ingredient flowers = Ingredient::fromTag("minecraft:flowers");
    Ingredient carpets = Ingredient::fromTag("minecraft:carpets");

    Ingredient merged = Ingredient::merge({flowers, carpets});

    // 应该匹配花朵
    Item* dandelion = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dandelion"));
    ASSERT_NE(dandelion, nullptr);
    EXPECT_TRUE(merged.test(*dandelion));

    // 应该匹配地毯
    Item* whiteCarpet = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "white_carpet"));
    ASSERT_NE(whiteCarpet, nullptr);
    EXPECT_TRUE(merged.test(*whiteCarpet));

    // 合并后不应该是标签类型（标签被展开为物品列表）
    EXPECT_FALSE(merged.hasTag());
}

// ========== getAllMatchingItems 测试 ==========

TEST_F(IngredientTest, GetAllMatchingItems_SingleItem)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);

    Ingredient ing = Ingredient::fromItem(*stone);
    auto items = ing.getAllMatchingItems();

    EXPECT_EQ(items.size(), 1u);
    EXPECT_EQ(items[0], stone);
}

TEST_F(IngredientTest, GetAllMatchingItems_TagIngredient)
{
    Ingredient ing = Ingredient::fromTag("minecraft:flowers");
    auto items = ing.getAllMatchingItems();

    // 应该包含花朵标签中的所有物品
    EXPECT_GE(items.size(), 10u);
}

TEST_F(IngredientTest, GetAllMatchingItems_EmptyIngredient)
{
    Ingredient ing;
    auto items = ing.getAllMatchingItems();

    EXPECT_TRUE(items.empty());
}

// ========== 相等比较测试 ==========

TEST_F(IngredientTest, EqualIngredients_AreEqual)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);

    Ingredient ing1 = Ingredient::fromItem(*stone);
    Ingredient ing2 = Ingredient::fromItem(*stone);

    EXPECT_TRUE(ing1 == ing2);
    EXPECT_FALSE(ing1 != ing2);
}

TEST_F(IngredientTest, DifferentIngredients_AreNotEqual)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    Item* dirt = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dirt"));

    ASSERT_NE(stone, nullptr);
    ASSERT_NE(dirt, nullptr);

    Ingredient ing1 = Ingredient::fromItem(*stone);
    Ingredient ing2 = Ingredient::fromItem(*dirt);

    EXPECT_FALSE(ing1 == ing2);
    EXPECT_TRUE(ing1 != ing2);
}

TEST_F(IngredientTest, EmptyIngredients_AreEqual)
{
    Ingredient ing1;
    Ingredient ing2;

    EXPECT_TRUE(ing1 == ing2);
}

TEST_F(IngredientTest, TagIngredients_SameTag_AreEqual)
{
    Ingredient ing1 = Ingredient::fromTag("minecraft:flowers");
    Ingredient ing2 = Ingredient::fromTag("minecraft:flowers");

    EXPECT_TRUE(ing1 == ing2);
}

TEST_F(IngredientTest, TagIngredients_DifferentTag_AreNotEqual)
{
    Ingredient ing1 = Ingredient::fromTag("minecraft:flowers");
    Ingredient ing2 = Ingredient::fromTag("minecraft:carpets");

    EXPECT_FALSE(ing1 == ing2);
}

// ========== 哈希测试 ==========

TEST_F(IngredientTest, SameIngredients_HaveSameHash)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);

    Ingredient ing1 = Ingredient::fromItem(*stone);
    Ingredient ing2 = Ingredient::fromItem(*stone);

    EXPECT_EQ(ing1.hash(), ing2.hash());
}

TEST_F(IngredientTest, DifferentIngredients_HaveDifferentHash)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    Item* dirt = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dirt"));

    ASSERT_NE(stone, nullptr);
    ASSERT_NE(dirt, nullptr);

    Ingredient ing1 = Ingredient::fromItem(*stone);
    Ingredient ing2 = Ingredient::fromItem(*dirt);

    EXPECT_NE(ing1.hash(), ing2.hash());
}

TEST_F(IngredientTest, TagIngredients_SameTag_SameHash)
{
    Ingredient ing1 = Ingredient::fromTag("minecraft:flowers");
    Ingredient ing2 = Ingredient::fromTag("minecraft:flowers");

    EXPECT_EQ(ing1.hash(), ing2.hash());
}

// ========== 延迟解析测试 ==========

TEST_F(IngredientTest, DeferredResolution_UnresolvedTag_DoesNotMatchUntilResolved)
{
    // 创建一个不存在的标签Ingredient
    Ingredient ing = Ingredient::fromTag("minecraft:nonexistent_tag");

    // 未解析的标签不应匹配任何物品
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    EXPECT_FALSE(ing.test(*stone));

    // 标记为非简单（因为标签未解析）
    EXPECT_FALSE(ing.isSimple());
}

// ========== Item 指针匹配测试 ==========

TEST_F(IngredientTest, MatchesItemPointer)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    Item* dirt = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dirt"));

    ASSERT_NE(stone, nullptr);
    ASSERT_NE(dirt, nullptr);

    Ingredient ing = Ingredient::fromItem(*stone);

    EXPECT_TRUE(ing.test(stone));
    EXPECT_FALSE(ing.test(dirt));
    EXPECT_FALSE(ing.test(static_cast<const Item*>(nullptr)));
}

TEST_F(IngredientTest, TagIngredient_MatchesItemPointer)
{
    Ingredient ing = Ingredient::fromTag("minecraft:flowers");

    Item* dandelion = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dandelion"));
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));

    ASSERT_NE(dandelion, nullptr);
    ASSERT_NE(stone, nullptr);

    EXPECT_TRUE(ing.test(dandelion));
    EXPECT_FALSE(ing.test(stone));
}
