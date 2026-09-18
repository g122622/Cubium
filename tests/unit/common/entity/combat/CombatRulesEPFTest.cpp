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

#include "common/entity/combat/CombatRules.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"

using namespace mc;
using namespace mc::entity::combat;
using namespace mc::item::enchant;

// ============================================================================
// CombatRules EPF 测试
// ============================================================================

class CombatRulesEPFTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
        Items::initialize();
    }

    void TearDown() override { EnchantmentRegistry::clear(); }
};

// ========== getDamageAfterMagicAbsorb 测试 ==========

TEST_F(CombatRulesEPFTest, GetDamageAfterMagicAbsorb_ZeroEPF)
{
    // EPF = 0 时，伤害不变
    EXPECT_FLOAT_EQ(CombatRules::getDamageAfterMagicAbsorb(10.0f, 0.0f), 10.0f);
    EXPECT_FLOAT_EQ(CombatRules::getDamageAfterMagicAbsorb(100.0f, 0.0f), 100.0f);
}

TEST_F(CombatRulesEPFTest, GetDamageAfterMagicAbsorb_EPF10)
{
    // EPF = 10 时，减伤 10/25 = 40%，伤害 × 0.6
    EXPECT_FLOAT_EQ(CombatRules::getDamageAfterMagicAbsorb(100.0f, 10.0f), 60.0f);
}

TEST_F(CombatRulesEPFTest, GetDamageAfterMagicAbsorb_EPF20)
{
    // EPF = 20 时，减伤 20/25 = 80%，伤害 × 0.2
    EXPECT_FLOAT_EQ(CombatRules::getDamageAfterMagicAbsorb(100.0f, 20.0f), 20.0f);
}

TEST_F(CombatRulesEPFTest, GetDamageAfterMagicAbsorb_EPFOver20Clamped)
{
    // EPF > 20 时，被限制为 20
    // EPF = 32 时，仍然按 EPF=20 计算
    EXPECT_FLOAT_EQ(CombatRules::getDamageAfterMagicAbsorb(100.0f, 32.0f), 20.0f);
}

TEST_F(CombatRulesEPFTest, GetDamageAfterMagicAbsorb_NegativeEPFClamped)
{
    // 负数 EPF 被限制为 0
    EXPECT_FLOAT_EQ(CombatRules::getDamageAfterMagicAbsorb(100.0f, -5.0f), 100.0f);
}

TEST_F(CombatRulesEPFTest, GetDamageAfterMagicAbsorb_ZeroDamage)
{
    // 0 伤害返回 0
    EXPECT_FLOAT_EQ(CombatRules::getDamageAfterMagicAbsorb(0.0f, 20.0f), 0.0f);
}

// ============================================================================
// EnchantmentHelper EPF 测试
// ============================================================================

class EnchantmentHelperEPFTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
        Items::initialize();
    }

    void TearDown() override { EnchantmentRegistry::clear(); }
};

// ========== 单物品保护附魔测试 ==========

TEST_F(EnchantmentHelperEPFTest, SingleProtectionItem_All)
{
    // 创建一个保护 IV 的物品（使用钻石胸甲）
    const mc::Item* diamondChestplate = mc::Items::DIAMOND_CHESTPLATE;
    ASSERT_NE(diamondChestplate, nullptr) << "DIAMOND_CHESTPLATE should be registered";

    ItemStack stack(diamondChestplate, 1);
    stack.addEnchantment("minecraft:protection", 4);

    // 保护 IV：EPF = level = 4
    // 对通用伤害（无特殊标志）
    i32 epf = EnchantmentHelper::getProtectionFactor(stack, 0);
    EXPECT_EQ(epf, 4);
}

TEST_F(EnchantmentHelperEPFTest, SingleProtectionItem_Fire)
{
    // 创建一个火焰保护 IV 的物品
    const mc::Item* diamondChestplate = mc::Items::DIAMOND_CHESTPLATE;
    ASSERT_NE(diamondChestplate, nullptr);

    ItemStack stack(diamondChestplate, 1);
    stack.addEnchantment("minecraft:fire_protection", 4);

    // 火焰保护 IV：对火焰伤害 EPF = level * 2 = 8
    i32 epf = EnchantmentHelper::getProtectionFactor(stack, DamageFlags::FIRE);
    EXPECT_EQ(epf, 8);

    // 对非火焰伤害无效
    epf = EnchantmentHelper::getProtectionFactor(stack, 0);
    EXPECT_EQ(epf, 0);
}

