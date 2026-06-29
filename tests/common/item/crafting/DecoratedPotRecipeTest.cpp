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
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "common/item/crafting/special/DecoratedPotRecipe.hpp"
#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/world/block/registry/TrailsBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/interactive/DecoratedPotBlockEntity.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::crafting;
using namespace mc::blockentity;

// ========== DecoratedPotRecipe 测试夹具 ==========

class DecoratedPotRecipeTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 初始化顺序：方块 -> 物品 -> 方块物品 -> 物品标签
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
        item::tag::ItemTags::initialize();
    }

    void SetUp() override
    {
        recipe = std::make_unique<DecoratedPotRecipe>(ResourceLocation("minecraft", "decorated_pot"));
    }

    std::unique_ptr<DecoratedPotRecipe> recipe;
};

// ========== 基本属性测试 ==========

TEST_F(DecoratedPotRecipeTest, Construction)
{
    EXPECT_EQ(recipe->getId().toString(), "minecraft:decorated_pot");
    EXPECT_EQ(recipe->getId().namespace_(), "minecraft");
    EXPECT_EQ(recipe->getId().path(), "decorated_pot");
}

TEST_F(DecoratedPotRecipeTest, RecipeType)
{
    EXPECT_EQ(recipe->getType(), RecipeType::Special);
}

TEST_F(DecoratedPotRecipeTest, IsSpecial)
{
    EXPECT_TRUE(recipe->isSpecial());
}

TEST_F(DecoratedPotRecipeTest, IsDynamic)
{
    EXPECT_TRUE(recipe->isDynamic());
}

TEST_F(DecoratedPotRecipeTest, GetResultItemIsEmpty)
{
    // 特殊配方的结果是动态生成的，静态结果应为空
    EXPECT_TRUE(recipe->getResultItem().isEmpty());
}

TEST_F(DecoratedPotRecipeTest, GetIngredientsIsEmpty)
{
    EXPECT_TRUE(recipe->getIngredients().empty());
}

TEST_F(DecoratedPotRecipeTest, CanFitInAlwaysReturnsTrue)
{
    EXPECT_TRUE(recipe->canFitIn(1, 1));
    EXPECT_TRUE(recipe->canFitIn(2, 2));
    EXPECT_TRUE(recipe->canFitIn(3, 3));
}

// ========== 匹配测试 ==========

TEST_F(DecoratedPotRecipeTest, Matches_FourBricksInCross)
{
    // 十字形排列4个砖块应该匹配
    CraftingInventory grid(3, 3);
    grid.setItemAt(1, 0, ItemStack(Items::BRICKS, 1)); // back
    grid.setItemAt(0, 1, ItemStack(Items::BRICKS, 1)); // left
    grid.setItemAt(2, 1, ItemStack(Items::BRICKS, 1)); // right
    grid.setItemAt(1, 2, ItemStack(Items::BRICKS, 1)); // front

    EXPECT_TRUE(recipe->matches(grid));
}

TEST_F(DecoratedPotRecipeTest, Matches_FourSherdsInCross)
{
    // 十字形排列4个陶片应该匹配
    CraftingInventory grid(3, 3);
    grid.setItemAt(1, 0, ItemStack(Items::ANGLER_POTTERY_SHERD, 1)); // back
    grid.setItemAt(0, 1, ItemStack(Items::HEART_POTTERY_SHERD, 1));  // left
    grid.setItemAt(2, 1, ItemStack(Items::SKULL_POTTERY_SHERD, 1));  // right
    grid.setItemAt(1, 2, ItemStack(Items::PRIZE_POTTERY_SHERD, 1));  // front

    EXPECT_TRUE(recipe->matches(grid));
}

TEST_F(DecoratedPotRecipeTest, Matches_MixedBricksAndSherds)
{
    // 混合砖块和陶片应该匹配
    CraftingInventory grid(3, 3);
    grid.setItemAt(1, 0, ItemStack(Items::BRICKS, 1));               // back
    grid.setItemAt(0, 1, ItemStack(Items::ANGLER_POTTERY_SHERD, 1)); // left
    grid.setItemAt(2, 1, ItemStack(Items::BRICKS, 1));               // right
    grid.setItemAt(1, 2, ItemStack(Items::HEART_POTTERY_SHERD, 1));  // front

    EXPECT_TRUE(recipe->matches(grid));
}

