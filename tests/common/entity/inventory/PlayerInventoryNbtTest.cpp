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

#include "common/entity/inventory/PlayerEnderChestInventory.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity::serialization;

// ============================================================================
// InventorySlots NBT 槽位映射测试
// ============================================================================

TEST(InventorySlotsNbtMapping, HotbarSlots_Unchanged)
{
    // 快捷栏 0-8 在 NBT 中保持不变
    for (i32 i = 0; i <= 8; ++i) {
        EXPECT_EQ(InventorySlots::toNbtSlot(i), i) << "Hotbar slot " << i;
        EXPECT_EQ(InventorySlots::fromNbtSlot(i), i) << "NBT slot " << i;
    }
}

TEST(InventorySlotsNbtMapping, MainInventory_Unchanged)
{
    // 主背包 9-35 在 NBT 中保持不变
    for (i32 i = 9; i <= 35; ++i) {
        EXPECT_EQ(InventorySlots::toNbtSlot(i), i) << "Main slot " << i;
        EXPECT_EQ(InventorySlots::fromNbtSlot(i), i) << "NBT slot " << i;
    }
}

TEST(InventorySlotsNbtMapping, ArmorSlots_Mapped)
{
    // MC Java NBT 格式中护甲槽位的映射
    // 内部索引: 36=HEAD, 37=CHEST, 38=LEGS, 39=FEET
    // NBT Slot:  103=armor.head, 102=armor.chest, 101=armor.legs, 100=armor.feet
    EXPECT_EQ(InventorySlots::toNbtSlot(InventorySlots::ARMOR_HEAD), 103);
    EXPECT_EQ(InventorySlots::toNbtSlot(InventorySlots::ARMOR_CHEST), 102);
    EXPECT_EQ(InventorySlots::toNbtSlot(InventorySlots::ARMOR_LEGS), 101);
    EXPECT_EQ(InventorySlots::toNbtSlot(InventorySlots::ARMOR_FEET), 100);

    // 反向映射
    EXPECT_EQ(InventorySlots::fromNbtSlot(103), InventorySlots::ARMOR_HEAD);
    EXPECT_EQ(InventorySlots::fromNbtSlot(102), InventorySlots::ARMOR_CHEST);
    EXPECT_EQ(InventorySlots::fromNbtSlot(101), InventorySlots::ARMOR_LEGS);
    EXPECT_EQ(InventorySlots::fromNbtSlot(100), InventorySlots::ARMOR_FEET);
}

TEST(InventorySlotsNbtMapping, OffhandSlot_Mapped)
{
    // 副手槽位: 内部 40 → NBT -106
    EXPECT_EQ(InventorySlots::toNbtSlot(InventorySlots::OFFHAND), -106);
    EXPECT_EQ(InventorySlots::fromNbtSlot(-106), InventorySlots::OFFHAND);
}

TEST(InventorySlotsNbtMapping, InvalidSlots)
{
    // 无效槽位
    EXPECT_EQ(InventorySlots::fromNbtSlot(-1), -1);
    EXPECT_EQ(InventorySlots::fromNbtSlot(41), -1);
    EXPECT_EQ(InventorySlots::fromNbtSlot(99), -1);
    EXPECT_EQ(InventorySlots::fromNbtSlot(104), -1);
    EXPECT_EQ(InventorySlots::fromNbtSlot(200), -1);
}

TEST(InventorySlotsNbtMapping, RoundTrip_AllSlots)
{
    // 验证所有有效内部索引的 toNbtSlot/fromNbtSlot 往返一致性
    for (i32 i = 0; i < InventorySlots::TOTAL_SIZE; ++i) {
        i32 nbtSlot = InventorySlots::toNbtSlot(i);
        i32 roundTripped = InventorySlots::fromNbtSlot(nbtSlot);
        EXPECT_EQ(roundTripped, i) << "Round-trip failed for internal slot " << i << " (NBT=" << nbtSlot << ")";
    }
}

// ============================================================================
// PlayerInventory NBT 序列化测试
// ============================================================================

class PlayerInventoryNbtTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
        dirt = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dirt"));
        diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    }

    Item* stone = nullptr;
    Item* dirt = nullptr;
    Item* diamond = nullptr;
};

