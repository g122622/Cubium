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

/**
 * @file CraftingStatisticsTest.cpp
 * @brief 合成统计和成就触发测试
 *
 * 测试以下功能：
 * - Player 基类的虚方法默认实现
 * - ServerPlayer 的统计和成就实现
 * - ResultSlot::onTake 触发统计
 * - FurnaceResultSlot::onTake 触发统计和经验
 * - RecipeUnlockedTrigger 条件实例
 */

#include "common/advancement/AdvancementManager.hpp"
#include "common/advancement/trigger/CriterionTriggers.hpp"
#include "common/advancement/trigger/impl/EffectTriggers.hpp"
#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/entity/inventory/Slot.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/stats/StatisticsManager.hpp"
#include <gtest/gtest.h>

namespace mc {
namespace {

/**
 * @brief 测试用背包实现
 */
class TestInventory : public IInventory {
public:
    explicit TestInventory(i32 size)
        : m_size(size)
        , m_slots(size)
    {}

    i32 getContainerSize() const override { return m_size; }
    bool isEmpty() const override
    {
        for (const auto& slot : m_slots) {
            if (!slot.isEmpty()) return true;
        }
        return false;
    }
    ItemStack getItem(i32 index) const override
    {
        if (index < 0 || index >= m_size) return ItemStack::EMPTY;
        return m_slots[static_cast<std::size_t>(index)];
    }
    ItemStack removeItem(i32 index, i32 count) override
    {
        if (index < 0 || index >= m_size) return ItemStack::EMPTY;
        ItemStack& slot = m_slots[static_cast<std::size_t>(index)];
        if (slot.isEmpty()) return ItemStack::EMPTY;
        ItemStack result = slot.split(count);
        setChanged();
        return result;
    }
    ItemStack removeItemNoUpdate(i32 index) override
    {
        if (index < 0 || index >= m_size) return ItemStack::EMPTY;
        ItemStack result = std::move(m_slots[static_cast<std::size_t>(index)]);
        m_slots[static_cast<std::size_t>(index)] = ItemStack::EMPTY;
        return result;
    }
    void setItem(i32 index, const ItemStack& stack) override
    {
        if (index < 0 || index >= m_size) return;
        m_slots[static_cast<std::size_t>(index)] = stack;
        setChanged();
    }
    void clear() override
    {
        for (auto& slot : m_slots) {
            slot = ItemStack::EMPTY;
        }
        setChanged();
    }
    void setChanged() override { m_changed = true; }
    bool isChanged() const { return m_changed; }
    void clearChanged() { m_changed = false; }

    // 实现可选方法
    i32 getMaxStackSize() const override { return 64; }
    bool canPlaceItem(i32 index, const ItemStack& stack) const override
    {
        (void)index;
        (void)stack;
        return true;
    }

private:
    i32 m_size;
    std::vector<ItemStack> m_slots;
    bool m_changed = false;
};

/**
 * @brief 合成统计测试夹具
 */
class CraftingStatisticsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化方块和物品注册表
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();

        // 注册内置触发器
        advancement::CriterionTriggers::instance().registerBuiltinTriggers();

        // 创建测试玩家
        m_player = std::make_unique<ServerPlayer>(1, "TestPlayer");
    }

    void TearDown() override
    {
        m_player.reset();
        advancement::CriterionTriggers::instance().clear();
    }

    std::unique_ptr<ServerPlayer> m_player;
};

// ========== Player 基类虚方法测试 ==========

TEST_F(CraftingStatisticsTest, PlayerBaseClass_DefaultAwardCraftedStat_DoesNotThrow)
{
    // Player 基类的默认实现应该不抛异常（空实现）
    EXPECT_NO_THROW(m_player->awardCraftedStat(ResourceLocation("minecraft:diamond"), 5));
}