TEST_F(DecoratedPotRecipeTest, NoMatch_2x2Grid)
{
    // 2x2网格不匹配
    CraftingInventory grid(2, 2);
    grid.setItemAt(0, 0, ItemStack(Items::BRICKS, 1));
    grid.setItemAt(1, 0, ItemStack(Items::BRICKS, 1));
    grid.setItemAt(0, 1, ItemStack(Items::BRICKS, 1));
    grid.setItemAt(1, 1, ItemStack(Items::BRICKS, 1));

    EXPECT_FALSE(recipe->matches(grid));
}

TEST_F(DecoratedPotRecipeTest, NoMatch_MissingPosition)
{
    // 缺少某个位置的物品不匹配
    CraftingInventory grid(3, 3);
    grid.setItemAt(1, 0, ItemStack(Items::BRICKS, 1)); // back
    grid.setItemAt(0, 1, ItemStack(Items::BRICKS, 1)); // left
    grid.setItemAt(2, 1, ItemStack(Items::BRICKS, 1)); // right
    // front (1,2) 缺失

    EXPECT_FALSE(recipe->matches(grid));
}

TEST_F(DecoratedPotRecipeTest, NoMatch_ExtraItemInCorner)
{
    // 角落有额外物品不匹配
    CraftingInventory grid(3, 3);
    grid.setItemAt(1, 0, ItemStack(Items::BRICKS, 1)); // back
    grid.setItemAt(0, 1, ItemStack(Items::BRICKS, 1)); // left
    grid.setItemAt(2, 1, ItemStack(Items::BRICKS, 1)); // right
    grid.setItemAt(1, 2, ItemStack(Items::BRICKS, 1)); // front
    grid.setItemAt(0, 0, ItemStack(Items::BRICKS, 1)); // 左上角多余物品

    EXPECT_FALSE(recipe->matches(grid));
}

TEST_F(DecoratedPotRecipeTest, NoMatch_ExtraItemInCenter)
{
    // 中心有额外物品不匹配
    CraftingInventory grid(3, 3);
    grid.setItemAt(1, 0, ItemStack(Items::BRICKS, 1)); // back
    grid.setItemAt(0, 1, ItemStack(Items::BRICKS, 1)); // left
    grid.setItemAt(2, 1, ItemStack(Items::BRICKS, 1)); // right
    grid.setItemAt(1, 2, ItemStack(Items::BRICKS, 1)); // front
    grid.setItemAt(1, 1, ItemStack(Items::BRICKS, 1)); // 中心多余物品

    EXPECT_FALSE(recipe->matches(grid));
}

TEST_F(DecoratedPotRecipeTest, NoMatch_NonIngredientItem)
{
    // 十字形排列但包含非 DECORATED_POT_INGREDIENTS 物品不匹配
    CraftingInventory grid(3, 3);
    grid.setItemAt(1, 0, ItemStack(Items::BRICKS, 1));               // back
    grid.setItemAt(0, 1, ItemStack(Items::ANGLER_POTTERY_SHERD, 1)); // left
    grid.setItemAt(2, 1, ItemStack(Items::STONE, 1));                // right - 石头不是陶罐原料
    grid.setItemAt(1, 2, ItemStack(Items::BRICKS, 1));               // front

    EXPECT_FALSE(recipe->matches(grid));
}

TEST_F(DecoratedPotRecipeTest, NoMatch_EmptyGrid)
{
    CraftingInventory grid(3, 3);
    EXPECT_FALSE(recipe->matches(grid));
}

// ========== 组装测试 ==========

TEST_F(DecoratedPotRecipeTest, Assemble_FourBricks)
{
    // 需要 DecoratedPot 方块物品注册才能生成结果
    if (block_registry::TrailsBlocks::DECORATED_POT == nullptr ||
        BlockItemRegistry::instance().getBlockItem(*block_registry::TrailsBlocks::DECORATED_POT) == nullptr) {
        GTEST_SKIP() << "DecoratedPot block item not registered in test environment";
    }

    // 4个砖块应该产生全部为 Blank 图案的陶罐
    CraftingInventory grid(3, 3);
    grid.setItemAt(1, 0, ItemStack(Items::BRICKS, 1)); // back
    grid.setItemAt(0, 1, ItemStack(Items::BRICKS, 1)); // left
    grid.setItemAt(2, 1, ItemStack(Items::BRICKS, 1)); // right
    grid.setItemAt(1, 2, ItemStack(Items::BRICKS, 1)); // front

    ItemStack result = recipe->assemble(grid);
    ASSERT_FALSE(result.isEmpty());

    // 验证结果物品的 BlockEntityTag 包含 sherds 数据
    const nlohmann::json* tag = result.getChildTag("BlockEntityTag");
    ASSERT_NE(tag, nullptr);
    ASSERT_TRUE(tag->contains("sherds"));
    ASSERT_TRUE((*tag)["sherds"].is_array());
    EXPECT_EQ((*tag)["sherds"].size(), 4);

    // 4个砖块对应的都是 minecraft:brick
    for (const auto& sherd : (*tag)["sherds"]) {
        EXPECT_EQ(sherd.get<std::string>(), "minecraft:brick");
    }
}

