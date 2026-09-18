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
 * LIABILITY IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

// BYPASSES_ENCHANTMENTS 标签运行时查询测试。
//
// 验证 LivingEntity::applyPotionDamageCalculations（LivingEntity.cpp:468）对 BYPASSES_ENCHANTMENTS
// 标签伤害源（成员集 = {sonic_boom}，DamageTypeTags.cpp:555-557）跳过附魔保护减伤，但对齐 vanilla
// LivingEntity.getDamageAfterMagicAbsorb:1821-1860 的精确语义：抗性药水减免在 BYPASSES_ENCHANTMENTS
// 早返回之前生效（line 1825），故 sonic_boom 仍受抗性药水减免，仅跳过附魔保护（line 1843）。
//
// 此前缺陷：applyPotionDamageCalculations 附魔保护分支仅用 source.isDamageAbsolute() 门控
// （LivingEntity.cpp:489），而 isDamageAbsolute() 对 sonic_boom 返回 false（sonicBoom() 工厂创建
// IndirectEntityDamageSource，该类未 override isDamageAbsolute，继承基类 DamageSource::isDamageAbsolute
// 返回 false，DamageSource.hpp:238；EnvironmentalDamage 的 isDamageAbsolute 仅对 OutOfWorld/Starve
// 返回 true，DamageSource.hpp:315）。因此 sonic_boom 错误进入附魔保护减伤分支，被保护附魔错误减免，
// 偏离 vanilla（vanilla 中监守者音爆设计为无视护甲与附魔保护，但仍受抗性药水减免）。
//
// 修复：附魔保护分支追加 !source.is(DamageTypeTags::BYPASSES_ENCHANTMENTS()) 门控，对齐 vanilla
// getDamageAfterMagicAbsorb:1843。
//
// 测试设计（三例交叉验证，TestLivingEntity 穿全套保护 IV 护甲 EPF=16）：
//   - SonicBoomBypassesEnchantmentProtection：sonic_boom 返回原值（不被附魔减免）。
//   - MagicReducedByEnchantmentProtection：magic 返回 10*(1-16/25)=3.6（被附魔减免，对照证明附魔本身
//     生效，排除"附魔未生效致 sonic_boom 也不减免"的假通过）。
//   - SonicBoomStillReducedByResistance：sonic_boom + 抗性 I 返回 10*0.8=8.0（抗性药水减免生效，
//     证明修复精确对齐 vanilla——sonic_boom 仅跳过附魔，保留抗性药水）。
//
// EPF 计算：全套保护 IV（头盔/胸甲/护腿/靴子各 protection IV），ProtectionEnchantment::Type::All
// 对所有伤害类型每级 EPF=level（ProtectionEnchantment.cpp:92），4 件 × level 4 = EPF 16（上限 20）。
// CombatRules::getDamageAfterMagicAbsorb(damage, 16) = damage * (1 - 16/25) = damage * 0.36。
// 抗性 I：CombatRules::getDamageAfterResistance(damage, 1) = damage * (1 - 1*0.2) = damage * 0.8。
//
// Ref: vanilla LivingEntity.java:1821-1860（getDamageAfterMagicAbsorb）
// Ref: LivingEntity.cpp:468（applyPotionDamageCalculations）/ :489（BYPASSES_ENCHANTMENTS 门控）
// Ref: DamageTypeTags.cpp:555（BYPASSES_ENCHANTMENTS 成员 = {SonicBoom}）
// Ref: DamageSource.hpp:1180（sonicBoom 工厂，IndirectEntityDamageSource + setBypassesArmor）

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/combat/CombatRules.hpp"
#include "common/entity/core/EquipmentSlot.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"

using namespace mc;

namespace {

// 测试用 LivingEntity 子类：暴露 protected applyPotionDamageCalculations 到 public 以便单元测试
// 直接调用验证减伤计算（参照 tests/entity/LivingEntityTests.cpp:140 TestLivingEntity 范式 +
// tests/common/entity/core/StepSoundTest.cpp:111 using 暴露 protected 方法范式）。
// 走基类 EquipmentComponent 链路（LivingEntity 构造 line 152 attach EquipmentComponent），
// setEquipment/getArmorSlots 数据源一致（非 Player 的 PlayerInventory 异源问题）。
class TestLivingEntity : public LivingEntity {
public:
    TestLivingEntity()
        : LivingEntity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }

    // 暴露 protected 方法供测试直接调用减伤计算。
    using LivingEntity::applyPotionDamageCalculations;
};

} // namespace

// ============================================================================
// BYPASSES_ENCHANTMENTS 测试
// ============================================================================

class BypassesEnchantmentsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 附魔注册表与物品初始化（参照 CombatRulesEPFTest 范式），使 addEnchantment("minecraft:protection", 4)
        // 能解析附魔类型。
        item::enchant::EnchantmentRegistry::clear();
        item::enchant::EnchantmentRegistry::initialize();
        Items::initialize();
        // 伤害类型标签初始化（进程级单例，s_initialized 守卫幂等）。BYPASSES_ENCHANTMENTS 标签成员集
        // （= {SonicBoom}）在 initialize() 中通过 addAll 注册（DamageTypeTags.cpp:555）。未初始化时标签
        // 成员集为空，source.is(BYPASSES_ENCHANTMENTS) 恒返 false，修复门控形同虚设——这正是首轮测试
        // sonic_boom 返回 3.6（被附魔错误减免）的根因。真实游戏启动时会调 initialize()，测试须模拟之。
        // 标签无 clear/reset，进程级常驻，初始化只会让其他测试更贴近 vanilla，不破坏。
        DamageTypeTags::initialize();
    }

    void TearDown() override { item::enchant::EnchantmentRegistry::clear(); }

    // 给实体穿上全套保护 IV 护甲（头盔/胸甲/护腿/靴子各 protection IV，EPF=16）。
    void equipFullProtectionIV(TestLivingEntity& entity) const
    {
        // 钻石护甲四件套（任何可附魔护甲均可，本测试只关心保护附魔 EPF，护甲值本身不影响附魔减伤）。
        ItemStack helmet(Items::DIAMOND_HELMET, 1);
        helmet.addEnchantment("minecraft:protection", 4);
        ItemStack chestplate(Items::DIAMOND_CHESTPLATE, 1);
        chestplate.addEnchantment("minecraft:protection", 4);
        ItemStack leggings(Items::DIAMOND_LEGGINGS, 1);
        leggings.addEnchantment("minecraft:protection", 4);
        ItemStack boots(Items::DIAMOND_BOOTS, 1);
        boots.addEnchantment("minecraft:protection", 4);

        entity.setEquipment(EquipmentSlot::Head, helmet);
        entity.setEquipment(EquipmentSlot::Chest, chestplate);
        entity.setEquipment(EquipmentSlot::Legs, leggings);
        entity.setEquipment(EquipmentSlot::Feet, boots);
    }
};

// sonic_boom 伤害源不被保护附魔减免（BYPASSES_ENCHANTMENTS 门控生效）。
//
// 穿全套保护 IV（EPF=16）的实体受 sonic_boom 10 点伤害，applyPotionDamageCalculations 应返回原值 10.0
// （跳过附魔保护减伤）。修复前此处会返回 10*(1-16/25)=3.6（错误被附魔减免）。
TEST_F(BypassesEnchantmentsTest, SonicBoomBypassesEnchantmentProtection)
{
    TestLivingEntity entity;
    equipFullProtectionIV(entity);

    // sonicBoom(guardian, target) 创建 IndirectEntityDamageSource(SonicBoom).setBypassesArmor()。
    // guardian/target 用实体自身即可——applyPotionDamageCalculations 不查 source 实体，只查伤害类型标签。
    auto source = DamageSources::sonicBoom(&entity, &entity);

    EXPECT_FLOAT_EQ(entity.applyPotionDamageCalculations(source, 10.0f), 10.0f);
}

// 对照：magic 伤害源被保护附魔减免（证明附魔本身生效，排除假通过）。
//
// 同一穿全套保护 IV 的实体受 magic 10 点伤害，应返回 10*(1-16/25)=3.6。若本测试失败（返回 10）说明
// 附魔未生效（EPF=0），则 SonicBoomBypassesEnchantmentProtection 的"返回 10"是假通过（附魔根本没起作用，
// 与 BYPASSES_ENCHANTMENTS 门控无关）。
TEST_F(BypassesEnchantmentsTest, MagicReducedByEnchantmentProtection)
{
    TestLivingEntity entity;
    equipFullProtectionIV(entity);

    auto source = DamageSources::magic();

    EXPECT_FLOAT_EQ(entity.applyPotionDamageCalculations(source, 10.0f), 3.6f);
}

// sonic_boom 仍受抗性药水减免（验证修复精确对齐 vanilla：仅跳过附魔，保留抗性药水）。
//
// vanilla getDamageAfterMagicAbsorb:1825 抗性药水减免在 BYPASSES_ENCHANTMENTS 早返回（line 1843）之前
// 生效，故 sonic_boom 受抗性药水减免。穿全套保护 IV + 抗性 I 的实体受 sonic_boom 10 点伤害：
// 抗性 I 先减免 10*0.8=8.0，附魔保护被 BYPASSES_ENCHANTMENTS 跳过，最终 8.0。
// 修复前若 BYPASSES_ENCHANTMENTS 门控缺失，抗性 I 后会再被附魔减免 8.0*0.36=2.88（错误）。
// 若误把抗性药水也跳过（如错用 BYPASSES_EFFECTS 早返回），则返回 10.0（错误）。
TEST_F(BypassesEnchantmentsTest, SonicBoomStillReducedByResistance)
{
    TestLivingEntity entity;
    equipFullProtectionIV(entity);

    // 施加抗性提升 I（amplifier=0 即等级 I，getEffectLevel 返回 1）。
    entity::effect::EffectInstance resistance(entity::effect::EffectType::Resistance,
        3600, // 持续时间（tick），远超测试时长
        0,    // amplifier=0 → 抗性 I
        false,
        true,
        true);
    entity.addEffect(std::move(resistance));
    ASSERT_TRUE(entity.hasEffect(entity::effect::EffectType::Resistance));

    auto source = DamageSources::sonicBoom(&entity, &entity);

    // 抗性 I：10 * (1 - 1*0.2) = 8.0；附魔保护被跳过。
    EXPECT_FLOAT_EQ(entity.applyPotionDamageCalculations(source, 10.0f), 8.0f);
}
