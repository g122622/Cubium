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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/**
 * @file ItemSlotArgumentTest.cpp
 * @brief ItemSlotArgument 和 ItemSlot 单元测试
 *
 * 测试物品槽位参数解析，包括：
 * - 命名槽位解析（weapon.mainhand, armor.head, hotbar.0 等）
 * - 纯数字槽位解析
 * - 无效槽位名称错误处理
 * - ItemSlot 类型的查询方法（isEquipmentSlot, isPlayerInventorySlot 等）
 * - ItemSlot 到 PlayerInventory 索引的映射
 * - ItemSlot 到 EquipmentSlot 索引的映射
 */

#include <gtest/gtest.h>

#include "common/command/StringReader.hpp"
#include "common/command/arguments/ItemSlotArgument.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"

using mc::command::CommandErrorType;
using mc::command::CommandException;
using mc::command::ItemSlot;
using mc::command::ItemSlotArgumentType;
using mc::command::StringReader;

// ========== ItemSlot 类测试 ==========

class ItemSlotTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ItemSlotTest, DefaultSlotIsInvalid)
{
    ItemSlot slot;
    EXPECT_FALSE(slot.isValid());
    EXPECT_EQ(slot.slotIndex(), -1);
}

TEST_F(ItemSlotTest, ExplicitIndexConstruction)
{
    ItemSlot slot(5);
    EXPECT_TRUE(slot.isValid());
    EXPECT_EQ(slot.slotIndex(), 5);
}

TEST_F(ItemSlotTest, PlayerInventorySlots)
{
    // 快捷栏 0-8
    EXPECT_TRUE(ItemSlot(0).isPlayerInventorySlot());
    EXPECT_TRUE(ItemSlot(8).isPlayerInventorySlot());
    // 主背包 9-35
    EXPECT_TRUE(ItemSlot(9).isPlayerInventorySlot());
    EXPECT_TRUE(ItemSlot(35).isPlayerInventorySlot());
    // 护甲 36-39
    EXPECT_TRUE(ItemSlot(36).isPlayerInventorySlot());
    EXPECT_TRUE(ItemSlot(39).isPlayerInventorySlot());
    // 副手 40
    EXPECT_TRUE(ItemSlot(40).isPlayerInventorySlot());
    // 边界外
    EXPECT_FALSE(ItemSlot(41).isPlayerInventorySlot());
    EXPECT_FALSE(ItemSlot(-1).isPlayerInventorySlot());
}

TEST_F(ItemSlotTest, EquipmentSlots)
{
    // 装备槽位 98-106
    EXPECT_TRUE(ItemSlot(98).isEquipmentSlot());  // weapon.mainhand
    EXPECT_TRUE(ItemSlot(99).isEquipmentSlot());  // weapon.offhand
    EXPECT_TRUE(ItemSlot(100).isEquipmentSlot()); // armor.head
    EXPECT_TRUE(ItemSlot(101).isEquipmentSlot()); // armor.chest
    EXPECT_TRUE(ItemSlot(102).isEquipmentSlot()); // armor.legs
    EXPECT_TRUE(ItemSlot(103).isEquipmentSlot()); // armor.feet
    EXPECT_TRUE(ItemSlot(105).isEquipmentSlot()); // armor.body
    EXPECT_TRUE(ItemSlot(106).isEquipmentSlot()); // saddle
    // 边界外
    EXPECT_FALSE(ItemSlot(97).isEquipmentSlot());
    EXPECT_FALSE(ItemSlot(107).isEquipmentSlot());
}

TEST_F(ItemSlotTest, EnderChestSlots)
{
    EXPECT_TRUE(ItemSlot(200).isEnderChestSlot());
    EXPECT_TRUE(ItemSlot(226).isEnderChestSlot());
    EXPECT_FALSE(ItemSlot(199).isEnderChestSlot());
    EXPECT_FALSE(ItemSlot(227).isEnderChestSlot());
}