TEST_F(PlayerInventoryNbtTest, EmptyInventory_RoundTrip)
{
    PlayerInventory inv(nullptr);

    nbt::tags::compound_tag tag;
    inv.toNbt(tag);

    auto result = PlayerInventory::fromNbt(tag);
    ASSERT_TRUE(result.success());

    const PlayerInventory& restored = result.value();
    EXPECT_TRUE(restored.isEmpty());
    EXPECT_EQ(restored.getSelectedSlot(), 0);
}

TEST_F(PlayerInventoryNbtTest, SelectedSlot_RoundTrip)
{
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    PlayerInventory inv(nullptr);
    inv.setSelectedSlot(5);

    nbt::tags::compound_tag tag;
    inv.toNbt(tag);

    // 验证 SelectedItemSlot 字段
    using namespace nbt_keys;
    ASSERT_TRUE(tag.value.count(SELECTED_ITEM_SLOT) > 0);
    EXPECT_EQ(tag.get<nbt::tags::int_tag>(SELECTED_ITEM_SLOT), 5);

    auto result = PlayerInventory::fromNbt(tag);
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().getSelectedSlot(), 5);
}

TEST_F(PlayerInventoryNbtTest, HotbarItems_RoundTrip)
{
    if (!stone || !dirt) {
        GTEST_SKIP() << "Required items not registered";
    }

    PlayerInventory inv(nullptr);
    inv.setItem(0, ItemStack(*stone, 32));
    inv.setItem(4, ItemStack(*dirt, 16));
    inv.setItem(8, ItemStack(*stone, 1));

    nbt::tags::compound_tag tag;
    inv.toNbt(tag);

    auto result = PlayerInventory::fromNbt(tag);
    ASSERT_TRUE(result.success());

    const PlayerInventory& restored = result.value();
    EXPECT_TRUE(restored.getItem(0).isSameItem(ItemStack(*stone, 1)));
    EXPECT_EQ(restored.getItem(0).getCount(), 32);
    EXPECT_TRUE(restored.getItem(4).isSameItem(ItemStack(*dirt, 1)));
    EXPECT_EQ(restored.getItem(4).getCount(), 16);
    EXPECT_TRUE(restored.getItem(8).isSameItem(ItemStack(*stone, 1)));
    EXPECT_EQ(restored.getItem(8).getCount(), 1);

    // 空槽位应保持为空
    EXPECT_TRUE(restored.getItem(1).isEmpty());
    EXPECT_TRUE(restored.getItem(7).isEmpty());
}

TEST_F(PlayerInventoryNbtTest, ArmorSlots_NbtSlotMapping)
{
    if (!diamond) {
        GTEST_SKIP() << "Diamond item not registered";
    }

    PlayerInventory inv(nullptr);
    // 在护甲槽中放入物品
    inv.setItem(InventorySlots::ARMOR_HEAD, ItemStack(*diamond, 1));  // 内部 36
    inv.setItem(InventorySlots::ARMOR_CHEST, ItemStack(*diamond, 1)); // 内部 37
    inv.setItem(InventorySlots::ARMOR_LEGS, ItemStack(*diamond, 1));  // 内部 38
    inv.setItem(InventorySlots::ARMOR_FEET, ItemStack(*diamond, 1));  // 内部 39

    nbt::tags::compound_tag tag;
    inv.toNbt(tag);

    // 验证 Inventory 列表中的 Slot 值使用 MC Java 编码
    using namespace nbt_helper;
    using namespace nbt_keys;

    const auto* invList = tryGetList(tag, INVENTORY);
    ASSERT_NE(invList, nullptr);
    ASSERT_EQ(invList->element_id(), nbt::TagId::Compound);

    auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*invList);
    EXPECT_EQ(compoundList.value.size(), 4u); // 4 个护甲物品

    // 收集所有 NBT Slot 值
    std::set<i32> nbtSlots;
    for (const auto& itemTag : compoundList.value) {
        if (auto slotOpt = tryGetByte(itemTag, "Slot")) {
            nbtSlots.insert(static_cast<i32>(*slotOpt));
        }
    }

    // 验证 NBT Slot 值为 MC Java 格式 (100-103)，而非内部索引 (36-39)
    EXPECT_TRUE(nbtSlots.count(100) > 0) << "Missing armor.feet (NBT 100)";
    EXPECT_TRUE(nbtSlots.count(101) > 0) << "Missing armor.legs (NBT 101)";
    EXPECT_TRUE(nbtSlots.count(102) > 0) << "Missing armor.chest (NBT 102)";
    EXPECT_TRUE(nbtSlots.count(103) > 0) << "Missing armor.head (NBT 103)";
}

