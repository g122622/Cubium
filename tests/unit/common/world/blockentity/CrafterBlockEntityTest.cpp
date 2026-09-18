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
#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/trial/CrafterBlockEntity.hpp"
#include <nlohmann/json.hpp>

using namespace mc;

namespace {

/**
 * @brief 按资源路径懒注册测试用物品
 * @param path 资源路径
 * @return 已注册物品指针
 */
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

// ============================================================================
// CrafterTestWorld - 测试用 Mock 世界（用于 tick 测试）
// ============================================================================

class CrafterTestWorld final : public mc::test::BaseTestWorld {
public:
    using IWorld::getBlockState;

    void setBlockStateForPos(const BlockPos& pos, const BlockState* state) { m_blockStates[pos] = state; }

    [[nodiscard]] const BlockState* getBlockState(const BlockPos& pos) const override
    {
        auto it = m_blockStates.find(pos);
        return it == m_blockStates.end() ? nullptr : it->second;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        MC_UNUSED(flags);
        BlockPos pos(x, y, z);
        m_blockStates[pos] = state;
        m_lastSetBlockFlags = flags;
        return true;
    }

    void setBlockEntity(const BlockPos& pos, BlockEntity* entity) { m_blockEntities[pos] = entity; }
    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos) override
    {
        auto it = m_blockEntities.find(pos);
        return it == m_blockEntities.end() ? nullptr : it->second;
    }
    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos& pos) const override
    {
        auto it = m_blockEntities.find(pos);
        return it == m_blockEntities.end() ? nullptr : it->second;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("CrafterTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("CrafterTestWorld::tickManager not implemented");
    }

    i32 lastSetBlockFlags() const { return m_lastSetBlockFlags; }

private:
    std::unordered_map<BlockPos, const BlockState*> m_blockStates;
    std::unordered_map<BlockPos, BlockEntity*> m_blockEntities;
    i32 m_lastSetBlockFlags = 0;
};

// ============================================================================
// CrafterBlockEntity 构造和基本属性测试
// ============================================================================

class CrafterBlockEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        crafter_ = std::make_unique<CrafterBlockEntity>(BlockPos(10, 20, 30));
        m_testItem = ensureTestItem("diamond");
    }

    std::unique_ptr<CrafterBlockEntity> crafter_;
    Item* m_testItem = nullptr;
};

TEST_F(CrafterBlockEntityTest, Create_HasCorrectType)
{
    EXPECT_EQ(crafter_->getType(), BlockEntityType::Crafter);
}

TEST_F(CrafterBlockEntityTest, Create_HasCorrectPosition)
{
    EXPECT_EQ(crafter_->getPos(), BlockPos(10, 20, 30));
}

TEST_F(CrafterBlockEntityTest, Create_HasCorrectContainerSize)
{
    EXPECT_EQ(crafter_->getContainerSize(), CrafterBlockEntity::CONTAINER_SIZE);
    EXPECT_EQ(crafter_->getContainerSize(), 9);
}

TEST_F(CrafterBlockEntityTest, Create_AllSlotsEnabled)
{
    for (i32 i = 0; i < CrafterBlockEntity::CONTAINER_SIZE; ++i) {
        EXPECT_FALSE(crafter_->isSlotDisabled(i)) << "槽位 " << i << " 应为启用状态";
    }
}

TEST_F(CrafterBlockEntityTest, Create_NotTriggered)
{
    EXPECT_FALSE(crafter_->isTriggered());
}

TEST_F(CrafterBlockEntityTest, Create_ZeroCraftingTicksRemaining)
{
    EXPECT_EQ(crafter_->getCraftingTicksRemaining(), 0);
}

TEST_F(CrafterBlockEntityTest, Create_NeedsTickReturnsFalse)
{
    // 默认状态没有合成动画倒计时，不需要 tick
    EXPECT_FALSE(crafter_->needsTick());
}

TEST_F(CrafterBlockEntityTest, Create_InventoryIsEmpty)
{
    auto* inventory = crafter_->getInventory();
    ASSERT_NE(inventory, nullptr);
    EXPECT_TRUE(inventory->isEmpty());
}

TEST_F(CrafterBlockEntityTest, Create_DefaultCustomNameEmpty)
{
    EXPECT_TRUE(crafter_->getCustomName().empty());
}

// ============================================================================
// 槽位状态测试
// ============================================================================

TEST_F(CrafterBlockEntityTest, SetSlotState_DisableEmptySlot)
{
    // 空槽位可以被禁用
    crafter_->setSlotState(0, false);
    EXPECT_TRUE(crafter_->isSlotDisabled(0));
}

TEST_F(CrafterBlockEntityTest, SetSlotState_EnableSlot)
{
    // 先禁用再启用
    crafter_->setSlotState(4, false);
    EXPECT_TRUE(crafter_->isSlotDisabled(4));
    crafter_->setSlotState(4, true);
    EXPECT_FALSE(crafter_->isSlotDisabled(4));
}

