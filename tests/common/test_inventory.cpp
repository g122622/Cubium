#include <gtest/gtest.h>
#include "../src/common/entity/inventory/IInventory.hpp"
#include "../src/common/entity/inventory/Slot.hpp"
#include "../src/common/entity/inventory/PlayerInventory.hpp"
#include "../src/common/item/armor/ArmorMaterial.hpp"
#include "../src/common/item/items/armor/ArmorItem.hpp"
#include "../src/common/item/core/ItemRegistry.hpp"
#include "../src/common/item/Items.hpp"

using namespace mc;

// ============================================================================
// Slot 索引常量测试
// ============================================================================

class InventorySlotsTest : public ::testing::Test {
protected:
    void SetUp() override {
        Items::initialize();
    }
};

TEST_F(InventorySlotsTest, ConstantsAreCorrect) {
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
    void SetUp() override {
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

TEST_F(PlayerInventoryTest, InitialState) {
    EXPECT_EQ(m_inventory->getContainerSize(), 41);
    EXPECT_TRUE(m_inventory->isEmpty());
    EXPECT_EQ(m_inventory->getSelectedSlot(), 0);
}

TEST_F(PlayerInventoryTest, SetAndGetItem) {
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(*m_diamond, 32);
    m_inventory->setItem(0, stack);

    EXPECT_FALSE(m_inventory->isEmpty());
    EXPECT_EQ(m_inventory->getItem(0).getCount(), 32);
    EXPECT_EQ(m_inventory->getItem(0).getItem(), m_diamond);
}

TEST_F(PlayerInventoryTest, HotbarOperations) {
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

TEST_F(PlayerInventoryTest, RemoveItem) {
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

TEST_F(PlayerInventoryTest, ClearInventory) {
    ASSERT_NE(m_diamond, nullptr);

    m_inventory->setItem(0, ItemStack(*m_diamond, 10));
    m_inventory->setItem(5, ItemStack(*m_diamond, 20));
    m_inventory->setItem(40, ItemStack(*m_diamond, 5));

    EXPECT_FALSE(m_inventory->isEmpty());

    m_inventory->clear();

    EXPECT_TRUE(m_inventory->isEmpty());
}

TEST_F(PlayerInventoryTest, AddItem) {
    ASSERT_NE(m_diamond, nullptr);

    // 添加到空背包
    ItemStack stack(*m_diamond, 32);
    i32 remaining = m_inventory->add(stack);

    EXPECT_EQ(remaining, 32);  // 全部添加成功
    EXPECT_TRUE(stack.isEmpty());

    // 检查物品在快捷栏
    EXPECT_EQ(m_inventory->getItem(0).getCount(), 32);
}

TEST_F(PlayerInventoryTest, AddItemMerging) {
    ASSERT_NE(m_diamond, nullptr);

    // 先放入一些钻石
    m_inventory->setItem(0, ItemStack(*m_diamond, 50));

    // 添加更多钻石（应该合并）
    ItemStack stack(*m_diamond, 20);
    i32 remaining = m_inventory->add(stack);

    // 槽位 0 从 50 变成 64（堆叠上限），剩余 6 个会放到下一个空槽位
    // add() 方法会继续尝试添加到空槽位
    EXPECT_EQ(remaining, 20);  // 全部添加成功
    EXPECT_EQ(m_inventory->getItem(0).getCount(), 64);  // 达到堆叠上限
    EXPECT_TRUE(stack.isEmpty());  // 全部添加成功，stack 变空

    // 剩余的 6 个应该放在下一个槽位
    EXPECT_EQ(m_inventory->getItem(1).getCount(), 6);
}

TEST_F(PlayerInventoryTest, AddMultipleItems) {
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

TEST_F(PlayerInventoryTest, FindSlot) {
    ASSERT_NE(m_diamond, nullptr);
    ASSERT_NE(m_stick, nullptr);

    m_inventory->setItem(5, ItemStack(*m_diamond, 10));
    m_inventory->setItem(20, ItemStack(*m_stick, 5));

    EXPECT_EQ(m_inventory->findSlot(*m_diamond), 5);
    EXPECT_EQ(m_inventory->findSlot(*m_stick), 20);
    EXPECT_EQ(m_inventory->findSlot(*ItemRegistry::instance().getItem(ResourceLocation("minecraft:coal"))), -1);
}

TEST_F(PlayerInventoryTest, CountItem) {
    ASSERT_NE(m_diamond, nullptr);

    m_inventory->setItem(0, ItemStack(*m_diamond, 10));
    m_inventory->setItem(5, ItemStack(*m_diamond, 20));
    m_inventory->setItem(30, ItemStack(*m_diamond, 15));

    EXPECT_EQ(m_inventory->countItem(*m_diamond), 45);
}

TEST_F(PlayerInventoryTest, HasItem) {
    ASSERT_NE(m_diamond, nullptr);
    ASSERT_NE(m_stick, nullptr);

    m_inventory->setItem(0, ItemStack(*m_diamond, 10));

    EXPECT_TRUE(m_inventory->hasItem(*m_diamond));
    EXPECT_FALSE(m_inventory->hasItem(*m_stick));
}

TEST_F(PlayerInventoryTest, GetFirstEmptySlot) {
    ASSERT_NE(m_diamond, nullptr);

    // 空背包
    EXPECT_EQ(m_inventory->getFirstEmptySlot(), 0);

    // 填充前几个槽位
    m_inventory->setItem(0, ItemStack(*m_diamond, 1));
    m_inventory->setItem(1, ItemStack(*m_diamond, 1));
    m_inventory->setItem(2, ItemStack(*m_diamond, 1));

    EXPECT_EQ(m_inventory->getFirstEmptySlot(), 3);
}

TEST_F(PlayerInventoryTest, SwapSlots) {
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

TEST_F(PlayerInventoryTest, PlaceItem) {
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

TEST_F(PlayerInventoryTest, ArmorSlots) {
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

TEST_F(PlayerInventoryTest, OffhandSlot) {
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(*m_diamond, 5);
    m_inventory->setOffhandItem(stack);

    EXPECT_EQ(m_inventory->getOffhandItem().getCount(), 5);
    EXPECT_EQ(m_inventory->getItem(InventorySlots::OFFHAND).getCount(), 5);
}

TEST_F(PlayerInventoryTest, SerializationEmpty) {
    network::PacketSerializer ser;
    m_inventory->serialize(ser);

    const std::vector<u8>& data = ser.buffer();
    EXPECT_GT(data.size(), 0);

    network::PacketDeserializer deser(data);
    auto result = PlayerInventory::deserialize(deser);
    EXPECT_TRUE(result.success());

    PlayerInventory& loaded = result.value();
    EXPECT_TRUE(loaded.isEmpty());
    EXPECT_EQ(loaded.getSelectedSlot(), 0);
}

TEST_F(PlayerInventoryTest, SerializationWithItems) {
    ASSERT_NE(m_diamond, nullptr);
    ASSERT_NE(m_stick, nullptr);

    // 设置一些物品
    m_inventory->setItem(0, ItemStack(*m_diamond, 32));
    m_inventory->setItem(5, ItemStack(*m_stick, 16));
    m_inventory->setItem(40, ItemStack(*m_diamond, 8));
    m_inventory->setSelectedSlot(3);

    // 序列化
    network::PacketSerializer ser;
    m_inventory->serialize(ser);

    // 反序列化
    network::PacketDeserializer deser(ser.buffer());
    auto result = PlayerInventory::deserialize(deser);
    EXPECT_TRUE(result.success());

    PlayerInventory& loaded = result.value();
    EXPECT_EQ(loaded.getSelectedSlot(), 3);
    EXPECT_EQ(loaded.getItem(0).getCount(), 32);
    EXPECT_EQ(loaded.getItem(0).getItem(), m_diamond);
    EXPECT_EQ(loaded.getItem(5).getCount(), 16);
    EXPECT_EQ(loaded.getItem(5).getItem(), m_stick);
    EXPECT_EQ(loaded.getItem(40).getCount(), 8);
}

TEST_F(PlayerInventoryTest, DamageableItemStacking) {
    ASSERT_NE(m_diamondSword, nullptr);

    // 有耐久度的物品堆叠数为1
    ItemStack sword(*m_diamondSword, 1);
    EXPECT_EQ(sword.getMaxStackSize(), 1);

    // 两把剑不能合并
    m_inventory->setItem(0, sword);
    ItemStack anotherSword(*m_diamondSword, 1);
    EXPECT_FALSE(m_inventory->getItem(0).canMergeWith(anotherSword));
}

TEST_F(PlayerInventoryTest, IsHotbar) {
    EXPECT_TRUE(PlayerInventory::isHotbar(0));
    EXPECT_TRUE(PlayerInventory::isHotbar(4));
    EXPECT_TRUE(PlayerInventory::isHotbar(8));
    EXPECT_FALSE(PlayerInventory::isHotbar(9));
    EXPECT_FALSE(PlayerInventory::isHotbar(-1));
    EXPECT_FALSE(PlayerInventory::isHotbar(40));
}

TEST_F(PlayerInventoryTest, GetBestHotbarSlot) {
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
    void SetUp() override {
        Items::initialize();
        m_inventory = std::make_unique<PlayerInventory>(nullptr);
        m_diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    }

    std::unique_ptr<PlayerInventory> m_inventory;
    Item* m_diamond = nullptr;
};

TEST_F(SlotTest, BasicOperations) {
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

TEST_F(SlotTest, MaxStackSize) {
    ASSERT_NE(m_diamond, nullptr);

    Slot slot(m_inventory.get(), 0, 0, 0);
    ItemStack stack(*m_diamond, 1);

    EXPECT_EQ(slot.getMaxStackSize(), 64);
    EXPECT_EQ(slot.getMaxStackSize(stack), 64);
}

TEST_F(SlotTest, MayPlace) {
    Slot slot(m_inventory.get(), 0, 0, 0);

    EXPECT_TRUE(slot.mayPlace(ItemStack::EMPTY));
}

TEST_F(SlotTest, ArmorSlotOnlyAcceptsMatchingArmorType) {
    auto makeArmorItem = [](const item::armor::ArmorMaterial& material,
                            item::armor::ArmorSlot slot) {
        return item::items::ArmorItem(
            material,
            slot,
            ItemProperties().maxDamage(material.getDurability(slot)));
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