TEST_F(ItemSlotTest, HorseSlots)
{
    EXPECT_TRUE(ItemSlot(500).isHorseSlot());
    EXPECT_TRUE(ItemSlot(514).isHorseSlot());
    EXPECT_FALSE(ItemSlot(499).isHorseSlot());
    EXPECT_FALSE(ItemSlot(515).isHorseSlot());
}

TEST_F(ItemSlotTest, CraftingSlots)
{
    EXPECT_TRUE(ItemSlot(500).isCraftingSlot());
    EXPECT_TRUE(ItemSlot(503).isCraftingSlot());
    EXPECT_FALSE(ItemSlot(504).isCraftingSlot());
}

TEST_F(ItemSlotTest, HorseChestSlot)
{
    EXPECT_TRUE(ItemSlot(499).isHorseChestSlot());
    EXPECT_FALSE(ItemSlot(500).isHorseChestSlot());
    EXPECT_FALSE(ItemSlot(498).isHorseChestSlot());
}

TEST_F(ItemSlotTest, CursorSlot)
{
    EXPECT_TRUE(ItemSlot(499).isCursorSlot());
    EXPECT_FALSE(ItemSlot(500).isCursorSlot());
    EXPECT_FALSE(ItemSlot(498).isCursorSlot());
}

TEST_F(ItemSlotTest, VillagerSlots)
{
    EXPECT_TRUE(ItemSlot(300).isVillagerSlot());
    EXPECT_TRUE(ItemSlot(307).isVillagerSlot());
    EXPECT_FALSE(ItemSlot(299).isVillagerSlot());
    EXPECT_FALSE(ItemSlot(308).isVillagerSlot());
}

TEST_F(ItemSlotTest, ToEnderChestSlot)
{
    // 末影箱槽位 200-226 映射到内部索引 0-26
    EXPECT_EQ(ItemSlot(200).toEnderChestSlot(), 0);
    EXPECT_EQ(ItemSlot(213).toEnderChestSlot(), 13);
    EXPECT_EQ(ItemSlot(226).toEnderChestSlot(), 26);
    // 非末影箱槽位返回 -1
    EXPECT_EQ(ItemSlot(199).toEnderChestSlot(), -1);
    EXPECT_EQ(ItemSlot(227).toEnderChestSlot(), -1);
    EXPECT_EQ(ItemSlot(0).toEnderChestSlot(), -1);
    EXPECT_EQ(ItemSlot(98).toEnderChestSlot(), -1);
}

TEST_F(ItemSlotTest, ToHorseSlot)
{
    // 马匹槽位 500-514 映射到内部索引 0-14
    EXPECT_EQ(ItemSlot(500).toHorseSlot(), 0);
    EXPECT_EQ(ItemSlot(507).toHorseSlot(), 7);
    EXPECT_EQ(ItemSlot(514).toHorseSlot(), 14);
    // 非马匹槽位返回 -1
    EXPECT_EQ(ItemSlot(499).toHorseSlot(), -1);
    EXPECT_EQ(ItemSlot(515).toHorseSlot(), -1);
    EXPECT_EQ(ItemSlot(0).toHorseSlot(), -1);
    EXPECT_EQ(ItemSlot(98).toHorseSlot(), -1);
}

TEST_F(ItemSlotTest, ToVillagerSlot)
{
    // 村民槽位 300-307 映射到内部索引 0-7
    EXPECT_EQ(ItemSlot(300).toVillagerSlot(), 0);
    EXPECT_EQ(ItemSlot(304).toVillagerSlot(), 4);
    EXPECT_EQ(ItemSlot(307).toVillagerSlot(), 7);
    // 非村明槽位返回 -1
    EXPECT_EQ(ItemSlot(299).toVillagerSlot(), -1);
    EXPECT_EQ(ItemSlot(308).toVillagerSlot(), -1);
    EXPECT_EQ(ItemSlot(0).toVillagerSlot(), -1);
}

