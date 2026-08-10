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

// 验证 SimpleInventory 的战利品表延迟填充回调机制：
// BarrelEntity / ChestEntity / DispenserBlockEntity 通过 setLootUnpackCallback
// 注入回调后，所有依赖容器内容的 IInventory 方法（isEmpty/getItem/setItem/
// removeItem/removeItemNoUpdate/clear）都会自动触发 _unpackLootTable(nullptr)，
// 这与 MC Java 中 RandomizableContainerBlockEntity 的行为一致。

#include "common/TestWorldHelper.hpp"
#include "common/world/blockentity/core/LootableContainerBlockEntity.hpp"
#include "common/world/blockentity/core/SimpleInventory.hpp"
#include "entity/entities/player/Player.hpp"
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/loot/LootTable.hpp"
#include "item/loot/LootTableManager.hpp"
#include "item/loot/entries/ItemLootEntry.hpp"
#include "world/IWorld.hpp"
#include "world/block/BlockPos.hpp"
#include "world/blockentity/interactive/DispenserBlockEntity.hpp"
#include "world/blockentity/storage/BarrelEntity.hpp"
#include "world/blockentity/storage/ChestEntity.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::blockentity;

namespace {

/**
 * @brief 按资源路径懒注册测试用物品
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

/**
 * @brief 创建简单的测试战利品表（包含单个物品 N 个）
 */
void registerSimpleLootTable(
    loot::LootTableManager& manager, const std::string& tableId, const std::string& itemId, i32 count)
{
    auto table = std::make_unique<loot::LootTable>();
    auto pool = std::make_unique<loot::LootPool>();
    pool->setRolls(loot::RandomValueRange(static_cast<f32>(count), static_cast<f32>(count)));
    auto entry = std::make_unique<loot::ItemLootEntry>(itemId, loot::RandomValueRange(1.0f, 1.0f), 1, 0);
    pool->addEntry(std::move(entry));
    table->addPool(std::move(pool));
    manager.registerTable(tableId, std::move(table));
}

/**
 * @brief 测试用世界，支持 LootTableManager
 *
 * 默认 lootTableManager() 返回内部空管理器。
 * 可通过 setLootTableManager() 注入自定义管理器（用于测试战利品填充）。
 * 可通过 setNoLootTableManager() 模拟客户端场景（lootTableManager() 返回 nullptr）。
 */
class LootCallbackTestWorld final : public mc::test::BaseTestWorld {
public:
    using IWorld::getBlockState;

    [[nodiscard]] const BlockState* getBlockState(i32, i32, i32) const override { return nullptr; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32, i32) const override { return true; }

    [[nodiscard]] const loot::LootTableManager* lootTableManager() const override
    {
        if (m_noManager) {
            return nullptr;
        }
        return m_customManager != nullptr ? m_customManager : &m_lootTableManager;
    }

    void setLootTableManager(loot::LootTableManager* manager) { m_customManager = manager; }
    void setNoLootTableManager() { m_noManager = true; }

private:
    loot::LootTableManager m_lootTableManager;
    loot::LootTableManager* m_customManager = nullptr;
    bool m_noManager = false;
};

} // namespace

// ========== BarrelEntity 战利品感知回调测试 ==========

class BarrelLootCallbackTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        m_diamond = ensureTestItem("diamond");
        m_ironIngot = ensureTestItem("iron_ingot");
        barrel_ = std::make_unique<BarrelEntity>(BlockPos(0, 0, 0));
        barrel_->setWorld(&world_);
    }

    LootCallbackTestWorld world_;
    std::unique_ptr<BarrelEntity> barrel_;
    Item* m_diamond = nullptr;
    Item* m_ironIngot = nullptr;
};

