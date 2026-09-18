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

#include "common/entity/damage/DamageSource.hpp"
#include "common/item/Items.hpp"
#include "common/item/armor/ArmorMaterial.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/armor/NautilusArmorItem.hpp"
#include "common/item/items/armor/WolfArmorItem.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;
using namespace mc::item::armor;
using namespace mc::item::items;
using namespace mc::item::tag;

// ============================================================================
// 测试夹具
// ============================================================================

class WolfAndNautilusArmorTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 初始化顺序：先初始化材质，再初始化物品
        ArmorMaterials::initialize();
        VanillaBlocks::initialize();
        Items::initialize();
        ItemTags::initialize();
    }
};

// ============================================================================
// ArmadilloScuteArmorMaterial 测试
// ============================================================================

TEST_F(WolfAndNautilusArmorTest, ArmadilloScuteArmorMaterial_GetName_ReturnsCorrectValue)
{
    // getName() 应返回 "armadillo_scute"
    EXPECT_EQ(ArmorMaterials::ARMADILLO_SCUTE.getName(), "armadillo_scute");
}

TEST_F(WolfAndNautilusArmorTest, ArmadilloScuteArmorMaterial_GetAssetId_ReturnsCorrectValue)
{
    // getAssetId() 应返回 "armadillo_scute"
    EXPECT_EQ(ArmorMaterials::ARMADILLO_SCUTE.getAssetId(), "armadillo_scute");
}

TEST_F(WolfAndNautilusArmorTest, ArmadilloScuteArmorMaterial_GetEnchantability_Returns10)
{
    // 附魔能力应为 10
    EXPECT_EQ(ArmorMaterials::ARMADILLO_SCUTE.getEnchantability(), 10);
}

TEST_F(WolfAndNautilusArmorTest, ArmadilloScuteArmorMaterial_GetDurability_ChestSlot_Returns64)
{
    // 胸甲槽位耐久度应为 64
    EXPECT_EQ(ArmorMaterials::ARMADILLO_SCUTE.getDurability(ArmorSlot::Chest), 64);
}

TEST_F(WolfAndNautilusArmorTest, ArmadilloScuteArmorMaterial_GetDefense_ChestSlot_Returns6)
{
    // 胸甲槽位防御值应为 6
    EXPECT_EQ(ArmorMaterials::ARMADILLO_SCUTE.getDefense(ArmorSlot::Chest), 6);
}

TEST_F(WolfAndNautilusArmorTest, ArmadilloScuteArmorMaterial_GetToughness_Returns0)
{
    // 韧性应为 0.0
    EXPECT_FLOAT_EQ(ArmorMaterials::ARMADILLO_SCUTE.getToughness(), 0.0f);
}

TEST_F(WolfAndNautilusArmorTest, ArmadilloScuteArmorMaterial_GetKnockbackResistance_Returns0)
{
    // 击退抗性应为 0.0
    EXPECT_FLOAT_EQ(ArmorMaterials::ARMADILLO_SCUTE.getKnockbackResistance(), 0.0f);
}

// ============================================================================
// WolfArmorItem 测试
// ============================================================================

TEST_F(WolfAndNautilusArmorTest, WolfArmorItem_Registered_HasCorrectProperties)
{
    // WOLF_ARMOR 应已注册
    ASSERT_NE(Items::WOLF_ARMOR, nullptr);
    // 最大堆叠数为 1
    EXPECT_EQ(Items::WOLF_ARMOR->maxStackSize(), 1);
    // 可损耗
    EXPECT_TRUE(Items::WOLF_ARMOR->isDamageable());
    // 最大耐久度为 64
    EXPECT_EQ(Items::WOLF_ARMOR->maxDamage(), 64);
}

TEST_F(WolfAndNautilusArmorTest, WolfArmorItem_IsDyeableArmor)
{
    // WOLF_ARMOR 应能 dynamic_cast 到 DyeableArmorItem
    ASSERT_NE(Items::WOLF_ARMOR, nullptr);
    auto* dyeable = dynamic_cast<DyeableArmorItem*>(Items::WOLF_ARMOR);
    EXPECT_NE(dyeable, nullptr);
}

TEST_F(WolfAndNautilusArmorTest, WolfArmorItem_IsArmor)
{
    // WOLF_ARMOR 的 isArmor() 应返回 true
    ASSERT_NE(Items::WOLF_ARMOR, nullptr);
    EXPECT_TRUE(Items::WOLF_ARMOR->isArmor());
}