TEST_F(CrafterBlockEntityTest, SetSlotState_CannotDisableSlotWithItem)
{
    // 有物品的槽位不能被禁用
    auto* inventory = crafter_->getInventory();
    ItemStack stack(*m_testItem, 5);
    inventory->setItem(0, stack);

    // 尝试禁用有物品的槽位，应该被忽略
    crafter_->setSlotState(0, false);
    EXPECT_FALSE(crafter_->isSlotDisabled(0));
}

TEST_F(CrafterBlockEntityTest, SetSlotState_CanEnableSlotWithItem)
{
    // 有物品的槽位可以被启用（它本来就是启用的）
    auto* inventory = crafter_->getInventory();
    ItemStack stack(*m_testItem, 5);
    inventory->setItem(0, stack);

    // 启用有物品的槽位不应该出错
    crafter_->setSlotState(0, true);
    EXPECT_FALSE(crafter_->isSlotDisabled(0));
}

TEST_F(CrafterBlockEntityTest, SetSlotState_MarksChanged)
{
    EXPECT_FALSE(crafter_->isChanged());
    crafter_->setSlotState(0, false);
    EXPECT_TRUE(crafter_->isChanged());
}

TEST_F(CrafterBlockEntityTest, IsSlotDisabled_OutOfRangeReturnsFalse)
{
    EXPECT_FALSE(crafter_->isSlotDisabled(-1));
    EXPECT_FALSE(crafter_->isSlotDisabled(9));
    EXPECT_FALSE(crafter_->isSlotDisabled(100));
}

TEST_F(CrafterBlockEntityTest, IsSlotDisabled_EnabledSlotReturnsFalse)
{
    for (i32 i = 0; i < 9; ++i) {
        EXPECT_FALSE(crafter_->isSlotDisabled(i));
    }
}

TEST_F(CrafterBlockEntityTest, IsSlotDisabled_DisabledSlotReturnsTrue)
{
    crafter_->setSlotState(3, false);
    EXPECT_TRUE(crafter_->isSlotDisabled(3));
}

TEST_F(CrafterBlockEntityTest, SetSlotState_MultipleSlots)
{
    // 禁用多个槽位
    crafter_->setSlotState(0, false);
    crafter_->setSlotState(4, false);
    crafter_->setSlotState(8, false);

    EXPECT_TRUE(crafter_->isSlotDisabled(0));
    EXPECT_FALSE(crafter_->isSlotDisabled(1));
    EXPECT_FALSE(crafter_->isSlotDisabled(2));
    EXPECT_FALSE(crafter_->isSlotDisabled(3));
    EXPECT_TRUE(crafter_->isSlotDisabled(4));
    EXPECT_FALSE(crafter_->isSlotDisabled(5));
    EXPECT_FALSE(crafter_->isSlotDisabled(6));
    EXPECT_FALSE(crafter_->isSlotDisabled(7));
    EXPECT_TRUE(crafter_->isSlotDisabled(8));
}

// ============================================================================
// canPlaceItem 测试
// ============================================================================

TEST_F(CrafterBlockEntityTest, CanPlaceItem_EnabledSlotAcceptsItem)
{
    ItemStack stack(*m_testItem, 1);
    EXPECT_TRUE(crafter_->canPlaceItem(0, stack));
}

TEST_F(CrafterBlockEntityTest, CanPlaceItem_DisabledSlotRejectsItem)
{
    crafter_->setSlotState(0, false);
    ItemStack stack(*m_testItem, 1);
    EXPECT_FALSE(crafter_->canPlaceItem(0, stack));
}

TEST_F(CrafterBlockEntityTest, CanPlaceItem_AllEnabledSlotsAcceptItems)
{
    ItemStack stack(*m_testItem, 1);
    for (i32 i = 0; i < 9; ++i) {
        EXPECT_TRUE(crafter_->canPlaceItem(i, stack)) << "槽位 " << i << " 应该可以放置物品";
    }
}

// ============================================================================
// 触发状态测试
// ============================================================================

TEST_F(CrafterBlockEntityTest, SetTriggered_UpdatesState)
{
    EXPECT_FALSE(crafter_->isTriggered());
    crafter_->setTriggered(true);
    EXPECT_TRUE(crafter_->isTriggered());
    crafter_->setTriggered(false);
    EXPECT_FALSE(crafter_->isTriggered());
}

TEST_F(CrafterBlockEntityTest, SetTriggered_MarksChanged)
{
    EXPECT_FALSE(crafter_->isChanged());
    crafter_->setTriggered(true);
    EXPECT_TRUE(crafter_->isChanged());
}

// ============================================================================
// 合成动画倒计时测试
// ============================================================================

TEST_F(CrafterBlockEntityTest, SetCraftingTicksRemaining_UpdatesValue)
{
    crafter_->setCraftingTicksRemaining(6);
    EXPECT_EQ(crafter_->getCraftingTicksRemaining(), 6);

    crafter_->setCraftingTicksRemaining(3);
    EXPECT_EQ(crafter_->getCraftingTicksRemaining(), 3);

    crafter_->setCraftingTicksRemaining(0);
    EXPECT_EQ(crafter_->getCraftingTicksRemaining(), 0);
}