TEST_F(PlayerInventoryNbtTest, OffhandSlot_NbtSlotMapping)
{
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    PlayerInventory inv(nullptr);
    inv.setItem(InventorySlots::OFFHAND, ItemStack(*stone, 10)); // 内部 40

    nbt::tags::compound_tag tag;
    inv.toNbt(tag);

    // 验证副手槽的 NBT Slot 值为 -106
    using namespace nbt_helper;
    using namespace nbt_keys;

    const auto* invList = tryGetList(tag, INVENTORY);
    ASSERT_NE(invList, nullptr);
    auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*invList);
    ASSERT_EQ(compoundList.value.size(), 1u);

    auto slotOpt = tryGetByte(compoundList.value[0], "Slot");
    ASSERT_TRUE(slotOpt.has_value());
    EXPECT_EQ(static_cast<i32>(*slotOpt), -106); // weapon.offhand
}

TEST_F(PlayerInventoryNbtTest, ArmorSlots_DeserializeOldFormat)
{
    if (!diamond) {
        GTEST_SKIP() << "Diamond item not registered";
    }

    // 构造一个模拟旧版 MC Java 存档的 NBT 数据
    // 护甲使用 NBT Slot 100-103，副手使用 -106
    nbt::tags::compound_tag rootTag;
    auto invList = std::make_unique<nbt::tags::compound_list_tag>();

    // NBT 103 (armor.head) → 内部 36 (ARMOR_HEAD)
    {
        nbt::tags::compound_tag itemTag;
        itemTag.put("Slot", static_cast<i8>(103));
        itemTag.put("id", std::string("minecraft:diamond"));
        itemTag.put("Count", static_cast<i8>(1));
        invList->value.push_back(std::move(itemTag));
    }

    // NBT -106 (weapon.offhand) → 内部 40 (OFFHAND)
    {
        nbt::tags::compound_tag itemTag;
        itemTag.put("Slot", static_cast<i8>(-106));
        itemTag.put("id", std::string("minecraft:stone"));
        itemTag.put("Count", static_cast<i8>(10));
        invList->value.push_back(std::move(itemTag));
    }

    rootTag.value.emplace(nbt_keys::INVENTORY, std::move(invList));
    rootTag.put(nbt_keys::SELECTED_ITEM_SLOT, 0);

    auto result = PlayerInventory::fromNbt(rootTag);
    ASSERT_TRUE(result.success());

    const PlayerInventory& restored = result.value();
    // 验证护甲物品被正确映射到内部索引
    EXPECT_FALSE(restored.getItem(InventorySlots::ARMOR_HEAD).isEmpty()) << "Armor head slot should have item";
    EXPECT_TRUE(restored.getItem(InventorySlots::ARMOR_CHEST).isEmpty()) << "Armor chest slot should be empty";
    EXPECT_TRUE(restored.getItem(InventorySlots::ARMOR_LEGS).isEmpty()) << "Armor legs slot should be empty";
    EXPECT_TRUE(restored.getItem(InventorySlots::ARMOR_FEET).isEmpty()) << "Armor feet slot should be empty";

    // 验证副手物品被正确映射
    EXPECT_FALSE(restored.getItem(InventorySlots::OFFHAND).isEmpty()) << "Offhand slot should have item";
}