TEST_F(EnchantmentHelperEPFTest, SingleProtectionItem_Fall)
{
    // 创建一个摔落保护 IV 的物品（使用靴子）
    const mc::Item* diamondBoots = mc::Items::DIAMOND_BOOTS;
    ASSERT_NE(diamondBoots, nullptr);

    ItemStack stack(diamondBoots, 1);
    stack.addEnchantment("minecraft:feather_falling", 4);

    // 摔落保护 IV：对摔落伤害 EPF = level * 3 = 12
    i32 epf = EnchantmentHelper::getProtectionFactor(stack, DamageFlags::FALL);
    EXPECT_EQ(epf, 12);

    // 对非摔落伤害无效
    epf = EnchantmentHelper::getProtectionFactor(stack, 0);
    EXPECT_EQ(epf, 0);
}

TEST_F(EnchantmentHelperEPFTest, SingleProtectionItem_Explosion)
{
    // 创建一个爆炸保护 IV 的物品
    const mc::Item* diamondChestplate = mc::Items::DIAMOND_CHESTPLATE;
    ASSERT_NE(diamondChestplate, nullptr);

    ItemStack stack(diamondChestplate, 1);
    stack.addEnchantment("minecraft:blast_protection", 4);

    // 爆炸保护 IV：对爆炸伤害 EPF = level * 2 = 8
    i32 epf = EnchantmentHelper::getProtectionFactor(stack, DamageFlags::EXPLOSION);
    EXPECT_EQ(epf, 8);

    // 对非爆炸伤害无效
    epf = EnchantmentHelper::getProtectionFactor(stack, 0);
    EXPECT_EQ(epf, 0);
}

TEST_F(EnchantmentHelperEPFTest, SingleProtectionItem_Projectile)
{
    // 创建一个弹射物保护 IV 的物品
    const mc::Item* diamondChestplate = mc::Items::DIAMOND_CHESTPLATE;
    ASSERT_NE(diamondChestplate, nullptr);

    ItemStack stack(diamondChestplate, 1);
    stack.addEnchantment("minecraft:projectile_protection", 4);

    // 弹射物保护 IV：对弹射物伤害 EPF = level * 2 = 8
    i32 epf = EnchantmentHelper::getProtectionFactor(stack, DamageFlags::PROJECTILE);
    EXPECT_EQ(epf, 8);

    // 对非弹射物伤害无效
    epf = EnchantmentHelper::getProtectionFactor(stack, 0);
    EXPECT_EQ(epf, 0);
}

// ========== 多物品护甲 EPF 总和测试 ==========

TEST_F(EnchantmentHelperEPFTest, FullArmorProtection4)
{
    const mc::Item* diamondHelmet = mc::Items::DIAMOND_HELMET;
    const mc::Item* diamondChestplate = mc::Items::DIAMOND_CHESTPLATE;
    const mc::Item* diamondLeggings = mc::Items::DIAMOND_LEGGINGS;
    const mc::Item* diamondBoots = mc::Items::DIAMOND_BOOTS;
    ASSERT_NE(diamondHelmet, nullptr);
    ASSERT_NE(diamondChestplate, nullptr);
    ASSERT_NE(diamondLeggings, nullptr);
    ASSERT_NE(diamondBoots, nullptr);

    // 全套保护 IV 装备
    ItemStack helmet(diamondHelmet, 1);
    ItemStack chestplate(diamondChestplate, 1);
    ItemStack leggings(diamondLeggings, 1);
    ItemStack boots(diamondBoots, 1);

    helmet.addEnchantment("minecraft:protection", 4);
    chestplate.addEnchantment("minecraft:protection", 4);
    leggings.addEnchantment("minecraft:protection", 4);
    boots.addEnchantment("minecraft:protection", 4);

    std::array<const ItemStack*, 4> armorSlots = {&helmet, &chestplate, &leggings, &boots};

    // 总 EPF = 4 + 4 + 4 + 4 = 16
    i32 totalEPF = EnchantmentHelper::getTotalArmorProtection(armorSlots, 0);
    EXPECT_EQ(totalEPF, 16);
}

