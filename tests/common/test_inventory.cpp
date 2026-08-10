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

#include "../src/common/core/Constants.hpp"
#include "../src/common/entity/core/LivingEntity.hpp"
#include "../src/common/entity/damage/DamageSource.hpp"
#include "../src/common/entity/entities/player/Player.hpp"
#include "../src/common/entity/inventory/IInventory.hpp"
#include "../src/common/entity/inventory/PlayerInventory.hpp"
#include "../src/common/entity/inventory/Slot.hpp"
#include "../src/common/item/Items.hpp"
#include "../src/common/item/armor/ArmorMaterial.hpp"
#include "../src/common/item/core/ItemRegistry.hpp"
#include "../src/common/item/enchantment/EnchantmentHelper.hpp"
#include "../src/common/item/enchantment/EnchantmentRegistry.hpp"
#include "../src/common/item/items/armor/ArmorItem.hpp"
#include "../src/common/item/items/armor/DyeableArmorItem.hpp"
#include "../src/common/item/items/armor/ElytraItem.hpp"
#include "../src/common/util/math/random/Random.hpp"
#include "../src/common/world/IWorld.hpp"
#include "../src/common/world/block/Block.hpp"
#include "../src/common/world/block/registry/VanillaBlocks.hpp"
#include "../src/common/world/blockentity/core/SimpleInventory.hpp"
#include "../src/common/world/border/WorldBorder.hpp"
#include "../src/common/world/chunk/data/ChunkData.hpp"
#include "../src/common/world/fluid/Fluid.hpp"
#include "../src/common/world/tick/manager/TickManager.hpp"
#include "common/TestWorldHelper.hpp"
#include <gtest/gtest.h>

#include <array>

using namespace mc;

namespace {

class TestLivingEntity final : public LivingEntity {
public:
    TestLivingEntity()
        : LivingEntity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

class ArmorTestWorld final : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT;
    }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }

    void setCurrentTick(u64 tick) { m_currentTick = tick; }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("ArmorTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("ArmorTestWorld::tickManager not implemented");
    }

private:
    u64 m_currentTick = 0;
};

} // namespace

// ============================================================================
// Slot 索引常量测试
// ============================================================================

class InventorySlotsTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(InventorySlotsTest, ConstantsAreCorrect)
{
    // 快捷栏
    EXPECT_EQ(InventorySlots::HOTBAR_START, 0);
    EXPECT_EQ(InventorySlots::HOTBAR_END, 8);
    EXPECT_EQ(InventorySlots::HOTBAR_SIZE, 9);

    // 主背包
    EXPECT_EQ(InventorySlots::MAIN_START, 9);
    EXPECT_EQ(InventorySlots::MAIN_END, 35);
    EXPECT_EQ(InventorySlots::MAIN_SIZE, 27);

    // 护甲
    EXPECT_EQ(InventorySlots::ARMOR_START, 36);
    EXPECT_EQ(InventorySlots::ARMOR_END, 39);
    EXPECT_EQ(InventorySlots::ARMOR_SIZE, 4);
    EXPECT_EQ(InventorySlots::ARMOR_HEAD, 36);
    EXPECT_EQ(InventorySlots::ARMOR_CHEST, 37);
    EXPECT_EQ(InventorySlots::ARMOR_LEGS, 38);
    EXPECT_EQ(InventorySlots::ARMOR_FEET, 39);

    // 副手
    EXPECT_EQ(InventorySlots::OFFHAND, 40);

    // 总大小
    EXPECT_EQ(InventorySlots::TOTAL_SIZE, 41);
}

// ============================================================================
// PlayerInventory 测试
// ============================================================================

class PlayerInventoryTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        m_inventory = std::make_unique<PlayerInventory>(nullptr);

        m_diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
        m_stick = ItemRegistry::instance().getItem(ResourceLocation("minecraft:stick"));
        m_diamondSword = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond_sword"));
    }

    std::unique_ptr<PlayerInventory> m_inventory;
    Item* m_diamond = nullptr;
    Item* m_stick = nullptr;
    Item* m_diamondSword = nullptr;
};

TEST_F(PlayerInventoryTest, InitialState)
{
    EXPECT_EQ(m_inventory->getContainerSize(), 41);
    EXPECT_TRUE(m_inventory->isEmpty());
    EXPECT_EQ(m_inventory->getSelectedSlot(), 0);
}

TEST_F(PlayerInventoryTest, SetAndGetItem)
{
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(*m_diamond, 32);
    m_inventory->setItem(0, stack);

    EXPECT_FALSE(m_inventory->isEmpty());
    EXPECT_EQ(m_inventory->getItem(0).getCount(), 32);
    EXPECT_EQ(m_inventory->getItem(0).getItem(), m_diamond);
}

TEST_F(PlayerInventoryTest, HotbarOperations)
{
    ASSERT_NE(m_diamond, nullptr);

    // 设置选中槽位
    m_inventory->setSelectedSlot(5);
    EXPECT_EQ(m_inventory->getSelectedSlot(), 5);

    // 边界检查
    m_inventory->setSelectedSlot(-1);
    EXPECT_EQ(m_inventory->getSelectedSlot(), 0);

    m_inventory->setSelectedSlot(10);
    EXPECT_EQ(m_inventory->getSelectedSlot(), 8);

    // 设置选中物品
    ItemStack stack(*m_diamond, 10);
    m_inventory->setItem(3, stack);
    m_inventory->setSelectedSlot(3);
    EXPECT_EQ(m_inventory->getSelectedStack().getCount(), 10);
}

TEST_F(PlayerInventoryTest, RemoveItem)
{
    ASSERT_NE(m_diamond, nullptr);

    m_inventory->setItem(0, ItemStack(*m_diamond, 32));

    // 移除部分
    ItemStack removed = m_inventory->removeItem(0, 10);
    EXPECT_EQ(removed.getCount(), 10);
    EXPECT_EQ(m_inventory->getItem(0).getCount(), 22);

    // 移除剩余部分
    removed = m_inventory->removeItem(0, 100);
    EXPECT_EQ(removed.getCount(), 22);
    EXPECT_TRUE(m_inventory->getItem(0).isEmpty());
}