TEST_F(WolfAndNautilusArmorTest, WolfArmorItem_GetDefaultColor)
{
    // 狼铠默认颜色应为 0xA06540（犰狳鳞甲棕色）
    ASSERT_NE(Items::WOLF_ARMOR, nullptr);
    auto* wolfArmor = dynamic_cast<WolfArmorItem*>(Items::WOLF_ARMOR);
    ASSERT_NE(wolfArmor, nullptr);
    EXPECT_EQ(wolfArmor->getDefaultColor(), 0xA06540u);
}

TEST_F(WolfAndNautilusArmorTest, WolfArmorItem_GetRepairMaterial)
{
    // 狼铠应能使用 armadillo_scute 修复
    ASSERT_NE(Items::WOLF_ARMOR, nullptr);
    ASSERT_NE(Items::ARMADILLO_SCUTE, nullptr);

    ItemStack wolfArmorStack(*Items::WOLF_ARMOR, 1);
    ItemStack repairStack(*Items::ARMADILLO_SCUTE, 1);

    EXPECT_TRUE(Items::WOLF_ARMOR->getIsRepairable(wolfArmorStack, repairStack));
}

TEST_F(WolfAndNautilusArmorTest, WolfArmorItem_GetDefense_ReturnsBodyValue)
{
    // 狼铠防御值应为 11（Body 槽位），而非材质 Chest 槽位的 6
    ASSERT_NE(Items::WOLF_ARMOR, nullptr);
    auto* wolfArmor = dynamic_cast<ArmorItem*>(Items::WOLF_ARMOR);
    ASSERT_NE(wolfArmor, nullptr);
    EXPECT_EQ(wolfArmor->getDefense(), 11);
}

// ============================================================================
// NautilusArmorItem 测试
// ============================================================================

TEST_F(WolfAndNautilusArmorTest, CopperNautilusArmorItem_Registered_HasCorrectProperties)
{
    // 铜鹦鹉螺铠甲应已注册
    ASSERT_NE(Items::COPPER_NAUTILUS_ARMOR, nullptr);
    // 最大堆叠数为 1
    EXPECT_EQ(Items::COPPER_NAUTILUS_ARMOR->maxStackSize(), 1);
    // 不可损耗
    EXPECT_FALSE(Items::COPPER_NAUTILUS_ARMOR->isDamageable());
}

TEST_F(WolfAndNautilusArmorTest, CopperNautilusArmorItem_GetArmorValue)
{
    // 铜鹦鹉螺铠甲护甲值为 4
    ASSERT_NE(Items::COPPER_NAUTILUS_ARMOR, nullptr);
    auto* copperNautilus = dynamic_cast<NautilusArmorItem*>(Items::COPPER_NAUTILUS_ARMOR);
    ASSERT_NE(copperNautilus, nullptr);
    EXPECT_EQ(copperNautilus->getArmorValue(), 4);
}

TEST_F(WolfAndNautilusArmorTest, IronNautilusArmorItem_GetArmorValue)
{
    // 铁鹦鹉螺铠甲护甲值为 5
    ASSERT_NE(Items::IRON_NAUTILUS_ARMOR, nullptr);
    auto* ironNautilus = dynamic_cast<NautilusArmorItem*>(Items::IRON_NAUTILUS_ARMOR);
    ASSERT_NE(ironNautilus, nullptr);
    EXPECT_EQ(ironNautilus->getArmorValue(), 5);
}

TEST_F(WolfAndNautilusArmorTest, GoldenNautilusArmorItem_GetArmorValue)
{
    // 金鹦鹉螺铠甲护甲值为 7
    ASSERT_NE(Items::GOLDEN_NAUTILUS_ARMOR, nullptr);
    auto* goldenNautilus = dynamic_cast<NautilusArmorItem*>(Items::GOLDEN_NAUTILUS_ARMOR);
    ASSERT_NE(goldenNautilus, nullptr);
    EXPECT_EQ(goldenNautilus->getArmorValue(), 7);
}

TEST_F(WolfAndNautilusArmorTest, DiamondNautilusArmorItem_GetArmorValue)
{
    // 钻石鹦鹉螺铠甲护甲值为 11
    ASSERT_NE(Items::DIAMOND_NAUTILUS_ARMOR, nullptr);
    auto* diamondNautilus = dynamic_cast<NautilusArmorItem*>(Items::DIAMOND_NAUTILUS_ARMOR);
    ASSERT_NE(diamondNautilus, nullptr);
    EXPECT_EQ(diamondNautilus->getArmorValue(), 11);
}

TEST_F(WolfAndNautilusArmorTest, NetheriteNautilusArmorItem_GetArmorValue)
{
    // 下界合金鹦鹉螺铠甲护甲值为 19
    ASSERT_NE(Items::NETHERITE_NAUTILUS_ARMOR, nullptr);
    auto* netheriteNautilus = dynamic_cast<NautilusArmorItem*>(Items::NETHERITE_NAUTILUS_ARMOR);
    ASSERT_NE(netheriteNautilus, nullptr);
    EXPECT_EQ(netheriteNautilus->getArmorValue(), 19);
}

