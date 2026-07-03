/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the the rights
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

/// @file ArmorMaterialTest.cpp
/// @brief ArmorSlot::Body 和 ArmorMaterial 单元测试
///
/// 测试 ArmorSlot::Body 槽位相关的功能：
/// - ArmorMaterial::getDefense(ArmorSlot::Body) 对全部 8 种材质返回正确值
/// - ArmorMaterial::getDurabilityMultiplier(ArmorSlot::Body) 返回 16
/// - ArmorMaterial::toEquipmentSlotIndex() 对所有 ArmorSlot 值的正确映射
/// - ArmorMaterial::getDurability(ArmorSlot::Body) 对犰狳鳞甲材质返回 64

#include <gtest/gtest.h>

#include "common/entity/core/LivingEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/armor/ArmorMaterial.hpp"
#include "common/item/items/armor/ArmorItem.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;
using namespace mc::item::armor;
using namespace mc::item::items;

// ============================================================================
// ArmorMaterial::getDefense(ArmorSlot::Body) 测试
// 参考 MC 1.21.11 ArmorMaterials 防御值表
// ============================================================================

class ArmorMaterialBodySlotTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        ArmorMaterials::initialize();
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(ArmorMaterialBodySlotTest, LeatherDefenseBodyReturns3)
{
    EXPECT_EQ(ArmorMaterials::LEATHER.getDefense(ArmorSlot::Body), 3);
}

TEST_F(ArmorMaterialBodySlotTest, ChainDefenseBodyReturns4)
{
    EXPECT_EQ(ArmorMaterials::CHAIN.getDefense(ArmorSlot::Body), 4);
}

TEST_F(ArmorMaterialBodySlotTest, CopperDefenseBodyReturns4)
{
    EXPECT_EQ(ArmorMaterials::COPPER.getDefense(ArmorSlot::Body), 4);
}

TEST_F(ArmorMaterialBodySlotTest, IronDefenseBodyReturns5)
{
    EXPECT_EQ(ArmorMaterials::IRON.getDefense(ArmorSlot::Body), 5);
}

TEST_F(ArmorMaterialBodySlotTest, GoldDefenseBodyReturns7)
{
    EXPECT_EQ(ArmorMaterials::GOLD.getDefense(ArmorSlot::Body), 7);
}

TEST_F(ArmorMaterialBodySlotTest, DiamondDefenseBodyReturns11)
{
    EXPECT_EQ(ArmorMaterials::DIAMOND.getDefense(ArmorSlot::Body), 11);
}

TEST_F(ArmorMaterialBodySlotTest, TurtleDefenseBodyReturns5)
{
    EXPECT_EQ(ArmorMaterials::TURTLE.getDefense(ArmorSlot::Body), 5);
}

TEST_F(ArmorMaterialBodySlotTest, NetheriteDefenseBodyReturns19)
{
    EXPECT_EQ(ArmorMaterials::NETHERITE.getDefense(ArmorSlot::Body), 19);
}

TEST_F(ArmorMaterialBodySlotTest, ArmadilloScuteDefenseBodyReturns11)
{
    EXPECT_EQ(ArmorMaterials::ARMADILLO_SCUTE.getDefense(ArmorSlot::Body), 11);
}

// ============================================================================
// ArmorMaterial::getDefense 其他槽位回归测试
// 确保 Body 槽位的添加没有影响其他槽位的防御值
// ============================================================================

TEST_F(ArmorMaterialBodySlotTest, DiamondDefenseAllSlots)
{
    // 钻石材质所有槽位防御值
    EXPECT_EQ(ArmorMaterials::DIAMOND.getDefense(ArmorSlot::Head), 3);
    EXPECT_EQ(ArmorMaterials::DIAMOND.getDefense(ArmorSlot::Chest), 8);
    EXPECT_EQ(ArmorMaterials::DIAMOND.getDefense(ArmorSlot::Legs), 6);
    EXPECT_EQ(ArmorMaterials::DIAMOND.getDefense(ArmorSlot::Feet), 3);
    EXPECT_EQ(ArmorMaterials::DIAMOND.getDefense(ArmorSlot::Body), 11);
}