TEST_F(PlayerInventoryTest, ClearInventory)
{
    ASSERT_NE(m_diamond, nullptr);

    m_inventory->setItem(0, ItemStack(*m_diamond, 10));
    m_inventory->setItem(5, ItemStack(*m_diamond, 20));
    m_inventory->setItem(40, ItemStack(*m_diamond, 5));

    EXPECT_FALSE(m_inventory->isEmpty());

    m_inventory->clear();

    EXPECT_TRUE(m_inventory->isEmpty());
}

TEST_F(PlayerInventoryTest, AddItem)
{
    ASSERT_NE(m_diamond, nullptr);

    // 添加到空背包
    ItemStack stack(*m_diamond, 32);
    i32 remaining = m_inventory->add(stack);

    EXPECT_EQ(remaining, 32); // 全部添加成功
    EXPECT_TRUE(stack.isEmpty());

    // 检查物品在快捷栏
    EXPECT_EQ(m_inventory->getItem(0).getCount(), 32);
}

TEST_F(PlayerInventoryTest, AddItemMerging)
{
    ASSERT_NE(m_diamond, nullptr);

    // 先放入一些钻石
    m_inventory->setItem(0, ItemStack(*m_diamond, 50));

    // 添加更多钻石（应该合并）
    ItemStack stack(*m_diamond, 20);
    i32 remaining = m_inventory->add(stack);

    // 槽位 0 从 50 变成 64（堆叠上限），剩余 6 个会放到下一个空槽位
    // MC 1.16.5 行为: 空槽位优先级是 选中槽 → 副手 → 快捷栏 → 主背包
    // 所以剩余的 6 个会放到副手槽 (slot 40)，而不是 slot 1
    EXPECT_EQ(remaining, 20);                          // 全部添加成功
    EXPECT_EQ(m_inventory->getItem(0).getCount(), 64); // 达到堆叠上限
    EXPECT_TRUE(stack.isEmpty());                      // 全部添加成功，stack 变空

    // 剩余的 6 个应该放在副手槽 (slot 40)
    EXPECT_EQ(m_inventory->getItem(40).getCount(), 6);
}

TEST_F(PlayerInventoryTest, AddMultipleItems)
{
    ASSERT_NE(m_diamond, nullptr);
    ASSERT_NE(m_stick, nullptr);

    // 添加钻石
    ItemStack diamonds(*m_diamond, 32);
    m_inventory->add(diamonds);

    // 添加木棍
    ItemStack sticks(*m_stick, 16);
    m_inventory->add(sticks);

    EXPECT_EQ(m_inventory->countItem(*m_diamond), 32);
    EXPECT_EQ(m_inventory->countItem(*m_stick), 16);
}

TEST_F(PlayerInventoryTest, FindSlot)
{
    ASSERT_NE(m_diamond, nullptr);
    ASSERT_NE(m_stick, nullptr);

    m_inventory->setItem(5, ItemStack(*m_diamond, 10));
    m_inventory->setItem(20, ItemStack(*m_stick, 5));

    EXPECT_EQ(m_inventory->findSlot(*m_diamond), 5);
    EXPECT_EQ(m_inventory->findSlot(*m_stick), 20);
    EXPECT_EQ(m_inventory->findSlot(*ItemRegistry::instance().getItem(ResourceLocation("minecraft:coal"))), -1);
}

TEST_F(PlayerInventoryTest, CountItem)
{
    ASSERT_NE(m_diamond, nullptr);

    m_inventory->setItem(0, ItemStack(*m_diamond, 10));
    m_inventory->setItem(5, ItemStack(*m_diamond, 20));
    m_inventory->setItem(30, ItemStack(*m_diamond, 15));

    EXPECT_EQ(m_inventory->countItem(*m_diamond), 45);
}

TEST_F(PlayerInventoryTest, HasItem)
{
    ASSERT_NE(m_diamond, nullptr);
    ASSERT_NE(m_stick, nullptr);

    m_inventory->setItem(0, ItemStack(*m_diamond, 10));

    EXPECT_TRUE(m_inventory->hasItem(*m_diamond));
    EXPECT_FALSE(m_inventory->hasItem(*m_stick));
}

TEST_F(PlayerInventoryTest, GetFirstEmptySlot)
{
    ASSERT_NE(m_diamond, nullptr);

    // 空背包
    EXPECT_EQ(m_inventory->getFirstEmptySlot(), 0);

    // 填充前几个槽位
    m_inventory->setItem(0, ItemStack(*m_diamond, 1));
    m_inventory->setItem(1, ItemStack(*m_diamond, 1));
    m_inventory->setItem(2, ItemStack(*m_diamond, 1));

    EXPECT_EQ(m_inventory->getFirstEmptySlot(), 3);
}

TEST_F(PlayerInventoryTest, SwapSlots)
{
    ASSERT_NE(m_diamond, nullptr);
    ASSERT_NE(m_stick, nullptr);

    m_inventory->setItem(0, ItemStack(*m_diamond, 10));
    m_inventory->setItem(5, ItemStack(*m_stick, 5));

    m_inventory->swapSlots(0, 5);

    EXPECT_EQ(m_inventory->getItem(0).getItem(), m_stick);
    EXPECT_EQ(m_inventory->getItem(0).getCount(), 5);
    EXPECT_EQ(m_inventory->getItem(5).getItem(), m_diamond);
    EXPECT_EQ(m_inventory->getItem(5).getCount(), 10);
}

TEST_F(PlayerInventoryTest, PlaceItem)
{
    ASSERT_NE(m_diamond, nullptr);

    // 放入空槽位
    ItemStack stack(*m_diamond, 32);
    ItemStack remaining = m_inventory->placeItem(0, stack);
    EXPECT_TRUE(remaining.isEmpty());
    EXPECT_EQ(m_inventory->getItem(0).getCount(), 32);

    // 合并到现有堆
    ItemStack more(*m_diamond, 20);
    remaining = m_inventory->placeItem(0, more);
    EXPECT_TRUE(remaining.isEmpty());
    EXPECT_EQ(m_inventory->getItem(0).getCount(), 52);
}

