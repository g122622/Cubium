#include <gtest/gtest.h>

#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/crafting/RecipeManager.hpp"
#include "common/item/crafting/ShapedRecipe.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/blockentity/CraftingTableEntity.hpp"
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
    void SetUp() override
    {
        m_player = std::make_unique<Player>(1, "MenuTester");
        m_playerInventory = std::make_unique<PlayerInventory>(m_player.get());
        m_blockEntity = std::make_unique<CraftingTableEntity>(BlockPos(0, 64, 0));
    }

    void TearDown() override { crafting::RecipeManager::instance().clear(); }

    std::unique_ptr<Player> m_player;
    std::unique_ptr<PlayerInventory> m_playerInventory;
    std::unique_ptr<CraftingTableEntity> m_blockEntity;
};

TEST_F(CraftingMenuTest, StillValid_WhenPlayerIsNearCraftingTable_ReturnsTrue)
{
    CraftingMenu menu(1, m_playerInventory.get(), m_blockEntity.get());
    m_player->setPosition(0.5f, 64.0f, 0.5f);

    EXPECT_TRUE(menu.stillValid(*m_player));
}

TEST_F(CraftingMenuTest, StillValid_WhenPlayerIsTooFar_ReturnsFalse)
{
    CraftingMenu menu(1, m_playerInventory.get(), m_blockEntity.get());
    m_player->setPosition(12.5f, 64.0f, 0.5f);

    EXPECT_FALSE(menu.stillValid(*m_player));
}

TEST_F(CraftingMenuTest, GetCurrentRecipeId_ReturnsEmptyWhenNoMatch)
{
    CraftingMenu menu(1, m_playerInventory.get(), m_blockEntity.get());

    // 空网格，没有匹配的配方
    ResourceLocation recipeId = menu.getCurrentRecipeId();
    EXPECT_TRUE(recipeId.path().empty());
}

TEST_F(CraftingMenuTest, GetCurrentRecipeId_ReturnsRecipeIdWhenMatch)
{
    // 创建一个测试配方（使用空原料，空网格匹配）
    auto recipe = std::make_unique<TestRecipe>(ResourceLocation("test", "test_recipe"),
        std::vector<crafting::Ingredient>(), // 空原料
        ItemStack()                          // 空结果
    );
    crafting::RecipeManager::instance().registerRecipe(std::move(recipe));

    CraftingMenu menu(1, m_playerInventory.get(), m_blockEntity.get());

    // 空网格匹配空原料配方
    menu.updateResult();

    ResourceLocation recipeId = menu.getCurrentRecipeId();
    EXPECT_EQ(recipeId.toString(), "test:test_recipe");
}

TEST_F(CraftingMenuTest, GetCurrentRecipeId_UpdatesAfterGridChange)
{
    // 注册两个配方：一个需要空网格，一个需要原料
    auto emptyRecipe = std::make_unique<TestRecipe>(
        ResourceLocation("test", "empty_recipe"), std::vector<crafting::Ingredient>(), ItemStack());
    crafting::RecipeManager::instance().registerRecipe(std::move(emptyRecipe));

    CraftingMenu menu(1, m_playerInventory.get(), m_blockEntity.get());

    // 初始状态：匹配空配方
    menu.updateResult();
    EXPECT_EQ(menu.getCurrentRecipeId().toString(), "test:empty_recipe");

    // 添加物品后，空网格配方不再匹配
    const Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    if (stone != nullptr) {
        menu.getCraftingGrid().setItem(0, ItemStack(*stone, 1));
        menu.updateResult();
        EXPECT_TRUE(menu.getCurrentRecipeId().path().empty()); // 没有匹配的配方
    }
}

// InventoryCraftingMenu 测试类
class InventoryCraftingMenuTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_player = std::make_unique<Player>(1, "InventoryMenuTester");
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
    // 注册一个空网格配方（2x2 网格）
    auto recipe = std::make_unique<TestRecipe>(
        ResourceLocation("test", "test_2x2_recipe"), std::vector<crafting::Ingredient>(), ItemStack());
    crafting::RecipeManager::instance().registerRecipe(std::move(recipe));

    InventoryCraftingMenu menu(1, m_playerInventory.get());

    // 空网格匹配
    menu.updateResult();

    ResourceLocation recipeId = menu.getCurrentRecipeId();
    EXPECT_EQ(recipeId.toString(), "test:test_2x2_recipe");
}

} // namespace