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
 */

#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/core/ItemStack.hpp"
#include "item/items/special/KnowledgeBookItem.hpp"
#include "item/items/weapon/ThrowableItems.hpp"
#include "world/block/registry/VanillaBlocks.hpp"
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/item/crafting/IRecipe.hpp"
#include "common/item/crafting/Ingredient.hpp"
#include "common/item/crafting/RecipeManager.hpp"
#include "common/world/IWorld.hpp"

using namespace mc;

namespace {

// ============================================================================
// 测试用世界 - 暴露BaseTestWorld的受保护构造函数
// ============================================================================

class KnowledgeBookTestWorld : public mc::test::BaseTestWorld {
public:
    KnowledgeBookTestWorld() = default;
};

// ============================================================================
// 测试用 Player - 追踪 unlockRecipe 调用
// ============================================================================

class KnowledgeBookTestPlayer : public Player {
public:
    explicit KnowledgeBookTestPlayer(IWorld* world = nullptr)
        : Player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
        if (world != nullptr) {
            setWorld(world);
        }
    }

    void unlockRecipe(const ResourceLocation& recipeId) override { m_unlockedRecipes.push_back(recipeId); }

    [[nodiscard]] const std::vector<ResourceLocation>& unlockedRecipes() const { return m_unlockedRecipes; }
    [[nodiscard]] i32 unlockedRecipeCount() const { return static_cast<i32>(m_unlockedRecipes.size()); }
    void clearUnlockedRecipes() { m_unlockedRecipes.clear(); }

private:
    std::vector<ResourceLocation> m_unlockedRecipes;
};

// ============================================================================
// 测试用配方 - 用于向RecipeManager注册测试配方
// ============================================================================

class TestRecipe : public crafting::CraftingRecipe {
public:
    explicit TestRecipe(const ResourceLocation& id)
        : m_id(id)
    {}

    [[nodiscard]] bool matches(const CraftingInventory& /*inventory*/) const override { return true; }
    [[nodiscard]] ItemStack assemble(const CraftingInventory& /*inventory*/) const override { return ItemStack(); }
    [[nodiscard]] ItemStack getResultItem() const override { return ItemStack(); }
    [[nodiscard]] std::vector<ItemStack> getRemainingItems(const CraftingInventory& inventory) const override
    {
        return crafting::RecipeUtils::getDefaultRemainingItems(inventory);
    }
    [[nodiscard]] const std::vector<crafting::Ingredient>& getIngredients() const override { return m_ingredients; }
    [[nodiscard]] ResourceLocation getId() const override { return m_id; }
    [[nodiscard]] crafting::RecipeType getType() const override { return crafting::RecipeType::ShapelessCrafting; }

private:
    ResourceLocation m_id;
    std::vector<crafting::Ingredient> m_ingredients;
};

} // anonymous namespace

// ============================================================================
// Trails & Tales 物品注册测试（火把花种子、瓶草荚果、蓝蛋、棕蛋、
// 追溯指南针、知识之书、火把花物品、瓶草物品）
// ============================================================================

class TrailsItemsRegistrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

// ============================================================================
// TORCHFLOWER_SEEDS 火把花种子
// ============================================================================

TEST_F(TrailsItemsRegistrationTest, TorchflowerSeeds_StaticPointerNotNull)
{
    ASSERT_NE(Items::TORCHFLOWER_SEEDS, nullptr) << "TORCHFLOWER_SEEDS should be registered";
}

TEST_F(TrailsItemsRegistrationTest, TorchflowerSeeds_CorrectItemId)
{
    EXPECT_EQ(Items::TORCHFLOWER_SEEDS->itemLocation(), ResourceLocation("minecraft", "torchflower_seeds"));
}

TEST_F(TrailsItemsRegistrationTest, TorchflowerSeeds_MaxStackSize64)
{
    EXPECT_EQ(Items::TORCHFLOWER_SEEDS->maxStackSize(), 64);
}