TEST_F(ItemSlotTest, ToPlayerInventorySlot_DirectSlots)
{
    // 直接映射的背包槽位
    EXPECT_EQ(ItemSlot(0).toPlayerInventorySlot(), 0);
    EXPECT_EQ(ItemSlot(8).toPlayerInventorySlot(), 8);
    EXPECT_EQ(ItemSlot(20).toPlayerInventorySlot(), 20);
    EXPECT_EQ(ItemSlot(36).toPlayerInventorySlot(), 36);
    EXPECT_EQ(ItemSlot(40).toPlayerInventorySlot(), 40);
}

TEST_F(ItemSlotTest, ToPlayerInventorySlot_EquipmentMapping)
{
    // weapon.mainhand -> 当前选中快捷栏槽位（默认0）
    EXPECT_EQ(ItemSlot(98).toPlayerInventorySlot(0), 0);
    EXPECT_EQ(ItemSlot(98).toPlayerInventorySlot(5), 5);

    // weapon.offhand -> OFFHAND (40)
    EXPECT_EQ(ItemSlot(99).toPlayerInventorySlot(), 40);

    // armor.head -> ARMOR_HEAD (36)
    EXPECT_EQ(ItemSlot(100).toPlayerInventorySlot(), 36);

    // armor.chest -> ARMOR_CHEST (37)
    EXPECT_EQ(ItemSlot(101).toPlayerInventorySlot(), 37);

    // armor.legs -> ARMOR_LEGS (38)
    EXPECT_EQ(ItemSlot(102).toPlayerInventorySlot(), 38);

    // armor.feet -> ARMOR_FEET (39)
    EXPECT_EQ(ItemSlot(103).toPlayerInventorySlot(), 39);

    // armor.body -> -1 (玩家无 Body 槽位，仅非玩家实体使用)
    EXPECT_EQ(ItemSlot(105).toPlayerInventorySlot(), -1);
}

TEST_F(ItemSlotTest, ToPlayerInventorySlot_InvalidSlots)
{
    // 超出范围的槽位
    EXPECT_EQ(ItemSlot(200).toPlayerInventorySlot(), -1);
    EXPECT_EQ(ItemSlot(300).toPlayerInventorySlot(), -1);
    EXPECT_EQ(ItemSlot(499).toPlayerInventorySlot(), -1);
}

TEST_F(ItemSlotTest, ToEquipmentSlotIndex)
{
    EXPECT_EQ(ItemSlot(98).toEquipmentSlotIndex(), 0);  // MainHand
    EXPECT_EQ(ItemSlot(99).toEquipmentSlotIndex(), 1);  // OffHand
    EXPECT_EQ(ItemSlot(100).toEquipmentSlotIndex(), 5); // Head
    EXPECT_EQ(ItemSlot(101).toEquipmentSlotIndex(), 4); // Chest
    EXPECT_EQ(ItemSlot(102).toEquipmentSlotIndex(), 3); // Legs
    EXPECT_EQ(ItemSlot(103).toEquipmentSlotIndex(), 2); // Feet
    EXPECT_EQ(ItemSlot(105).toEquipmentSlotIndex(), 6); // Body (EquipmentSlot::Body)
    EXPECT_EQ(ItemSlot(106).toEquipmentSlotIndex(), 7); // Saddle (EquipmentSlot::Saddle)
    // 非装备槽位
    EXPECT_EQ(ItemSlot(0).toEquipmentSlotIndex(), -1);
    EXPECT_EQ(ItemSlot(104).toEquipmentSlotIndex(), -1);
}

// ========== ItemSlotArgumentType 解析测试 ==========

class ItemSlotArgumentTypeTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ItemSlotArgumentTypeTest, ParseNumericSlot)
{
    auto parser = ItemSlotArgumentType::itemSlot();
    StringReader reader("5");
    ItemSlot result = parser->parse(reader);
    EXPECT_EQ(result.slotIndex(), 5);
    EXPECT_TRUE(result.isValid());
}