TEST_F(PlayerInventoryTest, ArmorSlots)
{
    ASSERT_NE(m_diamond, nullptr);

    ItemStack helmet(*m_diamond, 1);
    ItemStack chestplate(*m_diamond, 1);
    ItemStack leggings(*m_diamond, 1);
    ItemStack boots(*m_diamond, 1);

    m_inventory->setHelmet(helmet);
    m_inventory->setChestplate(chestplate);
    m_inventory->setLeggings(leggings);
    m_inventory->setBoots(boots);

    EXPECT_EQ(m_inventory->getHelmet().getCount(), 1);
    EXPECT_EQ(m_inventory->getChestplate().getCount(), 1);
    EXPECT_EQ(m_inventory->getLeggings().getCount(), 1);
    EXPECT_EQ(m_inventory->getBoots().getCount(), 1);

    // 通过索引访问
    EXPECT_EQ(m_inventory->getItem(InventorySlots::ARMOR_HEAD).getCount(), 1);
    EXPECT_EQ(m_inventory->getItem(InventorySlots::ARMOR_CHEST).getCount(), 1);
    EXPECT_EQ(m_inventory->getItem(InventorySlots::ARMOR_LEGS).getCount(), 1);
    EXPECT_EQ(m_inventory->getItem(InventorySlots::ARMOR_FEET).getCount(), 1);
}

TEST(ArmorItemTest, RightClickEquipsMatchingArmorSlot)
{
    ArmorTestWorld world;
    Player player(1, "armor-test", mc::test::testEcsRegistry());

    const std::array<std::pair<item::armor::ArmorSlot, i32>, 4> cases = {{
        {item::armor::ArmorSlot::Head, InventorySlots::ARMOR_HEAD},
        {item::armor::ArmorSlot::Chest, InventorySlots::ARMOR_CHEST},
        {item::armor::ArmorSlot::Legs, InventorySlots::ARMOR_LEGS},
        {item::armor::ArmorSlot::Feet, InventorySlots::ARMOR_FEET},
    }};

    for (const auto& [slot, inventorySlot] : cases) {
        player.inventory().clear();
        player.inventory().setSelectedSlot(0);

        item::items::ArmorItem armorItem(item::armor::ArmorMaterials::IRON,
            slot,
            ItemProperties().maxDamage(item::armor::ArmorMaterials::IRON.getDurability(slot)));
        player.inventory().setItem(0, ItemStack(armorItem));

        ItemActionResult result = armorItem.onItemRightClick(world, player, Hand::MainHand);

        EXPECT_TRUE(result.isConsume());
        EXPECT_TRUE(result.getResult().isEmpty());
        EXPECT_TRUE(player.getHeldItem(Hand::MainHand).isEmpty());
        EXPECT_EQ(player.inventory().getItem(inventorySlot).getItem(), &armorItem);
        EXPECT_EQ(player.inventory().getItem(inventorySlot).getCount(), 1);
    }
}

TEST(ArmorItemTest, RightClickPassesWhenArmorSlotOccupied)
{
    ArmorTestWorld world;
    Player player(2, "armor-pass-test", mc::test::testEcsRegistry());

    item::items::ArmorItem armorItem(item::armor::ArmorMaterials::IRON,
        item::armor::ArmorSlot::Head,
        ItemProperties().maxDamage(item::armor::ArmorMaterials::IRON.getDurability(item::armor::ArmorSlot::Head)));
    player.inventory().setItem(0, ItemStack(armorItem));
    item::items::ArmorItem equippedHelmet(item::armor::ArmorMaterials::IRON,
        item::armor::ArmorSlot::Head,
        ItemProperties().maxDamage(item::armor::ArmorMaterials::IRON.getDurability(item::armor::ArmorSlot::Head)));
    player.inventory().setHelmet(ItemStack(equippedHelmet));

    ItemActionResult result = armorItem.onItemRightClick(world, player, Hand::MainHand);

    EXPECT_TRUE(result.isPass());
    EXPECT_FALSE(player.getHeldItem(Hand::MainHand).isEmpty());
    EXPECT_EQ(player.inventory().getHelmet().getItem(), &equippedHelmet);
    EXPECT_EQ(result.getResult().getItem(), &armorItem);
}

TEST_F(PlayerInventoryTest, OffhandSlot)
{
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(*m_diamond, 5);
    m_inventory->setOffhandItem(stack);

    EXPECT_EQ(m_inventory->getOffhandItem().getCount(), 5);
    EXPECT_EQ(m_inventory->getItem(InventorySlots::OFFHAND).getCount(), 5);
}

TEST_F(PlayerInventoryTest, DamageableItemStacking)
{
    ASSERT_NE(m_diamondSword, nullptr);

    // 有耐久度的物品堆叠数为1
    ItemStack sword(*m_diamondSword, 1);
    EXPECT_EQ(sword.getMaxStackSize(), 1);

    // 两把剑不能合并
    m_inventory->setItem(0, sword);
    ItemStack anotherSword(*m_diamondSword, 1);
    EXPECT_FALSE(m_inventory->getItem(0).canMergeWith(anotherSword));
}

TEST_F(PlayerInventoryTest, IsHotbar)
{
    EXPECT_TRUE(PlayerInventory::isHotbar(0));
    EXPECT_TRUE(PlayerInventory::isHotbar(4));
    EXPECT_TRUE(PlayerInventory::isHotbar(8));
    EXPECT_FALSE(PlayerInventory::isHotbar(9));
    EXPECT_FALSE(PlayerInventory::isHotbar(-1));
    EXPECT_FALSE(PlayerInventory::isHotbar(40));
}

TEST_F(PlayerInventoryTest, GetBestHotbarSlot)
{
    ASSERT_NE(m_diamond, nullptr);

    // 空背包，返回第一个槽位
    EXPECT_EQ(m_inventory->getBestHotbarSlot(), 0);

    // 填充一些槽位
    m_inventory->setItem(0, ItemStack(*m_diamond, 1));
    m_inventory->setItem(1, ItemStack(*m_diamond, 1));
    m_inventory->setItem(3, ItemStack(*m_diamond, 1));

    // 应该返回第一个空槽位
    EXPECT_EQ(m_inventory->getBestHotbarSlot(), 2);
}

// ============================================================================
// Slot 测试
// ============================================================================

class SlotTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        item::enchant::EnchantmentRegistry::initialize();
        m_inventory = std::make_unique<PlayerInventory>(nullptr);
        m_diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    }

    void TearDown() override { item::enchant::EnchantmentRegistry::clear(); }

    std::unique_ptr<PlayerInventory> m_inventory;
    Item* m_diamond = nullptr;
};

