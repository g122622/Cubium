#include "item/crafting/ShapelessRecipe.hpp"
#include "entity/inventory/CraftingInventory.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/crafting/RecipeManager.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::crafting;

class ShapelessRecipeTest : public ::testing::Test {
protected:
    void SetUp() override { RecipeManager::instance().clear(); }

    void TearDown() override { RecipeManager::instance().clear(); }
};

// ========== 构造函数测试 ==========

TEST_F(ShapelessRecipeTest, Create_EmptyRecipe)
{
    auto recipe = std::make_unique<ShapelessRecipe>(
        ResourceLocation("test", "empty_recipe"), std::vector<Ingredient>(), ItemStack());

    EXPECT_EQ(recipe->getIngredients().size(), 0u);
    EXPECT_EQ(recipe->getType(), RecipeType::ShapelessCrafting);
    EXPECT_EQ(recipe->getId(), ResourceLocation("test", "empty_recipe"));
}

TEST_F(ShapelessRecipeTest, Create_SingleIngredientRecipe)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    Ingredient stoneIng = Ingredient::fromItem(*stone);
    auto recipe = std::make_unique<ShapelessRecipe>(
        ResourceLocation("test", "single_stone"), std::vector<Ingredient>{stoneIng}, ItemStack(*stone, 2));

    EXPECT_EQ(recipe->getIngredients().size(), 1u);
}

TEST_F(ShapelessRecipeTest, Create_MultipleIngredientRecipe)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    Item* dirt = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dirt"));
    if (!stone || !dirt) {
        GTEST_SKIP() << "Required items not registered";
    }

    Ingredient stoneIng = Ingredient::fromItem(*stone);
    Ingredient dirtIng = Ingredient::fromItem(*dirt);

    auto recipe = std::make_unique<ShapelessRecipe>(
        ResourceLocation("test", "mixed"), std::vector<Ingredient>{stoneIng, dirtIng}, ItemStack(*stone, 1));

    EXPECT_EQ(recipe->getIngredients().size(), 2u);
}

// ========== canFitIn测试 ==========

TEST_F(ShapelessRecipeTest, CanFitIn_EmptyRecipe_ReturnsTrue)
{
    auto recipe =
        std::make_unique<ShapelessRecipe>(ResourceLocation("test", "empty"), std::vector<Ingredient>(), ItemStack());

    EXPECT_TRUE(recipe->canFitIn(1, 1));
    EXPECT_TRUE(recipe->canFitIn(2, 2));
}

TEST_F(ShapelessRecipeTest, CanFitIn_SingleIngredient_ReturnsTrue)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    Ingredient stoneIng = Ingredient::fromItem(*stone);
    auto recipe = std::make_unique<ShapelessRecipe>(
        ResourceLocation("test", "single"), std::vector<Ingredient>{stoneIng}, ItemStack(*stone, 1));

    EXPECT_TRUE(recipe->canFitIn(1, 1));
    EXPECT_TRUE(recipe->canFitIn(2, 2));
}

TEST_F(ShapelessRecipeTest, CanFitIn_TooManyIngredients_ReturnsFalse)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    Ingredient stoneIng = Ingredient::fromItem(*stone);

    // 5个原料
    auto recipe = std::make_unique<ShapelessRecipe>(ResourceLocation("test", "many"),
        std::vector<Ingredient>{stoneIng, stoneIng, stoneIng, stoneIng, stoneIng},
        ItemStack(*stone, 1));

    EXPECT_FALSE(recipe->canFitIn(2, 2)); // 2x2 = 4槽位，不够
    EXPECT_TRUE(recipe->canFitIn(3, 3));  // 3x3 = 9槽位，足够
}

// ========== 空配方匹配测试 ==========

TEST_F(ShapelessRecipeTest, Matches_EmptyRecipe_EmptyInventory_ReturnsTrue)
{
    auto recipe =
        std::make_unique<ShapelessRecipe>(ResourceLocation("test", "empty"), std::vector<Ingredient>(), ItemStack());

    CraftingInventory inventory(3, 3);
    EXPECT_TRUE(recipe->matches(inventory));
}

TEST_F(ShapelessRecipeTest, Matches_EmptyRecipe_NonEmptyInventory_ReturnsFalse)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    auto recipe =
        std::make_unique<ShapelessRecipe>(ResourceLocation("test", "empty"), std::vector<Ingredient>(), ItemStack());

    CraftingInventory inventory(3, 3);
    inventory.setItem(0, ItemStack(*stone, 1));

    EXPECT_FALSE(recipe->matches(inventory)); // 原料数为0，网格有物品
}

// ========== 单原料匹配测试 ==========