TEST_F(WolfAndNautilusArmorTest, NetheriteNautilusArmorItem_HasRareRarity)
{
    // 下界合金鹦鹉螺铠甲稀有度应为 Rare
    ASSERT_NE(Items::NETHERITE_NAUTILUS_ARMOR, nullptr);
    EXPECT_EQ(Items::NETHERITE_NAUTILUS_ARMOR->rarity(), ItemRarity::Rare);
}

// ============================================================================
// NautilusArmorItem 护甲值来源契约测试
// 鹦鹉螺铠甲护甲值应从 ArmorMaterial::getDefense(ArmorSlot::Body) 推导，
// 与 MC 1.21.11 Item.Properties.nautilusArmor(ArmorMaterial) 通过
// ArmorMaterial.createAttributes(ArmorType.BODY) 取护甲值的语义一致。
// ============================================================================

TEST_F(WolfAndNautilusArmorTest, CopperNautilusArmorItem_ArmorValueMatchesMaterialBodyDefense)
{
    // 铜鹦鹉螺铠甲护甲值应等于 CopperArmorMaterial::getDefense(ArmorSlot::Body)
    ASSERT_NE(Items::COPPER_NAUTILUS_ARMOR, nullptr);
    auto* nautilus = dynamic_cast<NautilusArmorItem*>(Items::COPPER_NAUTILUS_ARMOR);
    ASSERT_NE(nautilus, nullptr);
    EXPECT_EQ(nautilus->getArmorValue(), ArmorMaterials::COPPER.getDefense(ArmorSlot::Body));
    EXPECT_EQ(&nautilus->getMaterial(), &ArmorMaterials::COPPER);
}

TEST_F(WolfAndNautilusArmorTest, IronNautilusArmorItem_ArmorValueMatchesMaterialBodyDefense)
{
    ASSERT_NE(Items::IRON_NAUTILUS_ARMOR, nullptr);
    auto* nautilus = dynamic_cast<NautilusArmorItem*>(Items::IRON_NAUTILUS_ARMOR);
    ASSERT_NE(nautilus, nullptr);
    EXPECT_EQ(nautilus->getArmorValue(), ArmorMaterials::IRON.getDefense(ArmorSlot::Body));
    EXPECT_EQ(&nautilus->getMaterial(), &ArmorMaterials::IRON);
}

TEST_F(WolfAndNautilusArmorTest, GoldenNautilusArmorItem_ArmorValueMatchesMaterialBodyDefense)
{
    ASSERT_NE(Items::GOLDEN_NAUTILUS_ARMOR, nullptr);
    auto* nautilus = dynamic_cast<NautilusArmorItem*>(Items::GOLDEN_NAUTILUS_ARMOR);
    ASSERT_NE(nautilus, nullptr);
    EXPECT_EQ(nautilus->getArmorValue(), ArmorMaterials::GOLD.getDefense(ArmorSlot::Body));
    EXPECT_EQ(&nautilus->getMaterial(), &ArmorMaterials::GOLD);
}

TEST_F(WolfAndNautilusArmorTest, DiamondNautilusArmorItem_ArmorValueMatchesMaterialBodyDefense)
{
    ASSERT_NE(Items::DIAMOND_NAUTILUS_ARMOR, nullptr);
    auto* nautilus = dynamic_cast<NautilusArmorItem*>(Items::DIAMOND_NAUTILUS_ARMOR);
    ASSERT_NE(nautilus, nullptr);
    EXPECT_EQ(nautilus->getArmorValue(), ArmorMaterials::DIAMOND.getDefense(ArmorSlot::Body));
    EXPECT_EQ(&nautilus->getMaterial(), &ArmorMaterials::DIAMOND);
}

TEST_F(WolfAndNautilusArmorTest, NetheriteNautilusArmorItem_ArmorValueMatchesMaterialBodyDefense)
{
    ASSERT_NE(Items::NETHERITE_NAUTILUS_ARMOR, nullptr);
    auto* nautilus = dynamic_cast<NautilusArmorItem*>(Items::NETHERITE_NAUTILUS_ARMOR);
    ASSERT_NE(nautilus, nullptr);
    EXPECT_EQ(nautilus->getArmorValue(), ArmorMaterials::NETHERITE.getDefense(ArmorSlot::Body));
    EXPECT_EQ(&nautilus->getMaterial(), &ArmorMaterials::NETHERITE);
}

// ============================================================================
// ItemTags 测试
// ============================================================================

