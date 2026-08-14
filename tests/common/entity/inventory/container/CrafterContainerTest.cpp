/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "entity/inventory/container/CrafterContainer.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/inventory/ContainerTypeUtils.hpp"
#include "entity/inventory/ContainerTypes.hpp"
#include "entity/inventory/CraftingInventory.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/inventory/Slot.hpp"
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/core/ItemStack.hpp"
#include "item/crafting/RecipeManager.hpp"
#include "item/crafting/ShapedRecipe.hpp"
#include "resource/ResourceLocation.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include "world/blockentity/trial/CrafterBlockEntity.hpp"
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"

using namespace mc;

namespace {

/// 按资源路径懒注册测试用物品
Item* ensureTestItem(const char* path)
{
    auto& registry = ItemRegistry::instance();
    const ResourceLocation id("minecraft", path);
    if (Item* existing = registry.getItem(id); existing != nullptr) {
        return existing;
    }
    return &registry.registerItem(id, ItemProperties().maxStackSize(64));
}

} // namespace

// ========== CrafterContainer 测试 ==========

class CrafterContainerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        player_ = std::make_unique<Player>(1, "CrafterTestPlayer", mc::test::testEcsRegistry());
        playerInventory_ = std::make_unique<PlayerInventory>(player_.get());
        // 创建合成器背包容器（9格，3x3合成网格）
        crafterInventory_ = std::make_unique<blockentity::SimpleInventory>(CrafterBlockEntity::CONTAINER_SIZE);
        // 注册测试用物品
        testItem_ = ensureTestItem("test_item");
    }

    void TearDown() override
    {
        // 控制析构顺序：先容器再背最后再玩家
    }

    std::unique_ptr<Player> player_;
    std::unique_ptr<PlayerInventory> playerInventory_;
    std::unique_ptr<blockentity::SimpleInventory> crafterInventory_;
    Item* testItem_ = nullptr;
};

// ========== 基本创建测试 ==========

TEST_F(CrafterContainerTest, Create_HasCorrectSlotCount)
{
    // 容器实际槽位数量 = 9合成格 + 1预览结果 + 27主背包 + 9快捷栏 = 46
    CrafterContainer container(ContainerId(1), playerInventory_.get(), crafterInventory_.get());
    EXPECT_EQ(container.getSlotCount(), 46);
}

TEST_F(CrafterContainerTest, ContainerId_IsCorrect)
{
    CrafterContainer container(ContainerId(42), playerInventory_.get(), crafterInventory_.get());
    EXPECT_EQ(container.getId(), ContainerId(42));
}

TEST_F(CrafterContainerTest, GetCrafterInventory_ReturnsCorrectInventory)
{
    CrafterContainer container(ContainerId(1), playerInventory_.get(), crafterInventory_.get());
    EXPECT_EQ(container.getCrafterInventory(), crafterInventory_.get());
}

TEST_F(CrafterContainerTest, GetCrafterEntity_WithoutEntity_ReturnsNullptr)
{
    CrafterContainer container(ContainerId(1), playerInventory_.get(), crafterInventory_.get());
    EXPECT_EQ(container.getCrafterEntity(), nullptr);
}

TEST_F(CrafterContainerTest, GetCrafterEntity_WithEntity_ReturnsEntity)
{
    auto crafterEntity = std::make_unique<CrafterBlockEntity>(BlockPos(10, 20, 30));
    CrafterContainer container(
        ContainerId(1), playerInventory_.get(), crafterEntity->getInventory(), crafterEntity.get());
    EXPECT_EQ(container.getCrafterEntity(), crafterEntity.get());
}

// ========== 常量验证测试 ==========

TEST_F(CrafterContainerTest, Constants_AreCorrect_MC121)
{
    // 合成网格常量
    EXPECT_EQ(CrafterContainer::CRAFT_SLOTS, 9);
    EXPECT_EQ(CrafterContainer::RESULT_SLOT, 9);
    EXPECT_EQ(CrafterContainer::CRAFTER_SLOTS, 10);

    // GUI布局常量 - MC 1.21 坐标
    EXPECT_EQ(CrafterContainer::CRAFT_SLOT_START_X, 26);
    EXPECT_EQ(CrafterContainer::CRAFT_SLOT_START_Y, 17);
    EXPECT_EQ(CrafterContainer::SLOT_SIZE, 18);
    EXPECT_EQ(CrafterContainer::RESULT_SLOT_X, 134);
    EXPECT_EQ(CrafterContainer::RESULT_SLOT_Y, 35);
    EXPECT_EQ(CrafterContainer::PLAYER_INV_Y, 84);
    EXPECT_EQ(CrafterContainer::HOTBAR_Y, 142);
}