TEST_F(BarrelLootCallbackTest, GetInventoryGetItem_TriggersLootFill)
{
    // 通过 getInventory()->getItem(slot) 访问应自动触发战利品表填充
    loot::LootTableManager manager;
    registerSimpleLootTable(manager, "minecraft:chests/barrel_test", "minecraft:diamond", 1);
    world_.setLootTableManager(&manager);

    barrel_->setLootTable(ResourceLocation("minecraft", "chests/barrel_test"), 42);
    EXPECT_TRUE(barrel_->hasLootTable());

    // 通过 getInventory() 路径访问，应触发填充
    IInventory* inv = barrel_->getInventory();
    ASSERT_NE(inv, nullptr);
    ItemStack slot0 = inv->getItem(0);

    // 填充后战利品表标记被清除
    EXPECT_FALSE(barrel_->hasLootTable());

    // 容器中应有钻石（在某个槽位）
    bool hasDiamond = false;
    for (i32 i = 0; i < barrel_->getContainerSize(); ++i) {
        ItemStack stack = inv->getItem(i);
        if (!stack.isEmpty() && stack.getItem() == m_diamond) {
            hasDiamond = true;
            break;
        }
    }
    EXPECT_TRUE(hasDiamond);
}

TEST_F(BarrelLootCallbackTest, GetInventoryIsEmpty_TriggersLootFill)
{
    // 通过 getInventory()->isEmpty() 访问应自动触发战利品表填充
    loot::LootTableManager manager;
    registerSimpleLootTable(manager, "minecraft:chests/barrel_empty_test", "minecraft:iron_ingot", 2);
    world_.setLootTableManager(&manager);

    barrel_->setLootTable(ResourceLocation("minecraft", "chests/barrel_empty_test"), 42);
    EXPECT_TRUE(barrel_->hasLootTable());

    // 通过 getInventory() 路径访问 isEmpty，应触发填充然后返回 false（容器有物品）
    IInventory* inv = barrel_->getInventory();
    ASSERT_NE(inv, nullptr);
    EXPECT_FALSE(inv->isEmpty());

    // 填充后战利品表标记被清除
    EXPECT_FALSE(barrel_->hasLootTable());
}

TEST_F(BarrelLootCallbackTest, GetInventorySetItem_TriggersLootFill)
{
    // 通过 getInventory()->setItem() 访问应自动触发战利品表填充
    loot::LootTableManager manager;
    registerSimpleLootTable(manager, "minecraft:chests/barrel_set_test", "minecraft:diamond", 1);
    world_.setLootTableManager(&manager);

    barrel_->setLootTable(ResourceLocation("minecraft", "chests/barrel_set_test"), 42);
    EXPECT_TRUE(barrel_->hasLootTable());

    // 通过 getInventory() 路径 setItem，应先触发填充（战利品生成），再设置槽位
    IInventory* inv = barrel_->getInventory();
    ASSERT_NE(inv, nullptr);
    inv->setItem(0, ItemStack(m_ironIngot, 1));

    // 填充后战利品表标记被清除
    EXPECT_FALSE(barrel_->hasLootTable());

    // 槽位 0 应是 iron_ingot（覆盖了战利品生成的钻石，因为 setItem 是直接覆盖）
    EXPECT_EQ(inv->getItem(0).getItem(), m_ironIngot);
}

TEST_F(BarrelLootCallbackTest, GetInventoryClear_TriggersLootFill)
{
    // 通过 getInventory()->clear() 访问应自动触发战利品表填充
    loot::LootTableManager manager;
    registerSimpleLootTable(manager, "minecraft:chests/barrel_clear_test", "minecraft:diamond", 3);
    world_.setLootTableManager(&manager);

    barrel_->setLootTable(ResourceLocation("minecraft", "chests/barrel_clear_test"), 42);
    EXPECT_TRUE(barrel_->hasLootTable());

    // 通过 getInventory() 路径 clear，应先触发填充（战利品生成），再清空
    IInventory* inv = barrel_->getInventory();
    ASSERT_NE(inv, nullptr);
    inv->clear();

    // 填充后战利品表标记被清除
    EXPECT_FALSE(barrel_->hasLootTable());

    // 容器应为空
    EXPECT_TRUE(inv->isEmpty());
}