TEST_F(ArmorMaterialBodySlotTest, NetheriteDefenseAllSlots)
{
    // 下界合金材质所有槽位防御值
    EXPECT_EQ(ArmorMaterials::NETHERITE.getDefense(ArmorSlot::Head), 3);
    EXPECT_EQ(ArmorMaterials::NETHERITE.getDefense(ArmorSlot::Chest), 8);
    EXPECT_EQ(ArmorMaterials::NETHERITE.getDefense(ArmorSlot::Legs), 6);
    EXPECT_EQ(ArmorMaterials::NETHERITE.getDefense(ArmorSlot::Feet), 3);
    EXPECT_EQ(ArmorMaterials::NETHERITE.getDefense(ArmorSlot::Body), 19);
}

TEST_F(ArmorMaterialBodySlotTest, ArmadilloScuteDefenseAllSlots)
{
    // 犰狳鳞甲材质所有槽位防御值
    // HEAD=3, CHEST=6, LEGS=8, FEET=3, BODY=11
    EXPECT_EQ(ArmorMaterials::ARMADILLO_SCUTE.getDefense(ArmorSlot::Head), 3);
    EXPECT_EQ(ArmorMaterials::ARMADILLO_SCUTE.getDefense(ArmorSlot::Chest), 6);
    EXPECT_EQ(ArmorMaterials::ARMADILLO_SCUTE.getDefense(ArmorSlot::Legs), 8);
    EXPECT_EQ(ArmorMaterials::ARMADILLO_SCUTE.getDefense(ArmorSlot::Feet), 3);
    EXPECT_EQ(ArmorMaterials::ARMADILLO_SCUTE.getDefense(ArmorSlot::Body), 11);
}

// ============================================================================
// ArmorMaterial::getDurabilityMultiplier(ArmorSlot::Body) 测试
// ============================================================================

TEST_F(ArmorMaterialBodySlotTest, GetDurabilityMultiplierBodyReturns16)
{
    // Body 槽位耐久度乘数应为 16（与 Chest 相同）
    EXPECT_EQ(ArmorMaterial::getDurabilityMultiplier(ArmorSlot::Body), 16);
}

TEST_F(ArmorMaterialBodySlotTest, GetDurabilityMultiplierAllSlots)
{
    // 验证所有槽位的耐久度乘数
    EXPECT_EQ(ArmorMaterial::getDurabilityMultiplier(ArmorSlot::Head), 11);
    EXPECT_EQ(ArmorMaterial::getDurabilityMultiplier(ArmorSlot::Chest), 16);
    EXPECT_EQ(ArmorMaterial::getDurabilityMultiplier(ArmorSlot::Legs), 15);
    EXPECT_EQ(ArmorMaterial::getDurabilityMultiplier(ArmorSlot::Feet), 13);
    EXPECT_EQ(ArmorMaterial::getDurabilityMultiplier(ArmorSlot::Body), 16);
}

// ============================================================================
// ArmorMaterial::getDurability(ArmorSlot::Body) 测试
// 耐久度 = baseDurability * getDurabilityMultiplier(slot)
// ============================================================================

TEST_F(ArmorMaterialBodySlotTest, ArmadilloScuteDurabilityBodyReturns64)
{
    // 犰狳鳞甲基础耐久度 4，Body 槽位乘数 16，总耐久度 = 4 * 16 = 64
    EXPECT_EQ(ArmorMaterials::ARMADILLO_SCUTE.getDurability(ArmorSlot::Body), 64);
}

TEST_F(ArmorMaterialBodySlotTest, LeatherDurabilityBodyReturns80)
{
    // 皮革基础耐久度 5，Body 槽位乘数 16，总耐久度 = 5 * 16 = 80
    EXPECT_EQ(ArmorMaterials::LEATHER.getDurability(ArmorSlot::Body), 80);
}

TEST_F(ArmorMaterialBodySlotTest, DiamondDurabilityBodyReturns528)
{
    // 钻石基础耐久度 33，Body 槽位乘数 16，总耐久度 = 33 * 16 = 528
    EXPECT_EQ(ArmorMaterials::DIAMOND.getDurability(ArmorSlot::Body), 528);
}

TEST_F(ArmorMaterialBodySlotTest, NetheriteDurabilityBodyReturns592)
{
    // 下界合金基础耐久度 37，Body 槽位乘数 16，总耐久度 = 37 * 16 = 592
    EXPECT_EQ(ArmorMaterials::NETHERITE.getDurability(ArmorSlot::Body), 592);
}

// ============================================================================
// ArmorMaterial::toEquipmentSlotIndex 测试
// 验证 ArmorSlot 到 EquipmentSlot 的正确映射
// ============================================================================

class ArmorSlotMappingTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { ArmorMaterials::initialize(); }
};

TEST_F(ArmorSlotMappingTest, HeadMapsToHeadEquipmentSlot)
{
    // ArmorSlot::Head (0) -> EquipmentSlot::Head (5)
    EXPECT_EQ(ArmorMaterial::toEquipmentSlotIndex(ArmorSlot::Head), static_cast<i32>(EquipmentSlot::Head));
}

TEST_F(ArmorSlotMappingTest, ChestMapsToChestEquipmentSlot)
{
    // ArmorSlot::Chest (1) -> EquipmentSlot::Chest (4)
    EXPECT_EQ(ArmorMaterial::toEquipmentSlotIndex(ArmorSlot::Chest), static_cast<i32>(EquipmentSlot::Chest));
}

TEST_F(ArmorSlotMappingTest, LegsMapsToLegsEquipmentSlot)
{
    // ArmorSlot::Legs (2) -> EquipmentSlot::Legs (3)
    EXPECT_EQ(ArmorMaterial::toEquipmentSlotIndex(ArmorSlot::Legs), static_cast<i32>(EquipmentSlot::Legs));
}

TEST_F(ArmorSlotMappingTest, FeetMapsToFeetEquipmentSlot)
{
    // ArmorSlot::Feet (3) -> EquipmentSlot::Feet (2)
    EXPECT_EQ(ArmorMaterial::toEquipmentSlotIndex(ArmorSlot::Feet), static_cast<i32>(EquipmentSlot::Feet));
}

TEST_F(ArmorSlotMappingTest, BodyMapsToBodyEquipmentSlot)
{
    // ArmorSlot::Body (4) -> EquipmentSlot::Body (6)
    EXPECT_EQ(ArmorMaterial::toEquipmentSlotIndex(ArmorSlot::Body), static_cast<i32>(EquipmentSlot::Body));
}

TEST_F(ArmorSlotMappingTest, AllSlotMappingsAreDistinct)
{
    // 确保所有 ArmorSlot 映射到不同的 EquipmentSlot 索引
    // 防止 static_cast 之类的错误映射导致多个 ArmorSlot 映射到同一个 EquipmentSlot
    i32 head = ArmorMaterial::toEquipmentSlotIndex(ArmorSlot::Head);
    i32 chest = ArmorMaterial::toEquipmentSlotIndex(ArmorSlot::Chest);
    i32 legs = ArmorMaterial::toEquipmentSlotIndex(ArmorSlot::Legs);
    i32 feet = ArmorMaterial::toEquipmentSlotIndex(ArmorSlot::Feet);
    i32 body = ArmorMaterial::toEquipmentSlotIndex(ArmorSlot::Body);

    EXPECT_NE(head, chest);
    EXPECT_NE(head, legs);
    EXPECT_NE(head, feet);
    EXPECT_NE(head, body);
    EXPECT_NE(chest, legs);
    EXPECT_NE(chest, feet);
    EXPECT_NE(chest, body);
    EXPECT_NE(legs, feet);
    EXPECT_NE(legs, body);
    EXPECT_NE(feet, body);
}

TEST_F(ArmorSlotMappingTest, NoArmorSlotMapsToMainHandOrOffHand)
{
    // ArmorSlot 不应映射到 MainHand (0) 或 OffHand (1)
    // 这验证了 toEquipmentSlotIndex 不再使用 static_cast<i32>(slot) 的旧错误映射
    for (u8 i = 0; i < static_cast<u8>(ArmorSlot::Body) + 1; ++i) {
        auto slot = static_cast<ArmorSlot>(i);
        i32 eqSlot = ArmorMaterial::toEquipmentSlotIndex(slot);
        EXPECT_NE(eqSlot, static_cast<i32>(EquipmentSlot::MainHand))
            << "ArmorSlot " << i << " should not map to MainHand";
        EXPECT_NE(eqSlot, static_cast<i32>(EquipmentSlot::OffHand))
            << "ArmorSlot " << i << " should not map to OffHand";
    }
}

// ============================================================================
// ArmorItem::isBodyArmor() 测试
// ============================================================================

class ArmorItemBodySlotTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        ArmorMaterials::initialize();
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(ArmorItemBodySlotTest, WolfArmorIsBodyArmor)
{
    ASSERT_NE(Items::WOLF_ARMOR, nullptr);
    const auto* wolfArmor = dynamic_cast<const ArmorItem*>(Items::WOLF_ARMOR);
    ASSERT_NE(wolfArmor, nullptr);
    EXPECT_TRUE(wolfArmor->isBodyArmor());
    // 狼铠不应被识别为其他槽位
    EXPECT_FALSE(wolfArmor->isHelmet());
    EXPECT_FALSE(wolfArmor->isChestplate());
    EXPECT_FALSE(wolfArmor->isLeggings());
    EXPECT_FALSE(wolfArmor->isBoots());
}

TEST_F(ArmorItemBodySlotTest, WolfArmorGetSlotReturnsBody)
{
    ASSERT_NE(Items::WOLF_ARMOR, nullptr);
    const auto* wolfArmor = dynamic_cast<const ArmorItem*>(Items::WOLF_ARMOR);
    ASSERT_NE(wolfArmor, nullptr);
    EXPECT_EQ(wolfArmor->getSlot(), ArmorSlot::Body);
}

TEST_F(ArmorItemBodySlotTest, WolfArmorGetDefenseReturns11FromMaterial)
{
    // 狼铠防御值应从 ArmadilloScuteArmorMaterial::getDefense(ArmorSlot::Body) 获取，返回 11
    // 而非旧的硬编码重写
    ASSERT_NE(Items::WOLF_ARMOR, nullptr);
    const auto* wolfArmor = dynamic_cast<const ArmorItem*>(Items::WOLF_ARMOR);
    ASSERT_NE(wolfArmor, nullptr);
    EXPECT_EQ(wolfArmor->getDefense(), 11);
}

TEST_F(ArmorItemBodySlotTest, WolfArmorGetMaterialReturnsArmadilloScute)
{
    ASSERT_NE(Items::WOLF_ARMOR, nullptr);
    const auto* wolfArmor = dynamic_cast<const ArmorItem*>(Items::WOLF_ARMOR);
    ASSERT_NE(wolfArmor, nullptr);
    EXPECT_EQ(&wolfArmor->getMaterial(), &ArmorMaterials::ARMADILLO_SCUTE);
}

TEST_F(ArmorItemBodySlotTest, WolfArmorDurabilityIs64)
{
    // 狼铠耐久度 = ArmadilloScuteArmorMaterial::getDurability(ArmorSlot::Body) = 4 * 16 = 64
    ASSERT_NE(Items::WOLF_ARMOR, nullptr);
    EXPECT_EQ(Items::WOLF_ARMOR->maxDamage(), 64);
}

// ============================================================================
// 非 Body 槽位护甲的 isBodyArmor 回归测试
// 确保其他护甲物品不被误判为 Body 护甲
// ============================================================================

TEST_F(ArmorItemBodySlotTest, IronHelmetIsNotBodyArmor)
{
    ASSERT_NE(Items::IRON_HELMET, nullptr);
    const auto* helmet = dynamic_cast<const ArmorItem*>(Items::IRON_HELMET);
    ASSERT_NE(helmet, nullptr);
    EXPECT_FALSE(helmet->isBodyArmor());
    EXPECT_TRUE(helmet->isHelmet());
}

TEST_F(ArmorItemBodySlotTest, IronChestplateIsNotBodyArmor)
{
    ASSERT_NE(Items::IRON_CHESTPLATE, nullptr);
    const auto* chestplate = dynamic_cast<const ArmorItem*>(Items::IRON_CHESTPLATE);
    ASSERT_NE(chestplate, nullptr);
    EXPECT_FALSE(chestplate->isBodyArmor());
    EXPECT_TRUE(chestplate->isChestplate());
}

TEST_F(ArmorItemBodySlotTest, IronLeggingsIsNotBodyArmor)
{
    ASSERT_NE(Items::IRON_LEGGINGS, nullptr);
    const auto* leggings = dynamic_cast<const ArmorItem*>(Items::IRON_LEGGINGS);
    ASSERT_NE(leggings, nullptr);
    EXPECT_FALSE(leggings->isBodyArmor());
    EXPECT_TRUE(leggings->isLeggings());
}

TEST_F(ArmorItemBodySlotTest, IronBootsIsNotBodyArmor)
{
    ASSERT_NE(Items::IRON_BOOTS, nullptr);
    const auto* boots = dynamic_cast<const ArmorItem*>(Items::IRON_BOOTS);
    ASSERT_NE(boots, nullptr);
    EXPECT_FALSE(boots->isBodyArmor());
    EXPECT_TRUE(boots->isBoots());
}