TEST_F(DecoratedPotRecipeTest, Assemble_MixedBricksAndSherds)
{
    if (block_registry::TrailsBlocks::DECORATED_POT == nullptr ||
        BlockItemRegistry::instance().getBlockItem(*block_registry::TrailsBlocks::DECORATED_POT) == nullptr) {
        GTEST_SKIP() << "DecoratedPot block item not registered in test environment";
    }

    // 混合砖块和陶片应该产生对应图案的陶罐
    CraftingInventory grid(3, 3);
    grid.setItemAt(1, 0, ItemStack(Items::BRICKS, 1));               // back = Blank
    grid.setItemAt(0, 1, ItemStack(Items::ANGLER_POTTERY_SHERD, 1)); // left = Angler
    grid.setItemAt(2, 1, ItemStack(Items::BRICKS, 1));               // right = Blank
    grid.setItemAt(1, 2, ItemStack(Items::HEART_POTTERY_SHERD, 1));  // front = Heart

    ItemStack result = recipe->assemble(grid);
    ASSERT_FALSE(result.isEmpty());

    const nlohmann::json* tag = result.getChildTag("BlockEntityTag");
    ASSERT_NE(tag, nullptr);
    ASSERT_TRUE(tag->contains("sherds"));
    ASSERT_TRUE((*tag)["sherds"].is_array());
    EXPECT_EQ((*tag)["sherds"].size(), 4);

    // back=brick, left=angler, right=brick, front=heart
    EXPECT_EQ((*tag)["sherds"][0].get<std::string>(), "minecraft:brick");
    EXPECT_EQ((*tag)["sherds"][1].get<std::string>(), "minecraft:angler_pottery_sherd");
    EXPECT_EQ((*tag)["sherds"][2].get<std::string>(), "minecraft:brick");
    EXPECT_EQ((*tag)["sherds"][3].get<std::string>(), "minecraft:heart_pottery_sherd");
}

TEST_F(DecoratedPotRecipeTest, Assemble_FourDifferentSherds)
{
    if (block_registry::TrailsBlocks::DECORATED_POT == nullptr ||
        BlockItemRegistry::instance().getBlockItem(*block_registry::TrailsBlocks::DECORATED_POT) == nullptr) {
        GTEST_SKIP() << "DecoratedPot block item not registered in test environment";
    }

    // 4个不同陶片
    CraftingInventory grid(3, 3);
    grid.setItemAt(1, 0, ItemStack(Items::ANGLER_POTTERY_SHERD, 1));  // back = Angler
    grid.setItemAt(0, 1, ItemStack(Items::ARCHER_POTTERY_SHERD, 1));  // left = Archer
    grid.setItemAt(2, 1, ItemStack(Items::ARMS_UP_POTTERY_SHERD, 1)); // right = ArmsUp
    grid.setItemAt(1, 2, ItemStack(Items::BLADE_POTTERY_SHERD, 1));   // front = Blade

    ItemStack result = recipe->assemble(grid);
    ASSERT_FALSE(result.isEmpty());

    const nlohmann::json* tag = result.getChildTag("BlockEntityTag");
    ASSERT_NE(tag, nullptr);
    ASSERT_TRUE(tag->contains("sherds"));

    EXPECT_EQ((*tag)["sherds"][0].get<std::string>(), "minecraft:angler_pottery_sherd");
    EXPECT_EQ((*tag)["sherds"][1].get<std::string>(), "minecraft:archer_pottery_sherd");
    EXPECT_EQ((*tag)["sherds"][2].get<std::string>(), "minecraft:arms_up_pottery_sherd");
    EXPECT_EQ((*tag)["sherds"][3].get<std::string>(), "minecraft:blade_pottery_sherd");
}

