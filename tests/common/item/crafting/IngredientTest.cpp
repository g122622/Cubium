#include <gtest/gtest.h>
#include "item/crafting/Ingredient.hpp"
#include "item/core/Item.hpp"
#include "item/core/ItemStack.hpp"
#include "item/core/ItemRegistry.hpp"

using namespace mc;
using namespace mc::crafting;

// 测试用的模拟Item类
class TestItem : public Item {
public:
    explicit TestItem(const String& name)
        : Item(ItemProperties().maxStackSize(64)) {
        // 使用反射或友元来设置名称，这里简化处理
    }
};

class IngredientTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 获取一些已注册的物品用于测试
        // 如果没有注册物品，需要先注册测试物品
    }
};

// ========== 空Ingredient测试 ==========

TEST_F(IngredientTest, EmptyIngredient_IsEmpty) {
    Ingredient ing;
    EXPECT_TRUE(ing.isEmpty());
    EXPECT_TRUE(ing.getMatchingStacks().empty());
}

TEST_F(IngredientTest, EmptyIngredient_DoesNotMatchAnyItem) {
    Ingredient ing;

    // 空Ingredient不应该匹配任何物品
    ItemStack emptyStack;
    EXPECT_FALSE(ing.test(emptyStack));
}

// ========== 单物品Ingredient测试 ==========

TEST_F(IngredientTest, FromSingleItem_HasMatchingStacks) {
    // 使用注册表中的物品
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered, skipping test";
    }

    Ingredient ing = Ingredient::fromItem(*stone);

    EXPECT_FALSE(ing.isEmpty());
    EXPECT_EQ(ing.getMatchingStacks().size(), 1u);
}

TEST_F(IngredientTest, FromSingleItem_MatchesCorrectItem) {
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered, skipping test";
    }

    Ingredient ing = Ingredient::fromItem(*stone);

    // 匹配正确的物品
    ItemStack stack(*stone, 1);
    EXPECT_TRUE(ing.test(stack));

    // 匹配任意数量
    ItemStack stack64(*stone, 64);
    EXPECT_TRUE(ing.test(stack64));
}

TEST_F(IngredientTest, FromSingleItem_DoesNotMatchOtherItem) {
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    Item* dirt = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dirt"));

    if (!stone || !dirt) {
        GTEST_SKIP() << "Required items not registered, skipping test";
    }

    Ingredient ing = Ingredient::fromItem(*stone);

    // 不匹配其他物品
    ItemStack dirtStack(*dirt, 1);
    EXPECT_FALSE(ing.test(dirtStack));
}

// ========== 多物品Ingredient测试 ==========

TEST_F(IngredientTest, FromMultipleItems_MatchesAllItems) {
    Item* oakPlanks = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "oak_planks"));
    Item* sprucePlanks = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "spruce_planks"));
    Item* birchPlanks = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "birch_planks"));

    if (!oakPlanks || !sprucePlanks || !birchPlanks) {
        GTEST_SKIP() << "Planks items not registered, skipping test";
    }

    Ingredient ing = Ingredient::fromItems({oakPlanks, sprucePlanks, birchPlanks});

    EXPECT_FALSE(ing.isEmpty());
    EXPECT_EQ(ing.getMatchingStacks().size(), 3u);

    // 匹配所有列出的物品
    EXPECT_TRUE(ing.test(*oakPlanks));
    EXPECT_TRUE(ing.test(*sprucePlanks));
    EXPECT_TRUE(ing.test(*birchPlanks));
}

TEST_F(IngredientTest, FromMultipleItems_DoesNotMatchOtherItems) {
    Item* oakPlanks = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "oak_planks"));
    Item* sprucePlanks = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "spruce_planks"));
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));

    if (!oakPlanks || !sprucePlanks || !stone) {
        GTEST_SKIP() << "Required items not registered, skipping test";
    }

    Ingredient ing = Ingredient::fromItems({oakPlanks, sprucePlanks});

    // 不匹配未列出的物品
    EXPECT_FALSE(ing.test(*stone));
}

TEST_F(IngredientTest, FromEmptyItemList_ReturnsEmptyIngredient) {
    std::vector<const Item*> emptyItems;
    Ingredient ing = Ingredient::fromItems(emptyItems);

    EXPECT_TRUE(ing.isEmpty());
}

// ========== 物品堆Ingredient测试 ==========

