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

#include "../src/common/entity/core/LivingEntity.hpp"
#include "../src/common/entity/inventory/PlayerInventory.hpp"
#include "../src/common/entity/serialization/EntityNbtKeys.hpp"
#include "../src/common/entity/serialization/EquipmentSlotNames.hpp"
#include "../src/common/entity/serialization/NbtHelper.hpp"
#include "../src/common/item/Items.hpp"
#include "../src/common/item/core/ItemRegistry.hpp"
#include "../src/common/util/nbt/Nbt.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity::serialization;

namespace {

/// 初始化物品注册表（测试前必须调用）
void ensureItemRegistryInitialized()
{
    static bool initialized = false;
    if (!initialized) {
        Items::initialize();
        initialized = true;
    }
}

/// 创建一个非空 ItemStack（使用石头物品）
ItemStack makeStoneStack(i32 count = 1)
{
    const Item* stone = Item::getItem(ResourceLocation("minecraft:stone"));
    if (stone == nullptr) {
        return ItemStack::EMPTY;
    }
    return ItemStack(*stone, count);
}

/// 创建一个不同类型的 ItemStack（使用泥土物品）
ItemStack makeDirtStack(i32 count = 1)
{
    const Item* dirt = Item::getItem(ResourceLocation("minecraft:dirt"));
    if (dirt == nullptr) {
        return ItemStack::EMPTY;
    }
    return ItemStack(*dirt, count);
}

} // anonymous namespace

// ============================================================================
// EquipmentSlotNames 单元测试
// ============================================================================

TEST(EquipmentSlotNamesTest, ToNameAllSlots)
{
    EXPECT_STREQ(EquipmentSlotNames::toName(EquipmentSlot::MainHand), "mainhand");
    EXPECT_STREQ(EquipmentSlotNames::toName(EquipmentSlot::OffHand), "offhand");
    EXPECT_STREQ(EquipmentSlotNames::toName(EquipmentSlot::Feet), "feet");
    EXPECT_STREQ(EquipmentSlotNames::toName(EquipmentSlot::Legs), "legs");
    EXPECT_STREQ(EquipmentSlotNames::toName(EquipmentSlot::Chest), "chest");
    EXPECT_STREQ(EquipmentSlotNames::toName(EquipmentSlot::Head), "head");
}

TEST(EquipmentSlotNamesTest, FromNameAllSlots)
{
    EXPECT_EQ(EquipmentSlotNames::fromName("mainhand"), EquipmentSlot::MainHand);
    EXPECT_EQ(EquipmentSlotNames::fromName("offhand"), EquipmentSlot::OffHand);
    EXPECT_EQ(EquipmentSlotNames::fromName("feet"), EquipmentSlot::Feet);
    EXPECT_EQ(EquipmentSlotNames::fromName("legs"), EquipmentSlot::Legs);
    EXPECT_EQ(EquipmentSlotNames::fromName("chest"), EquipmentSlot::Chest);
    EXPECT_EQ(EquipmentSlotNames::fromName("head"), EquipmentSlot::Head);
}

TEST(EquipmentSlotNamesTest, FromNameInvalid)
{
    EXPECT_EQ(EquipmentSlotNames::fromName("invalid"), std::nullopt);
    EXPECT_EQ(EquipmentSlotNames::fromName(""), std::nullopt);
    EXPECT_EQ(EquipmentSlotNames::fromName("MAINHAND"), std::nullopt);
    EXPECT_EQ(EquipmentSlotNames::fromName("Head"), std::nullopt);
}

TEST(EquipmentSlotNamesTest, RoundTrip)
{
    for (u8 i = 0; i < static_cast<u8>(EquipmentSlot::Count); ++i) {
        auto slot = static_cast<EquipmentSlot>(i);
        const char* name = EquipmentSlotNames::toName(slot);
        auto result = EquipmentSlotNames::fromName(name);
        ASSERT_TRUE(result.has_value()) << "fromName failed for slot " << i;
        EXPECT_EQ(result.value(), slot) << "Round-trip failed for slot " << i;
    }
}

// ============================================================================
// PlayerInventory::toNbt 单元测试 - MC 1.21.11 新格式
// ============================================================================

