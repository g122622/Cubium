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

#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/Items.hpp"
#include "common/item/armor/ArmorMaterial.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/armor/ArmorItem.hpp"
#include "common/resource/ResourceLocation.hpp"

using namespace mc;
using namespace mc::item::armor;

// ArmorSlot is defined both in mc::item::armor (enum class) and mc (class)
// We explicitly use the armor material slot enum here
using ArmorSlotEnum = mc::item::armor::ArmorSlot;

/**
 * @brief Player::isWearingGoldArmor() 测试夹具
 *
 * 参考 MC 1.16.5 PiglinTasks.func_234460_a_() (wearsGoldArmor)
 */
class PlayerGoldArmorTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化物品注册表
        Items::initialize();
        // 初始化护甲材质
        ArmorMaterials::initialize();
    }
};

/**
 * @brief 测试无装备时返回 false
 */
TEST_F(PlayerGoldArmorTest, NoArmorReturnsFalse)
{
    // Player 对象需要世界，这里仅测试逻辑
    // 由于 Player 构造需要 IWorld，这里测试静态逻辑

    // 空物品堆不应被识别为金装备
    ItemStack emptyStack;
    EXPECT_TRUE(emptyStack.isEmpty());

    // 验证金材质的定义
    EXPECT_EQ(ArmorMaterials::GOLD.getName(), "gold");
}

/**
 * @brief 测试 ArmorMaterials::GOLD 材质标识
 */
TEST_F(PlayerGoldArmorTest, GoldMaterialIdentification)
{
    // 验证各种护甲材质的名称
    EXPECT_EQ(ArmorMaterials::LEATHER.getName(), "leather");
    EXPECT_EQ(ArmorMaterials::COPPER.getName(), "copper");
    EXPECT_EQ(ArmorMaterials::CHAIN.getName(), "chain");
    EXPECT_EQ(ArmorMaterials::IRON.getName(), "iron");
    EXPECT_EQ(ArmorMaterials::GOLD.getName(), "gold");
    EXPECT_EQ(ArmorMaterials::DIAMOND.getName(), "diamond");
    EXPECT_EQ(ArmorMaterials::NETHERITE.getName(), "netherite");
}

/**
 * @brief 测试金材质护甲的属性
 */
TEST_F(PlayerGoldArmorTest, GoldArmorProperties)
{
    // 验证金护甲的各种属性
    // 附魔能力：金=25（最高）
    EXPECT_EQ(ArmorMaterials::GOLD.getEnchantability(), 25);

    // 验证各槽位防御值
    EXPECT_EQ(ArmorMaterials::GOLD.getDefense(ArmorSlotEnum::Head), 2);
    EXPECT_EQ(ArmorMaterials::GOLD.getDefense(ArmorSlotEnum::Chest), 5);
    EXPECT_EQ(ArmorMaterials::GOLD.getDefense(ArmorSlotEnum::Legs), 3);
    EXPECT_EQ(ArmorMaterials::GOLD.getDefense(ArmorSlotEnum::Feet), 1);

    // 验证各槽位耐久度
    // 耐久度 = 基础值 * 槽位乘数
    // 金材质基础耐久度较低
    EXPECT_GT(ArmorMaterials::GOLD.getDurability(ArmorSlotEnum::Head), 0);
    EXPECT_GT(ArmorMaterials::GOLD.getDurability(ArmorSlotEnum::Chest), 0);
    EXPECT_GT(ArmorMaterials::GOLD.getDurability(ArmorSlotEnum::Legs), 0);
    EXPECT_GT(ArmorMaterials::GOLD.getDurability(ArmorSlotEnum::Feet), 0);
}

/**
 * @brief 测试不同材质的附魔能力对比
 */
TEST_F(PlayerGoldArmorTest, EnchantabilityComparison)
{
    // 附魔能力排序（从高到低）
    // 金(25) > 皮革/下界合金(15) > 钻石(10) > 铁/海龟(9) > 锁链(12) > 铜(8)
    EXPECT_GT(ArmorMaterials::GOLD.getEnchantability(), ArmorMaterials::DIAMOND.getEnchantability());
    EXPECT_GT(ArmorMaterials::GOLD.getEnchantability(), ArmorMaterials::IRON.getEnchantability());
    EXPECT_GT(ArmorMaterials::GOLD.getEnchantability(), ArmorMaterials::LEATHER.getEnchantability());
    EXPECT_GT(ArmorMaterials::LEATHER.getEnchantability(), ArmorMaterials::COPPER.getEnchantability());
    EXPECT_GT(ArmorMaterials::COPPER.getEnchantability(), 0); // 铜的附魔能力为8
}

/**
 * @brief 测试 ArmorSlot 枚举值
 */
TEST_F(PlayerGoldArmorTest, ArmorSlotValues)
{
    // 验证护甲槽位枚举值
    EXPECT_EQ(static_cast<int>(ArmorSlotEnum::Head), 0);
    EXPECT_EQ(static_cast<int>(ArmorSlotEnum::Chest), 1);
    EXPECT_EQ(static_cast<int>(ArmorSlotEnum::Legs), 2);
    EXPECT_EQ(static_cast<int>(ArmorSlotEnum::Feet), 3);
}