TEST_F(EnchantmentHelperEPFTest, FullArmorBlastProtection4)
{
    const mc::Item* diamondHelmet = mc::Items::DIAMOND_HELMET;
    const mc::Item* diamondChestplate = mc::Items::DIAMOND_CHESTPLATE;
    const mc::Item* diamondLeggings = mc::Items::DIAMOND_LEGGINGS;
    const mc::Item* diamondBoots = mc::Items::DIAMOND_BOOTS;
    ASSERT_NE(diamondHelmet, nullptr);

    // 全套爆炸保护 IV 装备
    ItemStack helmet(diamondHelmet, 1);
    ItemStack chestplate(diamondChestplate, 1);
    ItemStack leggings(diamondLeggings, 1);
    ItemStack boots(diamondBoots, 1);

    helmet.addEnchantment("minecraft:blast_protection", 4);
    chestplate.addEnchantment("minecraft:blast_protection", 4);
    leggings.addEnchantment("minecraft:blast_protection", 4);
    boots.addEnchantment("minecraft:blast_protection", 4);

    std::array<const ItemStack*, 4> armorSlots = {&helmet, &chestplate, &leggings, &boots};

    // 总 EPF = 8 + 8 + 8 + 8 = 32，但上限为 20
    i32 totalEPF = EnchantmentHelper::getTotalArmorProtection(armorSlots, DamageFlags::EXPLOSION);
    EXPECT_EQ(totalEPF, 20);
}

TEST_F(EnchantmentHelperEPFTest, MixedProtection)
{
    const mc::Item* diamondHelmet = mc::Items::DIAMOND_HELMET;
    const mc::Item* diamondChestplate = mc::Items::DIAMOND_CHESTPLATE;
    const mc::Item* diamondLeggings = mc::Items::DIAMOND_LEGGINGS;
    const mc::Item* diamondBoots = mc::Items::DIAMOND_BOOTS;
    ASSERT_NE(diamondHelmet, nullptr);

    // 混合配置：保护 IV 头盔 + 火焰保护 IV 胸甲 + 保护 IV 护腿 + 摔落保护 IV 靴子
    ItemStack helmet(diamondHelmet, 1);
    ItemStack chestplate(diamondChestplate, 1);
    ItemStack leggings(diamondLeggings, 1);
    ItemStack boots(diamondBoots, 1);

    helmet.addEnchantment("minecraft:protection", 4);
    chestplate.addEnchantment("minecraft:fire_protection", 4);
    leggings.addEnchantment("minecraft:protection", 4);
    boots.addEnchantment("minecraft:feather_falling", 4);

    std::array<const ItemStack*, 4> armorSlots = {&helmet, &chestplate, &leggings, &boots};

    // 对火焰伤害：
    // 头盔保护 IV = 4，胸甲火焰保护 IV = 8，护腿保护 IV = 4，靴子摔落保护对火焰无效 = 0
    // 总 EPF = 4 + 8 + 4 + 0 = 16
    i32 fireEPF = EnchantmentHelper::getTotalArmorProtection(armorSlots, DamageFlags::FIRE);
    EXPECT_EQ(fireEPF, 16);

    // 对摔落伤害：
    // 头盔保护 IV = 4，胸甲火焰保护对摔落无效 = 0，护腿保护 IV = 4，靴子摔落保护 IV = 12
    // 总 EPF = 4 + 0 + 4 + 12 = 20（达到上限）
    i32 fallEPF = EnchantmentHelper::getTotalArmorProtection(armorSlots, DamageFlags::FALL);
    EXPECT_EQ(fallEPF, 20);

    // 对通用伤害：
    // 保护 IV 提供 level = 4，其他特殊保护对通用伤害无效
    // 总 EPF = 4 + 0 + 4 + 0 = 8
    i32 genericEPF = EnchantmentHelper::getTotalArmorProtection(armorSlots, 0);
    EXPECT_EQ(genericEPF, 8);
}

TEST_F(EnchantmentHelperEPFTest, EmptyArmorSlots)
{
    // 空护甲槽位
    ItemStack empty;
    std::array<const ItemStack*, 4> armorSlots = {&empty, &empty, &empty, &empty};

    i32 totalEPF = EnchantmentHelper::getTotalArmorProtection(armorSlots, 0);
    EXPECT_EQ(totalEPF, 0);
}

TEST_F(EnchantmentHelperEPFTest, NullArmorSlots)
{
    // 空指针槽位
    std::array<const ItemStack*, 4> armorSlots = {nullptr, nullptr, nullptr, nullptr};

    i32 totalEPF = EnchantmentHelper::getTotalArmorProtection(armorSlots, 0);
    EXPECT_EQ(totalEPF, 0);
}

// ========== 保护等级测试 ==========