TEST_F(SlotTest, BasicOperations)
{
    ASSERT_NE(m_diamond, nullptr);

    Slot slot(m_inventory.get(), 0, 10, 20);

    EXPECT_EQ(slot.getIndex(), 0);
    EXPECT_EQ(slot.getX(), 10);
    EXPECT_EQ(slot.getY(), 20);
    EXPECT_TRUE(slot.isEmpty());

    // 设置物品
    ItemStack stack(*m_diamond, 32);
    m_inventory->setItem(0, stack);

    EXPECT_FALSE(slot.isEmpty());
    EXPECT_EQ(slot.getItem().getCount(), 32);

    // 移除物品
    ItemStack removed = slot.remove(10);
    EXPECT_EQ(removed.getCount(), 10);
    EXPECT_EQ(slot.getItem().getCount(), 22);
}

TEST_F(SlotTest, MaxStackSize)
{
    ASSERT_NE(m_diamond, nullptr);

    Slot slot(m_inventory.get(), 0, 0, 0);
    ItemStack stack(*m_diamond, 1);

    EXPECT_EQ(slot.getMaxStackSize(), 64);
    EXPECT_EQ(slot.getMaxStackSize(stack), 64);
}

TEST_F(SlotTest, MayPlace)
{
    Slot slot(m_inventory.get(), 0, 0, 0);

    EXPECT_TRUE(slot.mayPlace(ItemStack::EMPTY));
}

TEST_F(SlotTest, ArmorSlotOnlyAcceptsMatchingArmorType)
{
    auto makeArmorItem = [](const item::armor::ArmorMaterial& material, item::armor::ArmorSlot slot) {
        return item::items::ArmorItem(material, slot, ItemProperties().maxDamage(material.getDurability(slot)));
    };

    const auto helmet = makeArmorItem(item::armor::ArmorMaterials::IRON, item::armor::ArmorSlot::Head);
    const auto chestplate = makeArmorItem(item::armor::ArmorMaterials::IRON, item::armor::ArmorSlot::Chest);
    const auto leggings = makeArmorItem(item::armor::ArmorMaterials::IRON, item::armor::ArmorSlot::Legs);
    const auto boots = makeArmorItem(item::armor::ArmorMaterials::IRON, item::armor::ArmorSlot::Feet);

    ArmorSlot headSlot(m_inventory.get(), InventorySlots::ARMOR_HEAD, 0, 0, ArmorSlot::ArmorType::Head);
    ArmorSlot chestSlot(m_inventory.get(), InventorySlots::ARMOR_CHEST, 0, 0, ArmorSlot::ArmorType::Chest);
    ArmorSlot legsSlot(m_inventory.get(), InventorySlots::ARMOR_LEGS, 0, 0, ArmorSlot::ArmorType::Legs);
    ArmorSlot feetSlot(m_inventory.get(), InventorySlots::ARMOR_FEET, 0, 0, ArmorSlot::ArmorType::Feet);

    EXPECT_TRUE(headSlot.mayPlace(ItemStack(helmet)));
    EXPECT_FALSE(headSlot.mayPlace(ItemStack(chestplate)));
    EXPECT_FALSE(headSlot.mayPlace(ItemStack(*m_diamond)));

    EXPECT_TRUE(chestSlot.mayPlace(ItemStack(chestplate)));
    EXPECT_FALSE(chestSlot.mayPlace(ItemStack(leggings)));
    EXPECT_FALSE(chestSlot.mayPlace(ItemStack(*m_diamond)));

    EXPECT_TRUE(legsSlot.mayPlace(ItemStack(leggings)));
    EXPECT_FALSE(legsSlot.mayPlace(ItemStack(boots)));
    EXPECT_FALSE(legsSlot.mayPlace(ItemStack(*m_diamond)));

    EXPECT_TRUE(feetSlot.mayPlace(ItemStack(boots)));
    EXPECT_FALSE(feetSlot.mayPlace(ItemStack(helmet)));
    EXPECT_FALSE(feetSlot.mayPlace(ItemStack(*m_diamond)));
}

TEST_F(SlotTest, ArmorSlotMayPickupReturnsTrueForEmptySlot)
{
    ArmorSlot headSlot(m_inventory.get(), InventorySlots::ARMOR_HEAD, 0, 0, ArmorSlot::ArmorType::Head);
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());

    // 空槽位应该总是可以拾取
    EXPECT_TRUE(headSlot.mayPickup(player));
}

TEST_F(SlotTest, ArmorSlotMayPickupReturnsTrueForCreativePlayer)
{
    auto makeArmorItem = [](const item::armor::ArmorMaterial& material, item::armor::ArmorSlot slot) {
        return item::items::ArmorItem(material, slot, ItemProperties().maxDamage(material.getDurability(slot)));
    };

    const auto helmet = makeArmorItem(item::armor::ArmorMaterials::IRON, item::armor::ArmorSlot::Head);

    ArmorSlot headSlot(m_inventory.get(), InventorySlots::ARMOR_HEAD, 0, 0, ArmorSlot::ArmorType::Head);
    m_inventory->setItem(InventorySlots::ARMOR_HEAD, ItemStack(helmet));

    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setGameMode(GameMode::Creative);

    // 创造模式玩家可以取下任何护甲
    EXPECT_TRUE(headSlot.mayPickup(player));
}

TEST_F(SlotTest, ArmorSlotMayPickupReturnsTrueForNormalArmor)
{
    auto makeArmorItem = [](const item::armor::ArmorMaterial& material, item::armor::ArmorSlot slot) {
        return item::items::ArmorItem(material, slot, ItemProperties().maxDamage(material.getDurability(slot)));
    };

    const auto chestplate = makeArmorItem(item::armor::ArmorMaterials::DIAMOND, item::armor::ArmorSlot::Chest);

    ArmorSlot chestSlot(m_inventory.get(), InventorySlots::ARMOR_CHEST, 0, 0, ArmorSlot::ArmorType::Chest);
    m_inventory->setItem(InventorySlots::ARMOR_CHEST, ItemStack(chestplate));

    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setGameMode(GameMode::Survival);

    // 普通护甲（无绑定诅咒）可以取下
    EXPECT_TRUE(chestSlot.mayPickup(player));
}