TEST_F(CraftingStatisticsTest, PlayerBaseClass_DefaultOnItemCrafted_DoesNotThrow)
{
    // Player 基类的默认实现应该不抛异常
    ItemStack stack(Items::DIAMOND, 10);
    EXPECT_NO_THROW(m_player->onItemCrafted(stack, 10));
}

TEST_F(CraftingStatisticsTest, PlayerBaseClass_DefaultUnlockRecipe_DoesNotThrow)
{
    // Player 基类的默认实现应该不抛异常
    EXPECT_NO_THROW(m_player->unlockRecipe(ResourceLocation("minecraft:diamond")));
}

// ========== ServerPlayer 统计测试 ==========

TEST_F(CraftingStatisticsTest, ServerPlayer_AwardCraftedStat_UpdatesStatistics)
{
    // 调用 awardCraftedStat 应该更新统计管理器
    ResourceLocation itemId("minecraft:diamond");
    m_player->awardCraftedStat(itemId, 5);

    // 验证统计已更新
    const auto& stats = m_player->getStats();
    EXPECT_EQ(stats.getValue(server::stats::StatType::Crafted, itemId), 5);
}

TEST_F(CraftingStatisticsTest, ServerPlayer_AwardCraftedStat_MultipleTimes_Accumulates)
{
    ResourceLocation itemId("minecraft:diamond");

    m_player->awardCraftedStat(itemId, 5);
    m_player->awardCraftedStat(itemId, 3);
    m_player->awardCraftedStat(itemId, 2);

    const auto& stats = m_player->getStats();
    EXPECT_EQ(stats.getValue(server::stats::StatType::Crafted, itemId), 10);
}

TEST_F(CraftingStatisticsTest, ServerPlayer_OnItemCrafted_UpdatesStatistics)
{
    // 调用 onItemCrafted 应该更新统计
    ItemStack stack(Items::DIAMOND, 10);
    m_player->onItemCrafted(stack, 10);

    const auto& stats = m_player->getStats();
    EXPECT_EQ(stats.getValue(server::stats::StatType::Crafted, ResourceLocation("minecraft:diamond")), 10);
}

TEST_F(CraftingStatisticsTest, ServerPlayer_OnItemCrafted_EmptyStack_NoStatisticsUpdate)
{
    // 空物品堆不应该更新统计
    ItemStack emptyStack;
    m_player->onItemCrafted(emptyStack, 0);

    const auto& stats = m_player->getStats();
    auto allStats = stats.getAllStats();
    EXPECT_TRUE(allStats.empty());
}

TEST_F(CraftingStatisticsTest, ServerPlayer_OnItemCrafted_NullItem_NoStatisticsUpdate)
{
    // 空物品堆不应该更新统计
    ItemStack nullStack(nullptr, 1);
    m_player->onItemCrafted(nullStack, 1);

    const auto& stats = m_player->getStats();
    auto allStats = stats.getAllStats();
    EXPECT_TRUE(allStats.empty());
}

TEST_F(CraftingStatisticsTest, ServerPlayer_OnItemCrafted_DifferentItems_TrackedSeparately)
{
    ItemStack diamondStack(Items::DIAMOND, 5);
    ItemStack ironStack(Items::IRON_INGOT, 10);

    m_player->onItemCrafted(diamondStack, 5);
    m_player->onItemCrafted(ironStack, 10);

    const auto& stats = m_player->getStats();
    EXPECT_EQ(stats.getValue(server::stats::StatType::Crafted, ResourceLocation("minecraft:diamond")), 5);
    EXPECT_EQ(stats.getValue(server::stats::StatType::Crafted, ResourceLocation("minecraft:iron_ingot")), 10);
}

// ========== RecipeUnlockedTrigger 测试 ==========

TEST_F(CraftingStatisticsTest, RecipeUnlockedTrigger_IsRegistered)
{
    // 验证 RecipeUnlockedTrigger 已注册
    auto* trigger = advancement::CriterionTriggers::instance().getTrigger<advancement::RecipeUnlockedTrigger>();
    ASSERT_NE(trigger, nullptr);
}