class PlayerInventoryNbtTest : public ::testing::Test {
protected:
    void SetUp() override { ensureItemRegistryInitialized(); }
};

TEST_F(PlayerInventoryNbtTest, ToNbtEmptyInventory)
{
    PlayerInventory inventory;

    nbt::tags::compound_tag tag;
    inventory.toNbt(tag);

    // Inventory 列表应存在但为空
    auto* invList = nbt_helper::tryGetList(tag, nbt_keys::INVENTORY);
    ASSERT_NE(invList, nullptr);
    EXPECT_EQ(invList->size(), 0u);

    // equipment 标签不应存在（空背包没有装备）
    auto* equipment = nbt_helper::tryGetCompound(tag, nbt_keys::EQUIPMENT);
    EXPECT_EQ(equipment, nullptr);

    // SelectedItemSlot 应为 0
    auto selectedSlot = nbt_helper::tryGetInt(tag, nbt_keys::SELECTED_ITEM_SLOT);
    ASSERT_TRUE(selectedSlot.has_value());
    EXPECT_EQ(*selectedSlot, 0);
}

TEST_F(PlayerInventoryNbtTest, ToNbtHotbarOnlyNoEquipment)
{
    PlayerInventory inventory;
    ItemStack stone = makeStoneStack(32);
    inventory.setItem(0, stone);
    inventory.setItem(5, makeDirtStack(16));

    nbt::tags::compound_tag tag;
    inventory.toNbt(tag);

    // Inventory 列表应仅包含 2 个物品
    auto* invList = nbt_helper::tryGetList(tag, nbt_keys::INVENTORY);
    ASSERT_NE(invList, nullptr);
    ASSERT_EQ(invList->size(), 2u);

    // equipment 标签不应存在
    auto* equipment = nbt_helper::tryGetCompound(tag, nbt_keys::EQUIPMENT);
    EXPECT_EQ(equipment, nullptr);
}

TEST_F(PlayerInventoryNbtTest, ToNbtEquipmentNotInInventoryOutput)
{
    // PlayerInventory::toNbt 不写入 equipment 标签
    // equipment 标签由 LivingEntity::addAdditionalSaveData() 负责写入
    // 此处验证即使设置了护甲/副手，toNbt 输出中也不包含 equipment
    PlayerInventory inventory;
    inventory.setHelmet(makeStoneStack(1));
    inventory.setChestplate(makeDirtStack(1));
    inventory.setOffhandItem(makeStoneStack(10));

    nbt::tags::compound_tag tag;
    inventory.toNbt(tag);

    // equipment 标签不应存在（由 LivingEntity 负责写入）
    auto* equipment = nbt_helper::tryGetCompound(tag, nbt_keys::EQUIPMENT);
    EXPECT_EQ(equipment, nullptr);
}

TEST_F(PlayerInventoryNbtTest, ToNbtInventoryOnlyHotbarAndMain)
{
    PlayerInventory inventory;

    // 设置快捷栏物品
    inventory.setItem(0, makeStoneStack(10));
    // 设置主背包物品
    inventory.setItem(20, makeDirtStack(5));
    // 设置护甲（不应出现在 Inventory 列表中）
    inventory.setHelmet(makeStoneStack(1));

    nbt::tags::compound_tag tag;
    inventory.toNbt(tag);

    // Inventory 列表应仅包含 Slot 0-35 的物品（不含护甲和副手）
    auto* invList = nbt_helper::tryGetList(tag, nbt_keys::INVENTORY);
    ASSERT_NE(invList, nullptr);
    ASSERT_EQ(invList->size(), 2u); // 只有 Slot 0 和 Slot 20

    // 验证 Slot 值在 0-35 范围内
    if (invList->element_id() == nbt::TagId::Compound) {
        auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*invList);
        for (const auto& itemTag : compoundList.value) {
            auto slotOpt = nbt_helper::tryGetByte(itemTag, "Slot");
            ASSERT_TRUE(slotOpt.has_value());
            i8 slot = *slotOpt;
            EXPECT_GE(slot, 0);
            EXPECT_LE(slot, 35) << "Inventory list should only contain slots 0-35 in new format";
        }
    }
}