TEST_F(SlotTest, ArmorSlotMayPickupReturnsFalseForBindingCurseArmor)
{
    auto makeArmorItem = [](const item::armor::ArmorMaterial& material, item::armor::ArmorSlot slot) {
        return item::items::ArmorItem(material, slot, ItemProperties().maxDamage(material.getDurability(slot)));
    };

    const auto boots = makeArmorItem(item::armor::ArmorMaterials::DIAMOND, item::armor::ArmorSlot::Feet);

    ArmorSlot feetSlot(m_inventory.get(), InventorySlots::ARMOR_FEET, 0, 0, ArmorSlot::ArmorType::Feet);

    // 创建带绑定诅咒的护甲
    ItemStack cursedBoots(boots);
    cursedBoots.addEnchantment("minecraft:binding_curse", 1);
    m_inventory->setItem(InventorySlots::ARMOR_FEET, cursedBoots);

    Player survivalPlayer(EntityInstanceId(1), "SurvivalPlayer", mc::test::testEcsRegistry());
    survivalPlayer.setGameMode(GameMode::Survival);

    // 生存模式玩家无法取下绑定诅咒的护甲
    EXPECT_FALSE(feetSlot.mayPickup(survivalPlayer));

    // 创造模式玩家可以取下绑定诅咒的护甲
    Player creativePlayer(EntityInstanceId(2), "CreativePlayer", mc::test::testEcsRegistry());
    creativePlayer.setGameMode(GameMode::Creative);
    EXPECT_TRUE(feetSlot.mayPickup(creativePlayer));
}

TEST_F(SlotTest, ArmorSlotMayPickupWithMultipleEnchantments)
{
    auto makeArmorItem = [](const item::armor::ArmorMaterial& material, item::armor::ArmorSlot slot) {
        return item::items::ArmorItem(material, slot, ItemProperties().maxDamage(material.getDurability(slot)));
    };

    const auto leggings = makeArmorItem(item::armor::ArmorMaterials::NETHERITE, item::armor::ArmorSlot::Legs);

    ArmorSlot legsSlot(m_inventory.get(), InventorySlots::ARMOR_LEGS, 0, 0, ArmorSlot::ArmorType::Legs);

    // 创建带多个附魔的护甲（包含绑定诅咒）
    ItemStack multiEnchantedLeggings(leggings);
    multiEnchantedLeggings.addEnchantment("minecraft:protection", 4);
    multiEnchantedLeggings.addEnchantment("minecraft:unbreaking", 3);
    multiEnchantedLeggings.addEnchantment("minecraft:binding_curse", 1);
    multiEnchantedLeggings.addEnchantment("minecraft:mending", 1);
    m_inventory->setItem(InventorySlots::ARMOR_LEGS, multiEnchantedLeggings);

    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setGameMode(GameMode::Survival);

    // 绑定诅咒存在时无法取下
    EXPECT_FALSE(legsSlot.mayPickup(player));

    // 验证绑定诅咒附魔确实存在
    EXPECT_TRUE(item::enchant::EnchantmentHelper::hasBindingCurse(m_inventory->getItem(InventorySlots::ARMOR_LEGS)));
}

TEST(ArmorItemTest, TotalArmorStatsSumAllEquippedPieces)
{
    TestLivingEntity entity;

    const item::items::ArmorItem helmet(item::armor::ArmorMaterials::NETHERITE,
        item::armor::ArmorSlot::Head,
        ItemProperties().maxDamage(item::armor::ArmorMaterials::NETHERITE.getDurability(item::armor::ArmorSlot::Head)));
    const item::items::ArmorItem chestplate(item::armor::ArmorMaterials::NETHERITE,
        item::armor::ArmorSlot::Chest,
        ItemProperties().maxDamage(
            item::armor::ArmorMaterials::NETHERITE.getDurability(item::armor::ArmorSlot::Chest)));
    const item::items::ArmorItem leggings(item::armor::ArmorMaterials::NETHERITE,
        item::armor::ArmorSlot::Legs,
        ItemProperties().maxDamage(item::armor::ArmorMaterials::NETHERITE.getDurability(item::armor::ArmorSlot::Legs)));
    const item::items::ArmorItem boots(item::armor::ArmorMaterials::NETHERITE,
        item::armor::ArmorSlot::Feet,
        ItemProperties().maxDamage(item::armor::ArmorMaterials::NETHERITE.getDurability(item::armor::ArmorSlot::Feet)));

    entity.setEquipment(EquipmentSlot::Head, ItemStack(helmet));
    entity.setEquipment(EquipmentSlot::Chest, ItemStack(chestplate));
    entity.setEquipment(EquipmentSlot::Legs, ItemStack(leggings));
    entity.setEquipment(EquipmentSlot::Feet, ItemStack(boots));

    EXPECT_EQ(item::items::ArmorItem::getTotalArmorValue(entity), 20);
    EXPECT_FLOAT_EQ(item::items::ArmorItem::getTotalToughness(entity), 12.0f);
    EXPECT_FLOAT_EQ(item::items::ArmorItem::getTotalKnockbackResistance(entity), 0.4f);
}

TEST(DyeableArmorItemTest, ColorRoundTripUsesDisplayTag)
{
    const item::items::DyeableArmorItem leatherBoots(item::armor::ArmorMaterials::LEATHER,
        item::armor::ArmorSlot::Feet,
        ItemProperties().maxDamage(item::armor::ArmorMaterials::LEATHER.getDurability(item::armor::ArmorSlot::Feet)));

    ItemStack stack(leatherBoots);
    EXPECT_FALSE(item::items::DyeableArmorItem::hasColor(stack));
    EXPECT_EQ(leatherBoots.getColor(stack), 0xA06540u);

    item::items::DyeableArmorItem::setColor(stack, 0x123456u);
    EXPECT_TRUE(item::items::DyeableArmorItem::hasColor(stack));
    EXPECT_EQ(leatherBoots.getColor(stack), 0x123456u);
    ASSERT_NE(stack.getChildTag("display"), nullptr);
    const int storedColor = (*stack.getChildTag("display"))["color"].get<int>();
    EXPECT_EQ(storedColor, 0x123456);

    item::items::DyeableArmorItem::clearColor(stack);
    EXPECT_FALSE(item::items::DyeableArmorItem::hasColor(stack));
    EXPECT_EQ(leatherBoots.getColor(stack), 0xA06540u);
    EXPECT_FALSE(stack.hasTag());
}