TEST_F(CraftingStatisticsTest, RecipeUnlockedTrigger_CreateInstanceWithRecipe)
{
    // 创建带配方ID的条件实例
    ResourceLocation recipeId("minecraft:diamond_sword");

    auto instance = std::make_shared<advancement::RecipeUnlockedTriggerInstance>(recipeId);
    ASSERT_NE(instance, nullptr);

    // 验证条件可以匹配正确的配方
    EXPECT_TRUE(instance->test(recipeId));
    EXPECT_FALSE(instance->test(ResourceLocation("minecraft:iron_sword")));
}

TEST_F(CraftingStatisticsTest, RecipeUnlockedTrigger_CreateAnyInstance)
{
    // 创建默认实例（匹配任何配方）
    // 通过 fromJson(nullptr) 创建的实例应该匹配任何配方
    nlohmann::json nullJson = nullptr;
    auto* trigger = advancement::CriterionTriggers::instance().getTrigger<advancement::RecipeUnlockedTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(nullJson);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<advancement::RecipeUnlockedTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 通过 fromJson(nullptr) 创建的实例应该匹配任何配方
    EXPECT_TRUE(instance->test(ResourceLocation("minecraft:diamond_sword")));
    EXPECT_TRUE(instance->test(ResourceLocation("minecraft:iron_ingot")));
    EXPECT_TRUE(instance->test(ResourceLocation("minecraft:oak_planks")));
}

TEST_F(CraftingStatisticsTest, RecipeUnlockedTrigger_Serialization)
{
    ResourceLocation recipeId("minecraft:diamond_sword");
    auto instance = std::make_shared<advancement::RecipeUnlockedTriggerInstance>(recipeId);

    // 序列化
    nlohmann::json json = instance->conditionsToJson();
    EXPECT_TRUE(json.contains("recipe"));
    EXPECT_EQ(json["recipe"].get<std::string>(), "minecraft:diamond_sword");
}

TEST_F(CraftingStatisticsTest, RecipeUnlockedTrigger_FromJson)
{
    nlohmann::json json = R"({
        "recipe": "minecraft:iron_sword"
    })"_json;

    auto* trigger = advancement::CriterionTriggers::instance().getTrigger<advancement::RecipeUnlockedTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(json);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<advancement::RecipeUnlockedTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
    EXPECT_TRUE(instance->test(ResourceLocation("minecraft:iron_sword")));
    EXPECT_FALSE(instance->test(ResourceLocation("minecraft:diamond_sword")));
}

// ========== ResultSlot 统计触发测试 ==========

TEST_F(CraftingStatisticsTest, ResultSlot_OnCrafting_UpdatesPlayerStatistics)
{
    // 创建测试背包和槽位
    auto inventory = std::make_unique<TestInventory>(1);
    auto craftingGrid = std::make_unique<CraftingInventory>(2, 2);

    // 创建结果槽位（使用 ResultSlot）
    ResultSlot slot(inventory.get(), 0, 0, 0, craftingGrid.get(), m_player.get());

    // 设置合成结果
    ItemStack resultStack(Items::DIAMOND, 5);
    inventory->setItem(0, resultStack);

    // 模拟合成完成 - 先调用 onCrafting(stack, amount) 追踪数量
    slot.onCrafting(resultStack, 5);

    // 验证统计已更新
    const auto& stats = m_player->getStats();
    EXPECT_EQ(stats.getValue(server::stats::StatType::Crafted, ResourceLocation("minecraft:diamond")), 5);
}