TEST_F(TrailsItemsRegistrationTest, TorchflowerSeeds_RegistryLookup)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "torchflower_seeds"));
    EXPECT_EQ(item, Items::TORCHFLOWER_SEEDS);
}

// ============================================================================
// PITCHER_POD 瓶草荚果
// ============================================================================

TEST_F(TrailsItemsRegistrationTest, PitcherPod_StaticPointerNotNull)
{
    ASSERT_NE(Items::PITCHER_POD, nullptr) << "PITCHER_POD should be registered";
}

TEST_F(TrailsItemsRegistrationTest, PitcherPod_CorrectItemId)
{
    EXPECT_EQ(Items::PITCHER_POD->itemLocation(), ResourceLocation("minecraft", "pitcher_pod"));
}

TEST_F(TrailsItemsRegistrationTest, PitcherPod_MaxStackSize64)
{
    EXPECT_EQ(Items::PITCHER_POD->maxStackSize(), 64);
}

TEST_F(TrailsItemsRegistrationTest, PitcherPod_RegistryLookup)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "pitcher_pod"));
    EXPECT_EQ(item, Items::PITCHER_POD);
}

// ============================================================================
// BLUE_EGG 蓝蛋
// ============================================================================

TEST_F(TrailsItemsRegistrationTest, BlueEgg_StaticPointerNotNull)
{
    ASSERT_NE(Items::BLUE_EGG, nullptr) << "BLUE_EGG should be registered";
}

TEST_F(TrailsItemsRegistrationTest, BlueEgg_CorrectItemId)
{
    EXPECT_EQ(Items::BLUE_EGG->itemLocation(), ResourceLocation("minecraft", "blue_egg"));
}

TEST_F(TrailsItemsRegistrationTest, BlueEgg_MaxStackSize16)
{
    // MC 1.20.5+: 蛋类物品堆叠数为16
    EXPECT_EQ(Items::BLUE_EGG->maxStackSize(), 16);
}

TEST_F(TrailsItemsRegistrationTest, BlueEgg_IsEggItem)
{
    // 蓝蛋应使用EggItem类，可投掷
    auto* eggItem = dynamic_cast<const item::EggItem*>(Items::BLUE_EGG);
    EXPECT_NE(eggItem, nullptr) << "BLUE_EGG should be an EggItem";
}

TEST_F(TrailsItemsRegistrationTest, BlueEgg_RegistryLookup)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "blue_egg"));
    EXPECT_EQ(item, Items::BLUE_EGG);
}

// ============================================================================
// BROWN_EGG 棕蛋
// ============================================================================

TEST_F(TrailsItemsRegistrationTest, BrownEgg_StaticPointerNotNull)
{
    ASSERT_NE(Items::BROWN_EGG, nullptr) << "BROWN_EGG should be registered";
}

TEST_F(TrailsItemsRegistrationTest, BrownEgg_CorrectItemId)
{
    EXPECT_EQ(Items::BROWN_EGG->itemLocation(), ResourceLocation("minecraft", "brown_egg"));
}

TEST_F(TrailsItemsRegistrationTest, BrownEgg_MaxStackSize16)
{
    EXPECT_EQ(Items::BROWN_EGG->maxStackSize(), 16);
}

TEST_F(TrailsItemsRegistrationTest, BrownEgg_IsEggItem)
{
    auto* eggItem = dynamic_cast<const item::EggItem*>(Items::BROWN_EGG);
    EXPECT_NE(eggItem, nullptr) << "BROWN_EGG should be an EggItem";
}

TEST_F(TrailsItemsRegistrationTest, BrownEgg_RegistryLookup)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "brown_egg"));
    EXPECT_EQ(item, Items::BROWN_EGG);
}

// ============================================================================
// RECOVERY_COMPASS 追溯指南针
// ============================================================================

TEST_F(TrailsItemsRegistrationTest, RecoveryCompass_StaticPointerNotNull)
{
    ASSERT_NE(Items::RECOVERY_COMPASS, nullptr) << "RECOVERY_COMPASS should be registered";
}