// ========== stillValid 距离检查测试 ==========

TEST_F(CrafterContainerTest, StillValid_WithoutEntity_ReturnsTrue)
{
    // 当没有关联 CrafterBlockEntity 时，返回 true
    CrafterContainer container(ContainerId(1), playerInventory_.get(), crafterInventory_.get());
    EXPECT_TRUE(container.stillValid(*player_));
}

TEST_F(CrafterContainerTest, StillValid_WithEntity_WhenPlayerIsNearCrafter_ReturnsTrue)
{
    auto crafterEntity = std::make_unique<CrafterBlockEntity>(BlockPos(0, 64, 0));

    CrafterContainer container(
        ContainerId(1), playerInventory_.get(), crafterEntity->getInventory(), crafterEntity.get());

    // 玩家在合成器附近（距离小于8格）
    // 合成器中心在 (0.5, 64.5, 0.5)，玩家在 (0.5, 64.0, 0.5)
    // 距离平方 = 0 + 0.25 + 0 = 0.25 < 64
    player_->setPosition(0.5f, 64.0f, 0.5f);
    EXPECT_TRUE(container.stillValid(*player_));

    // 玩家在合成器8格边缘（距离刚好等于8格）
    // 合成器中心在 (0.5, 64.5, 0.5)，玩家在 (8.5, 64.5, 0.5)
    // 距离平方 = 64 + 0 + 0 = 64 <= 64（边界情况，应该返回true）
    player_->setPosition(8.5f, 64.5f, 0.5f);
    EXPECT_TRUE(container.stillValid(*player_));
}

TEST_F(CrafterContainerTest, StillValid_WithEntity_WhenPlayerIsTooFar_ReturnsFalse)
{
    auto crafterEntity = std::make_unique<CrafterBlockEntity>(BlockPos(0, 64, 0));

    CrafterContainer container(
        ContainerId(1), playerInventory_.get(), crafterEntity->getInventory(), crafterEntity.get());

    // 玩家距离合成器超过8格
    // 合成器中心在 (0.5, 64.5, 0.5)，玩家在 (12.5, 64.0, 0.5)
    // 距离平方 = 144 + 0.25 + 0 = 144.25 > 64
    player_->setPosition(12.5f, 64.0f, 0.5f);
    EXPECT_FALSE(container.stillValid(*player_));

    // 玩家距离合成器刚好超过8格（8.01格）
    // 合成器中心在 (0.5, 64.5, 0.5)，玩家在 (8.51, 64.5, 0.5)
    // 距离平方 = 64.1601 > 64
    player_->setPosition(8.51f, 64.5f, 0.5f);
    EXPECT_FALSE(container.stillValid(*player_));
}

// ========== 槽位禁用状态测试 ==========

TEST_F(CrafterContainerTest, IsSlotDisabled_WithoutEntity_ReturnsFalse)
{
    CrafterContainer container(ContainerId(1), playerInventory_.get(), crafterInventory_.get());

    // 没有实体时，所有槽位都不应报告禁用
    for (i32 i = 0; i < CrafterContainer::CRAFT_SLOTS; ++i) {
        EXPECT_FALSE(container.isSlotDisabled(i));
    }
}

TEST_F(CrafterContainerTest, IsSlotDisabled_WithEntity_InitialStateAllEnabled)
{
    auto crafterEntity = std::make_unique<CrafterBlockEntity>(BlockPos(10, 20, 30));
    CrafterContainer container(
        ContainerId(1), playerInventory_.get(), crafterEntity->getInventory(), crafterEntity.get());

    // 初始状态下所有槽位应该启用
    for (i32 i = 0; i < CrafterContainer::CRAFT_SLOTS; ++i) {
        EXPECT_FALSE(container.isSlotDisabled(i));
    }
}