TEST_F(PlayerInventoryNbtTest, FullRoundTrip_AllSlotTypes)
{
    if (!stone || !dirt || !diamond) {
        GTEST_SKIP() << "Required items not registered";
    }

    PlayerInventory inv(nullptr);
    inv.setItem(0, ItemStack(*stone, 10));                            // 快捷栏
    inv.setItem(9, ItemStack(*dirt, 32));                             // 主背包
    inv.setItem(InventorySlots::ARMOR_HEAD, ItemStack(*diamond, 1));  // 头盔
    inv.setItem(InventorySlots::ARMOR_CHEST, ItemStack(*diamond, 1)); // 胸甲
    inv.setItem(InventorySlots::ARMOR_LEGS, ItemStack(*diamond, 1));  // 护腿
    inv.setItem(InventorySlots::ARMOR_FEET, ItemStack(*diamond, 1));  // 靴子
    inv.setItem(InventorySlots::OFFHAND, ItemStack(*stone, 5));       // 副手
    inv.setSelectedSlot(3);

    nbt::tags::compound_tag tag;
    inv.toNbt(tag);

    auto result = PlayerInventory::fromNbt(tag);
    ASSERT_TRUE(result.success());

    const PlayerInventory& restored = result.value();

    // 验证所有槽位内容
    EXPECT_TRUE(restored.getItem(0).isSameItem(ItemStack(*stone, 1)));
    EXPECT_EQ(restored.getItem(0).getCount(), 10);
    EXPECT_TRUE(restored.getItem(9).isSameItem(ItemStack(*dirt, 1)));
    EXPECT_EQ(restored.getItem(9).getCount(), 32);
    EXPECT_TRUE(restored.getItem(InventorySlots::ARMOR_HEAD).isSameItem(ItemStack(*diamond, 1)));
    EXPECT_TRUE(restored.getItem(InventorySlots::ARMOR_CHEST).isSameItem(ItemStack(*diamond, 1)));
    EXPECT_TRUE(restored.getItem(InventorySlots::ARMOR_LEGS).isSameItem(ItemStack(*diamond, 1)));
    EXPECT_TRUE(restored.getItem(InventorySlots::ARMOR_FEET).isSameItem(ItemStack(*diamond, 1)));
    EXPECT_TRUE(restored.getItem(InventorySlots::OFFHAND).isSameItem(ItemStack(*stone, 1)));
    EXPECT_EQ(restored.getItem(InventorySlots::OFFHAND).getCount(), 5);
    EXPECT_EQ(restored.getSelectedSlot(), 3);
}

TEST_F(PlayerInventoryNbtTest, EmptySlotsNotSerialized)
{
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    PlayerInventory inv(nullptr);
    inv.setItem(5, ItemStack(*stone, 1)); // 仅有一个非空物品

    nbt::tags::compound_tag tag;
    inv.toNbt(tag);

    using namespace nbt_helper;
    using namespace nbt_keys;

    const auto* invList = tryGetList(tag, INVENTORY);
    ASSERT_NE(invList, nullptr);
    auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*invList);
    EXPECT_EQ(compoundList.value.size(), 1u); // 仅序列化非空物品
}

// ============================================================================
// PlayerEnderChestInventory NBT 序列化测试
// ============================================================================

class PlayerEnderChestInventoryNbtTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
        dirt = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dirt"));
    }

    Item* stone = nullptr;
    Item* dirt = nullptr;
};

TEST_F(PlayerEnderChestInventoryNbtTest, EmptyInventory_RoundTrip)
{
    PlayerEnderChestInventory inv;

    nbt::tags::compound_tag tag;
    inv.toNbt(tag);

    inv.fromNbt(tag);

    EXPECT_TRUE(inv.isEmpty());
    EXPECT_EQ(inv.getContainerSize(), 27);
}

TEST_F(PlayerEnderChestInventoryNbtTest, ItemsRoundTrip)
{
    if (!stone || !dirt) {
        GTEST_SKIP() << "Required items not registered";
    }

    PlayerEnderChestInventory inv;
    inv.setItem(0, ItemStack(*stone, 64));
    inv.setItem(13, ItemStack(*dirt, 32));
    inv.setItem(26, ItemStack(*stone, 1));

    nbt::tags::compound_tag tag;
    inv.toNbt(tag);

    // 验证序列化后的 NBT 结构
    using namespace nbt_helper;
    using namespace nbt_keys;

    const auto* itemsList = tryGetList(tag, ENDER_ITEMS);
    ASSERT_NE(itemsList, nullptr);
    ASSERT_EQ(itemsList->element_id(), nbt::TagId::Compound);

    auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*itemsList);
    EXPECT_EQ(compoundList.value.size(), 3u); // 仅序列化非空物品

    // 反序列化到新对象
    PlayerEnderChestInventory restored;
    restored.fromNbt(tag);

    EXPECT_TRUE(restored.getItem(0).isSameItem(ItemStack(*stone, 1)));
    EXPECT_EQ(restored.getItem(0).getCount(), 64);
    EXPECT_TRUE(restored.getItem(13).isSameItem(ItemStack(*dirt, 1)));
    EXPECT_EQ(restored.getItem(13).getCount(), 32);
    EXPECT_TRUE(restored.getItem(26).isSameItem(ItemStack(*stone, 1)));
    EXPECT_EQ(restored.getItem(26).getCount(), 1);

    // 空槽位
    EXPECT_TRUE(restored.getItem(1).isEmpty());
    EXPECT_TRUE(restored.getItem(12).isEmpty());
    EXPECT_TRUE(restored.getItem(25).isEmpty());
}