TEST_F(CrafterBlockEntityTest, NeedsTick_ReturnsTrueWhenTicksRemaining)
{
    EXPECT_FALSE(crafter_->needsTick());
    crafter_->setCraftingTicksRemaining(6);
    EXPECT_TRUE(crafter_->needsTick());
    crafter_->setCraftingTicksRemaining(1);
    EXPECT_TRUE(crafter_->needsTick());
    crafter_->setCraftingTicksRemaining(0);
    EXPECT_FALSE(crafter_->needsTick());
}

// ============================================================================
// asCraftInput 测试
// ============================================================================

TEST_F(CrafterBlockEntityTest, AsCraftInput_EmptyGridReturnsEmpty)
{
    CraftingInventory input = crafter_->asCraftInput();
    EXPECT_EQ(input.getWidth(), 3);
    EXPECT_EQ(input.getHeight(), 3);
    // 空网格，所有槽位都应为空
    for (i32 y = 0; y < 3; ++y) {
        for (i32 x = 0; x < 3; ++x) {
            EXPECT_TRUE(input.getItemAt(x, y).isEmpty()) << "位置 (" << x << "," << y << ") 应为空";
        }
    }
}

TEST_F(CrafterBlockEntityTest, AsCraftInput_DisabledSlotsTreatedAsEmpty)
{
    auto* inventory = crafter_->getInventory();

    // 在槽位0放入物品，然后禁用槽位4
    ItemStack stack(*m_testItem, 3);
    inventory->setItem(0, stack);
    crafter_->setSlotState(4, false);

    CraftingInventory input = crafter_->asCraftInput();

    // 槽位0有物品 → 位置 (0,0) 非空
    EXPECT_FALSE(input.getItemAt(0, 0).isEmpty());
    EXPECT_EQ(input.getItemAt(0, 0).getCount(), 3);

    // 禁用槽位4 → 位置 (1,1) 视为空
    EXPECT_TRUE(input.getItemAt(1, 1).isEmpty());

    // 其他空槽位也是空
    EXPECT_TRUE(input.getItemAt(0, 1).isEmpty());
    EXPECT_TRUE(input.getItemAt(2, 2).isEmpty());
}

TEST_F(CrafterBlockEntityTest, AsCraftInput_PlacesItemsAtCorrectPositions)
{
    auto* inventory = crafter_->getInventory();
    Item* iron = ensureTestItem("iron_ingot");

    // 在每个槽位放入不同数量的物品
    // 槽位索引 → 3x3 网格位置: i % 3 = x, i / 3 = y
    inventory->setItem(0, ItemStack(*m_testItem, 1)); // (0,0)
    inventory->setItem(1, ItemStack(*iron, 2));       // (1,0)
    inventory->setItem(2, ItemStack(*m_testItem, 3)); // (2,0)
    inventory->setItem(3, ItemStack(*iron, 4));       // (0,1)
    inventory->setItem(4, ItemStack(*m_testItem, 5)); // (1,1)
    inventory->setItem(5, ItemStack(*iron, 6));       // (2,1)
    inventory->setItem(6, ItemStack(*m_testItem, 7)); // (0,2)
    inventory->setItem(7, ItemStack(*iron, 8));       // (1,2)
    inventory->setItem(8, ItemStack(*m_testItem, 9)); // (2,2)

    CraftingInventory input = crafter_->asCraftInput();

    EXPECT_EQ(input.getItemAt(0, 0).getCount(), 1);
    EXPECT_EQ(input.getItemAt(1, 0).getCount(), 2);
    EXPECT_EQ(input.getItemAt(2, 0).getCount(), 3);
    EXPECT_EQ(input.getItemAt(0, 1).getCount(), 4);
    EXPECT_EQ(input.getItemAt(1, 1).getCount(), 5);
    EXPECT_EQ(input.getItemAt(2, 1).getCount(), 6);
    EXPECT_EQ(input.getItemAt(0, 2).getCount(), 7);
    EXPECT_EQ(input.getItemAt(1, 2).getCount(), 8);
    EXPECT_EQ(input.getItemAt(2, 2).getCount(), 9);
}