TEST_F(CrafterContainerTest, SetSlotState_DisableSlot)
{
    auto crafterEntity = std::make_unique<CrafterBlockEntity>(BlockPos(10, 20, 30));
    CrafterContainer container(
        ContainerId(1), playerInventory_.get(), crafterEntity->getInventory(), crafterEntity.get());

    // 禁用槽位0
    container.setSlotState(0, false);
    EXPECT_TRUE(container.isSlotDisabled(0));

    // 其他槽位应仍为启用
    for (i32 i = 1; i < CrafterContainer::CRAFT_SLOTS; ++i) {
        EXPECT_FALSE(container.isSlotDisabled(i));
    }
}

TEST_F(CrafterContainerTest, SetSlotState_EnableSlot)
{
    auto crafterEntity = std::make_unique<CrafterBlockEntity>(BlockPos(10, 20, 30));
    CrafterContainer container(
        ContainerId(1), playerInventory_.get(), crafterEntity->getInventory(), crafterEntity.get());

    // 禁用后再启用
    container.setSlotState(4, false);
    EXPECT_TRUE(container.isSlotDisabled(4));

    container.setSlotState(4, true);
    EXPECT_FALSE(container.isSlotDisabled(4));
}

TEST_F(CrafterContainerTest, IsSlotDisabled_InvalidIndex_ReturnsFalse)
{
    auto crafterEntity = std::make_unique<CrafterBlockEntity>(BlockPos(10, 20, 30));
    CrafterContainer container(
        ContainerId(1), playerInventory_.get(), crafterEntity->getInventory(), crafterEntity.get());

    // 越界索引应返回false
    EXPECT_FALSE(container.isSlotDisabled(-1));
    EXPECT_FALSE(container.isSlotDisabled(9));
    EXPECT_FALSE(container.isSlotDisabled(100));
}

TEST_F(CrafterContainerTest, SetSlotState_InvalidIndex_DoesNothing)
{
    auto crafterEntity = std::make_unique<CrafterBlockEntity>(BlockPos(10, 20, 30));
    CrafterContainer container(
        ContainerId(1), playerInventory_.get(), crafterEntity->getInventory(), crafterEntity.get());

    // 越界索引不应崩溃，也不影响有效槽位
    container.setSlotState(-1, false);
    container.setSlotState(9, false);
    container.setSlotState(100, false);

    for (i32 i = 0; i < CrafterContainer::CRAFT_SLOTS; ++i) {
        EXPECT_FALSE(container.isSlotDisabled(i));
    }
}

// ========== CrafterInputSlot 禁用槽位测试 ==========

TEST_F(CrafterContainerTest, CrafterInputSlot_DisabledSlot_RejectsPlacement)
{
    auto crafterEntity = std::make_unique<CrafterBlockEntity>(BlockPos(10, 20, 30));
    CrafterContainer container(
        ContainerId(1), playerInventory_.get(), crafterEntity->getInventory(), crafterEntity.get());

    // 禁用槽位0
    container.setSlotState(0, false);
    EXPECT_TRUE(container.isSlotDisabled(0));

    // 禁用槽位不允许放置物品
    Slot* slot0 = container.getSlot(0);
    ASSERT_NE(slot0, nullptr);
    ItemStack testStack(testItem_, 1);
    EXPECT_FALSE(slot0->mayPlace(testStack));
}

TEST_F(CrafterContainerTest, CrafterInputSlot_EnabledSlot_AllowsPlacement)
{
    auto crafterEntity = std::make_unique<CrafterBlockEntity>(BlockPos(10, 20, 30));
    CrafterContainer container(
        ContainerId(1), playerInventory_.get(), crafterEntity->getInventory(), crafterEntity.get());

    // 启用槽位允许放置物品
    Slot* slot0 = container.getSlot(0);
    ASSERT_NE(slot0, nullptr);
    ItemStack testStack(testItem_, 1);
    EXPECT_TRUE(slot0->mayPlace(testStack));
}

// ========== CrafterResultSlot 预览结果槽位测试 ==========

TEST_F(CrafterContainerTest, CrafterResultSlot_RejectsPlacement)
{
    CrafterContainer container(ContainerId(1), playerInventory_.get(), crafterInventory_.get());

    // 预览结果槽位（索引9）不允许放置物品
    Slot* resultSlot = container.getSlot(CrafterContainer::RESULT_SLOT);
    ASSERT_NE(resultSlot, nullptr);
    ItemStack testStack(testItem_, 1);
    EXPECT_FALSE(resultSlot->mayPlace(testStack));
}