TEST_F(TrailsItemsRegistrationTest, RecoveryCompass_CorrectItemId)
{
    EXPECT_EQ(Items::RECOVERY_COMPASS->itemLocation(), ResourceLocation("minecraft", "recovery_compass"));
}

TEST_F(TrailsItemsRegistrationTest, RecoveryCompass_MaxStackSize1)
{
    // 指南针类物品堆叠数为1
    EXPECT_EQ(Items::RECOVERY_COMPASS->maxStackSize(), 1);
}

TEST_F(TrailsItemsRegistrationTest, RecoveryCompass_RegistryLookup)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "recovery_compass"));
    EXPECT_EQ(item, Items::RECOVERY_COMPASS);
}

// ============================================================================
// KNOWLEDGE_BOOK 知识之书
// ============================================================================

TEST_F(TrailsItemsRegistrationTest, KnowledgeBook_StaticPointerNotNull)
{
    ASSERT_NE(Items::KNOWLEDGE_BOOK, nullptr) << "KNOWLEDGE_BOOK should be registered";
}

TEST_F(TrailsItemsRegistrationTest, KnowledgeBook_CorrectItemId)
{
    EXPECT_EQ(Items::KNOWLEDGE_BOOK->itemLocation(), ResourceLocation("minecraft", "knowledge_book"));
}

TEST_F(TrailsItemsRegistrationTest, KnowledgeBook_MaxStackSize1)
{
    EXPECT_EQ(Items::KNOWLEDGE_BOOK->maxStackSize(), 1);
}

TEST_F(TrailsItemsRegistrationTest, KnowledgeBook_IsKnowledgeBookItem)
{
    auto* kbItem = dynamic_cast<const item::items::KnowledgeBookItem*>(Items::KNOWLEDGE_BOOK);
    EXPECT_NE(kbItem, nullptr) << "KNOWLEDGE_BOOK should be a KnowledgeBookItem";
}

TEST_F(TrailsItemsRegistrationTest, KnowledgeBook_RegistryLookup)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "knowledge_book"));
    EXPECT_EQ(item, Items::KNOWLEDGE_BOOK);
}

// ============================================================================
// TORCHFLOWER 火把花（方块物品）
// ============================================================================

TEST_F(TrailsItemsRegistrationTest, Torchflower_StaticPointerNotNull)
{
    ASSERT_NE(Items::TORCHFLOWER, nullptr) << "TORCHFLOWER should be registered";
}

TEST_F(TrailsItemsRegistrationTest, Torchflower_CorrectItemId)
{
    EXPECT_EQ(Items::TORCHFLOWER->itemLocation(), ResourceLocation("minecraft", "torchflower"));
}

TEST_F(TrailsItemsRegistrationTest, Torchflower_MaxStackSize64)
{
    EXPECT_EQ(Items::TORCHFLOWER->maxStackSize(), 64);
}

TEST_F(TrailsItemsRegistrationTest, Torchflower_RegistryLookup)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "torchflower"));
    EXPECT_EQ(item, Items::TORCHFLOWER);
}

// ============================================================================
// PITCHER_PLANT 瓶草（方块物品）
// ============================================================================

TEST_F(TrailsItemsRegistrationTest, PitcherPlant_StaticPointerNotNull)
{
    ASSERT_NE(Items::PITCHER_PLANT, nullptr) << "PITCHER_PLANT should be registered";
}

TEST_F(TrailsItemsRegistrationTest, PitcherPlant_CorrectItemId)
{
    EXPECT_EQ(Items::PITCHER_PLANT->itemLocation(), ResourceLocation("minecraft", "pitcher_plant"));
}

TEST_F(TrailsItemsRegistrationTest, PitcherPlant_MaxStackSize64)
{
    EXPECT_EQ(Items::PITCHER_PLANT->maxStackSize(), 64);
}

TEST_F(TrailsItemsRegistrationTest, PitcherPlant_RegistryLookup)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "pitcher_plant"));
    EXPECT_EQ(item, Items::PITCHER_PLANT);
}

// ============================================================================
// 批量验证 - 确保所有新增物品都能通过注册表查找到
// ============================================================================