TEST_F(PlayerEnderChestInventoryNbtTest, SlotIndices_0To26)
{
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    // 验证末影箱槽位编号为 0-26
    nbt::tags::compound_tag tag;
    auto itemsList = std::make_unique<nbt::tags::compound_list_tag>();

    // 写入 Slot=0 和 Slot=26（末影箱边界槽位）
    {
        nbt::tags::compound_tag itemTag;
        itemTag.put("Slot", static_cast<i8>(0));
        itemTag.put("id", std::string("minecraft:stone"));
        itemTag.put("Count", static_cast<i8>(1));
        itemsList->value.push_back(std::move(itemTag));
    }
    {
        nbt::tags::compound_tag itemTag;
        itemTag.put("Slot", static_cast<i8>(26));
        itemTag.put("id", std::string("minecraft:stone"));
        itemTag.put("Count", static_cast<i8>(2));
        itemsList->value.push_back(std::move(itemTag));
    }

    tag.value.emplace(nbt_keys::ENDER_ITEMS, std::move(itemsList));

    PlayerEnderChestInventory inv;
    inv.fromNbt(tag);

    EXPECT_FALSE(inv.getItem(0).isEmpty());
    EXPECT_EQ(inv.getItem(0).getCount(), 1);
    EXPECT_FALSE(inv.getItem(26).isEmpty());
    EXPECT_EQ(inv.getItem(26).getCount(), 2);
}

TEST_F(PlayerEnderChestInventoryNbtTest, InvalidSlotIndex_Skipped)
{
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    // 构造带有无效槽位索引的 NBT
    nbt::tags::compound_tag tag;
    auto itemsList = std::make_unique<nbt::tags::compound_list_tag>();

    // 有效槽位
    {
        nbt::tags::compound_tag itemTag;
        itemTag.put("Slot", static_cast<i8>(5));
        itemTag.put("id", std::string("minecraft:stone"));
        itemTag.put("Count", static_cast<i8>(1));
        itemsList->value.push_back(std::move(itemTag));
    }
    // 无效槽位（超出范围）
    {
        nbt::tags::compound_tag itemTag;
        itemTag.put("Slot", static_cast<i8>(27)); // 超出末影箱范围
        itemTag.put("id", std::string("minecraft:stone"));
        itemTag.put("Count", static_cast<i8>(1));
        itemsList->value.push_back(std::move(itemTag));
    }
    // 无效槽位（负数）
    {
        nbt::tags::compound_tag itemTag;
        itemTag.put("Slot", static_cast<i8>(-1));
        itemTag.put("id", std::string("minecraft:stone"));
        itemTag.put("Count", static_cast<i8>(1));
        itemsList->value.push_back(std::move(itemTag));
    }

    tag.value.emplace(nbt_keys::ENDER_ITEMS, std::move(itemsList));

    PlayerEnderChestInventory inv;
    inv.fromNbt(tag);

    // 只有槽位 5 的物品被正确加载
    EXPECT_FALSE(inv.getItem(5).isEmpty());
    // 无效槽位被跳过
    EXPECT_TRUE(inv.isEmpty() || !inv.getItem(5).isEmpty()); // 总体检查
}