TEST_F(ShapelessRecipeTest, Matches_SingleIngredient_MatchingItem_ReturnsTrue)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    Ingredient stoneIng = Ingredient::fromItem(*stone);
    auto recipe = std::make_unique<ShapelessRecipe>(
        ResourceLocation("test", "single_stone"), std::vector<Ingredient>{stoneIng}, ItemStack(*stone, 2));

    CraftingInventory inventory(3, 3);
    inventory.setItem(0, ItemStack(*stone, 1));

    EXPECT_TRUE(recipe->matches(inventory));
}

TEST_F(ShapelessRecipeTest, Matches_SingleIngredient_PositionIndependent)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    Ingredient stoneIng = Ingredient::fromItem(*stone);
    auto recipe = std::make_unique<ShapelessRecipe>(
        ResourceLocation("test", "single_stone"), std::vector<Ingredient>{stoneIng}, ItemStack(*stone, 2));

    // 放在任意位置都应该匹配
    CraftingInventory inventory(3, 3);
    inventory.setItem(8, ItemStack(*stone, 1)); // 右下角

    EXPECT_TRUE(recipe->matches(inventory));
}

TEST_F(ShapelessRecipeTest, Matches_SingleIngredient_WrongItem_ReturnsFalse)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    Item* dirt = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dirt"));
    if (!stone || !dirt) {
        GTEST_SKIP() << "Required items not registered";
    }

    Ingredient stoneIng = Ingredient::fromItem(*stone);
    auto recipe = std::make_unique<ShapelessRecipe>(
        ResourceLocation("test", "single_stone"), std::vector<Ingredient>{stoneIng}, ItemStack(*stone, 2));

    CraftingInventory inventory(3, 3);
    inventory.setItem(0, ItemStack(*dirt, 1)); // 放置泥土

    EXPECT_FALSE(recipe->matches(inventory));
}

// ========== 多原料匹配测试 ==========

TEST_F(ShapelessRecipeTest, Matches_TwoIngredients_BothPresent_ReturnsTrue)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    Item* dirt = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dirt"));
    if (!stone || !dirt) {
        GTEST_SKIP() << "Required items not registered";
    }

    Ingredient stoneIng = Ingredient::fromItem(*stone);
    Ingredient dirtIng = Ingredient::fromItem(*dirt);

    auto recipe = std::make_unique<ShapelessRecipe>(
        ResourceLocation("test", "stone_and_dirt"), std::vector<Ingredient>{stoneIng, dirtIng}, ItemStack(*stone, 1));

    CraftingInventory inventory(3, 3);
    inventory.setItem(0, ItemStack(*stone, 1));
    inventory.setItem(1, ItemStack(*dirt, 1));

    EXPECT_TRUE(recipe->matches(inventory));
}

TEST_F(ShapelessRecipeTest, Matches_TwoIngredients_OrderIndependent)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    Item* dirt = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dirt"));
    if (!stone || !dirt) {
        GTEST_SKIP() << "Required items not registered";
    }

    Ingredient stoneIng = Ingredient::fromItem(*stone);
    Ingredient dirtIng = Ingredient::fromItem(*dirt);

    auto recipe = std::make_unique<ShapelessRecipe>(
        ResourceLocation("test", "stone_and_dirt"), std::vector<Ingredient>{stoneIng, dirtIng}, ItemStack(*stone, 1));

    // 顺序相反
    CraftingInventory inventory(3, 3);
    inventory.setItem(0, ItemStack(*dirt, 1));
    inventory.setItem(5, ItemStack(*stone, 1));

    EXPECT_TRUE(recipe->matches(inventory));
}

TEST_F(ShapelessRecipeTest, Matches_TwoIngredients_MissingOne_ReturnsFalse)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    Item* dirt = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dirt"));
    if (!stone || !dirt) {
        GTEST_SKIP() << "Required items not registered";
    }

    Ingredient stoneIng = Ingredient::fromItem(*stone);
    Ingredient dirtIng = Ingredient::fromItem(*dirt);

    auto recipe = std::make_unique<ShapelessRecipe>(
        ResourceLocation("test", "stone_and_dirt"), std::vector<Ingredient>{stoneIng, dirtIng}, ItemStack(*stone, 1));

    // 只有石头
    CraftingInventory inventory(3, 3);
    inventory.setItem(0, ItemStack(*stone, 1));

    EXPECT_FALSE(recipe->matches(inventory));
}

TEST_F(ShapelessRecipeTest, Matches_ExtraItems_ReturnsFalse)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    Item* dirt = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dirt"));
    if (!stone || !dirt) {
        GTEST_SKIP() << "Required items not registered";
    }

    Ingredient stoneIng = Ingredient::fromItem(*stone);

    // 1个原料的配方
    auto recipe = std::make_unique<ShapelessRecipe>(
        ResourceLocation("test", "single_stone"), std::vector<Ingredient>{stoneIng}, ItemStack(*stone, 2));

    // 放置了额外的物品
    CraftingInventory inventory(3, 3);
    inventory.setItem(0, ItemStack(*stone, 1));
    inventory.setItem(1, ItemStack(*dirt, 1)); // 额外物品

    EXPECT_FALSE(recipe->matches(inventory)); // 原料数=1，网格物品数=2
}