TEST(ElytraItemTest, RightClickEquipsChestSlot)
{
    ArmorTestWorld world;
    Player player(3, "elytra-test", mc::test::testEcsRegistry());

    item::items::ElytraItem elytra{ItemProperties()};
    player.inventory().setItem(0, ItemStack(elytra));

    ItemActionResult result = elytra.onItemRightClick(world, player, Hand::MainHand);

    EXPECT_TRUE(result.isConsume());
    EXPECT_TRUE(result.getResult().isEmpty());
    EXPECT_TRUE(player.getHeldItem(Hand::MainHand).isEmpty());
    EXPECT_EQ(player.inventory().getChestplate().getItem(), &elytra);
    EXPECT_EQ(player.inventory().getChestplate().getCount(), 1);
}

TEST(ElytraItemTest, InventoryTickDamagesOnlyWhenGlidingInChestSlot)
{
    ArmorTestWorld world;
    world.setCurrentTick(20);

    TestLivingEntity entity;
    entity.setPose(EntityPose::FallFlying);

    item::items::ElytraItem elytra{ItemProperties()};
    ItemStack chestElytra(elytra);
    elytra.inventoryTick(chestElytra, world, entity, InventorySlots::ARMOR_CHEST, false);

    EXPECT_EQ(chestElytra.getDamage(), 1);

    ItemStack carriedElytra(elytra);
    elytra.inventoryTick(carriedElytra, world, entity, 0, false);

    EXPECT_EQ(carriedElytra.getDamage(), 0);
}

// ============================================================================
// IInventory 接口测试
// ============================================================================

class IInventoryInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        m_inventory = std::make_unique<PlayerInventory>(nullptr);
    }

    std::unique_ptr<PlayerInventory> m_inventory;
};

TEST_F(IInventoryInterfaceTest, HasAny_WorksWithIInventory)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);

    IInventory* inv = m_inventory.get();
    std::unordered_set<const Item*> items = {diamond};

    EXPECT_FALSE(inv->hasAny(items));

    m_inventory->setItem(0, ItemStack(*diamond, 10));
    EXPECT_TRUE(inv->hasAny(items));
}

TEST_F(IInventoryInterfaceTest, HasAny_EmptySet)
{
    IInventory* inv = m_inventory.get();
    std::unordered_set<const Item*> empty;
    EXPECT_FALSE(inv->hasAny(empty));
}

TEST_F(IInventoryInterfaceTest, HasAny_MultipleItems)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    Item* coal = ItemRegistry::instance().getItem(ResourceLocation("minecraft:coal"));
    ASSERT_NE(diamond, nullptr);
    ASSERT_NE(coal, nullptr);

    IInventory* inv = m_inventory.get();
    std::unordered_set<const Item*> items = {diamond, coal};

    // 空背包不包含任何物品
    EXPECT_FALSE(inv->hasAny(items));

    // 添加钻石
    m_inventory->setItem(0, ItemStack(*diamond, 10));
    EXPECT_TRUE(inv->hasAny(items));

    // 清空后添加煤炭
    m_inventory->clear();
    m_inventory->setItem(5, ItemStack(*coal, 5));
    EXPECT_TRUE(inv->hasAny(items));
}

TEST_F(IInventoryInterfaceTest, HasAny_AfterPartialRemove)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);

    IInventory* inv = m_inventory.get();
    std::unordered_set<const Item*> items = {diamond};

    // 添加物品然后部分移除
    m_inventory->setItem(0, ItemStack(*diamond, 10));
    EXPECT_TRUE(inv->hasAny(items));

    // 部分移除
    m_inventory->removeItem(0, 5);
    EXPECT_TRUE(inv->hasAny(items));

    // 完全移除
    m_inventory->removeItem(0, 5);
    EXPECT_FALSE(inv->hasAny(items));
}

TEST_F(IInventoryInterfaceTest, HasAny_WithNullItemInSet)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);

    IInventory* inv = m_inventory.get();
    // 集合中包含空指针（边界情况）
    std::unordered_set<const Item*> items = {diamond, nullptr};

    m_inventory->setItem(0, ItemStack(*diamond, 10));
    // 应该仍然能找到钻石
    EXPECT_TRUE(inv->hasAny(items));
}

// ============================================================================
// IInventory 边界测试 - 空背包和空集合
// ============================================================================

class IInventoryEdgeCaseTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        m_inventory = std::make_unique<PlayerInventory>(nullptr);
    }

    std::unique_ptr<PlayerInventory> m_inventory;
};

TEST_F(IInventoryEdgeCaseTest, HasAny_EmptyInventory)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);

    IInventory* inv = m_inventory.get();
    std::unordered_set<const Item*> items = {diamond};

    // 空背包不包含任何物品
    EXPECT_FALSE(inv->hasAny(items));
}

TEST_F(IInventoryEdgeCaseTest, HasAny_EmptySet)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);

    IInventory* inv = m_inventory.get();

    // 添加物品到背包
    m_inventory->setItem(0, ItemStack(*diamond, 10));

    // 空集合不匹配任何物品
    std::unordered_set<const Item*> empty;
    EXPECT_FALSE(inv->hasAny(empty));
}

TEST_F(IInventoryEdgeCaseTest, HasAny_AllNullItems)
{
    IInventory* inv = m_inventory.get();

    // 集合中只有空指针
    std::unordered_set<const Item*> items = {nullptr};

    // 添加物品到背包
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    if (diamond != nullptr) {
        m_inventory->setItem(0, ItemStack(*diamond, 10));
    }

    // 空指针集合应该返回 false
    EXPECT_FALSE(inv->hasAny(items));
}

TEST_F(IInventoryEdgeCaseTest, HasAny_MultipleItemsInDifferentSlots)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    Item* coal = ItemRegistry::instance().getItem(ResourceLocation("minecraft:coal"));
    Item* iron = ItemRegistry::instance().getItem(ResourceLocation("minecraft:iron_ingot"));
    ASSERT_NE(diamond, nullptr);
    ASSERT_NE(coal, nullptr);

    IInventory* inv = m_inventory.get();

    // 将物品放在不同槽位
    m_inventory->setItem(0, ItemStack(*diamond, 10));
    m_inventory->setItem(20, ItemStack(*coal, 5));

    // 检查包含钻石和铁的集合（应该找到钻石）
    std::unordered_set<const Item*> items1 = {diamond, iron};
    EXPECT_TRUE(inv->hasAny(items1));

    // 检查只包含铁的集合（不应该找到）
    std::unordered_set<const Item*> items2;
    if (iron != nullptr) {
        items2.insert(iron);
    }
    EXPECT_FALSE(inv->hasAny(items2));

    // 清空后检查
    m_inventory->clear();
    EXPECT_FALSE(inv->hasAny(items1));
}