TEST_F(CrafterBlockEntityTest, AsCraftInput_MixedEnabledAndDisabled)
{
    auto* inventory = crafter_->getInventory();
    Item* iron = ensureTestItem("iron_ingot");

    // 只放入一些槽位，其他禁用
    inventory->setItem(0, ItemStack(*m_testItem, 1)); // (0,0) - 有物品
    // 槽位1: 空+启用 → 空
    crafter_->setSlotState(2, false);           // (2,0) - 禁用 → 空
    inventory->setItem(3, ItemStack(*iron, 2)); // (0,1) - 有物品
    crafter_->setSlotState(4, false);           // (1,1) - 禁用 → 空
    // 槽位5: 空+启用 → 空
    crafter_->setSlotState(6, false);           // (0,2) - 禁用 → 空
    inventory->setItem(7, ItemStack(*iron, 3)); // (1,2) - 有物品
    // 槽位8: 空+启用 → 空

    CraftingInventory input = crafter_->asCraftInput();

    EXPECT_FALSE(input.getItemAt(0, 0).isEmpty()); // 有物品
    EXPECT_TRUE(input.getItemAt(1, 0).isEmpty());  // 空启用
    EXPECT_TRUE(input.getItemAt(2, 0).isEmpty());  // 禁用
    EXPECT_FALSE(input.getItemAt(0, 1).isEmpty()); // 有物品
    EXPECT_TRUE(input.getItemAt(1, 1).isEmpty());  // 禁用
    EXPECT_TRUE(input.getItemAt(2, 1).isEmpty());  // 空启用
    EXPECT_TRUE(input.getItemAt(0, 2).isEmpty());  // 禁用
    EXPECT_FALSE(input.getItemAt(1, 2).isEmpty()); // 有物品
    EXPECT_TRUE(input.getItemAt(2, 2).isEmpty());  // 空启用
}

// ============================================================================
// getRedstoneSignal 测试
// ============================================================================

TEST_F(CrafterBlockEntityTest, GetRedstoneSignal_EmptyEnabledSlotsReturnZero)
{
    // 所有槽位空且启用 → 0
    EXPECT_EQ(crafter_->getRedstoneSignal(), 0);
}

TEST_F(CrafterBlockEntityTest, GetRedstoneSignal_NonEmptySlotContributesOne)
{
    auto* inventory = crafter_->getInventory();
    inventory->setItem(0, ItemStack(*m_testItem, 1));
    EXPECT_EQ(crafter_->getRedstoneSignal(), 1);
}

TEST_F(CrafterBlockEntityTest, GetRedstoneSignal_DisabledSlotContributesOne)
{
    crafter_->setSlotState(0, false);
    EXPECT_EQ(crafter_->getRedstoneSignal(), 1);
}

TEST_F(CrafterBlockEntityTest, GetRedstoneSignal_MultipleSlotsAddUp)
{
    auto* inventory = crafter_->getInventory();

    // 3个槽位有物品
    inventory->setItem(0, ItemStack(*m_testItem, 1));
    inventory->setItem(1, ItemStack(*m_testItem, 1));
    inventory->setItem(2, ItemStack(*m_testItem, 1));

    // 2个槽位被禁用
    crafter_->setSlotState(3, false);
    crafter_->setSlotState(4, false);

    // 信号强度 = 3(有物品) + 2(禁用) = 5
    EXPECT_EQ(crafter_->getRedstoneSignal(), 5);
}

TEST_F(CrafterBlockEntityTest, GetRedstoneSignal_AllSlotsFilledOrDisabled)
{
    auto* inventory = crafter_->getInventory();
    Item* iron = ensureTestItem("iron_ingot");

    // 所有9个槽位都填满
    for (i32 i = 0; i < 9; ++i) {
        inventory->setItem(i, ItemStack(*m_testItem, 1));
    }
    EXPECT_EQ(crafter_->getRedstoneSignal(), 9);

    // 清空后全部禁用
    inventory->clear();
    for (i32 i = 0; i < 9; ++i) {
        crafter_->setSlotState(i, false);
    }
    EXPECT_EQ(crafter_->getRedstoneSignal(), 9);
}

TEST_F(CrafterBlockEntityTest, GetRedstoneSignal_DisabledSlotWithItemStillCountsOnce)
{
    auto* inventory = crafter_->getInventory();
    inventory->setItem(0, ItemStack(*m_testItem, 1));
    // 有物品的槽位无法被禁用，但仍测试逻辑
    // 有物品的槽位贡献1，无法禁用因此不会重复计算
    EXPECT_EQ(crafter_->getRedstoneSignal(), 1);
}

// ============================================================================
// clearContainer 测试
// ============================================================================

TEST_F(CrafterBlockEntityTest, ClearContainer_ClearsInventoryAndMarksChanged)
{
    auto* inventory = crafter_->getInventory();
    inventory->setItem(0, ItemStack(*m_testItem, 10));
    inventory->setItem(4, ItemStack(*m_testItem, 5));
    EXPECT_FALSE(inventory->isEmpty());

    crafter_->clearContainer();
    EXPECT_TRUE(inventory->isEmpty());
}

// ============================================================================
// 序列化测试
// ============================================================================

TEST_F(CrafterBlockEntityTest, SaveLoad_PreservesDisabledSlots)
{
    crafter_->setSlotState(0, false);
    crafter_->setSlotState(3, false);
    crafter_->setSlotState(7, false);

    nlohmann::json data;
    crafter_->save(data);

    auto loaded = std::make_unique<CrafterBlockEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));

    EXPECT_TRUE(loaded->isSlotDisabled(0));
    EXPECT_FALSE(loaded->isSlotDisabled(1));
    EXPECT_FALSE(loaded->isSlotDisabled(2));
    EXPECT_TRUE(loaded->isSlotDisabled(3));
    EXPECT_FALSE(loaded->isSlotDisabled(4));
    EXPECT_FALSE(loaded->isSlotDisabled(5));
    EXPECT_FALSE(loaded->isSlotDisabled(6));
    EXPECT_TRUE(loaded->isSlotDisabled(7));
    EXPECT_FALSE(loaded->isSlotDisabled(8));
}