TEST_F(ItemSlotArgumentTypeTest, ParseNumericSlotZero)
{
    auto parser = ItemSlotArgumentType::itemSlot();
    StringReader reader("0");
    ItemSlot result = parser->parse(reader);
    EXPECT_EQ(result.slotIndex(), 0);
    EXPECT_TRUE(result.isValid());
}

TEST_F(ItemSlotArgumentTypeTest, ParseNumericSlotMaxContainer)
{
    auto parser = ItemSlotArgumentType::itemSlot();
    StringReader reader("53");
    ItemSlot result = parser->parse(reader);
    EXPECT_EQ(result.slotIndex(), 53);
}

TEST_F(ItemSlotArgumentTypeTest, ParseNegativeNumericSlotFails)
{
    auto parser = ItemSlotArgumentType::itemSlot();
    StringReader reader("-1");
    EXPECT_THROW(parser->parse(reader), CommandException);
}

TEST_F(ItemSlotArgumentTypeTest, ParseHotbarSlots)
{
    auto parser = ItemSlotArgumentType::itemSlot();

    StringReader r0("hotbar.0");
    EXPECT_EQ(parser->parse(r0).slotIndex(), 0);

    StringReader r4("hotbar.4");
    EXPECT_EQ(parser->parse(r4).slotIndex(), 4);

    StringReader r8("hotbar.8");
    EXPECT_EQ(parser->parse(r8).slotIndex(), 8);
}

TEST_F(ItemSlotArgumentTypeTest, ParseHotbarOutOfRangeFails)
{
    auto parser = ItemSlotArgumentType::itemSlot();
    StringReader reader("hotbar.9");
    EXPECT_THROW(parser->parse(reader), CommandException);
}

TEST_F(ItemSlotArgumentTypeTest, ParseContainerSlots)
{
    auto parser = ItemSlotArgumentType::itemSlot();

    StringReader r0("container.0");
    EXPECT_EQ(parser->parse(r0).slotIndex(), 0);

    StringReader r27("container.27");
    EXPECT_EQ(parser->parse(r27).slotIndex(), 27);

    StringReader r53("container.53");
    EXPECT_EQ(parser->parse(r53).slotIndex(), 53);
}

TEST_F(ItemSlotArgumentTypeTest, ParseContainerOutOfRangeFails)
{
    auto parser = ItemSlotArgumentType::itemSlot();
    StringReader reader("container.54");
    EXPECT_THROW(parser->parse(reader), CommandException);
}

TEST_F(ItemSlotArgumentTypeTest, ParseInventorySlots)
{
    auto parser = ItemSlotArgumentType::itemSlot();

    StringReader r0("inventory.0");
    EXPECT_EQ(parser->parse(r0).slotIndex(), 9); // 偏移到主背包起始

    StringReader r26("inventory.26");
    EXPECT_EQ(parser->parse(r26).slotIndex(), 35); // 偏移到主背包末尾
}

TEST_F(ItemSlotArgumentTypeTest, ParseWeaponSlots)
{
    auto parser = ItemSlotArgumentType::itemSlot();

    StringReader r1("weapon");
    EXPECT_EQ(parser->parse(r1).slotIndex(), 98);

    StringReader r2("weapon.mainhand");
    EXPECT_EQ(parser->parse(r2).slotIndex(), 98);

    StringReader r3("weapon.offhand");
    EXPECT_EQ(parser->parse(r3).slotIndex(), 99);
}

TEST_F(ItemSlotArgumentTypeTest, ParseArmorSlots)
{
    auto parser = ItemSlotArgumentType::itemSlot();

    StringReader r1("armor.head");
    EXPECT_EQ(parser->parse(r1).slotIndex(), 100);

    StringReader r2("armor.chest");
    EXPECT_EQ(parser->parse(r2).slotIndex(), 101);

    StringReader r3("armor.legs");
    EXPECT_EQ(parser->parse(r3).slotIndex(), 102);

    StringReader r4("armor.feet");
    EXPECT_EQ(parser->parse(r4).slotIndex(), 103);

    StringReader r5("armor.body");
    EXPECT_EQ(parser->parse(r5).slotIndex(), 105);
}