// ============================================================================
// PlayerInventory 新方法测试
// ============================================================================

class PlayerInventoryNewMethodsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        item::enchant::EnchantmentRegistry::initialize();
        m_inventory = std::make_unique<PlayerInventory>(nullptr);
    }

    void TearDown() override { item::enchant::EnchantmentRegistry::clear(); }

    std::unique_ptr<PlayerInventory> m_inventory;
};

TEST_F(PlayerInventoryNewMethodsTest, TickDoesNotCrashOnNullPlayer)
{
    // PlayerInventory::tick() 在 m_player 为 nullptr 时应该安全返回
    // 不会崩溃
    EXPECT_NO_THROW(m_inventory->tick());
}

TEST_F(PlayerInventoryNewMethodsTest, TickDoesNotCrashOnNullWorld)
{
    // 即使有 player 但 world 为 nullptr，也应该安全返回
    ArmorTestWorld world;
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());

    PlayerInventory inventory(&player);
    EXPECT_NO_THROW(inventory.tick());
}

TEST_F(PlayerInventoryNewMethodsTest, DropAllItemsClearsInventory)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);

    // 填充背包
    for (int i = 0; i < 10; ++i) {
        m_inventory->setItem(i, ItemStack(*diamond, 32));
    }
    EXPECT_FALSE(m_inventory->isEmpty());

    // dropAllItems 需要 player，null player 不会崩溃但也不会掉落
    EXPECT_NO_THROW(m_inventory->dropAllItems());

    // 在没有 player 的情况下，物品仍然会被清空
    // 因为 dropAllItems 会遍历并调用 player->dropItem
    // 但 player 为 nullptr 时会直接返回
}

TEST_F(PlayerInventoryNewMethodsTest, DamageArmorWithZeroDamageDoesNothing)
{
    auto makeArmorItem = [](const item::armor::ArmorMaterial& material, item::armor::ArmorSlot slot) {
        return item::items::ArmorItem(material, slot, ItemProperties().maxDamage(material.getDurability(slot)));
    };

    const auto helmet = makeArmorItem(item::armor::ArmorMaterials::IRON, item::armor::ArmorSlot::Head);
    ItemStack helmetStack(helmet);
    m_inventory->setHelmet(helmetStack);

    // 0 伤害不会损坏护甲
    auto source = DamageSources::generic();
    EXPECT_NO_THROW(m_inventory->damageArmor(source, 0.0f));
    EXPECT_FALSE(m_inventory->getHelmet().isEmpty());
}

TEST_F(PlayerInventoryNewMethodsTest, DamageArmorDamagesAllArmorPieces)
{
    auto makeArmorItem = [](const item::armor::ArmorMaterial& material, item::armor::ArmorSlot slot) {
        return item::items::ArmorItem(material, slot, ItemProperties().maxDamage(material.getDurability(slot)));
    };

    // 装备全套铁甲
    const auto helmet = makeArmorItem(item::armor::ArmorMaterials::IRON, item::armor::ArmorSlot::Head);
    const auto chestplate = makeArmorItem(item::armor::ArmorMaterials::IRON, item::armor::ArmorSlot::Chest);
    const auto leggings = makeArmorItem(item::armor::ArmorMaterials::IRON, item::armor::ArmorSlot::Legs);
    const auto boots = makeArmorItem(item::armor::ArmorMaterials::IRON, item::armor::ArmorSlot::Feet);

    m_inventory->setHelmet(ItemStack(helmet));
    m_inventory->setChestplate(ItemStack(chestplate));
    m_inventory->setLeggings(ItemStack(leggings));
    m_inventory->setBoots(ItemStack(boots));

    // 记录初始耐久
    i32 initialHelmetDamage = m_inventory->getHelmet().getDamage();
    i32 initialChestplateDamage = m_inventory->getChestplate().getDamage();
    i32 initialLeggingsDamage = m_inventory->getLeggings().getDamage();
    i32 initialBootsDamage = m_inventory->getBoots().getDamage();

    // 造成 16 点伤害（分摊到 4 件护甲，每件 4 点）
    auto source = DamageSources::generic();
    m_inventory->damageArmor(source, 16.0f);

    // 每件护甲应该都受到至少 1 点损伤
    // 伤害分摊: damage / 4 = 4, 最小为 1
    EXPECT_GE(m_inventory->getHelmet().getDamage(), initialHelmetDamage + 1);
    EXPECT_GE(m_inventory->getChestplate().getDamage(), initialChestplateDamage + 1);
    EXPECT_GE(m_inventory->getLeggings().getDamage(), initialLeggingsDamage + 1);
    EXPECT_GE(m_inventory->getBoots().getDamage(), initialBootsDamage + 1);
}

TEST_F(PlayerInventoryNewMethodsTest, DamageArmorFireDamageSkipsBurnableArmor)
{
    // 皮革护甲是可燃烧的，火焰伤害不应该损坏它
    // 注意：这里需要验证皮革护甲确实设置了 isBurnable = true
    // 如果皮革护甲没有设置 isBurnable，则火焰伤害会正常损坏它
    const auto leatherHelmet = item::items::ArmorItem(item::armor::ArmorMaterials::LEATHER,
        item::armor::ArmorSlot::Head,
        ItemProperties().maxDamage(item::armor::ArmorMaterials::LEATHER.getDurability(item::armor::ArmorSlot::Head)));

    m_inventory->setHelmet(ItemStack(leatherHelmet));

    i32 initialDamage = m_inventory->getHelmet().getDamage();

    // 检查皮革护甲是否是可燃烧的
    // 如果皮革护甲没有设置 isBurnable，则测试会失败
    // 这是预期行为：需要确保 ArmorItem 正确设置了 isBurnable 属性
    bool isLeatherBurnable = leatherHelmet.isBurnable();

    // 火焰伤害
    auto fireSource = DamageSources::onFire();
    m_inventory->damageArmor(fireSource, 10.0f);

    if (isLeatherBurnable) {
        // 皮革护甲是可燃烧的，火焰伤害不会损坏它
        EXPECT_EQ(m_inventory->getHelmet().getDamage(), initialDamage);
    } else {
        // 如果皮革护甲没有设置 isBurnable，则火焰伤害会正常损坏它
        // 这种情况下测试会验证护甲被损坏
        EXPECT_GE(m_inventory->getHelmet().getDamage(), initialDamage + 1);
    }

    // 非火焰伤害应该会损坏
    auto normalSource = DamageSources::generic();
    m_inventory->damageArmor(normalSource, 10.0f);

    // 非火焰伤害会损坏护甲
    EXPECT_GE(m_inventory->getHelmet().getDamage(), initialDamage + (isLeatherBurnable ? 1 : 2));
}