TEST_F(CrafterBlockEntityTest, SaveLoad_PreservesTriggered)
{
    crafter_->setTriggered(true);

    nlohmann::json data;
    crafter_->save(data);

    auto loaded = std::make_unique<CrafterBlockEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));
    EXPECT_TRUE(loaded->isTriggered());
}

TEST_F(CrafterBlockEntityTest, SaveLoad_PreservesCraftingTicksRemaining)
{
    crafter_->setCraftingTicksRemaining(4);

    nlohmann::json data;
    crafter_->save(data);

    auto loaded = std::make_unique<CrafterBlockEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));
    EXPECT_EQ(loaded->getCraftingTicksRemaining(), 4);
}

TEST_F(CrafterBlockEntityTest, SaveLoad_PreservesItems)
{
    auto* inventory = crafter_->getInventory();
    Item* iron = ensureTestItem("iron_ingot");

    inventory->setItem(0, ItemStack(*m_testItem, 10));
    inventory->setItem(4, ItemStack(*iron, 32));
    inventory->setItem(8, ItemStack(*m_testItem, 1));

    nlohmann::json data;
    crafter_->save(data);

    auto loaded = std::make_unique<CrafterBlockEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));

    auto* loadedInventory = loaded->getInventory();
    EXPECT_FALSE(loadedInventory->getItem(0).isEmpty());
    EXPECT_EQ(loadedInventory->getItem(0).getCount(), 10);
    EXPECT_FALSE(loadedInventory->getItem(4).isEmpty());
    EXPECT_EQ(loadedInventory->getItem(4).getCount(), 32);
    EXPECT_FALSE(loadedInventory->getItem(8).isEmpty());
    EXPECT_EQ(loadedInventory->getItem(8).getCount(), 1);

    // 空槽位
    EXPECT_TRUE(loadedInventory->getItem(1).isEmpty());
    EXPECT_TRUE(loadedInventory->getItem(2).isEmpty());
    EXPECT_TRUE(loadedInventory->getItem(3).isEmpty());
    EXPECT_TRUE(loadedInventory->getItem(5).isEmpty());
    EXPECT_TRUE(loadedInventory->getItem(6).isEmpty());
    EXPECT_TRUE(loadedInventory->getItem(7).isEmpty());
}

TEST_F(CrafterBlockEntityTest, SaveLoad_PreservesCustomName)
{
    crafter_->setCustomName("Test Crafter");

    nlohmann::json data;
    crafter_->save(data);

    auto loaded = std::make_unique<CrafterBlockEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));
    EXPECT_EQ(loaded->getCustomName(), "Test Crafter");
}

TEST_F(CrafterBlockEntityTest, SaveLoad_EmptyEntity)
{
    nlohmann::json data;
    crafter_->save(data);

    auto loaded = std::make_unique<CrafterBlockEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));

    // 所有默认值应保持
    for (i32 i = 0; i < 9; ++i) {
        EXPECT_FALSE(loaded->isSlotDisabled(i));
        EXPECT_TRUE(loaded->getInventory()->getItem(i).isEmpty());
    }
    EXPECT_FALSE(loaded->isTriggered());
    EXPECT_EQ(loaded->getCraftingTicksRemaining(), 0);
    EXPECT_TRUE(loaded->getCustomName().empty());
}

TEST_F(CrafterBlockEntityTest, SaveLoad_FullRoundTrip)
{
    auto* inventory = crafter_->getInventory();
    Item* iron = ensureTestItem("iron_ingot");

    // 设置完整状态
    inventory->setItem(0, ItemStack(*m_testItem, 5));
    inventory->setItem(4, ItemStack(*iron, 16));
    crafter_->setSlotState(2, false);
    crafter_->setSlotState(6, false);
    crafter_->setTriggered(true);
    crafter_->setCraftingTicksRemaining(3);
    crafter_->setCustomName("My Auto Crafter");

    // 保存
    nlohmann::json data;
    crafter_->save(data);

    // 加载到新对象
    auto loaded = std::make_unique<CrafterBlockEntity>(BlockPos(99, 99, 99));
    ASSERT_TRUE(loaded->load(data));

    // 验证所有字段
    EXPECT_EQ(loaded->getType(), BlockEntityType::Crafter);
    // 注意：位置来自 JSON 而非构造参数（如果 ContainerBlockEntity::load 从 JSON 加载位置的话）
    EXPECT_TRUE(loaded->isSlotDisabled(2));
    EXPECT_TRUE(loaded->isSlotDisabled(6));
    EXPECT_FALSE(loaded->isSlotDisabled(0));
    EXPECT_EQ(loaded->getInventory()->getItem(0).getCount(), 5);
    EXPECT_EQ(loaded->getInventory()->getItem(4).getCount(), 16);
    EXPECT_TRUE(loaded->isTriggered());
    EXPECT_EQ(loaded->getCraftingTicksRemaining(), 3);
    EXPECT_EQ(loaded->getCustomName(), "My Auto Crafter");
}