TEST_F(ItemSlotArgumentTypeTest, ParseSaddle)
{
    auto parser = ItemSlotArgumentType::itemSlot();
    StringReader reader("saddle");
    EXPECT_EQ(parser->parse(reader).slotIndex(), 106);
}

TEST_F(ItemSlotArgumentTypeTest, ParseEnderChestSlots)
{
    auto parser = ItemSlotArgumentType::itemSlot();

    StringReader r0("enderchest.0");
    EXPECT_EQ(parser->parse(r0).slotIndex(), 200);

    StringReader r26("enderchest.26");
    EXPECT_EQ(parser->parse(r26).slotIndex(), 226);
}

TEST_F(ItemSlotArgumentTypeTest, ParseVillagerSlots)
{
    auto parser = ItemSlotArgumentType::itemSlot();

    StringReader r0("villager.0");
    EXPECT_EQ(parser->parse(r0).slotIndex(), 300);

    StringReader r7("villager.7");
    EXPECT_EQ(parser->parse(r7).slotIndex(), 307);
}

TEST_F(ItemSlotArgumentTypeTest, ParseHorseSlots)
{
    auto parser = ItemSlotArgumentType::itemSlot();

    StringReader r0("horse.0");
    EXPECT_EQ(parser->parse(r0).slotIndex(), 500);

    StringReader r14("horse.14");
    EXPECT_EQ(parser->parse(r14).slotIndex(), 514);
}

TEST_F(ItemSlotArgumentTypeTest, ParsePlayerCraftingSlots)
{
    auto parser = ItemSlotArgumentType::itemSlot();

    StringReader r0("player.crafting.0");
    EXPECT_EQ(parser->parse(r0).slotIndex(), 500);

    StringReader r3("player.crafting.3");
    EXPECT_EQ(parser->parse(r3).slotIndex(), 503);
}

TEST_F(ItemSlotArgumentTypeTest, ParsePlayerCursor)
{
    auto parser = ItemSlotArgumentType::itemSlot();
    StringReader reader("player.cursor");
    EXPECT_EQ(parser->parse(reader).slotIndex(), 499);
}

TEST_F(ItemSlotArgumentTypeTest, ParseHorseChest)
{
    auto parser = ItemSlotArgumentType::itemSlot();
    StringReader reader("horse.chest");
    EXPECT_EQ(parser->parse(reader).slotIndex(), 499);
}

TEST_F(ItemSlotArgumentTypeTest, ParseUnknownSlotNameFails)
{
    auto parser = ItemSlotArgumentType::itemSlot();
    StringReader reader("unknown.slot");
    EXPECT_THROW(parser->parse(reader), CommandException);
}

TEST_F(ItemSlotArgumentTypeTest, ParseEmptyStringFails)
{
    auto parser = ItemSlotArgumentType::itemSlot();
    StringReader reader("");
    EXPECT_THROW(parser->parse(reader), CommandException);
}

TEST_F(ItemSlotArgumentTypeTest, GetTypeName)
{
    auto parser = ItemSlotArgumentType::itemSlot();
    EXPECT_EQ(parser->getTypeName(), "item_slot");
}

TEST_F(ItemSlotArgumentTypeTest, GetExamples)
{
    auto parser = ItemSlotArgumentType::itemSlot();
    auto examples = parser->getExamples();
    EXPECT_FALSE(examples.empty());
    // 验证包含一些关键示例
    bool hasContainer = false;
    bool hasWeapon = false;
    bool hasArmor = false;
    for (const auto& ex : examples) {
        if (ex.find("container") != std::string::npos) hasContainer = true;
        if (ex.find("weapon") != std::string::npos) hasWeapon = true;
        if (ex.find("armor") != std::string::npos) hasArmor = true;
    }
    EXPECT_TRUE(hasContainer);
    EXPECT_TRUE(hasWeapon);
    EXPECT_TRUE(hasArmor);
}