TEST_F(DecoratedPotRecipeTest, Assemble_SherdPositionsMapCorrectly)
{
    if (block_registry::TrailsBlocks::DECORATED_POT == nullptr ||
        BlockItemRegistry::instance().getBlockItem(*block_registry::TrailsBlocks::DECORATED_POT) == nullptr) {
        GTEST_SKIP() << "DecoratedPot block item not registered in test environment";
    }

    // 验证位置映射：back=(1,0), left=(0,1), right=(2,1), front=(1,2)
    // back 对应 sherds[0], left 对应 sherds[1], right 对应 sherds[2], front 对应 sherds[3]
    CraftingInventory grid(3, 3);
    grid.setItemAt(1, 0, ItemStack(Items::ANGLER_POTTERY_SHERD, 1)); // back -> sherds[0]
    grid.setItemAt(0, 1, ItemStack(Items::HEART_POTTERY_SHERD, 1));  // left -> sherds[1]
    grid.setItemAt(2, 1, ItemStack(Items::SKULL_POTTERY_SHERD, 1));  // right -> sherds[2]
    grid.setItemAt(1, 2, ItemStack(Items::PRIZE_POTTERY_SHERD, 1));  // front -> sherds[3]

    ItemStack result = recipe->assemble(grid);
    ASSERT_FALSE(result.isEmpty());

    const nlohmann::json* tag = result.getChildTag("BlockEntityTag");
    ASSERT_NE(tag, nullptr);

    EXPECT_EQ((*tag)["sherds"][0].get<std::string>(), "minecraft:angler_pottery_sherd"); // back
    EXPECT_EQ((*tag)["sherds"][1].get<std::string>(), "minecraft:heart_pottery_sherd");  // left
    EXPECT_EQ((*tag)["sherds"][2].get<std::string>(), "minecraft:skull_pottery_sherd");  // right
    EXPECT_EQ((*tag)["sherds"][3].get<std::string>(), "minecraft:prize_pottery_sherd");  // front
}

// ========== 剩余物品测试 ==========

TEST_F(DecoratedPotRecipeTest, GetRemainingItems_AllConsumed)
{
    // 所有输入物品都被消耗，没有剩余
    CraftingInventory grid(3, 3);
    grid.setItemAt(1, 0, ItemStack(Items::BRICKS, 1));
    grid.setItemAt(0, 1, ItemStack(Items::BRICKS, 1));
    grid.setItemAt(2, 1, ItemStack(Items::BRICKS, 1));
    grid.setItemAt(1, 2, ItemStack(Items::BRICKS, 1));

    auto remaining = recipe->getRemainingItems(grid);
    EXPECT_EQ(static_cast<i32>(remaining.size()), grid.getContainerSize());

    // 所有剩余物品都应为空
    for (const auto& stack : remaining) {
        EXPECT_TRUE(stack.isEmpty());
    }
}

// ========== 边界情况测试 ==========

TEST_F(DecoratedPotRecipeTest, NoMatch_WrongPositions)
{
    // 4个砖块但不在十字形位置不匹配
    CraftingInventory grid(3, 3);
    grid.setItemAt(0, 0, ItemStack(Items::BRICKS, 1)); // 左上角
    grid.setItemAt(1, 0, ItemStack(Items::BRICKS, 1)); // 上中
    grid.setItemAt(2, 0, ItemStack(Items::BRICKS, 1)); // 右上角
    grid.setItemAt(1, 1, ItemStack(Items::BRICKS, 1)); // 中心

    EXPECT_FALSE(recipe->matches(grid));
}

TEST_F(DecoratedPotRecipeTest, Assemble_ResultItemIsDecoratedPot)
{
    if (block_registry::TrailsBlocks::DECORATED_POT == nullptr ||
        BlockItemRegistry::instance().getBlockItem(*block_registry::TrailsBlocks::DECORATED_POT) == nullptr) {
        GTEST_SKIP() << "DecoratedPot block item not registered in test environment";
    }

    // 验证结果物品是饰纹陶罐
    CraftingInventory grid(3, 3);
    grid.setItemAt(1, 0, ItemStack(Items::BRICKS, 1));
    grid.setItemAt(0, 1, ItemStack(Items::BRICKS, 1));
    grid.setItemAt(2, 1, ItemStack(Items::BRICKS, 1));
    grid.setItemAt(1, 2, ItemStack(Items::BRICKS, 1));

    ItemStack result = recipe->assemble(grid);
    ASSERT_FALSE(result.isEmpty());
    EXPECT_EQ(result.getCount(), 1);

    // 结果物品应该有 BlockEntityTag 且 id 为 minecraft:decorated_pot
    const nlohmann::json* tag = result.getChildTag("BlockEntityTag");
    ASSERT_NE(tag, nullptr);
    ASSERT_TRUE(tag->contains("id"));
    EXPECT_EQ((*tag)["id"].get<std::string>(), "minecraft:decorated_pot");
}