TEST_F(PlayerInventoryNbtTest, ToNbtSelectedSlotPreserved)
{
    PlayerInventory inventory;
    inventory.setSelectedSlot(7);

    nbt::tags::compound_tag tag;
    inventory.toNbt(tag);

    auto selectedSlot = nbt_helper::tryGetInt(tag, nbt_keys::SELECTED_ITEM_SLOT);
    ASSERT_TRUE(selectedSlot.has_value());
    EXPECT_EQ(*selectedSlot, 7);
}

// ============================================================================
// PlayerInventory::fromNbt 单元测试 - 新格式读取
// ============================================================================

TEST_F(PlayerInventoryNbtTest, FromNbtNewFormat)
{
    // 构建新格式 NBT
    nbt::tags::compound_tag tag;

    // Inventory 列表（仅快捷栏和主背包）
    auto invList = std::make_unique<nbt::tags::compound_list_tag>();
    {
        nbt::tags::compound_tag itemTag;
        itemTag.put("Slot", static_cast<i8>(0));
        makeStoneStack(10).toNbt(itemTag);
        invList->value.push_back(std::move(itemTag));
    }
    tag.value.emplace(nbt_keys::INVENTORY, std::move(invList));

    // equipment 复合标签
    auto equipment = std::make_unique<nbt::tags::compound_tag>();
    {
        nbt::tags::compound_tag headTag;
        makeDirtStack(1).toNbt(headTag);
        equipment->value.emplace("head", std::make_unique<nbt::tags::compound_tag>(std::move(headTag)));
    }
    tag.value.emplace(nbt_keys::EQUIPMENT, std::move(equipment));

    // SelectedItemSlot
    tag.put(nbt_keys::SELECTED_ITEM_SLOT, static_cast<i32>(3));

    auto result = PlayerInventory::fromNbt(tag);
    ASSERT_TRUE(result.success());

    PlayerInventory& inventory = result.value();
    EXPECT_EQ(inventory.getSelectedSlot(), 3);

    // 验证快捷栏物品
    EXPECT_FALSE(inventory.getItem(0).isEmpty());

    // 验证护甲
    EXPECT_FALSE(inventory.getHelmet().isEmpty());
    EXPECT_TRUE(inventory.getChestplate().isEmpty());
    EXPECT_TRUE(inventory.getLeggings().isEmpty());
    EXPECT_TRUE(inventory.getBoots().isEmpty());

    // 验证副手为空
    EXPECT_TRUE(inventory.getOffhandItem().isEmpty());
}

TEST_F(PlayerInventoryNbtTest, FromNbtOldFormat)
{
    // 构建旧格式 NBT（没有 equipment 字段，护甲用 Slot 100-103，副手用 -106）
    nbt::tags::compound_tag tag;

    auto invList = std::make_unique<nbt::tags::compound_list_tag>();
    {
        // 快捷栏物品 Slot 0
        nbt::tags::compound_tag itemTag;
        itemTag.put("Slot", static_cast<i8>(0));
        makeStoneStack(5).toNbt(itemTag);
        invList->value.push_back(std::move(itemTag));
    }
    {
        // 头盔 Slot 103（旧格式）
        nbt::tags::compound_tag itemTag;
        itemTag.put("Slot", static_cast<i8>(103));
        makeDirtStack(1).toNbt(itemTag);
        invList->value.push_back(std::move(itemTag));
    }
    {
        // 副手 Slot -106（旧格式）
        nbt::tags::compound_tag itemTag;
        itemTag.put("Slot", static_cast<i8>(-106));
        makeStoneStack(3).toNbt(itemTag);
        invList->value.push_back(std::move(itemTag));
    }
    tag.value.emplace(nbt_keys::INVENTORY, std::move(invList));

    // 不设置 equipment 字段，模拟旧版存档

    auto result = PlayerInventory::fromNbt(tag);
    ASSERT_TRUE(result.success());

    PlayerInventory& inventory = result.value();

    // 验证快捷栏物品
    EXPECT_FALSE(inventory.getItem(0).isEmpty());

    // 验证旧格式护甲正确读取
    EXPECT_FALSE(inventory.getHelmet().isEmpty());
    EXPECT_EQ(inventory.getHelmet().getCount(), 1);

    // 验证旧格式副手正确读取
    EXPECT_FALSE(inventory.getOffhandItem().isEmpty());
    EXPECT_EQ(inventory.getOffhandItem().getCount(), 3);
}

