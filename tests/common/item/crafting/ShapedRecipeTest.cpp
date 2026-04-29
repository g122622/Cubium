#include <gtest/gtest.h>
#include "item/crafting/ShapedRecipe.hpp"
#include "item/crafting/RecipeManager.hpp"
#include "entity/inventory/CraftingInventory.hpp"
#include "item/core/ItemRegistry.hpp"

using namespace mc;
using namespace mc::crafting;

class ShapedRecipeTest : public ::testing::Test {
protected:
    void SetUp() override {
        RecipeManager::instance().clear();
    }

    void TearDown() override {
        RecipeManager::instance().clear();
    }
};

// ========== 构造函数测试 ==========

TEST_F(ShapedRecipeTest, Create_2x2Recipe) {
    auto recipe = std::make_unique<ShapedRecipe>(
        ResourceLocation("test", "planks_to_crafting_table"),
        2, 2,
        std::vector<Ingredient>(),
        ItemStack()
    );

    EXPECT_EQ(recipe->getRecipeWidth(), 2);
    EXPECT_EQ(recipe->getRecipeHeight(), 2);
    EXPECT_EQ(recipe->getType(), RecipeType::ShapedCrafting);
    EXPECT_EQ(recipe->getId(), ResourceLocation("test", "planks_to_crafting_table"));
}

TEST_F(ShapedRecipeTest, Create_3x3Recipe) {
    auto recipe = std::make_unique<ShapedRecipe>(
        ResourceLocation("test", "large_recipe"),
        3, 3,
        std::vector<Ingredient>(),
        ItemStack()
    );

    EXPECT_EQ(recipe->getRecipeWidth(), 3);
    EXPECT_EQ(recipe->getRecipeHeight(), 3);
}

// ========== canFitIn测试 ==========

TEST_F(ShapedRecipeTest, CanFitIn_SmallerGrid_ReturnsFalse) {
    auto recipe = std::make_unique<ShapedRecipe>(
        ResourceLocation("test", "large_recipe"),
        3, 3,
        std::vector<Ingredient>(),
        ItemStack()
    );

    EXPECT_FALSE(recipe->canFitIn(2, 2));
    EXPECT_FALSE(recipe->canFitIn(3, 2));
    EXPECT_FALSE(recipe->canFitIn(2, 3));
}

TEST_F(ShapedRecipeTest, CanFitIn_SameSize_ReturnsTrue) {
    auto recipe = std::make_unique<ShapedRecipe>(
        ResourceLocation("test", "small_recipe"),
        2, 2,
        std::vector<Ingredient>(),
        ItemStack()
    );

    EXPECT_TRUE(recipe->canFitIn(2, 2));
}

TEST_F(ShapedRecipeTest, CanFitIn_LargerGrid_ReturnsTrue) {
    auto recipe = std::make_unique<ShapedRecipe>(
        ResourceLocation("test", "small_recipe"),
        2, 2,
        std::vector<Ingredient>(),
        ItemStack()
    );

    EXPECT_TRUE(recipe->canFitIn(3, 3));
}

// ========== 空配方匹配测试 ==========

TEST_F(ShapedRecipeTest, Matches_EmptyRecipe_EmptyInventory_ReturnsTrue) {
    auto recipe = std::make_unique<ShapedRecipe>(
        ResourceLocation("test", "empty_recipe"),
        1, 1,
        std::vector<Ingredient>{Ingredient()}, // 空原料
        ItemStack()
    );

    CraftingInventory inventory(3, 3);
    // MC 原版行为: 空 Ingredient 匹配空 ItemStack，所以空配方匹配空网格
    EXPECT_TRUE(recipe->matches(inventory));
}

TEST_F(ShapedRecipeTest, Matches_EmptyRecipe_NonEmptyInventory_ReturnsFalse) {
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    auto recipe = std::make_unique<ShapedRecipe>(
        ResourceLocation("test", "empty_recipe"),
        0, 0,
        std::vector<Ingredient>(),
        ItemStack()
    );

    CraftingInventory inventory(3, 3);
    inventory.setItem(0, ItemStack(*stone, 1));

    EXPECT_FALSE(recipe->matches(inventory));
}

// ========== 有序匹配测试 ==========

TEST_F(ShapedRecipeTest, Matches_SingleItem_CorrectPosition) {
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    // 1x1配方：单个石头
    Ingredient stoneIng = Ingredient::fromItem(*stone);
    auto recipe = std::make_unique<ShapedRecipe>(
        ResourceLocation("test", "single_stone"),
        1, 1,
        std::vector<Ingredient>{stoneIng},
        ItemStack(*stone, 2)
    );

    // 在(0,0)位置放置石头
    CraftingInventory inventory(3, 3);
    inventory.setItemAt(0, 0, ItemStack(*stone, 1));

    EXPECT_TRUE(recipe->matches(inventory));
}