TEST_F(EnchantmentHelperEPFTest, ProtectionLevels)
{
    const mc::Item* diamondChestplate = mc::Items::DIAMOND_CHESTPLATE;
    ASSERT_NE(diamondChestplate, nullptr);

    // 保护 I: EPF = 1
    ItemStack prot1(diamondChestplate, 1);
    prot1.addEnchantment("minecraft:protection", 1);
    EXPECT_EQ(EnchantmentHelper::getProtectionFactor(prot1, 0), 1);

    // 保护 II: EPF = 2
    ItemStack prot2(diamondChestplate, 1);
    prot2.addEnchantment("minecraft:protection", 2);
    EXPECT_EQ(EnchantmentHelper::getProtectionFactor(prot2, 0), 2);

    // 保护 III: EPF = 3
    ItemStack prot3(diamondChestplate, 1);
    prot3.addEnchantment("minecraft:protection", 3);
    EXPECT_EQ(EnchantmentHelper::getProtectionFactor(prot3, 0), 3);

    // 保护 IV: EPF = 4
    ItemStack prot4(diamondChestplate, 1);
    prot4.addEnchantment("minecraft:protection", 4);
    EXPECT_EQ(EnchantmentHelper::getProtectionFactor(prot4, 0), 4);
}

// ========== 多附魔叠加测试 ==========

TEST_F(EnchantmentHelperEPFTest, MultipleEnchantmentsOnSameItem)
{
    const mc::Item* diamondChestplate = mc::Items::DIAMOND_CHESTPLATE;
    ASSERT_NE(diamondChestplate, nullptr);

    // 理论测试：同一物品上多个保护附魔（实际游戏中不兼容，但代码层面应该叠加）
    ItemStack stack(diamondChestplate, 1);
    stack.addEnchantment("minecraft:protection", 4);
    stack.addEnchantment("minecraft:fire_protection", 4);

    // 对通用伤害：保护 IV = 4，火焰保护对通用伤害无效
    i32 genericEPF = EnchantmentHelper::getProtectionFactor(stack, 0);
    EXPECT_EQ(genericEPF, 4);

    // 对火焰伤害：保护 IV = 4 + 火焰保护 IV = 4 + 8 = 12
    i32 fireEPF = EnchantmentHelper::getProtectionFactor(stack, DamageFlags::FIRE);
    EXPECT_EQ(fireEPF, 12);
}

// ========== 完整伤害计算测试 ==========

TEST_F(CombatRulesEPFTest, FullDamageCalculation)
{
    // 100 点伤害，EPF = 16 (64% 减伤)
    f32 damage = CombatRules::getDamageAfterMagicAbsorb(100.0f, 16.0f);
    EXPECT_FLOAT_EQ(damage, 36.0f); // 100 * (1 - 16/25) = 100 * 0.36 = 36

    // 100 点伤害，EPF = 20 (80% 减伤)
    damage = CombatRules::getDamageAfterMagicAbsorb(100.0f, 20.0f);
    EXPECT_FLOAT_EQ(damage, 20.0f); // 100 * (1 - 20/25) = 100 * 0.2 = 20
}

// ============================================================================
// CombatRules::getDamageAfterAbsorb 破甲 Breach 修正测试（任务 #311）
// ============================================================================
//
// 验证 getDamageAfterAbsorb(damage, armor, toughness, breachLevel) 四参数重载正确接入
// BreachEnchantment::getArmorEffectivenessModifier（每级 -0.15），对齐 vanilla
// CombatRules.getDamageAfterArmor（CombatRules.java:16-30）：
//   f  = 2 + toughness / 4
//   f1 = clamp(armor - damage / f, armor * 0.2, 20)   // effectiveArmor
//   f2 = f1 / 25                                       // armorRatio
//   f3 = clamp(f2 + breachModifier, 0, 1)              // Breach 修正后有效率
//   final = damage * (1 - f3)
//
// 任务 #311 修复前偏差：getDamageAfterAbsorb 三参数版无 Breach，主伤害管线
// LivingEntity::applyArmorCalculations 直接调三参数版，致重锤破甲附魔定义了但运行时
// 从未消费。本测试固定四参数版 Breach 数值，捕捉 Breach 修正回归。

TEST_F(CombatRulesEPFTest, BreachLevelZeroEqualsThreeParamVersion)
{
    // breachLevel=0 时四参数重载应等价于三参数版（无修正）
    f32 three = CombatRules::getDamageAfterAbsorb(100.0f, 20.0f, 0.0f);
    f32 four0 = CombatRules::getDamageAfterAbsorb(100.0f, 20.0f, 0.0f, 0);
    EXPECT_FLOAT_EQ(four0, three);
}