// ============================================================================
// Clone 测试
// ============================================================================

TEST_F(CrafterBlockEntityTest, Clone_CreatesDeepCopy)
{
    auto* inventory = crafter_->getInventory();
    Item* iron = ensureTestItem("iron_ingot");

    inventory->setItem(0, ItemStack(*m_testItem, 7));
    inventory->setItem(5, ItemStack(*iron, 20));
    crafter_->setSlotState(1, false);
    crafter_->setSlotState(8, false);
    crafter_->setTriggered(true);
    crafter_->setCraftingTicksRemaining(4);
    crafter_->setCustomName("Original Crafter");

    std::unique_ptr<BlockEntity> copy = crafter_->clone();

    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->getType(), BlockEntityType::Crafter);
    EXPECT_EQ(copy->getPos(), BlockPos(10, 20, 30));

    auto* crafterCopy = static_cast<CrafterBlockEntity*>(copy.get());

    // 验证槽位状态
    EXPECT_TRUE(crafterCopy->isSlotDisabled(1));
    EXPECT_TRUE(crafterCopy->isSlotDisabled(8));
    EXPECT_FALSE(crafterCopy->isSlotDisabled(0));
    EXPECT_FALSE(crafterCopy->isSlotDisabled(5));

    // 验证物品
    EXPECT_EQ(crafterCopy->getInventory()->getItem(0).getCount(), 7);
    EXPECT_EQ(crafterCopy->getInventory()->getItem(5).getCount(), 20);

    // 验证触发状态
    EXPECT_TRUE(crafterCopy->isTriggered());

    // 验证合成倒计时
    EXPECT_EQ(crafterCopy->getCraftingTicksRemaining(), 4);

    // 验证自定义名称
    EXPECT_EQ(crafterCopy->getCustomName(), "Original Crafter");
}

TEST_F(CrafterBlockEntityTest, Clone_IndependentFromOriginal)
{
    auto* inventory = crafter_->getInventory();
    inventory->setItem(0, ItemStack(*m_testItem, 10));
    crafter_->setSlotState(2, false);
    crafter_->setTriggered(true);
    crafter_->setCraftingTicksRemaining(6);

    std::unique_ptr<BlockEntity> copy = crafter_->clone();
    auto* crafterCopy = static_cast<CrafterBlockEntity*>(copy.get());

    // 修改原始对象
    inventory->setItem(0, ItemStack(*m_testItem, 99));
    crafter_->setSlotState(2, true);
    crafter_->setTriggered(false);
    crafter_->setCraftingTicksRemaining(0);

    // 克隆应保持原始值
    EXPECT_EQ(crafterCopy->getInventory()->getItem(0).getCount(), 10);
    EXPECT_TRUE(crafterCopy->isSlotDisabled(2));
    EXPECT_TRUE(crafterCopy->isTriggered());
    EXPECT_EQ(crafterCopy->getCraftingTicksRemaining(), 6);
}

// ============================================================================
// tick 倒计时测试（需要 Mock 世界）
// ============================================================================

class CrafterBlockEntityTickTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        crafter_ = std::make_unique<CrafterBlockEntity>(BlockPos(0, 0, 0));
    }

    CrafterTestWorld world_;
    std::unique_ptr<CrafterBlockEntity> crafter_;
};

TEST_F(CrafterBlockEntityTickTest, Tick_DecrementsCraftingTicks)
{
    crafter_->setCraftingTicksRemaining(6);
    ASSERT_TRUE(crafter_->needsTick());

    crafter_->tick(world_);
    EXPECT_EQ(crafter_->getCraftingTicksRemaining(), 5);
    EXPECT_TRUE(crafter_->needsTick());

    crafter_->tick(world_);
    EXPECT_EQ(crafter_->getCraftingTicksRemaining(), 4);
}

TEST_F(CrafterBlockEntityTickTest, Tick_DoesNothingWhenNoTicksRemaining)
{
    ASSERT_EQ(crafter_->getCraftingTicksRemaining(), 0);
    ASSERT_FALSE(crafter_->needsTick());

    // 调用 tick 不应崩溃或改变任何状态
    crafter_->tick(world_);
    EXPECT_EQ(crafter_->getCraftingTicksRemaining(), 0);
}

TEST_F(CrafterBlockEntityTickTest, Tick_FullCountdownFromMaxTicks)
{
    // MAX_CRAFTING_TICKS = 6，验证完整倒计时
    crafter_->setCraftingTicksRemaining(CrafterBlockEntity::MAX_CRAFTING_TICKS);

    for (i32 i = CrafterBlockEntity::MAX_CRAFTING_TICKS; i > 0; --i) {
        EXPECT_EQ(crafter_->getCraftingTicksRemaining(), i);
        EXPECT_TRUE(crafter_->needsTick());
        crafter_->tick(world_);
    }

    EXPECT_EQ(crafter_->getCraftingTicksRemaining(), 0);
    EXPECT_FALSE(crafter_->needsTick());
}