TEST_F(BarrelLootCallbackTest, GetInventoryRemoveItem_TriggersLootFill)
{
    // 通过 getInventory()->removeItem() 访问应自动触发战利品表填充
    loot::LootTableManager manager;
    registerSimpleLootTable(manager, "minecraft:chests/barrel_remove_test", "minecraft:diamond", 2);
    world_.setLootTableManager(&manager);

    barrel_->setLootTable(ResourceLocation("minecraft", "chests/barrel_remove_test"), 42);
    EXPECT_TRUE(barrel_->hasLootTable());

    // 通过 getInventory() 路径 removeItem，应先触发填充（生成 2 钻石），再移除
    IInventory* inv = barrel_->getInventory();
    ASSERT_NE(inv, nullptr);

    // 找到第一个非空槽位并移除
    i32 firstNonEmpty = -1;
    for (i32 i = 0; i < inv->getContainerSize(); ++i) {
        if (!inv->getItem(i).isEmpty()) {
            firstNonEmpty = i;
            break;
        }
    }
    ASSERT_GE(firstNonEmpty, 0);

    ItemStack removed = inv->removeItem(firstNonEmpty, 1);
    EXPECT_FALSE(removed.isEmpty());
    EXPECT_EQ(removed.getItem(), m_diamond);

    // 填充后战利品表标记被清除
    EXPECT_FALSE(barrel_->hasLootTable());
}

TEST_F(BarrelLootCallbackTest, GetInventoryRemoveItemNoUpdate_TriggersLootFill)
{
    // 通过 getInventory()->removeItemNoUpdate() 访问应自动触发战利品表填充
    loot::LootTableManager manager;
    registerSimpleLootTable(manager, "minecraft:chests/barrel_remove_noupdate_test", "minecraft:diamond", 1);
    world_.setLootTableManager(&manager);

    barrel_->setLootTable(ResourceLocation("minecraft", "chests/barrel_remove_noupdate_test"), 42);
    EXPECT_TRUE(barrel_->hasLootTable());

    IInventory* inv = barrel_->getInventory();
    ASSERT_NE(inv, nullptr);

    // 找到第一个非空槽位
    i32 firstNonEmpty = -1;
    for (i32 i = 0; i < inv->getContainerSize(); ++i) {
        if (!inv->getItem(i).isEmpty()) {
            firstNonEmpty = i;
            break;
        }
    }
    ASSERT_GE(firstNonEmpty, 0);

    ItemStack removed = inv->removeItemNoUpdate(firstNonEmpty);
    EXPECT_FALSE(removed.isEmpty());
    EXPECT_FALSE(barrel_->hasLootTable());
}

// ========== ChestEntity 战利品感知回调测试 ==========

class ChestLootCallbackTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        m_diamond = ensureTestItem("diamond");
        chest_ = std::make_unique<ChestEntity>(BlockPos(0, 0, 0));
        chest_->setWorld(&world_);
    }

    LootCallbackTestWorld world_;
    std::unique_ptr<ChestEntity> chest_;
    Item* m_diamond = nullptr;
};

TEST_F(ChestLootCallbackTest, GetInventoryGetItem_TriggersLootFill)
{
    loot::LootTableManager manager;
    registerSimpleLootTable(manager, "minecraft:chests/chest_test", "minecraft:diamond", 1);
    world_.setLootTableManager(&manager);

    chest_->setLootTable(ResourceLocation("minecraft", "chests/chest_test"), 42);
    EXPECT_TRUE(chest_->hasLootTable());

    IInventory* inv = chest_->getInventory();
    ASSERT_NE(inv, nullptr);
    ItemStack slot0 = inv->getItem(0);

    EXPECT_FALSE(chest_->hasLootTable());

    bool hasDiamond = false;
    for (i32 i = 0; i < chest_->getContainerSize(); ++i) {
        ItemStack stack = inv->getItem(i);
        if (!stack.isEmpty() && stack.getItem() == m_diamond) {
            hasDiamond = true;
            break;
        }
    }
    EXPECT_TRUE(hasDiamond);
}