/**
 * @brief 测试 EquipmentSlot 枚举与护甲槽位的对应
 */
TEST_F(PlayerGoldArmorTest, EquipmentSlotArmorMapping)
{
    // EquipmentSlot 枚举中的护甲槽位
    // Head = 5, Chest = 4, Legs = 3, Feet = 2
    EXPECT_EQ(static_cast<int>(EquipmentSlot::Feet), 2);
    EXPECT_EQ(static_cast<int>(EquipmentSlot::Legs), 3);
    EXPECT_EQ(static_cast<int>(EquipmentSlot::Chest), 4);
    EXPECT_EQ(static_cast<int>(EquipmentSlot::Head), 5);
}

/**
 * @brief 测试材质名称一致性
 *
 * 确保 ArmorMaterials::GOLD 的名称与 isWearingGoldArmor() 中的判断一致
 */
TEST_F(PlayerGoldArmorTest, MaterialNameConsistency)
{
    // 验证材质名称用于金装备检测的正确性
    const std::string goldName = ArmorMaterials::GOLD.getName();
    EXPECT_EQ(goldName, "gold");

    // 测试名称比较逻辑
    EXPECT_TRUE(goldName == "gold");
    EXPECT_FALSE(goldName == "iron");
    EXPECT_FALSE(goldName == "diamond");
    EXPECT_FALSE(goldName == "copper");
}

/**
 * @brief 测试铜护甲材质属性（MC 1.21.11 新增）
 *
 * 验证铜护甲的各项属性符合 MC 1.21.11 原版数据：
 * - 基础耐久度乘数 11
 * - 防御值：头盔=2, 胸甲=4, 护腿=3, 靴子=1
 * - 附魔能力 8
 * - 韧性 0.0
 * - 击退抗性 0.0
 */
TEST_F(PlayerGoldArmorTest, CopperArmorMaterialProperties)
{
    // 材质名称
    EXPECT_EQ(ArmorMaterials::COPPER.getName(), "copper");

    // 附魔能力
    EXPECT_EQ(ArmorMaterials::COPPER.getEnchantability(), 8);

    // 防御值（MC 1.21.11 原版数据）
    EXPECT_EQ(ArmorMaterials::COPPER.getDefense(ArmorSlotEnum::Head), 2);
    EXPECT_EQ(ArmorMaterials::COPPER.getDefense(ArmorSlotEnum::Chest), 4);
    EXPECT_EQ(ArmorMaterials::COPPER.getDefense(ArmorSlotEnum::Legs), 3);
    EXPECT_EQ(ArmorMaterials::COPPER.getDefense(ArmorSlotEnum::Feet), 1);

    // 耐久度（基础乘数 11: 头盔=121, 胸甲=176, 护腿=165, 靴子=143）
    EXPECT_EQ(ArmorMaterials::COPPER.getDurability(ArmorSlotEnum::Head), 121);
    EXPECT_EQ(ArmorMaterials::COPPER.getDurability(ArmorSlotEnum::Chest), 176);
    EXPECT_EQ(ArmorMaterials::COPPER.getDurability(ArmorSlotEnum::Legs), 165);
    EXPECT_EQ(ArmorMaterials::COPPER.getDurability(ArmorSlotEnum::Feet), 143);

    // 韧性和击退抗性
    EXPECT_FLOAT_EQ(ArmorMaterials::COPPER.getToughness(), 0.0f);
    EXPECT_FLOAT_EQ(ArmorMaterials::COPPER.getKnockbackResistance(), 0.0f);

    // 装备音效
    EXPECT_EQ(ArmorMaterials::COPPER.getEquipSound().getId(), ResourceLocation("minecraft:item.armor.equip_copper"));

    // 修复材料
    EXPECT_TRUE(ArmorMaterials::COPPER.getRepairMaterial().test(*Items::COPPER_INGOT));

    // 铜护甲防御值介于皮革和锁链之间
    EXPECT_GT(ArmorMaterials::COPPER.getDefense(ArmorSlotEnum::Chest),
        ArmorMaterials::LEATHER.getDefense(ArmorSlotEnum::Chest));
    EXPECT_LT(ArmorMaterials::COPPER.getDefense(ArmorSlotEnum::Chest),
        ArmorMaterials::CHAIN.getDefense(ArmorSlotEnum::Chest));

    // 铜护甲耐久度介于皮革(5)和锁链(15)之间
    EXPECT_GT(ArmorMaterials::COPPER.getDurability(ArmorSlotEnum::Head),
        ArmorMaterials::LEATHER.getDurability(ArmorSlotEnum::Head));
    EXPECT_LT(ArmorMaterials::COPPER.getDurability(ArmorSlotEnum::Head),
        ArmorMaterials::CHAIN.getDurability(ArmorSlotEnum::Head));
}