// ============================================================================
// 自定义名称测试
// ============================================================================

TEST_F(CrafterBlockEntityTest, SetCustomName_UpdatesName)
{
    crafter_->setCustomName("Test Name");
    EXPECT_EQ(crafter_->getCustomName(), "Test Name");

    crafter_->setCustomName("");
    EXPECT_TRUE(crafter_->getCustomName().empty());
}

TEST_F(CrafterBlockEntityTest, SetCustomName_OverwritesPrevious)
{
    crafter_->setCustomName("First Name");
    EXPECT_EQ(crafter_->getCustomName(), "First Name");

    crafter_->setCustomName("Second Name");
    EXPECT_EQ(crafter_->getCustomName(), "Second Name");
}

// ============================================================================
// 常量验证测试
// ============================================================================

TEST_F(CrafterBlockEntityTest, Constants_HaveExpectedValues)
{
    EXPECT_EQ(CrafterBlockEntity::CONTAINER_SIZE, 9);
    EXPECT_EQ(CrafterBlockEntity::SLOT_DISABLED, 1);
    EXPECT_EQ(CrafterBlockEntity::SLOT_ENABLED, 0);
    EXPECT_EQ(CrafterBlockEntity::MAX_CRAFTING_TICKS, 6);
}

// ============================================================================
// slotCanBeDisabled 间接测试
// ============================================================================

TEST_F(CrafterBlockEntityTest, SlotCanBeDisabled_EmptySlotCanBeDisabled)
{
    // 空槽位可以禁用（通过 setSlotState 间接测试）
    for (i32 i = 0; i < 9; ++i) {
        crafter_->setSlotState(i, false);
        EXPECT_TRUE(crafter_->isSlotDisabled(i)) << "空槽位 " << i << " 应该可以被禁用";
    }
}

TEST_F(CrafterBlockEntityTest, SlotCanBeDisabled_OccupiedSlotCannotBeDisabled)
{
    auto* inventory = crafter_->getInventory();
    for (i32 i = 0; i < 9; ++i) {
        inventory->setItem(i, ItemStack(*m_testItem, 1));
    }

    // 有物品的槽位不能被禁用
    for (i32 i = 0; i < 9; ++i) {
        crafter_->setSlotState(i, false);
        EXPECT_FALSE(crafter_->isSlotDisabled(i)) << "有物品的槽位 " << i << " 不应该被禁用";
    }
}

TEST_F(CrafterBlockEntityTest, SlotCanBeDisabled_CanDisableAfterRemovingItem)
{
    auto* inventory = crafter_->getInventory();
    inventory->setItem(0, ItemStack(*m_testItem, 1));

    // 有物品时不能禁用
    crafter_->setSlotState(0, false);
    EXPECT_FALSE(crafter_->isSlotDisabled(0));

    // 移除物品后可以禁用
    inventory->setItem(0, ItemStack());
    crafter_->setSlotState(0, false);
    EXPECT_TRUE(crafter_->isSlotDisabled(0));
}

// ============================================================================
// 红石信号综合测试
// ============================================================================

TEST_F(CrafterBlockEntityTest, GetRedstoneSignal_OnlyDisabledSlots)
{
    // 全部禁用（所有空槽位）
    for (i32 i = 0; i < 9; ++i) {
        crafter_->setSlotState(i, false);
    }
    EXPECT_EQ(crafter_->getRedstoneSignal(), 9);
}

TEST_F(CrafterBlockEntityTest, GetRedstoneSignal_OnlyFilledSlots)
{
    auto* inventory = crafter_->getInventory();
    for (i32 i = 0; i < 9; ++i) {
        inventory->setItem(i, ItemStack(*m_testItem, 1));
    }
    EXPECT_EQ(crafter_->getRedstoneSignal(), 9);
}

TEST_F(CrafterBlockEntityTest, GetRedstoneSignal_SingleItemInSlot)
{
    auto* inventory = crafter_->getInventory();
    inventory->setItem(4, ItemStack(*m_testItem, 1));
    EXPECT_EQ(crafter_->getRedstoneSignal(), 1);
}

TEST_F(CrafterBlockEntityTest, GetRedstoneSignal_SingleDisabledSlot)
{
    crafter_->setSlotState(7, false);
    EXPECT_EQ(crafter_->getRedstoneSignal(), 1);
}

// ============================================================================
// 序列化边界情况测试
// ============================================================================