TEST_F(WolfAndNautilusArmorTest, ItemTags_RepairsWolfArmor_ContainsArmadilloScute)
{
    // REPAIRS_WOLF_ARMOR 标签应包含 armadillo_scute
    ASSERT_NE(Items::ARMADILLO_SCUTE, nullptr);
    EXPECT_TRUE(ItemTags::REPAIRS_WOLF_ARMOR().contains(Items::ARMADILLO_SCUTE));
}

TEST_F(WolfAndNautilusArmorTest, ItemTags_Dyeable_ContainsWolfArmor)
{
    // DYEABLE 标签应包含 wolf_armor
    ASSERT_NE(Items::WOLF_ARMOR, nullptr);
    EXPECT_TRUE(ItemTags::DYEABLE().contains(Items::WOLF_ARMOR));
}

TEST_F(WolfAndNautilusArmorTest, ItemTags_PiglinLoved_ContainsGoldenNautilusArmor)
{
    // PIGLIN_LOVED 标签应包含 golden_nautilus_armor
    ASSERT_NE(Items::GOLDEN_NAUTILUS_ARMOR, nullptr);
    EXPECT_TRUE(ItemTags::PIGLIN_LOVED().contains(Items::GOLDEN_NAUTILUS_ARMOR));
}

TEST_F(WolfAndNautilusArmorTest, ItemTags_FireResistant_ContainsNetheriteNautilusArmor)
{
    // FIRE_RESISTANT 标签应包含 netherite_nautilus_armor（与下界合金马铠机制一致）
    ASSERT_NE(Items::NETHERITE_NAUTILUS_ARMOR, nullptr);
    EXPECT_TRUE(ItemTags::FIRE_RESISTANT().contains(Items::NETHERITE_NAUTILUS_ARMOR));
}

TEST_F(WolfAndNautilusArmorTest, ItemTags_FireResistant_DoesNotContainNonNetheriteNautilusArmor)
{
    // 非下界合金鹦鹉螺铠甲不应在 FIRE_RESISTANT 标签中
    ASSERT_NE(Items::COPPER_NAUTILUS_ARMOR, nullptr);
    EXPECT_FALSE(ItemTags::FIRE_RESISTANT().contains(Items::COPPER_NAUTILUS_ARMOR));
    ASSERT_NE(Items::IRON_NAUTILUS_ARMOR, nullptr);
    EXPECT_FALSE(ItemTags::FIRE_RESISTANT().contains(Items::IRON_NAUTILUS_ARMOR));
    ASSERT_NE(Items::GOLDEN_NAUTILUS_ARMOR, nullptr);
    EXPECT_FALSE(ItemTags::FIRE_RESISTANT().contains(Items::GOLDEN_NAUTILUS_ARMOR));
    ASSERT_NE(Items::DIAMOND_NAUTILUS_ARMOR, nullptr);
    EXPECT_FALSE(ItemTags::FIRE_RESISTANT().contains(Items::DIAMOND_NAUTILUS_ARMOR));
}

TEST_F(WolfAndNautilusArmorTest, NetheriteNautilusArmorItem_StackCannotBeHurtByFire)
{
    // 下界合金鹦鹉螺铠甲物品堆应免疫火焰伤害源
    ASSERT_NE(Items::NETHERITE_NAUTILUS_ARMOR, nullptr);
    ItemStack stack(*Items::NETHERITE_NAUTILUS_ARMOR, 1);
    EXPECT_FALSE(stack.canBeHurtBy(DamageSources::inFire()));
    EXPECT_FALSE(stack.canBeHurtBy(DamageSources::lava()));
}

TEST_F(WolfAndNautilusArmorTest, NetheriteNautilusArmorItem_StackCanBeHurtByGenericDamage)
{
    // 下界合金鹦鹉螺铠甲物品堆仍可被普通伤害源伤害（仅防火）
    ASSERT_NE(Items::NETHERITE_NAUTILUS_ARMOR, nullptr);
    ItemStack stack(*Items::NETHERITE_NAUTILUS_ARMOR, 1);
    EXPECT_TRUE(stack.canBeHurtBy(DamageSources::generic()));
}

TEST_F(WolfAndNautilusArmorTest, NonNetheriteNautilusArmorItem_StackCanBeHurtByFire)
{
    // 非下界合金鹦鹉螺铠甲物品堆不免疫火焰伤害源
    ASSERT_NE(Items::DIAMOND_NAUTILUS_ARMOR, nullptr);
    ItemStack stack(*Items::DIAMOND_NAUTILUS_ARMOR, 1);
    EXPECT_TRUE(stack.canBeHurtBy(DamageSources::inFire()));
    EXPECT_TRUE(stack.canBeHurtBy(DamageSources::lava()));
}