TEST_F(CombatRulesEPFTest, BreachHighDamageHighArmorCompletelyBypassesArmor)
{
    // 100 伤害，armor=20，toughness=0，无 Breach：
    //   f1 = clamp(20 - 100/2, 4, 20) = clamp(-30, 4, 20) = 4
    //   f2 = 4/25 = 0.16，final = 100 * 0.84 = 84
    EXPECT_FLOAT_EQ(CombatRules::getDamageAfterAbsorb(100.0f, 20.0f, 0.0f, 0), 84.0f);

    // Breach IV（-0.6）：f3 = clamp(0.16 - 0.6, 0, 1) = 0，final = 100 * 1.0 = 100
    // 破甲 IV 在高伤害低有效护甲场景完全破除护甲减伤
    EXPECT_FLOAT_EQ(CombatRules::getDamageAfterAbsorb(100.0f, 20.0f, 0.0f, 4), 100.0f);
}

TEST_F(CombatRulesEPFTest, BreachLowDamageHighArmorPartialBypass)
{
    // 10 伤害，armor=20，toughness=0，无 Breach：
    //   f1 = clamp(20 - 10/2, 4, 20) = clamp(15, 4, 20) = 15
    //   f2 = 15/25 = 0.6，final = 10 * 0.4 = 4.0
    EXPECT_FLOAT_EQ(CombatRules::getDamageAfterAbsorb(10.0f, 20.0f, 0.0f, 0), 4.0f);

    // Breach I（-0.15）：f3 = clamp(0.6 - 0.15, 0, 1) = 0.45，final = 10 * 0.55 = 5.5
    EXPECT_FLOAT_EQ(CombatRules::getDamageAfterAbsorb(10.0f, 20.0f, 0.0f, 1), 5.5f);

    // Breach II（-0.30）：f3 = clamp(0.6 - 0.30, 0, 1) = 0.30，final = 10 * 0.70 = 7.0
    EXPECT_FLOAT_EQ(CombatRules::getDamageAfterAbsorb(10.0f, 20.0f, 0.0f, 2), 7.0f);

    // Breach III（-0.45）：f3 = clamp(0.6 - 0.45, 0, 1) = 0.15，final = 10 * 0.85 = 8.5
    EXPECT_FLOAT_EQ(CombatRules::getDamageAfterAbsorb(10.0f, 20.0f, 0.0f, 3), 8.5f);

    // Breach IV（-0.60）：f3 = clamp(0.6 - 0.60, 0, 1) = 0.0，final = 10 * 1.0 = 10.0
    // 护甲有效率被夹到 0，伤害完全穿透
    EXPECT_FLOAT_EQ(CombatRules::getDamageAfterAbsorb(10.0f, 20.0f, 0.0f, 4), 10.0f);
}

TEST_F(CombatRulesEPFTest, BreachModifierClampedAtMostOne)
{
    // armor=0 时无 Breach：f1 = clamp(0 - damage/f, 0, 20)
    //   damage=10, f=2 → f1 = clamp(-5, 0, 20) = 0 → f2=0 → final = 10（armor=0 本就无减伤）
    EXPECT_FLOAT_EQ(CombatRules::getDamageAfterAbsorb(10.0f, 0.0f, 0.0f, 0), 10.0f);
    // armor=0 + Breach IV：f3 = clamp(0 - 0.6, 0, 1) = 0 → final = 10（不会因负值反向增伤）
    EXPECT_FLOAT_EQ(CombatRules::getDamageAfterAbsorb(10.0f, 0.0f, 0.0f, 4), 10.0f);
}

// ============================================================================
// EnchantmentHelper::getBreachLevel 武器查询测试（任务 #311）
// ============================================================================

TEST_F(EnchantmentHelperEPFTest, GetBreachLevelFromEnchantedMace)
{
    // 重锤施加 Breach IV，getBreachLevel 应返回 4
    const mc::Item* mace = mc::Items::MACE;
    ASSERT_NE(mace, nullptr) << "MACE should be registered";

    ItemStack maceStack(mace, 1);
    maceStack.addEnchantment("minecraft:breach", 4);
    EXPECT_EQ(EnchantmentHelper::getBreachLevel(maceStack), 4);
}

TEST_F(EnchantmentHelperEPFTest, GetBreachLevelNoEnchantmentReturnsZero)
{
    const mc::Item* mace = mc::Items::MACE;
    ASSERT_NE(mace, nullptr);

    ItemStack maceStack(mace, 1); // 无附魔
    EXPECT_EQ(EnchantmentHelper::getBreachLevel(maceStack), 0);
}

TEST_F(EnchantmentHelperEPFTest, GetBreachLevelEmptyStackReturnsZero)
{
    ItemStack empty;
    EXPECT_EQ(EnchantmentHelper::getBreachLevel(empty), 0);
}