TEST_F(TrailsItemsRegistrationTest, AllTrailsItems_RegistryLookup)
{
    const char* itemNames[] = {"torchflower_seeds",
        "pitcher_pod",
        "blue_egg",
        "brown_egg",
        "recovery_compass",
        "knowledge_book",
        "torchflower",
        "pitcher_plant"};

    for (const char* name : itemNames) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        EXPECT_NE(item, nullptr) << "Missing Trails & Tales item: minecraft:" << name;
    }
}

// ============================================================================
// KnowledgeBookItem 行为测试
// ============================================================================

class KnowledgeBookItemBehaviorTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    void SetUp() override
    {
        m_world = std::make_unique<KnowledgeBookTestWorld>();
        m_player = std::make_unique<KnowledgeBookTestPlayer>(m_world.get());
    }

    void TearDown() override { crafting::RecipeManager::instance().clear(); }

    std::unique_ptr<KnowledgeBookTestWorld> m_world;
    std::unique_ptr<KnowledgeBookTestPlayer> m_player;
};

TEST_F(KnowledgeBookItemBehaviorTest, NoTag_ReturnsFail)
{
    // 知识之书没有NBT标签时应返回失败
    ItemStack stack(Items::KNOWLEDGE_BOOK, 1);
    m_player->inventory().getSelectedStackRef() = stack;

    auto result = Items::KNOWLEDGE_BOOK->onItemRightClick(*m_world, *m_player, Hand::MainHand);

    EXPECT_TRUE(result.isFail());
    EXPECT_EQ(m_player->unlockedRecipeCount(), 0);
}

TEST_F(KnowledgeBookItemBehaviorTest, EmptyRecipesArray_ReturnsFail)
{
    // NBT中recipes为空数组时应返回失败
    ItemStack stack(Items::KNOWLEDGE_BOOK, 1);
    stack.getOrCreateTag()["recipes"] = nlohmann::json::array();
    m_player->inventory().getSelectedStackRef() = stack;

    auto result = Items::KNOWLEDGE_BOOK->onItemRightClick(*m_world, *m_player, Hand::MainHand);

    EXPECT_TRUE(result.isFail());
    EXPECT_EQ(m_player->unlockedRecipeCount(), 0);
}

TEST_F(KnowledgeBookItemBehaviorTest, InvalidRecipesFormat_ReturnsFail)
{
    // recipes字段不是数组时应返回失败
    ItemStack stack(Items::KNOWLEDGE_BOOK, 1);
    stack.getOrCreateTag()["recipes"] = "not_an_array";
    m_player->inventory().getSelectedStackRef() = stack;

    auto result = Items::KNOWLEDGE_BOOK->onItemRightClick(*m_world, *m_player, Hand::MainHand);

    EXPECT_TRUE(result.isFail());
    EXPECT_EQ(m_player->unlockedRecipeCount(), 0);
}

TEST_F(KnowledgeBookItemBehaviorTest, NonStringRecipeEntries_Skipped)
{
    // 数组中的非字符串条目应被跳过
    ItemStack stack(Items::KNOWLEDGE_BOOK, 1);
    stack.getOrCreateTag()["recipes"] = nlohmann::json::array({42, true, 3.14});
    m_player->inventory().getSelectedStackRef() = stack;

    auto result = Items::KNOWLEDGE_BOOK->onItemRightClick(*m_world, *m_player, Hand::MainHand);

    // 所有条目都不是字符串，因此没有有效的配方，应返回失败
    EXPECT_TRUE(result.isFail());
    EXPECT_EQ(m_player->unlockedRecipeCount(), 0);
}

TEST_F(KnowledgeBookItemBehaviorTest, UnknownRecipeId_SkippedAndNoUnlock)
{
    // 未知配方ID应被跳过，不触发unlockRecipe
    ItemStack stack(Items::KNOWLEDGE_BOOK, 1);
    stack.getOrCreateTag()["recipes"] = nlohmann::json::array({"minecraft:nonexistent_recipe"});
    m_player->inventory().getSelectedStackRef() = stack;

    auto result = Items::KNOWLEDGE_BOOK->onItemRightClick(*m_world, *m_player, Hand::MainHand);

    // 未知配方被跳过后无有效配方，应返回失败
    EXPECT_TRUE(result.isFail());
    EXPECT_EQ(m_player->unlockedRecipeCount(), 0);
}