TEST_F(PlayerInventoryNbtTest, FromNbtEmptyTag)
{
    nbt::tags::compound_tag tag;

    auto result = PlayerInventory::fromNbt(tag);
    ASSERT_TRUE(result.success());

    PlayerInventory& inventory = result.value();
    EXPECT_EQ(inventory.getSelectedSlot(), 0);
    EXPECT_TRUE(inventory.isEmpty());
}

// ============================================================================
// PlayerInventory::toNbt + fromNbt 往返测试
// ============================================================================

TEST_F(PlayerInventoryNbtTest, RoundTripHotbarAndMainOnly)
{
    // PlayerInventory::toNbt 只序列化快捷栏和主背包（Slot 0-35），
    // 护甲和副手由 LivingEntity::addAdditionalSaveData() 通过 equipment 标签序列化。
    // 因此通过 PlayerInventory 的往返测试仅验证 Slot 0-35 的物品。
    PlayerInventory original;
    original.setItem(0, makeStoneStack(32));
    original.setItem(15, makeDirtStack(8));
    original.setItem(35, makeStoneStack(1));
    original.setSelectedSlot(4);

    // 序列化
    nbt::tags::compound_tag tag;
    original.toNbt(tag);

    // 反序列化
    auto result = PlayerInventory::fromNbt(tag);
    ASSERT_TRUE(result.success());

    PlayerInventory& restored = result.value();

    // 验证快捷栏/主背包物品
    EXPECT_FALSE(restored.getItem(0).isEmpty());
    EXPECT_EQ(restored.getItem(0).getCount(), 32);
    EXPECT_FALSE(restored.getItem(15).isEmpty());
    EXPECT_EQ(restored.getItem(15).getCount(), 8);
    EXPECT_FALSE(restored.getItem(35).isEmpty());
    EXPECT_EQ(restored.getItem(35).getCount(), 1);

    // 验证选中槽位
    EXPECT_EQ(restored.getSelectedSlot(), 4);
}