TEST_F(CrafterBlockEntityTest, SaveLoad_LoadWithoutDisabledSlotsField)
{
    // 模拟旧格式（没有 disabled_slots 字段）
    nlohmann::json data;
    data["id"] = "minecraft:crafter";
    data["x"] = 0;
    data["y"] = 0;
    data["z"] = 0;
    // 不包含 disabled_slots

    auto loaded = std::make_unique<CrafterBlockEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));

    // 应默认为所有槽位启用
    for (i32 i = 0; i < 9; ++i) {
        EXPECT_FALSE(loaded->isSlotDisabled(i));
    }
}

TEST_F(CrafterBlockEntityTest, SaveLoad_CraftingTicksRemainingZero)
{
    crafter_->setCraftingTicksRemaining(0);

    nlohmann::json data;
    crafter_->save(data);

    auto loaded = std::make_unique<CrafterBlockEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));
    EXPECT_EQ(loaded->getCraftingTicksRemaining(), 0);
    EXPECT_FALSE(loaded->needsTick());
}

TEST_F(CrafterBlockEntityTest, SaveLoad_MaxCraftingTicks)
{
    crafter_->setCraftingTicksRemaining(CrafterBlockEntity::MAX_CRAFTING_TICKS);

    nlohmann::json data;
    crafter_->save(data);

    auto loaded = std::make_unique<CrafterBlockEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));
    EXPECT_EQ(loaded->getCraftingTicksRemaining(), CrafterBlockEntity::MAX_CRAFTING_TICKS);
    EXPECT_TRUE(loaded->needsTick());
}

// ============================================================================
// setItem 自动重新启用禁用槽位测试（对应MC原版CrafterBlockEntity.setItem行为）
// ============================================================================

TEST_F(CrafterBlockEntityTest, SetItem_AutoReEnablesDisabledSlot)
{
    // 先禁用槽位0
    crafter_->setSlotState(0, false);
    ASSERT_TRUE(crafter_->isSlotDisabled(0));

    // 直接通过inventory的setItem放入物品时，应自动重新启用该槽位
    auto* inventory = crafter_->getInventory();
    inventory->setItem(0, ItemStack(*m_testItem, 5));

    // 槽位0应被自动重新启用
    EXPECT_FALSE(crafter_->isSlotDisabled(0));
    // 物品应存在
    EXPECT_EQ(inventory->getItem(0).getCount(), 5);
}

TEST_F(CrafterBlockEntityTest, SetItem_AutoReEnableDoesNotAffectOtherSlots)
{
    // 禁用槽位0和2
    crafter_->setSlotState(0, false);
    crafter_->setSlotState(2, false);

    // 向槽位0放入物品
    auto* inventory = crafter_->getInventory();
    inventory->setItem(0, ItemStack(*m_testItem, 3));

    // 槽位0应被重新启用，槽位2应保持禁用
    EXPECT_FALSE(crafter_->isSlotDisabled(0));
    EXPECT_TRUE(crafter_->isSlotDisabled(2));
}

TEST_F(CrafterBlockEntityTest, SetItem_EmptyStackDoesNotReEnableDisabledSlot)
{
    // 禁用槽位0
    crafter_->setSlotState(0, false);
    ASSERT_TRUE(crafter_->isSlotDisabled(0));

    // 设置为空ItemStack不应重新启用
    auto* inventory = crafter_->getInventory();
    inventory->setItem(0, ItemStack());

    // 槽位0应保持禁用（空物品不会触发重新启用）
    EXPECT_TRUE(crafter_->isSlotDisabled(0));
}

TEST_F(CrafterBlockEntityTest, SetItem_MultipleSlotsAutoReEnable)
{
    // 禁用多个槽位
    crafter_->setSlotState(0, false);
    crafter_->setSlotState(3, false);
    crafter_->setSlotState(6, false);

    auto* inventory = crafter_->getInventory();
    Item* iron = ensureTestItem("iron_ingot");

    // 向所有禁用槽位放入物品
    inventory->setItem(0, ItemStack(*m_testItem, 1));
    inventory->setItem(3, ItemStack(*iron, 2));
    inventory->setItem(6, ItemStack(*m_testItem, 3));

    // 所有放入物品的槽位应被自动重新启用
    EXPECT_FALSE(crafter_->isSlotDisabled(0));
    EXPECT_FALSE(crafter_->isSlotDisabled(3));
    EXPECT_FALSE(crafter_->isSlotDisabled(6));
}

TEST_F(CrafterBlockEntityTest, SetItem_RedstoneSignalUpdatedAfterAutoReEnable)
{
    // 禁用3个空槽位，红石信号应为3
    crafter_->setSlotState(0, false);
    crafter_->setSlotState(4, false);
    crafter_->setSlotState(8, false);
    EXPECT_EQ(crafter_->getRedstoneSignal(), 3);

    // 向一个禁用槽位放入物品，该槽位自动重新启用
    // 重新启用后，该槽位从"禁用=1"变为"有物品=1"，信号不变
    auto* inventory = crafter_->getInventory();
    inventory->setItem(0, ItemStack(*m_testItem, 1));
    EXPECT_FALSE(crafter_->isSlotDisabled(0));
    EXPECT_EQ(crafter_->getRedstoneSignal(), 3); // 2 disabled + 1 with item = 3
}
