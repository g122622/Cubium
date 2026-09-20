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

#include "common/TestWorldHelper.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/entity/inventory/Slot.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/crafting/Ingredient.hpp"
#include "common/item/crafting/RecipeManager.hpp"
#include "common/item/crafting/ShapedRecipe.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "server/menu/CraftingMenu.hpp"

#include <memory>

using namespace mc;

namespace {

// 简单的测试配方类（用于测试 getCurrentRecipeId）
class TestRecipe : public crafting::CraftingRecipe {
public:
    TestRecipe(const ResourceLocation& id, std::vector<crafting::Ingredient> ingredients, ItemStack result)
        : m_id(id)
        , m_ingredients(std::move(ingredients))
        , m_result(std::move(result))
    {}

    bool matches(const CraftingInventory& inventory) const override
    {
        if (m_ingredients.empty()) {
            return inventory.isEmpty();
        }
        // 简化版：检查是否有足够匹配的槽位
        std::vector<bool> used(inventory.getContainerSize(), false);
        for (const crafting::Ingredient& ing : m_ingredients) {
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
        return crafting::RecipeUtils::getDefaultRemainingItems(inventory);
    }

    const std::vector<crafting::Ingredient>& getIngredients() const override { return m_ingredients; }

    ResourceLocation getId() const override { return m_id; }
    crafting::RecipeType getType() const override { return crafting::RecipeType::ShapelessCrafting; }

private:
    ResourceLocation m_id;
    std::vector<crafting::Ingredient> m_ingredients;
    ItemStack m_result;
};

class CraftingMenuTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 配方匹配按 Item* 比对，需要真实物品才能构造出「配料非空」的配方。
        Items::initialize();
    }

    void SetUp() override
    {
        m_player = std::make_unique<Player>(1, "MenuTester", mc::test::testEcsRegistry());
        m_playerInventory = std::make_unique<PlayerInventory>(m_player.get());
    }

    void TearDown() override { crafting::RecipeManager::instance().clear(); }

    std::unique_ptr<Player> m_player;
    std::unique_ptr<PlayerInventory> m_playerInventory;
};

TEST_F(CraftingMenuTest, StillValid_AlwaysReturnsTrueForPureContainerMenu)
{
    // vanilla 工作台不是方块实体，CraftingMenu 为纯容器菜单，stillValid 恒返回 true。
    CraftingMenu menu(1, m_playerInventory.get());
    m_player->setPosition(12.5f, 64.0f, 0.5f);

    EXPECT_TRUE(menu.stillValid(*m_player));
}

TEST_F(CraftingMenuTest, GetCurrentRecipeId_ReturnsEmptyWhenNoMatch)
{
    CraftingMenu menu(1, m_playerInventory.get());

    // 空网格，没有匹配的配方
    ResourceLocation recipeId = menu.getCurrentRecipeId();
    EXPECT_TRUE(recipeId.path().empty());
}

TEST_F(CraftingMenuTest, GetCurrentRecipeId_ReturnsRecipeIdWhenMatch)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr) << "Stone item not registered";

    // 配料用真实物品。不能用空配料（Ingredient()）代替「无配料要求」——空配料的语义是
    // 「该槽位必须为空」，那是 pattern 里空格占位符的语义，被它匹配的配方不是有效形态。
    auto recipe = std::make_unique<TestRecipe>(ResourceLocation("test", "test_recipe"),
        std::vector<crafting::Ingredient>{crafting::Ingredient::fromItem(*stone)},
        ItemStack(*stone, 1));
    crafting::RecipeManager::instance().registerRecipe(std::move(recipe));

    CraftingMenu menu(1, m_playerInventory.get());

    // 网格为空时不匹配任何配方：空网格永不产出结果。
    menu.updateResult();
    EXPECT_TRUE(menu.getCurrentRecipeId().path().empty());

    // 放上配料后匹配。
    menu.getCraftingGrid().setItem(0, ItemStack(*stone, 1));
    menu.updateResult();
    EXPECT_EQ(menu.getCurrentRecipeId().toString(), "test:test_recipe");
}