TEST_F(ShapedRecipeTest, Matches_SingleItem_WrongItem_ReturnsFalse) {
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    Item* dirt = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dirt"));
    if (!stone || !dirt) {
        GTEST_SKIP() << "Required items not registered";
    }

    Ingredient stoneIng = Ingredient::fromItem(*stone);
    auto recipe = std::make_unique<ShapedRecipe>(
        ResourceLocation("test", "single_stone"),
        1, 1,
        std::vector<Ingredient>{stoneIng},
        ItemStack(*stone, 2)
    );

    // 在(0,0)位置放置泥土（错误物品）
    CraftingInventory inventory(3, 3);
    inventory.setItemAt(0, 0, ItemStack(*dirt, 1));

    EXPECT_FALSE(recipe->matches(inventory));
}

TEST_F(ShapedRecipeTest, Matches_SingleItem_OffsetPosition) {
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    Ingredient stoneIng = Ingredient::fromItem(*stone);
    auto recipe = std::make_unique<ShapedRecipe>(
        ResourceLocation("test", "single_stone"),
        1, 1,
        std::vector<Ingredient>{stoneIng},
        ItemStack(*stone, 2)
    );

    // 在(1,1)位置放置石头（应该也能匹配）
    CraftingInventory inventory(3, 3);
    inventory.setItemAt(1, 1, ItemStack(*stone, 1));

    EXPECT_TRUE(recipe->matches(inventory));
}

TEST_F(ShapedRecipeTest, Matches_2x2Recipe_CorrectPattern) {
    Item* planks = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "oak_planks"));
    if (!planks) {
        GTEST_SKIP() << "Oak planks not registered";
    }

    // 2x2配方：4个木板
    Ingredient planksIng = Ingredient::fromItem(*planks);
    auto recipe = std::make_unique<ShapedRecipe>(
        ResourceLocation("test", "crafting_table"),
        2, 2,
        std::vector<Ingredient>{planksIng, planksIng, planksIng, planksIng},
        ItemStack(*planks, 1)
    );

    // 在左上角放置4个木板
    CraftingInventory inventory(3, 3);
    inventory.setItemAt(0, 0, ItemStack(*planks, 1));
    inventory.setItemAt(1, 0, ItemStack(*planks, 1));
    inventory.setItemAt(0, 1, ItemStack(*planks, 1));
    inventory.setItemAt(1, 1, ItemStack(*planks, 1));

    EXPECT_TRUE(recipe->matches(inventory));
}

TEST_F(ShapedRecipeTest, Matches_2x2Recipe_OffsetPattern) {
    Item* planks = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "oak_planks"));
    if (!planks) {
        GTEST_SKIP() << "Oak planks not registered";
    }

    Ingredient planksIng = Ingredient::fromItem(*planks);
    auto recipe = std::make_unique<ShapedRecipe>(
        ResourceLocation("test", "crafting_table"),
        2, 2,
        std::vector<Ingredient>{planksIng, planksIng, planksIng, planksIng},
        ItemStack(*planks, 1)
    );

    // 在中心位置放置4个木板（偏移）
    CraftingInventory inventory(3, 3);
    inventory.setItemAt(1, 1, ItemStack(*planks, 1));
    inventory.setItemAt(2, 1, ItemStack(*planks, 1));
    inventory.setItemAt(1, 2, ItemStack(*planks, 1));
    inventory.setItemAt(2, 2, ItemStack(*planks, 1));

    EXPECT_TRUE(recipe->matches(inventory));
}

TEST_F(ShapedRecipeTest, Matches_2x2Recipe_IncompletePattern_ReturnsFalse) {
    Item* planks = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "oak_planks"));
    if (!planks) {
        GTEST_SKIP() << "Oak planks not registered";
    }

    Ingredient planksIng = Ingredient::fromItem(*planks);
    auto recipe = std::make_unique<ShapedRecipe>(
        ResourceLocation("test", "crafting_table"),
        2, 2,
        std::vector<Ingredient>{planksIng, planksIng, planksIng, planksIng},
        ItemStack(*planks, 1)
    );

    // 只放3个木板
    CraftingInventory inventory(3, 3);
    inventory.setItemAt(0, 0, ItemStack(*planks, 1));
    inventory.setItemAt(1, 0, ItemStack(*planks, 1));
    inventory.setItemAt(0, 1, ItemStack(*planks, 1));
    // 缺少(1,1)

    EXPECT_FALSE(recipe->matches(inventory));
}