TEST_F(ChestLootCallbackTest, MoveConstructor_PreservesCallbackBinding)
{
    // 验证 ChestEntity 移动构造后回调正确绑定到新对象
    loot::LootTableManager manager;
    registerSimpleLootTable(manager, "minecraft:chests/chest_move_test", "minecraft:diamond", 1);
    world_.setLootTableManager(&manager);

    chest_->setLootTable(ResourceLocation("minecraft", "chests/chest_move_test"), 42);
    EXPECT_TRUE(chest_->hasLootTable());

    // 移动构造
    ChestEntity movedChest = std::move(*chest_);

    // 移动后，新对象的回调应绑定到新 this 指针
    // 通过新对象访问 getInventory()->getItem 应触发填充
    IInventory* inv = movedChest.getInventory();
    ASSERT_NE(inv, nullptr);
    ItemStack slot0 = inv->getItem(0);

    // 战利品表应被填充
    EXPECT_FALSE(movedChest.hasLootTable());

    // 容器中应有钻石
    bool hasDiamond = false;
    for (i32 i = 0; i < movedChest.getContainerSize(); ++i) {
        ItemStack stack = inv->getItem(i);
        if (!stack.isEmpty() && stack.getItem() == m_diamond) {
            hasDiamond = true;
            break;
        }
    }
    EXPECT_TRUE(hasDiamond);
}

TEST_F(ChestLootCallbackTest, MoveAssignment_PreservesCallbackBinding)
{
    loot::LootTableManager manager;
    registerSimpleLootTable(manager, "minecraft:chests/chest_moveassign_test", "minecraft:diamond", 1);
    world_.setLootTableManager(&manager);

    chest_->setLootTable(ResourceLocation("minecraft", "chests/chest_moveassign_test"), 42);
    EXPECT_TRUE(chest_->hasLootTable());

    ChestEntity targetChest(BlockPos(1, 2, 3));
    targetChest = std::move(*chest_);

    IInventory* inv = targetChest.getInventory();
    ASSERT_NE(inv, nullptr);
    ItemStack slot0 = inv->getItem(0);

    EXPECT_FALSE(targetChest.hasLootTable());
}

// ========== DispenserBlockEntity 战利品感知回调测试 ==========

class DispenserLootCallbackTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        m_diamond = ensureTestItem("diamond");
        dispenser_ = std::make_unique<DispenserBlockEntity>(BlockEntityType::Dispenser, BlockPos(0, 0, 0));
        dispenser_->setWorld(&world_);
    }

    LootCallbackTestWorld world_;
    std::unique_ptr<DispenserBlockEntity> dispenser_;
    Item* m_diamond = nullptr;
};

TEST_F(DispenserLootCallbackTest, GetInventoryGetItem_TriggersLootFill)
{
    loot::LootTableManager manager;
    registerSimpleLootTable(manager, "minecraft:chests/dispenser_test", "minecraft:diamond", 1);
    world_.setLootTableManager(&manager);

    dispenser_->setLootTable(ResourceLocation("minecraft", "chests/dispenser_test"), 42);
    EXPECT_TRUE(dispenser_->hasLootTable());

    IInventory* inv = dispenser_->getInventory();
    ASSERT_NE(inv, nullptr);
    ItemStack slot0 = inv->getItem(0);

    EXPECT_FALSE(dispenser_->hasLootTable());

    bool hasDiamond = false;
    for (i32 i = 0; i < dispenser_->getContainerSize(); ++i) {
        ItemStack stack = inv->getItem(i);
        if (!stack.isEmpty() && stack.getItem() == m_diamond) {
            hasDiamond = true;
            break;
        }
    }
    EXPECT_TRUE(hasDiamond);
}

TEST_F(DispenserLootCallbackTest, ClearContainer_TriggersLootFill)
{
    // DispenserBlockEntity::clearContainer 应触发战利品表填充后再清空
    loot::LootTableManager manager;
    registerSimpleLootTable(manager, "minecraft:chests/dispenser_clear_test", "minecraft:diamond", 2);
    world_.setLootTableManager(&manager);

    dispenser_->setLootTable(ResourceLocation("minecraft", "chests/dispenser_clear_test"), 42);
    EXPECT_TRUE(dispenser_->hasLootTable());

    // 调用 clearContainer 应先填充再清空
    dispenser_->clearContainer();

    // 战利品表应被填充（标记被清除）
    EXPECT_FALSE(dispenser_->hasLootTable());

    // 容器应为空
    EXPECT_TRUE(dispenser_->isEmpty());
}