// ========== 多原料类型测试 ==========

TEST_F(ShapelessRecipeTest, Matches_SameItemType_MultipleSlots)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    Ingredient stoneIng = Ingredient::fromItem(*stone);

    // 需要2个石头
    auto recipe = std::make_unique<ShapelessRecipe>(
        ResourceLocation("test", "two_stones"), std::vector<Ingredient>{stoneIng, stoneIng}, ItemStack(*stone, 4));

    CraftingInventory inventory(3, 3);
    inventory.setItem(0, ItemStack(*stone, 1));
    inventory.setItem(1, ItemStack(*stone, 1));

    EXPECT_TRUE(recipe->matches(inventory));
}

TEST_F(ShapelessRecipeTest, Matches_SameItemType_OnlyOneSlot_ReturnsFalse)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    Ingredient stoneIng = Ingredient::fromItem(*stone);

    // 需要2个石头
    auto recipe = std::make_unique<ShapelessRecipe>(
        ResourceLocation("test", "two_stones"), std::vector<Ingredient>{stoneIng, stoneIng}, ItemStack(*stone, 4));

    // 只有1个石头
    CraftingInventory inventory(3, 3);
    inventory.setItem(0, ItemStack(*stone, 1));

    EXPECT_FALSE(recipe->matches(inventory)); // 原料数=2，网格物品数=1
}

// ========== assemble测试 ==========

TEST_F(ShapelessRecipeTest, Assemble_ReturnsCorrectResult)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    Ingredient stoneIng = Ingredient::fromItem(*stone);
    ItemStack result(*stone, 4);

    auto recipe = std::make_unique<ShapelessRecipe>(
        ResourceLocation("test", "stone_x4"), std::vector<Ingredient>{stoneIng}, result);

    CraftingInventory inventory(3, 3);
    inventory.setItem(0, ItemStack(*stone, 1));

    ItemStack assembled = recipe->assemble(inventory);
    EXPECT_EQ(assembled.getCount(), 4);
    EXPECT_TRUE(assembled.isSameItem(result));
}

// ========== 分组测试 ==========

TEST_F(ShapelessRecipeTest, GetGroup_ReturnsGroup)
{
    auto recipe = std::make_unique<ShapelessRecipe>(
        ResourceLocation("test", "recipe"), std::vector<Ingredient>(), ItemStack(), "test_group");

    EXPECT_EQ(recipe->getGroup(), "test_group");
}

TEST_F(ShapelessRecipeTest, GetGroup_NoGroup_ReturnsEmpty)
{
    auto recipe =
        std::make_unique<ShapelessRecipe>(ResourceLocation("test", "recipe"), std::vector<Ingredient>(), ItemStack());

    EXPECT_EQ(recipe->getGroup(), "");
}

// ========== 注册到RecipeManager测试 ==========

TEST_F(ShapelessRecipeTest, RegisterToManager_Success)
{
    auto recipe = std::make_unique<ShapelessRecipe>(
        ResourceLocation("test", "shapeless_recipe"), std::vector<Ingredient>(), ItemStack());

    bool result = RecipeManager::instance().registerRecipe(std::move(recipe));
    EXPECT_TRUE(result);
    EXPECT_TRUE(RecipeManager::instance().hasRecipe(ResourceLocation("test", "shapeless_recipe")));
}

TEST_F(ShapelessRecipeTest, GetRecipeFromManager_CorrectType)
{
    auto recipe = std::make_unique<ShapelessRecipe>(
        ResourceLocation("test", "shapeless_recipe"), std::vector<Ingredient>(), ItemStack());

    RecipeManager::instance().registerRecipe(std::move(recipe));

    const CraftingRecipe* found = RecipeManager::instance().getRecipe(ResourceLocation("test", "shapeless_recipe"));
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->getType(), RecipeType::ShapelessCrafting);
}

// ========== RecipeManager查找测试 ==========

TEST_F(ShapelessRecipeTest, RecipeManager_FindMatchingShapeless)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    Ingredient stoneIng = Ingredient::fromItem(*stone);

    auto recipe = std::make_unique<ShapelessRecipe>(
        ResourceLocation("test", "single_stone"), std::vector<Ingredient>{stoneIng}, ItemStack(*stone, 2));

    RecipeManager::instance().registerRecipe(std::move(recipe));

    CraftingInventory inventory(3, 3);
    inventory.setItem(4, ItemStack(*stone, 1)); // 中心位置

    const CraftingRecipe* found = RecipeManager::instance().findMatchingRecipe(inventory);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->getId(), ResourceLocation("test", "single_stone"));
}