TEST_F(ShapedRecipeTest, Matches_ExtraItems_ReturnsFalse) {
    Item* planks = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "oak_planks"));
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    if (!planks || !stone) {
        GTEST_SKIP() << "Required items not registered";
    }

    Ingredient planksIng = Ingredient::fromItem(*planks);
    auto recipe = std::make_unique<ShapedRecipe>(
        ResourceLocation("test", "crafting_table"),
        2, 2,
        std::vector<Ingredient>{planksIng, planksIng, planksIng, planksIng},
        ItemStack(*planks, 1)
    );

    // 放置正确的4个木板 + 额外的石头
    CraftingInventory inventory(3, 3);
    inventory.setItemAt(0, 0, ItemStack(*planks, 1));
    inventory.setItemAt(1, 0, ItemStack(*planks, 1));
    inventory.setItemAt(0, 1, ItemStack(*planks, 1));
    inventory.setItemAt(1, 1, ItemStack(*planks, 1));
    inventory.setItemAt(2, 0, ItemStack(*stone, 1)); // 额外物品

    EXPECT_FALSE(recipe->matches(inventory));
}

// ========== 镜像匹配测试 ==========

TEST_F(ShapedRecipeTest, Matches_MirroredPattern_ReturnsTrue) {
    Item* planks = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "oak_planks"));
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    if (!planks || !stone) {
        GTEST_SKIP() << "Required items not registered";
    }

    // 非对称配方：木板和石头
    // 正向: P S
    // 镜像: S P
    Ingredient planksIng = Ingredient::fromItem(*planks);
    Ingredient stoneIng = Ingredient::fromItem(*stone);
    auto recipe = std::make_unique<ShapedRecipe>(
        ResourceLocation("test", "asymmetric"),
        2, 1,
        std::vector<Ingredient>{planksIng, stoneIng},
        ItemStack(*planks, 1)
    );

    // 放置镜像形式：石头在左，木板在右
    CraftingInventory inventory(3, 3);
    inventory.setItemAt(0, 0, ItemStack(*stone, 1)); // 石头在左
    inventory.setItemAt(1, 0, ItemStack(*planks, 1)); // 木板在右

    EXPECT_TRUE(recipe->matches(inventory)); // 应该匹配镜像
}

// ========== assemble测试 ==========

TEST_F(ShapedRecipeTest, Assemble_ReturnsCorrectResult) {
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    Ingredient stoneIng = Ingredient::fromItem(*stone);
    ItemStack result(*stone, 4);
    auto recipe = std::make_unique<ShapedRecipe>(
        ResourceLocation("test", "stone_x4"),
        1, 1,
        std::vector<Ingredient>{stoneIng},
        result
    );

    CraftingInventory inventory(3, 3);
    inventory.setItemAt(0, 0, ItemStack(*stone, 1));

    ItemStack assembled = recipe->assemble(inventory);
    EXPECT_EQ(assembled.getCount(), 4);
    EXPECT_TRUE(assembled.isSameItem(result));
}

// ========== 分组测试 ==========

TEST_F(ShapedRecipeTest, GetGroup_ReturnsGroup) {
    auto recipe = std::make_unique<ShapedRecipe>(
        ResourceLocation("test", "recipe"),
        1, 1,
        std::vector<Ingredient>(),
        ItemStack(),
        "test_group"
    );

    EXPECT_EQ(recipe->getGroup(), "test_group");
}

TEST_F(ShapedRecipeTest, GetGroup_NoGroup_ReturnsEmpty) {
    auto recipe = std::make_unique<ShapedRecipe>(
        ResourceLocation("test", "recipe"),
        1, 1,
        std::vector<Ingredient>(),
        ItemStack()
    );

    EXPECT_EQ(recipe->getGroup(), "");
}

// ========== 注册到RecipeManager测试 ==========

TEST_F(ShapedRecipeTest, RegisterToManager_Success) {
    auto recipe = std::make_unique<ShapedRecipe>(
        ResourceLocation("test", "shaped_recipe"),
        2, 2,
        std::vector<Ingredient>(),
        ItemStack()
    );

    bool result = RecipeManager::instance().registerRecipe(std::move(recipe));
    EXPECT_TRUE(result);
    EXPECT_TRUE(RecipeManager::instance().hasRecipe(ResourceLocation("test", "shaped_recipe")));
}

TEST_F(ShapedRecipeTest, GetRecipeFromManager_CorrectType) {
    auto recipe = std::make_unique<ShapedRecipe>(
        ResourceLocation("test", "shaped_recipe"),
        2, 2,
        std::vector<Ingredient>(),
        ItemStack()
    );

    RecipeManager::instance().registerRecipe(std::move(recipe));

    const CraftingRecipe* found = RecipeManager::instance().getRecipe(
        ResourceLocation("test", "shaped_recipe"));
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->getType(), RecipeType::ShapedCrafting);
}