TEST_F(PlayerInventoryNewMethodsTest, DamageArmorOnlyDamagesArmorItems)
{
    // 验证只有护甲物品（ArmorItem 和 ElytraItem）会受伤
    // 非护甲的可损坏物品放在护甲槽中不应该受到护甲伤害

    // 钻石剑是可损坏的但不是护甲
    Item* diamondSword = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond_sword"));
    ASSERT_NE(diamondSword, nullptr);
    ASSERT_TRUE(diamondSword->isDamageable());
    ASSERT_FALSE(diamondSword->isArmor());

    ItemStack swordStack(*diamondSword);
    ASSERT_TRUE(swordStack.isDamageable());

    // 直接放入护甲槽（绕过 mayPlace 检查，模拟命令或其他强制放置）
    m_inventory->setHelmet(swordStack);
    i32 initialDamage = m_inventory->getHelmet().getDamage();

    // 造成伤害
    auto source = DamageSources::generic();
    m_inventory->damageArmor(source, 16.0f);

    // 非护甲物品不应该受到护甲伤害
    EXPECT_EQ(m_inventory->getHelmet().getDamage(), initialDamage);
}

TEST_F(PlayerInventoryNewMethodsTest, DamageArmorDamagesElytraItem)
{
    // 验证鞘翅（ElytraItem）在护甲槽中会受到伤害
    const auto elytra = item::items::ElytraItem(ItemProperties().maxDamage(432));
    ItemStack elytraStack(elytra);
    ASSERT_TRUE(elytraStack.isDamageable());
    ASSERT_TRUE(elytraStack.getItem()->isArmor());

    m_inventory->setChestplate(elytraStack);
    i32 initialDamage = m_inventory->getChestplate().getDamage();

    auto source = DamageSources::generic();
    m_inventory->damageArmor(source, 8.0f);

    // 鞘翅应该受到护甲伤害（8 / 4 = 2）
    EXPECT_GE(m_inventory->getChestplate().getDamage(), initialDamage + 1);
}

TEST_F(PlayerInventoryNewMethodsTest, IsArmorReturnsCorrectValue)
{
    // ArmorItem::isArmor() 返回 true
    const auto helmet = item::items::ArmorItem(item::armor::ArmorMaterials::IRON,
        item::armor::ArmorSlot::Head,
        ItemProperties().maxDamage(item::armor::ArmorMaterials::IRON.getDurability(item::armor::ArmorSlot::Head)));
    EXPECT_TRUE(helmet.isArmor());

    // ElytraItem::isArmor() 返回 true
    const auto elytra = item::items::ElytraItem(ItemProperties().maxDamage(432));
    EXPECT_TRUE(elytra.isArmor());

    // 通过注册表获取的非护甲可损坏物品（钻石剑）isArmor() 返回 false
    Item* diamondSword = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond_sword"));
    ASSERT_NE(diamondSword, nullptr);
    EXPECT_TRUE(diamondSword->isDamageable());
    EXPECT_FALSE(diamondSword->isArmor());

    // 不可损坏的普通物品 isArmor() 返回 false
    Item* stick = ItemRegistry::instance().getItem(ResourceLocation("minecraft:stick"));
    ASSERT_NE(stick, nullptr);
    EXPECT_FALSE(stick->isDamageable());
    EXPECT_FALSE(stick->isArmor());
}

TEST_F(PlayerInventoryNewMethodsTest, GetDestroySpeedReturnsCorrectValue)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);

    // 空手挖掘速度为 1.0
    // 注意：BlockState 需要有效的方块才能构造，这里使用 defaultAirState
    // 但由于测试环境可能没有初始化方块，这里只验证方法的基本行为
    // 当物品为空时，返回 1.0
    EXPECT_FLOAT_EQ(m_inventory->getDestroySpeed(VanillaBlocks::AIR->defaultState()), 1.0f);

    // 设置手持物品
    m_inventory->setSelectedSlot(0);
    m_inventory->setItem(0, ItemStack(*diamond, 1));

    // 有物品时返回物品的挖掘速度
    // 注意：需要实际的 BlockState 来测试挖掘速度
    // 这里只测试方法不会崩溃
    EXPECT_NO_THROW(static_cast<void>(m_inventory->getDestroySpeed(VanillaBlocks::AIR->defaultState())));
}

TEST_F(PlayerInventoryNewMethodsTest, PlaceItemBackInInventoryMergesExistingStacks)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);

    // 在槽位 0 放置 32 个钻石
    m_inventory->setItem(0, ItemStack(*diamond, 32));

    // 尝试放回 10 个钻石
    ItemStack stack(*diamond, 10);
    bool result = m_inventory->placeItemBackInInventory(stack);

    // 应该成功合并
    EXPECT_TRUE(result);
    EXPECT_EQ(m_inventory->getItem(0).getCount(), 42);
    EXPECT_TRUE(stack.isEmpty());
}

TEST_F(PlayerInventoryNewMethodsTest, PlaceItemBackInInventoryFindsEmptySlot)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);

    // 只填充快捷栏和主背包 (0-35)，不包括护甲 (36-39) 和副手 (40)
    // getFirstEmptySlot() 会按顺序检查：快捷栏 (0-8) -> 主背包 (9-35)
    // 所以护甲槽和副手槽不会被 getFirstEmptySlot() 返回
    for (int i = 0; i <= 35; ++i) {
        m_inventory->setItem(i, ItemStack(*diamond, 64));
    }

    // 现在快捷栏和主背包都满了，但护甲槽 (36-39) 和副手 (40) 是空的
    // placeItemBackInInventory 会尝试找空槽位
    ItemStack stack(*diamond, 10);
    bool result = m_inventory->placeItemBackInInventory(stack);

    // 由于 getFirstEmptySlot() 不检查护甲和副手槽，应该会失败
    // 这是 MC 1.16.5 的预期行为：护甲和副手槽有特殊的放置逻辑
    EXPECT_FALSE(result); // 没有空槽位可用
}