TEST_F(CraftingStatisticsTest, ResultSlot_OnCrafting_MultipleTimes_Accumulates)
{
    auto inventory = std::make_unique<TestInventory>(1);
    auto craftingGrid = std::make_unique<CraftingInventory>(2, 2);
    ResultSlot slot(inventory.get(), 0, 0, 0, craftingGrid.get(), m_player.get());

    // 模拟多次合成完成
    slot.onCrafting(ItemStack(Items::DIAMOND, 3), 3);
    slot.onCrafting(ItemStack(Items::DIAMOND, 2), 2);
    slot.onCrafting(ItemStack(Items::DIAMOND, 5), 5);

    // 验证统计累积
    const auto& stats = m_player->getStats();
    EXPECT_EQ(stats.getValue(server::stats::StatType::Crafted, ResourceLocation("minecraft:diamond")), 10);
}

TEST_F(CraftingStatisticsTest, ResultSlot_OnCrafting_NoPlayer_NoStatisticsUpdate)
{
    auto inventory = std::make_unique<TestInventory>(1);
    auto craftingGrid = std::make_unique<CraftingInventory>(2, 2);

    // 创建没有玩家的结果槽位
    ResultSlot slot(inventory.get(), 0, 0, 0, craftingGrid.get(), nullptr);

    ItemStack resultStack(Items::DIAMOND, 5);
    inventory->setItem(0, resultStack);

    // 不应该抛异常
    EXPECT_NO_THROW(slot.onCrafting(resultStack, 5));

    // 统计不应该被更新（没有玩家）
    const auto& stats = m_player->getStats();
    auto allStats = stats.getAllStats();
    EXPECT_TRUE(allStats.empty());
}

// ========== FurnaceResultSlot 统计触发测试 ==========

TEST_F(CraftingStatisticsTest, FurnaceResultSlot_OnTake_UpdatesPlayerStatistics)
{
    // 创建测试背包
    auto inventory = std::make_unique<TestInventory>(1);

    // 创建熔炉结果槽位（没有熔炉实体）
    mc::FurnaceResultSlot slot(m_player.get(), inventory.get(), 0, 0, 0, nullptr);

    // 设置熔炼结果
    ItemStack resultStack(Items::IRON_INGOT, 3);
    inventory->setItem(0, resultStack);

    // 模拟取出物品
    slot.onTake(*m_player, resultStack);

    // 验证统计已更新
    const auto& stats = m_player->getStats();
    EXPECT_EQ(stats.getValue(server::stats::StatType::Crafted, ResourceLocation("minecraft:iron_ingot")), 3);
}

TEST_F(CraftingStatisticsTest, FurnaceResultSlot_OnTake_MultipleTimes_Accumulates)
{
    auto inventory = std::make_unique<TestInventory>(1);
    mc::FurnaceResultSlot slot(m_player.get(), inventory.get(), 0, 0, 0, nullptr);

    // 模拟多次取出
    ItemStack resultStack1(Items::IRON_INGOT, 5);
    inventory->setItem(0, resultStack1);
    slot.onTake(*m_player, resultStack1);

    ItemStack resultStack2(Items::IRON_INGOT, 3);
    inventory->setItem(0, resultStack2);
    slot.onTake(*m_player, resultStack2);

    // 验证统计累积
    const auto& stats = m_player->getStats();
    EXPECT_EQ(stats.getValue(server::stats::StatType::Crafted, ResourceLocation("minecraft:iron_ingot")), 8);
}

TEST_F(CraftingStatisticsTest, FurnaceResultSlot_OnTake_NoPlayer_NoStatisticsUpdate)
{
    auto inventory = std::make_unique<TestInventory>(1);

    // 创建没有玩家的熔炉结果槽位
    mc::FurnaceResultSlot slot(nullptr, inventory.get(), 0, 0, 0, nullptr);

    ItemStack resultStack(Items::IRON_INGOT, 3);
    inventory->setItem(0, resultStack);

    // 不应该抛异常
    EXPECT_NO_THROW(slot.onTake(*m_player, resultStack));

    // 统计不应该被更新
    const auto& stats = m_player->getStats();
    auto allStats = stats.getAllStats();
    EXPECT_TRUE(allStats.empty());
}

// ========== 多态性测试 ==========