TEST_F(PlayerInventoryNbtTest, RoundTripWithEquipmentViaFromNbt)
{
    // 完整的装备往返测试：手动构建包含 equipment 标签的 NBT，
    // 通过 PlayerInventory::fromNbt 读取，再通过 toNbt 写出，
    // 验证 fromNbt 能正确读取 equipment 标签中的护甲和副手。
    // 注意: toNbt 不回写 equipment 标签（由 LivingEntity 负责），
    // 因此只能验证 fromNbt 的读取正确性。

    nbt::tags::compound_tag tag;

    // Inventory 列表（仅快捷栏和主背包）
    auto invList = std::make_unique<nbt::tags::compound_list_tag>();
    {
        nbt::tags::compound_tag itemTag;
        itemTag.put("Slot", static_cast<i8>(0));
        makeStoneStack(32).toNbt(itemTag);
        invList->value.push_back(std::move(itemTag));
    }
    {
        nbt::tags::compound_tag itemTag;
        itemTag.put("Slot", static_cast<i8>(15));
        makeDirtStack(8).toNbt(itemTag);
        invList->value.push_back(std::move(itemTag));
    }
    tag.value.emplace(nbt_keys::INVENTORY, std::move(invList));

    // equipment 复合标签（新格式）
    auto equipment = std::make_unique<nbt::tags::compound_tag>();
    {
        nbt::tags::compound_tag headTag;
        makeStoneStack(1).toNbt(headTag);
        equipment->value.emplace("head", std::make_unique<nbt::tags::compound_tag>(std::move(headTag)));
    }
    {
        nbt::tags::compound_tag chestTag;
        makeDirtStack(1).toNbt(chestTag);
        equipment->value.emplace("chest", std::make_unique<nbt::tags::compound_tag>(std::move(chestTag)));
    }
    {
        nbt::tags::compound_tag legsTag;
        makeStoneStack(1).toNbt(legsTag);
        equipment->value.emplace("legs", std::make_unique<nbt::tags::compound_tag>(std::move(legsTag)));
    }
    {
        nbt::tags::compound_tag feetTag;
        makeDirtStack(1).toNbt(feetTag);
        equipment->value.emplace("feet", std::make_unique<nbt::tags::compound_tag>(std::move(feetTag)));
    }
    {
        nbt::tags::compound_tag offhandTag;
        makeStoneStack(5).toNbt(offhandTag);
        equipment->value.emplace("offhand", std::make_unique<nbt::tags::compound_tag>(std::move(offhandTag)));
    }
    tag.value.emplace(nbt_keys::EQUIPMENT, std::move(equipment));

    // SelectedItemSlot
    tag.put(nbt_keys::SELECTED_ITEM_SLOT, static_cast<i32>(4));

    // 反序列化
    auto result = PlayerInventory::fromNbt(tag);
    ASSERT_TRUE(result.success());

    PlayerInventory& restored = result.value();

    // 验证快捷栏/主背包物品
    EXPECT_FALSE(restored.getItem(0).isEmpty());
    EXPECT_EQ(restored.getItem(0).getCount(), 32);
    EXPECT_FALSE(restored.getItem(15).isEmpty());
    EXPECT_EQ(restored.getItem(15).getCount(), 8);

    // 验证护甲
    EXPECT_FALSE(restored.getHelmet().isEmpty());
    EXPECT_EQ(restored.getHelmet().getCount(), 1);
    EXPECT_FALSE(restored.getChestplate().isEmpty());
    EXPECT_EQ(restored.getChestplate().getCount(), 1);
    EXPECT_FALSE(restored.getLeggings().isEmpty());
    EXPECT_EQ(restored.getLeggings().getCount(), 1);
    EXPECT_FALSE(restored.getBoots().isEmpty());
    EXPECT_EQ(restored.getBoots().getCount(), 1);

    // 验证副手
    EXPECT_FALSE(restored.getOffhandItem().isEmpty());
    EXPECT_EQ(restored.getOffhandItem().getCount(), 5);

    // 验证选中槽位
    EXPECT_EQ(restored.getSelectedSlot(), 4);
}

TEST_F(PlayerInventoryNbtTest, RoundTripWithoutEquipment)
{
    // 不包含装备时的 PlayerInventory 往返测试
    PlayerInventory original;
    original.setItem(0, makeStoneStack(10));
    original.setItem(35, makeDirtStack(20));
    original.setSelectedSlot(8);

    nbt::tags::compound_tag tag;
    original.toNbt(tag);

    auto result = PlayerInventory::fromNbt(tag);
    ASSERT_TRUE(result.success());

    PlayerInventory& restored = result.value();

    EXPECT_FALSE(restored.getItem(0).isEmpty());
    EXPECT_EQ(restored.getItem(0).getCount(), 10);
    EXPECT_FALSE(restored.getItem(35).isEmpty());
    EXPECT_EQ(restored.getItem(35).getCount(), 20);

    // 护甲和副手应为空
    EXPECT_TRUE(restored.getHelmet().isEmpty());
    EXPECT_TRUE(restored.getChestplate().isEmpty());
    EXPECT_TRUE(restored.getLeggings().isEmpty());
    EXPECT_TRUE(restored.getBoots().isEmpty());
    EXPECT_TRUE(restored.getOffhandItem().isEmpty());

    EXPECT_EQ(restored.getSelectedSlot(), 8);
}

TEST_F(PlayerInventoryNbtTest, RoundTripEmptyInventory)
{
    PlayerInventory original;

    nbt::tags::compound_tag tag;
    original.toNbt(tag);

    auto result = PlayerInventory::fromNbt(tag);
    ASSERT_TRUE(result.success());

    PlayerInventory& restored = result.value();
    EXPECT_TRUE(restored.isEmpty());
    EXPECT_EQ(restored.getSelectedSlot(), 0);
}
