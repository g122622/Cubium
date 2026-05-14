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