TEST_F(KnowledgeBookItemBehaviorTest, ValidRecipes_UnlocksAndSucceeds)
{
    // 知识之书包含有效配方时应解锁配方并返回成功
    // 注册一个测试配方以使RecipeManager::hasRecipe返回true
    ResourceLocation testRecipeId("minecraft", "test_knowledge_book_recipe");
    auto recipe = std::make_unique<TestRecipe>(testRecipeId);
    crafting::RecipeManager::instance().registerRecipe(std::move(recipe));

    ItemStack stack(Items::KNOWLEDGE_BOOK, 2);
    stack.getOrCreateTag()["recipes"] = nlohmann::json::array({testRecipeId.toString()});
    m_player->inventory().getSelectedStackRef() = stack;

    auto result = Items::KNOWLEDGE_BOOK->onItemRightClick(*m_world, *m_player, Hand::MainHand);

    EXPECT_TRUE(result.isSuccess());
    EXPECT_EQ(m_player->unlockedRecipeCount(), 1);
    EXPECT_EQ(m_player->unlockedRecipes()[0], testRecipeId);

    // 生存模式下物品应被消耗一个
    EXPECT_EQ(m_player->inventory().getSelectedStack().getCount(), 1);
}

TEST_F(KnowledgeBookItemBehaviorTest, MixedValidAndInvalidRecipes_OnlyUnlocksValid)
{
    // 混合有效和无效配方时只解锁有效配方
    ResourceLocation validRecipe1("minecraft", "test_kb_recipe_valid1");
    ResourceLocation validRecipe2("minecraft", "test_kb_recipe_valid2");
    crafting::RecipeManager::instance().registerRecipe(std::make_unique<TestRecipe>(validRecipe1));
    crafting::RecipeManager::instance().registerRecipe(std::make_unique<TestRecipe>(validRecipe2));

    ItemStack stack(Items::KNOWLEDGE_BOOK, 1);
    stack.getOrCreateTag()["recipes"] =
        nlohmann::json::array({validRecipe1.toString(), "minecraft:invalid_recipe", 42, validRecipe2.toString()});
    m_player->inventory().getSelectedStackRef() = stack;

    auto result = Items::KNOWLEDGE_BOOK->onItemRightClick(*m_world, *m_player, Hand::MainHand);

    EXPECT_TRUE(result.isSuccess());
    EXPECT_EQ(m_player->unlockedRecipeCount(), 2);
    EXPECT_EQ(m_player->unlockedRecipes()[0], validRecipe1);
    EXPECT_EQ(m_player->unlockedRecipes()[1], validRecipe2);
}

TEST_F(KnowledgeBookItemBehaviorTest, CreativeMode_DoesNotConsumeItem)
{
    // 创造模式下不应消耗物品
    ResourceLocation testRecipe("minecraft", "test_kb_creative_recipe");
    crafting::RecipeManager::instance().registerRecipe(std::make_unique<TestRecipe>(testRecipe));

    // 设置为创造模式
    m_player->setGameMode(GameMode::Creative);

    ItemStack stack(Items::KNOWLEDGE_BOOK, 1);
    stack.getOrCreateTag()["recipes"] = nlohmann::json::array({testRecipe.toString()});
    m_player->inventory().getSelectedStackRef() = stack;

    auto result = Items::KNOWLEDGE_BOOK->onItemRightClick(*m_world, *m_player, Hand::MainHand);

    EXPECT_TRUE(result.isSuccess());
    EXPECT_EQ(m_player->unlockedRecipeCount(), 1);
    // 创造模式下物品不应被消耗
    EXPECT_EQ(m_player->inventory().getSelectedStack().getCount(), 1);
}