TEST_F(IngredientTest, FromStacks_MatchesAllStacks) {
    Item* oakPlanks = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "oak_planks"));
    Item* sprucePlanks = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "spruce_planks"));

    if (!oakPlanks || !sprucePlanks) {
        GTEST_SKIP() << "Planks items not registered, skipping test";
    }

    std::vector<ItemStack> stacks = {
        ItemStack(*oakPlanks, 1),
        ItemStack(*sprucePlanks, 1)
    };

    Ingredient ing = Ingredient::fromStacks(stacks);

    EXPECT_FALSE(ing.isEmpty());
    EXPECT_EQ(ing.getMatchingStacks().size(), 2u);

    // 匹配物品类型（数量不影响匹配）
    EXPECT_TRUE(ing.test(*oakPlanks));
    EXPECT_TRUE(ing.test(*sprucePlanks));
}

// ========== 标签Ingredient测试 ==========

TEST_F(IngredientTest, FromTag_HasTagFlag) {
    Ingredient ing = Ingredient::fromTag("minecraft:planks");

    EXPECT_TRUE(ing.hasTag());
    EXPECT_EQ(ing.getTag(), "minecraft:planks");

    // 当前版本标签系统未实现，所以匹配应该失败
    // 注意：这个行为可能会在未来版本改变
    Item* oakPlanks = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "oak_planks"));
    if (oakPlanks) {
        // 标签系统未实现时返回false
        EXPECT_FALSE(ing.test(*oakPlanks));
    }
}

// ========== 相等比较测试 ==========

TEST_F(IngredientTest, EqualIngredients_AreEqual) {
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered, skipping test";
    }

    Ingredient ing1 = Ingredient::fromItem(*stone);
    Ingredient ing2 = Ingredient::fromItem(*stone);

    EXPECT_TRUE(ing1 == ing2);
    EXPECT_FALSE(ing1 != ing2);
}

TEST_F(IngredientTest, DifferentIngredients_AreNotEqual) {
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    Item* dirt = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dirt"));

    if (!stone || !dirt) {
        GTEST_SKIP() << "Required items not registered, skipping test";
    }

    Ingredient ing1 = Ingredient::fromItem(*stone);
    Ingredient ing2 = Ingredient::fromItem(*dirt);

    EXPECT_FALSE(ing1 == ing2);
    EXPECT_TRUE(ing1 != ing2);
}

TEST_F(IngredientTest, EmptyIngredients_AreEqual) {
    Ingredient ing1;
    Ingredient ing2;

    EXPECT_TRUE(ing1 == ing2);
}

// ========== 哈希测试 ==========

TEST_F(IngredientTest, SameIngredients_HaveSameHash) {
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered, skipping test";
    }

    Ingredient ing1 = Ingredient::fromItem(*stone);
    Ingredient ing2 = Ingredient::fromItem(*stone);

    EXPECT_EQ(ing1.hash(), ing2.hash());
}

TEST_F(IngredientTest, DifferentIngredients_HaveDifferentHash) {
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    Item* dirt = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dirt"));

    if (!stone || !dirt) {
        GTEST_SKIP() << "Required items not registered, skipping test";
    }

    Ingredient ing1 = Ingredient::fromItem(*stone);
    Ingredient ing2 = Ingredient::fromItem(*dirt);

    // 不同物品应该有不同的哈希（虽然不是绝对保证）
    EXPECT_NE(ing1.hash(), ing2.hash());
}

// ========== 空物品堆测试 ==========

TEST_F(IngredientTest, DoesNotMatchEmptyStack) {
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered, skipping test";
    }

    Ingredient ing = Ingredient::fromItem(*stone);

    // 不匹配空堆
    ItemStack emptyStack;
    EXPECT_FALSE(ing.test(emptyStack));
}

TEST_F(IngredientTest, NullItemPointer_ReturnsEmptyIngredient) {
    Ingredient ing = Ingredient::fromItem(static_cast<const Item*>(nullptr));

    EXPECT_TRUE(ing.isEmpty());
}

// ========== Item指针匹配测试 ==========

TEST_F(IngredientTest, MatchesItemPointer) {
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    Item* dirt = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dirt"));

    if (!stone || !dirt) {
        GTEST_SKIP() << "Required items not registered, skipping test";
    }

    Ingredient ing = Ingredient::fromItem(*stone);

    // 匹配正确的物品指针
    EXPECT_TRUE(ing.test(stone));

    // 不匹配错误的物品指针
    EXPECT_FALSE(ing.test(dirt));

    // 不匹配空指针
    EXPECT_FALSE(ing.test(static_cast<const Item*>(nullptr)));
}