TEST_F(DispenserLootCallbackTest, GetDispenseSlot_TriggersLootFill)
{
    // getDispenseSlot 内部调用 m_inventory.getItem()，应触发战利品表填充
    loot::LootTableManager manager;
    registerSimpleLootTable(manager, "minecraft:chests/dispenser_dispense_test", "minecraft:diamond", 1);
    world_.setLootTableManager(&manager);

    dispenser_->setLootTable(ResourceLocation("minecraft", "chests/dispenser_dispense_test"), 42);
    EXPECT_TRUE(dispenser_->hasLootTable());

    // getDispenseSlot 应触发填充
    i32 slot = dispenser_->getDispenseSlot();

    // 战利品表应被填充
    EXPECT_FALSE(dispenser_->hasLootTable());

    // 应该有一个非空槽位被选中
    EXPECT_GE(slot, 0);
}

// ========== 简化场景：SimpleInventory 直接设置回调 ==========

class SimpleInventoryCallbackTest : public ::testing::Test {
protected:
    void SetUp() override { callbackCallCount_ = 0; }

    i32 callbackCallCount_ = 0;
};

TEST_F(SimpleInventoryCallbackTest, SetCallback_TriggeredOnAllAccessMethods)
{
    SimpleInventory inv(5);
    inv.setLootUnpackCallback([this]() { ++callbackCallCount_; });

    // 初始调用次数为 0
    EXPECT_EQ(callbackCallCount_, 0);

    // isEmpty 触发回调
    (void)inv.isEmpty();
    EXPECT_EQ(callbackCallCount_, 1);

    // getItem 触发回调
    (void)inv.getItem(0);
    EXPECT_EQ(callbackCallCount_, 2);

    // setItem 触发回调
    auto diamond = ensureTestItem("simple_diamond");
    inv.setItem(0, ItemStack(diamond, 1));
    EXPECT_EQ(callbackCallCount_, 3);

    // removeItem 触发回调
    (void)inv.removeItem(0, 1);
    EXPECT_EQ(callbackCallCount_, 4);

    // removeItemNoUpdate 触发回调
    inv.setItem(0, ItemStack(diamond, 1));
    (void)inv.removeItemNoUpdate(0);
    EXPECT_EQ(callbackCallCount_, 6); // setItem + removeItemNoUpdate

    // clear 触发回调
    inv.clear();
    EXPECT_EQ(callbackCallCount_, 7);
}

TEST_F(SimpleInventoryCallbackTest, NoCallback_AllMethodsWorkNormally)
{
    // 不设置回调时，所有方法应正常工作
    SimpleInventory inv(3);
    auto diamond = ensureTestItem("simple_diamond_2");

    EXPECT_NO_THROW((void)inv.isEmpty());
    EXPECT_NO_THROW((void)inv.getItem(0));
    EXPECT_NO_THROW(inv.setItem(0, ItemStack(diamond, 1)));
    EXPECT_NO_THROW((void)inv.removeItem(0, 1));
    EXPECT_NO_THROW((void)inv.removeItemNoUpdate(0));
    EXPECT_NO_THROW(inv.clear());
}

TEST_F(SimpleInventoryCallbackTest, MoveConstructor_PreservesCallback)
{
    SimpleInventory inv(3);
    i32 callCount = 0;
    inv.setLootUnpackCallback([&callCount]() { ++callCount; });

    // 移动构造
    SimpleInventory movedInv = std::move(inv);

    // 移动后回调应被转移（注意：lambda 捕获的引用仍然有效，因为是外部变量）
    (void)movedInv.isEmpty();
    EXPECT_EQ(callCount, 1);
}

TEST_F(SimpleInventoryCallbackTest, MoveAssignment_PreservesCallback)
{
    SimpleInventory inv(3);
    i32 callCount = 0;
    inv.setLootUnpackCallback([&callCount]() { ++callCount; });

    SimpleInventory target(2);
    target = std::move(inv);

    (void)target.isEmpty();
    EXPECT_EQ(callCount, 1);
}