TEST_F(CrafterContainerTest, CrafterResultSlot_RejectsPickup)
{
    CrafterContainer container(ContainerId(1), playerInventory_.get(), crafterInventory_.get());

    // 预览结果槽位不允许取出物品（自动合成器通过红石触发，非手动提取）
    Slot* resultSlot = container.getSlot(CrafterContainer::RESULT_SLOT);
    ASSERT_NE(resultSlot, nullptr);
    EXPECT_FALSE(resultSlot->mayPickup(*player_));
}

TEST_F(CrafterContainerTest, CrafterResultSlot_IsValid)
{
    CrafterContainer container(ContainerId(1), playerInventory_.get(), crafterInventory_.get());

    // 预览结果槽位始终有效
    Slot* resultSlot = container.getSlot(CrafterContainer::RESULT_SLOT);
    ASSERT_NE(resultSlot, nullptr);
    EXPECT_TRUE(resultSlot->isValid());
}

// ========== clicked(QuickMove) 快速移动测试 ==========

TEST_F(CrafterContainerTest, QuickMove_FromCraftGrid_ToPlayerInventory)
{
    CrafterContainer container(ContainerId(1), playerInventory_.get(), crafterInventory_.get());

    // 在合成网格槽位0放入物品
    ItemStack testStack(testItem_, 32);
    crafterInventory_->setItem(0, testStack.copy());

    // 从合成网格（槽位0）Shift+点击到玩家背包（通过公开的clicked接口）
    container.clicked(0, 0, ClickType::QuickMove, *player_);

    // 验证物品已从合成网格移走
    EXPECT_TRUE(crafterInventory_->getItem(0).isEmpty());

    // 验证物品已移到玩家背包中
    bool found = false;
    for (i32 i = 0; i < playerInventory_->getContainerSize(); ++i) {
        if (!playerInventory_->getItem(i).isEmpty() && playerInventory_->getItem(i).getItem() == testItem_) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(CrafterContainerTest, QuickMove_FromResultSlot_DoesNothing)
{
    CrafterContainer container(ContainerId(1), playerInventory_.get(), crafterInventory_.get());

    // 预览结果槽位不允许快速移动，clicked应无效果
    ItemStack carried = container.clicked(CrafterContainer::RESULT_SLOT, 0, ClickType::QuickMove, *player_);
    EXPECT_TRUE(carried.isEmpty());
}

TEST_F(CrafterContainerTest, QuickMove_FromEmptyCraftSlot_DoesNothing)
{
    CrafterContainer container(ContainerId(1), playerInventory_.get(), crafterInventory_.get());

    // 空槽位快速移动应无效果
    ItemStack carried = container.clicked(0, 0, ClickType::QuickMove, *player_);
    EXPECT_TRUE(carried.isEmpty());
}

TEST_F(CrafterContainerTest, QuickMove_FromPlayerInventory_ToCraftGrid)
{
    CrafterContainer container(ContainerId(1), playerInventory_.get(), crafterInventory_.get());

    // 在玩家主背包槽位放入物品
    // 玩家主背包索引9（主背包第1格），对应容器槽位索引 = CRAFTER_SLOTS + 0 = 10
    ItemStack testStack(testItem_, 16);
    playerInventory_->setItem(9, testStack.copy());

    // 从玩家背包槽位快速移动到合成网格（通过公开的clicked接口）
    container.clicked(CrafterContainer::CRAFTER_SLOTS, 0, ClickType::QuickMove, *player_);

    // 验证物品已从玩家背包移走
    EXPECT_TRUE(playerInventory_->getItem(9).isEmpty());

    // 验证物品已移到合成网格中
    bool found = false;
    for (i32 i = 0; i < CrafterContainer::CRAFT_SLOTS; ++i) {
        if (!crafterInventory_->getItem(i).isEmpty() && crafterInventory_->getItem(i).getItem() == testItem_) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

// ========== slotsChanged 测试 ==========

TEST_F(CrafterContainerTest, SlotsChanged_DoesNotCrash)
{
    CrafterContainer container(ContainerId(1), playerInventory_.get(), crafterInventory_.get());

    // slotsChanged 不应崩溃
    container.slotsChanged(crafterInventory_.get());
    container.slotsChanged(playerInventory_.get());

    SUCCEED() << "slotsChanged did not crash";
}

TEST_F(CrafterContainerTest, SlotsChanged_UpdatesResultOnInventoryChange)
{
    auto crafterEntity = std::make_unique<CrafterBlockEntity>(BlockPos(10, 20, 30));
    CrafterContainer container(
        ContainerId(1), playerInventory_.get(), crafterEntity->getInventory(), crafterEntity.get());

    // 空网格不应匹配任何配方
    container.slotsChanged(crafterEntity->getInventory());
    EXPECT_FALSE(container.getResultInventory().hasResult());
}

TEST_F(CrafterContainerTest, SetSlotState_UpdatesResult)
{
    auto crafterEntity = std::make_unique<CrafterBlockEntity>(BlockPos(10, 20, 30));
    CrafterContainer container(
        ContainerId(1), playerInventory_.get(), crafterEntity->getInventory(), crafterEntity.get());

    // 禁用一个槽位应触发 updateResult
    container.setSlotState(0, false);
    // 结果应为空（空网格不会匹配任何配方）
    EXPECT_FALSE(container.getResultInventory().hasResult());
}

TEST_F(CrafterContainerTest, UpdateResult_EmptyGrid_NoMatch)
{
    auto crafterEntity = std::make_unique<CrafterBlockEntity>(BlockPos(10, 20, 30));
    CrafterContainer container(
        ContainerId(1), playerInventory_.get(), crafterEntity->getInventory(), crafterEntity.get());

    // 空网格不应匹配任何配方
    container.updateResult();
    EXPECT_FALSE(container.getResultInventory().hasResult());
    EXPECT_EQ(container.getCurrentRecipeId(), ResourceLocation());
}

TEST_F(CrafterContainerTest, UpdateResult_WithEntity_UsesAsCraftInput)
{
    auto crafterEntity = std::make_unique<CrafterBlockEntity>(BlockPos(10, 20, 30));
    CrafterContainer container(
        ContainerId(1), playerInventory_.get(), crafterEntity->getInventory(), crafterEntity.get());

    // 禁用槽位0
    container.setSlotState(0, false);
    EXPECT_TRUE(container.isSlotDisabled(0));

    // asCraftInput 应将禁用槽位视为空，空网格不应匹配任何配方
    container.updateResult();
    EXPECT_FALSE(container.getResultInventory().hasResult());
}

TEST_F(CrafterContainerTest, UpdateResult_WithoutEntity_FallbackPath)
{
    // 无 CrafterBlockEntity 时，使用直接从 IInventory 构建输入的回退路径
    CrafterContainer container(ContainerId(1), playerInventory_.get(), crafterInventory_.get());

    // 空网格不应匹配任何配方
    container.updateResult();
    EXPECT_FALSE(container.getResultInventory().hasResult());
    EXPECT_EQ(container.getCurrentRecipeId(), ResourceLocation());
}

TEST_F(CrafterContainerTest, GetResultInventory_InitiallyEmpty)
{
    CrafterContainer container(ContainerId(1), playerInventory_.get(), crafterInventory_.get());

    // 空网格的结果应为空
    EXPECT_TRUE(container.getResultInventory().getItem(0).isEmpty());
    EXPECT_FALSE(container.getResultInventory().hasResult());
}

// ========== ContainerType 枚举值测试 ==========

TEST(CrafterTypeTest, ContainerType_HasCorrectValue)
{
    // MC 1.21.11 协议ID: Crafter = 24
    EXPECT_EQ(static_cast<u8>(ContainerType::Crafter), 24);
}

TEST(CrafterTypeTest, ContainerTypes_GetSlotCount_Crafter)
{
    EXPECT_EQ(ContainerTypes::getSlotCount(ContainerType::Crafter), 9);
}

TEST(CrafterTypeTest, ContainerTypes_GetDefaultTitle_Crafter)
{
    EXPECT_STREQ(ContainerTypes::getDefaultTitle(ContainerType::Crafter), "Crafter");
}

// ========== 析构顺序测试 ==========

TEST_F(CrafterContainerTest, DestructionOrder_DoesNotCrash)
{
    // 验证容器析构不会因为背包/实体析构顺序而崩溃
    // 容器先析构，然后背最后析构，最后玩家析构
    {
        CrafterContainer container(ContainerId(1), playerInventory_.get(), crafterInventory_.get());
    }
    // 容器已析构，背包和玩家仍然存活
    SUCCEED() << "Container destruction did not crash";
}

TEST_F(CrafterContainerTest, DestructionOrder_WithEntity_DoesNotCrash)
{
    auto crafterEntity = std::make_unique<CrafterBlockEntity>(BlockPos(10, 20, 30));
    {
        CrafterContainer container(
            ContainerId(1), playerInventory_.get(), crafterEntity->getInventory(), crafterEntity.get());
    }
    SUCCEED() << "Container with entity destruction did not crash";
}

// ========== 配方匹配集成测试 ==========

class CrafterRecipeIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 确保 RecipeManager 为空，避免跨测试污染
        crafting::RecipeManager::instance().clear();

        player_ = std::make_unique<Player>(1, "RecipeTestPlayer", mc::test::testEcsRegistry());
        playerInventory_ = std::make_unique<PlayerInventory>(player_.get());
        crafterEntity_ = std::make_unique<CrafterBlockEntity>(BlockPos(0, 64, 0));
    }

    void TearDown() override { crafting::RecipeManager::instance().clear(); }

    std::unique_ptr<Player> player_;
    std::unique_ptr<PlayerInventory> playerInventory_;
    std::unique_ptr<CrafterBlockEntity> crafterEntity_;
};

// 测试：注册 2x2 有序配方，放置匹配物品后预览结果正确
TEST_F(CrafterRecipeIntegrationTest, ShapedRecipe_MatchingItems_ProducesResult)
{
    Item* planks = ensureTestItem("oak_planks");
    Item* craftingTable = ensureTestItem("crafting_table");
    ASSERT_NE(planks, nullptr);
    ASSERT_NE(craftingTable, nullptr);

    // 注册 2x2 合成台配方（4个橡木板 → 1个合成台）
    crafting::Ingredient planksIng = crafting::Ingredient::fromItem(*planks);
    auto recipe = std::make_unique<crafting::ShapedRecipe>(ResourceLocation("minecraft", "crafting_table"),
        2, // width
        2, // height
        std::vector<crafting::Ingredient>{planksIng, planksIng, planksIng, planksIng},
        ItemStack(*craftingTable, 1));
    ASSERT_TRUE(crafting::RecipeManager::instance().registerRecipe(std::move(recipe)));

    // 在合成器网格左上角放置 4 个橡木板
    // 槽位布局：(0,0)=slot0, (1,0)=slot1, (2,0)=slot2
    //           (0,1)=slot3, (1,1)=slot4, (2,1)=slot5
    //           (0,2)=slot6, (1,2)=slot7, (2,2)=slot8
    IInventory* inv = crafterEntity_->getInventory();
    inv->setItem(0, ItemStack(*planks, 1)); // (0,0)
    inv->setItem(1, ItemStack(*planks, 1)); // (1,0)
    inv->setItem(3, ItemStack(*planks, 1)); // (0,1)
    inv->setItem(4, ItemStack(*planks, 1)); // (1,1)

    CrafterContainer container(
        ContainerId(1), playerInventory_.get(), crafterEntity_->getInventory(), crafterEntity_.get());
    container.updateResult();

    // 预览结果应为 1 个合成台
    const CraftResultInventory& resultInv = container.getResultInventory();
    ASSERT_TRUE(resultInv.hasResult());
    EXPECT_EQ(resultInv.getItem(0).getItem(), craftingTable);
    EXPECT_EQ(resultInv.getItem(0).getCount(), 1);

    // 当前配方 ID 应匹配
    EXPECT_EQ(container.getCurrentRecipeId(), ResourceLocation("minecraft", "crafting_table"));
}

// 测试：物品不匹配配方时预览结果为空
TEST_F(CrafterRecipeIntegrationTest, ShapedRecipe_IncompletePattern_NoResult)
{
    Item* planks = ensureTestItem("oak_planks");
    Item* craftingTable = ensureTestItem("crafting_table");

    // 注册 2x2 合成台配方
    crafting::Ingredient planksIng = crafting::Ingredient::fromItem(*planks);
    auto recipe = std::make_unique<crafting::ShapedRecipe>(ResourceLocation("minecraft", "crafting_table"),
        2,
        2,
        std::vector<crafting::Ingredient>{planksIng, planksIng, planksIng, planksIng},
        ItemStack(*craftingTable, 1));
    crafting::RecipeManager::instance().registerRecipe(std::move(recipe));

    // 只放 3 个橡木板（缺少 slot4），配方不匹配
    IInventory* inv = crafterEntity_->getInventory();
    inv->setItem(0, ItemStack(*planks, 1));
    inv->setItem(1, ItemStack(*planks, 1));
    inv->setItem(3, ItemStack(*planks, 1));
    // slot 4 为空

    CrafterContainer container(
        ContainerId(1), playerInventory_.get(), crafterEntity_->getInventory(), crafterEntity_.get());
    container.updateResult();

    EXPECT_FALSE(container.getResultInventory().hasResult());
    EXPECT_EQ(container.getCurrentRecipeId(), ResourceLocation());
}

// 测试：禁用槽位被视为空，导致配方不匹配
TEST_F(CrafterRecipeIntegrationTest, DisabledSlot_TreatedAsEmpty_NoResult)
{
    Item* planks = ensureTestItem("oak_planks");
    Item* craftingTable = ensureTestItem("crafting_table");

    crafting::Ingredient planksIng = crafting::Ingredient::fromItem(*planks);
    auto recipe = std::make_unique<crafting::ShapedRecipe>(ResourceLocation("minecraft", "crafting_table"),
        2,
        2,
        std::vector<crafting::Ingredient>{planksIng, planksIng, planksIng, planksIng},
        ItemStack(*craftingTable, 1));
    crafting::RecipeManager::instance().registerRecipe(std::move(recipe));

    // 先禁用 slot4（空槽位才能禁用），再放置其他物品
    crafterEntity_->setSlotState(4, false);
    ASSERT_TRUE(crafterEntity_->isSlotDisabled(4));

    // 放置 3 个橡木板（slot4 被禁用，即使放物品也会自动启用，所以这里不放）
    IInventory* inv = crafterEntity_->getInventory();
    inv->setItem(0, ItemStack(*planks, 1));
    inv->setItem(1, ItemStack(*planks, 1));
    inv->setItem(3, ItemStack(*planks, 1));

    CrafterContainer container(
        ContainerId(1), playerInventory_.get(), crafterEntity_->getInventory(), crafterEntity_.get());
    container.updateResult();

    // 禁用槽位被视为空，只有 3 个有效物品，配方不匹配
    EXPECT_FALSE(container.getResultInventory().hasResult());
}

// 测试：getCurrentRecipeId 在有匹配配方时返回正确的 ID
TEST_F(CrafterRecipeIntegrationTest, GetCurrentRecipeId_Matching_ReturnsRecipeId)
{
    Item* planks = ensureTestItem("oak_planks");
    Item* craftingTable = ensureTestItem("crafting_table");

    crafting::Ingredient planksIng = crafting::Ingredient::fromItem(*planks);
    auto recipe = std::make_unique<crafting::ShapedRecipe>(ResourceLocation("minecraft", "crafting_table"),
        2,
        2,
        std::vector<crafting::Ingredient>{planksIng, planksIng, planksIng, planksIng},
        ItemStack(*craftingTable, 1));
    crafting::RecipeManager::instance().registerRecipe(std::move(recipe));

    // 放置匹配物品
    IInventory* inv = crafterEntity_->getInventory();
    inv->setItem(0, ItemStack(*planks, 1));
    inv->setItem(1, ItemStack(*planks, 1));
    inv->setItem(3, ItemStack(*planks, 1));
    inv->setItem(4, ItemStack(*planks, 1));

    CrafterContainer container(
        ContainerId(1), playerInventory_.get(), crafterEntity_->getInventory(), crafterEntity_.get());
    container.updateResult();

    EXPECT_EQ(container.getCurrentRecipeId(), ResourceLocation("minecraft", "crafting_table"));
}

// 测试：getCurrentRecipeId 在无匹配配方时返回空 ID
TEST_F(CrafterRecipeIntegrationTest, GetCurrentRecipeId_NoMatch_ReturnsEmpty)
{
    Item* planks = ensureTestItem("oak_planks");

    // 不注册任何配方

    IInventory* inv = crafterEntity_->getInventory();
    inv->setItem(0, ItemStack(*planks, 1));

    CrafterContainer container(
        ContainerId(1), playerInventory_.get(), crafterEntity_->getInventory(), crafterEntity_.get());
    container.updateResult();

    EXPECT_EQ(container.getCurrentRecipeId(), ResourceLocation());
}

// 测试：slotsChanged 触发预览结果更新
TEST_F(CrafterRecipeIntegrationTest, SlotsChanged_TriggersResultUpdate)
{
    Item* planks = ensureTestItem("oak_planks");
    Item* craftingTable = ensureTestItem("crafting_table");

    crafting::Ingredient planksIng = crafting::Ingredient::fromItem(*planks);
    auto recipe = std::make_unique<crafting::ShapedRecipe>(ResourceLocation("minecraft", "crafting_table"),
        2,
        2,
        std::vector<crafting::Ingredient>{planksIng, planksIng, planksIng, planksIng},
        ItemStack(*craftingTable, 1));
    crafting::RecipeManager::instance().registerRecipe(std::move(recipe));

    IInventory* inv = crafterEntity_->getInventory();

    CrafterContainer container(
        ContainerId(1), playerInventory_.get(), crafterEntity_->getInventory(), crafterEntity_.get());

    // 初始状态：空网格，无匹配
    EXPECT_FALSE(container.getResultInventory().hasResult());

    // 放置匹配物品后，调用 slotsChanged 触发更新
    inv->setItem(0, ItemStack(*planks, 1));
    inv->setItem(1, ItemStack(*planks, 1));
    inv->setItem(3, ItemStack(*planks, 1));
    inv->setItem(4, ItemStack(*planks, 1));
    container.slotsChanged(inv);

    // 预览结果应更新为合成台
    ASSERT_TRUE(container.getResultInventory().hasResult());
    EXPECT_EQ(container.getResultInventory().getItem(0).getItem(), craftingTable);

    // 移除一个物品，slotsChanged 应清除结果
    inv->setItem(4, ItemStack());
    container.slotsChanged(inv);
    EXPECT_FALSE(container.getResultInventory().hasResult());
}

// 测试：setSlotState 禁用槽位后触发预览结果更新
TEST_F(CrafterRecipeIntegrationTest, SetSlotState_DisableSlot_UpdatesResult)
{
    Item* planks = ensureTestItem("oak_planks");
    Item* craftingTable = ensureTestItem("crafting_table");

    crafting::Ingredient planksIng = crafting::Ingredient::fromItem(*planks);
    auto recipe = std::make_unique<crafting::ShapedRecipe>(ResourceLocation("minecraft", "crafting_table"),
        2,
        2,
        std::vector<crafting::Ingredient>{planksIng, planksIng, planksIng, planksIng},
        ItemStack(*craftingTable, 1));
    crafting::RecipeManager::instance().registerRecipe(std::move(recipe));

    // 放置 4 个橡木板（左上角 2x2），先不放 slot4
    // 改用 slot 0,1,3,6 的布局（不匹配 2x2 有序配方）
    IInventory* inv = crafterEntity_->getInventory();
    inv->setItem(0, ItemStack(*planks, 1));
    inv->setItem(1, ItemStack(*planks, 1));
    inv->setItem(3, ItemStack(*planks, 1));

    CrafterContainer container(
        ContainerId(1), playerInventory_.get(), crafterEntity_->getInventory(), crafterEntity_.get());

    // 初始：只有 3 个物品，不匹配 2x2 配方
    EXPECT_FALSE(container.getResultInventory().hasResult());

    // 在空 slot4 上放入物品，使其匹配 2x2 配方
    inv->setItem(4, ItemStack(*planks, 1));
    container.slotsChanged(inv);

    // 现在应该匹配
    ASSERT_TRUE(container.getResultInventory().hasResult());
    EXPECT_EQ(container.getResultInventory().getItem(0).getItem(), craftingTable);

    // 移除 slot4 的物品
    inv->setItem(4, ItemStack());

    // 禁用空 slot4 并验证结果被清除
    container.setSlotState(4, false);
    EXPECT_TRUE(container.isSlotDisabled(4));
    EXPECT_FALSE(container.getResultInventory().hasResult());
}