TEST_F(CraftingMenuTest, GetCurrentRecipeId_UpdatesAfterGridChange)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    Item* dirt = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dirt"));
    ASSERT_NE(stone, nullptr) << "Stone item not registered";
    ASSERT_NE(dirt, nullptr) << "Dirt item not registered";

    auto recipe = std::make_unique<TestRecipe>(ResourceLocation("test", "stone_recipe"),
        std::vector<crafting::Ingredient>{crafting::Ingredient::fromItem(*stone)},
        ItemStack(*stone, 1));
    crafting::RecipeManager::instance().registerRecipe(std::move(recipe));

    CraftingMenu menu(1, m_playerInventory.get());

    // 初始状态：网格为空，无匹配配方。
    menu.updateResult();
    EXPECT_TRUE(menu.getCurrentRecipeId().path().empty());

    // 放上配方要求的物品后匹配。
    menu.getCraftingGrid().setItem(0, ItemStack(*stone, 1));
    menu.updateResult();
    EXPECT_EQ(menu.getCurrentRecipeId().toString(), "test:stone_recipe");

    // 换成配方不接受的物品后不再匹配。
    menu.getCraftingGrid().setItem(0, ItemStack(*dirt, 1));
    menu.updateResult();
    EXPECT_TRUE(menu.getCurrentRecipeId().path().empty());
}

// InventoryCraftingMenu 测试类
class InventoryCraftingMenuTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { Items::initialize(); }

    void SetUp() override
    {
        m_player = std::make_unique<Player>(1, "InventoryMenuTester", mc::test::testEcsRegistry());
        m_playerInventory = std::make_unique<PlayerInventory>(m_player.get());
    }

    void TearDown() override { crafting::RecipeManager::instance().clear(); }

    std::unique_ptr<Player> m_player;
    std::unique_ptr<PlayerInventory> m_playerInventory;
};

TEST_F(InventoryCraftingMenuTest, GetCurrentRecipeId_ReturnsEmptyWhenNoMatch)
{
    InventoryCraftingMenu menu(1, m_playerInventory.get());

    // 空网格，没有匹配的配方
    ResourceLocation recipeId = menu.getCurrentRecipeId();
    EXPECT_TRUE(recipeId.path().empty());
}

TEST_F(InventoryCraftingMenuTest, GetCurrentRecipeId_ReturnsRecipeIdWhenMatch)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr) << "Stone item not registered";

    // 配料用真实物品，理由同 CraftingMenuTest 同名用例。
    auto recipe = std::make_unique<TestRecipe>(ResourceLocation("test", "test_2x2_recipe"),
        std::vector<crafting::Ingredient>{crafting::Ingredient::fromItem(*stone)},
        ItemStack(*stone, 1));
    crafting::RecipeManager::instance().registerRecipe(std::move(recipe));

    InventoryCraftingMenu menu(1, m_playerInventory.get());

    // 网格为空时不匹配。
    menu.updateResult();
    EXPECT_TRUE(menu.getCurrentRecipeId().path().empty());

    menu.getCraftingGrid().setItem(0, ItemStack(*stone, 1));
    menu.updateResult();
    EXPECT_EQ(menu.getCurrentRecipeId().toString(), "test:test_2x2_recipe");
}

TEST_F(InventoryCraftingMenuTest, EmptyGrid_NeverProducesResult)
{
    // 回归锚点：曾有一条配料全部未注册的配方（minecraft:clay，配料 clay_ball 未实现）
    // 匹配到完全空的 2x2 网格，背包合成结果槽（菜单槽位 0）凭空出现一个粘土并随全量同步
    // 下发到客户端。空网格永不产出结果，这条断言把该类缺陷钉死。
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr) << "Stone item not registered";

    // 退化配方：无配料（其 matches 对空网格恒为真），但仍不该产出结果。
    auto recipe = std::make_unique<TestRecipe>(
        ResourceLocation("test", "degenerate_recipe"), std::vector<crafting::Ingredient>(), ItemStack(*stone, 1));
    crafting::RecipeManager::instance().registerRecipe(std::move(recipe));

    InventoryCraftingMenu menu(1, m_playerInventory.get());
    menu.updateResult();

    EXPECT_TRUE(menu.getCurrentRecipeId().path().empty()) << "空网格不得匹配到任何配方";
    const Slot* resultSlot = menu.getSlot(InventoryCraftingMenu::RESULT_SLOT);
    ASSERT_NE(resultSlot, nullptr);
    EXPECT_TRUE(resultSlot->getItem().isEmpty()) << "空网格的合成结果槽必须为空";
}

} // namespace