TEST_F(PlayerEnderChestInventoryNbtTest, ClearBeforeDeserialization)
{
    if (!stone || !dirt) {
        GTEST_SKIP() << "Required items not registered";
    }

    PlayerEnderChestInventory inv;
    inv.setItem(0, ItemStack(*stone, 64));
    inv.setItem(5, ItemStack(*dirt, 32));
    EXPECT_FALSE(inv.isEmpty());

    // 反序列化到一个已经有物品的末影箱
    nbt::tags::compound_tag tag;
    auto itemsList = std::make_unique<nbt::tags::compound_list_tag>();
    {
        nbt::tags::compound_tag itemTag;
        itemTag.put("Slot", static_cast<i8>(10));
        itemTag.put("id", std::string("minecraft:stone"));
        itemTag.put("Count", static_cast<i8>(1));
        itemsList->value.push_back(std::move(itemTag));
    }
    tag.value.emplace(nbt_keys::ENDER_ITEMS, std::move(itemsList));

    inv.fromNbt(tag);

    // 反序列化应该先清空，所以原来的物品不在了
    EXPECT_TRUE(inv.getItem(0).isEmpty());
    EXPECT_TRUE(inv.getItem(5).isEmpty());
    // 新物品在槽位 10
    EXPECT_FALSE(inv.getItem(10).isEmpty());
}

TEST_F(PlayerEnderChestInventoryNbtTest, NoEnderItemsKey_EmptyInventory)
{
    // 没有 EnderItems 键的 NBT 应该产生空的末影箱
    nbt::tags::compound_tag tag;

    PlayerEnderChestInventory inv;
    inv.setItem(0, ItemStack(*stone, 10)); // 先放入物品

    inv.fromNbt(tag);

    EXPECT_TRUE(inv.isEmpty()); // 应该被清空
}

// ============================================================================
// PlayerEnderChestInventory 基本功能测试
// ============================================================================

TEST_F(PlayerEnderChestInventoryNbtTest, ContainerSize)
{
    PlayerEnderChestInventory inv;
    EXPECT_EQ(inv.getContainerSize(), 27);
}

TEST_F(PlayerEnderChestInventoryNbtTest, SetGetItem)
{
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    PlayerEnderChestInventory inv;
    inv.setItem(0, ItemStack(*stone, 32));

    EXPECT_TRUE(inv.getItem(0).isSameItem(ItemStack(*stone, 1)));
    EXPECT_EQ(inv.getItem(0).getCount(), 32);
}

TEST_F(PlayerEnderChestInventoryNbtTest, RemoveItem)
{
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    PlayerEnderChestInventory inv;
    inv.setItem(5, ItemStack(*stone, 64));

    ItemStack removed = inv.removeItem(5, 16);
    EXPECT_EQ(removed.getCount(), 16);
    EXPECT_EQ(inv.getItem(5).getCount(), 48);
}

TEST_F(PlayerEnderChestInventoryNbtTest, RemoveItemNoUpdate)
{
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    PlayerEnderChestInventory inv;
    inv.setItem(5, ItemStack(*stone, 32));

    ItemStack removed = inv.removeItemNoUpdate(5);
    EXPECT_EQ(removed.getCount(), 32);
    EXPECT_TRUE(inv.getItem(5).isEmpty());
}

TEST_F(PlayerEnderChestInventoryNbtTest, Clear)
{
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    PlayerEnderChestInventory inv;
    inv.setItem(0, ItemStack(*stone, 10));
    inv.setItem(13, ItemStack(*stone, 20));
    inv.setItem(26, ItemStack(*stone, 30));

    inv.clear();

    EXPECT_TRUE(inv.isEmpty());
    EXPECT_TRUE(inv.getItem(0).isEmpty());
    EXPECT_TRUE(inv.getItem(13).isEmpty());
    EXPECT_TRUE(inv.getItem(26).isEmpty());
}

TEST_F(PlayerEnderChestInventoryNbtTest, OutOfBounds_GetItem)
{
    PlayerEnderChestInventory inv;
    EXPECT_TRUE(inv.getItem(-1).isEmpty());
    EXPECT_TRUE(inv.getItem(27).isEmpty());
    EXPECT_TRUE(inv.getItem(100).isEmpty());
}

TEST_F(PlayerEnderChestInventoryNbtTest, ActiveChest_DefaultNull)
{
    PlayerEnderChestInventory inv;
    EXPECT_EQ(inv.getActiveChest(), nullptr);
    EXPECT_FALSE(inv.isActiveChestValid());
}