TEST_F(CraftingStatisticsTest, PolymorphicCall_ThroughPlayerBasePointer)
{
    // 通过基类指针调用应该使用 ServerPlayer 的实现
    Player* basePtr = m_player.get();

    ItemStack stack(Items::DIAMOND, 7);
    basePtr->onItemCrafted(stack, 7);

    // 验证统计已更新（使用了 ServerPlayer 的实现）
    const auto& stats = m_player->getStats();
    EXPECT_EQ(stats.getValue(server::stats::StatType::Crafted, ResourceLocation("minecraft:diamond")), 7);
}

TEST_F(CraftingStatisticsTest, PolymorphicCall_AwardCraftedStat)
{
    Player* basePtr = m_player.get();

    basePtr->awardCraftedStat(ResourceLocation("minecraft:gold_ingot"), 15);

    const auto& stats = m_player->getStats();
    EXPECT_EQ(stats.getValue(server::stats::StatType::Crafted, ResourceLocation("minecraft:gold_ingot")), 15);
}

// ========== 集成测试 ==========

TEST_F(CraftingStatisticsTest, Integration_FullCraftingWorkflow)
{
    // 模拟完整的合成流程
    auto inventory = std::make_unique<TestInventory>(1);
    auto craftingGrid = std::make_unique<CraftingInventory>(2, 2);
    ResultSlot slot(inventory.get(), 0, 0, 0, craftingGrid.get(), m_player.get());

    // 设置合成结果
    ItemStack resultStack(Items::DIAMOND_SWORD, 1);
    inventory->setItem(0, resultStack);

    // 模拟合成完成
    slot.onCrafting(resultStack, 1);

    // 验证统计已更新
    const auto& stats = m_player->getStats();
    EXPECT_EQ(stats.getValue(server::stats::StatType::Crafted, ResourceLocation("minecraft:diamond_sword")), 1);
}

TEST_F(CraftingStatisticsTest, Integration_FullSmeltingWorkflow)
{
    // 模拟完整的熔炼流程
    auto inventory = std::make_unique<TestInventory>(1);
    mc::FurnaceResultSlot slot(m_player.get(), inventory.get(), 0, 0, 0, nullptr);

    // 设置熔炼结果
    ItemStack resultStack(Items::IRON_INGOT, 5);
    inventory->setItem(0, resultStack);

    // 模拟取出
    slot.onTake(*m_player, resultStack);

    // 验证统计已更新
    const auto& stats = m_player->getStats();
    EXPECT_EQ(stats.getValue(server::stats::StatType::Crafted, ResourceLocation("minecraft:iron_ingot")), 5);
}

// ========== 边界情况测试 ==========

TEST_F(CraftingStatisticsTest, AwardCraftedStat_ZeroAmount_NoEffect)
{
    ResourceLocation itemId("minecraft:diamond");
    m_player->awardCraftedStat(itemId, 0);

    const auto& stats = m_player->getStats();
    // 0 数量不应该创建统计条目
    EXPECT_FALSE(stats.hasStat(server::stats::StatType::Crafted, itemId));
}

TEST_F(CraftingStatisticsTest, AwardCraftedStat_LargeAmount)
{
    ResourceLocation itemId("minecraft:cobblestone");
    m_player->awardCraftedStat(itemId, 1000000);

    const auto& stats = m_player->getStats();
    EXPECT_EQ(stats.getValue(server::stats::StatType::Crafted, itemId), 1000000);
}

TEST_F(CraftingStatisticsTest, OnItemCrafted_StackLargerThanCraftedCount)
{
    // 物品堆数量可能大于合成数量
    ItemStack stack(Items::DIAMOND, 64); // 满堆叠
    m_player->onItemCrafted(stack, 5);   // 但只合成了5个

    const auto& stats = m_player->getStats();
    // 统计应该使用传入的数量，而不是物品堆数量
    EXPECT_EQ(stats.getValue(server::stats::StatType::Crafted, ResourceLocation("minecraft:diamond")), 5);
}

} // namespace
} // namespace mc
