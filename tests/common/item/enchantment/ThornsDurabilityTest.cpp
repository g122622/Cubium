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

// 荆棘附魔耐久消耗与反伤数值测试（对齐 vanilla 1.21.11 THORNS）。
//
// vanilla 1.21.11 THORNS（Enchantments.java:337-346）POST_ATTACK(VICTIM→ATTACKER)，
// AllOf.entityEffects(DamageEntity, ChangeItemDamage)，概率 perLevel 0.15：
//   - DamageEntity(constant 1.0, constant 5.0, THORNS)：触发时对攻击者造成 [1.0, 5.0) 随机荆棘伤害
//     （Mth.randomBetween(random, 1.0F, 5.0F)，与等级无关）。
//   - ChangeItemDamage(constant 2.0)：触发时使触发荆棘的护甲扣 2 耐久
//     （ChangeItemDamage.java:25-26 itemstack.hurtAndBreak((int)2.0, ...)）。
//
// Cubium 链路：
//   victim.hurt(mobAttack(attacker)) → LivingEntity::actuallyHurt 步骤 9（LivingEntity.cpp:446）
//   → EnchantmentHelper::applyThornsEnchantments(victim, attacker)（EnchantmentHelper.cpp:358）
//   → 按 [Head,Chest,Legs,Feet] 遍历 victim 护甲，每件带荆棘的护甲调
//   → ThornsEnchantment::onUserHurt(victim, attacker, armor, slot, level)（ThornsEnchantment.cpp:53）
//   → shouldTrigger(level, rng)（perLevel 0.15 概率门控）触发后：
//       1. attacker.hurt(thorns(victim), getThornsDamage(rng))（反伤 [1.0,5.0)）
//       2. LivingEntity::hurtAndBreak(armor, 2, victim, slot)（护甲扣 2 耐久）
//
// 此前缺陷（任务 #276 修复）：
//   1. getThornsDamage 用老版本公式 level>10?level-10:1+nextInt(4)（1-4 整数 + 多余 level>10 分支），
//      偏离 vanilla [1.0,5.0) 随机浮点。修复：改 random.nextFloat(1.0f, 5.0f)。
//   2. onUserHurt 耐久消耗未接入（注释承认"需要在调用方处理"但调用方无装备引用）。
//      修复：onUserHurt 签名加 ItemStack& enchantedItem + EquipmentSlot slot，触发时调
//      hurtAndBreak(enchantedItem, 2, ...) 自扣 2 耐久（对齐 ChangeItemDamage）。
//
// 测试设计（确定性 + 概率性结合）：
//   - ThornsReflectsDamageInRange：触发荆棘后 attacker 受反伤 ∈ [1.0, 5.0)（采样多次验证范围）。
//     用高等级 thorns（level=10，概率 100%+ 确保每次触发）消除概率不确定性，专注验证反伤数值范围。
//   - ThornsConsumesArmorDurability：触发荆棘后 victim 胸甲耐久损耗 +2（每次触发扣 2）。
//     同样用 level=10 确保触发，验证 ChangeItemDamage(constant 2.0) 接入。
//   - ThornsDoesNotConsumeDurabilityWhenNotTriggered：level=1 单次攻击概率 15%，不强制触发。
//     此测试改为验证"未穿荆棘护甲时无耐久消耗 + 无反伤"（确定性负向对照）。
//
// 注：shouldTrigger 用 victim.id() ^ ticksExisted() 作种子。TestLivingEntity 静态构造 ticksExisted=0，
// id 固定，seed 确定。但 shouldTrigger 内 random.nextFloat() < level*0.15，level=10 时 1.0 < 1.5 恒真
// （nextFloat ∈ [0,1)），故 level=10 每次必触发，消除概率不确定性。
//
// Ref: Enchantments.java:337-346（THORNS 定义）
// Ref: DamageEntity.java:28（Mth.randomBetween [1.0,5.0)）
// Ref: ChangeItemDamage.java:25-26（hurtAndBreak((int)amount)）
// Ref: ThornsEnchantment.cpp:53（onUserHurt 反伤 + 耐久消耗）
// Ref: EnchantmentHelper.cpp:358（applyThornsEnchantments 遍历护甲）

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/EquipmentSlot.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/item/enchantment/enchantments/AllEnchantments.hpp"

using namespace mc;

namespace {

// 测试用 LivingEntity 子类：作 victim（穿荆棘护甲）/ attacker（受反伤）。参照 GuardianThornsTest
// 的 TestLivingEntity 范式。actuallyHurt 走基类 LivingEntity::actuallyHurt（含步骤 9 荆棘分支）。
class TestLivingEntity : public LivingEntity {
public:
    TestLivingEntity()
        : LivingEntity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

// BaseTestWorld 默认构造为 protected，派生公开（吸收 actuallyHurt 的 playHurtSound）。
class TestWorld final : public mc::test::BaseTestWorld {
public:
    TestWorld() = default;
};

} // namespace

class ThornsDurabilityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        item::enchant::EnchantmentRegistry::clear();
        item::enchant::EnchantmentRegistry::initialize();
        Items::initialize();
    }

    void TearDown() override { item::enchant::EnchantmentRegistry::clear(); }

    TestWorld m_world;
};

// 触发荆棘后攻击者受反伤（验证 DamageEntity 反伤作用于 attacker，端到端链路接通）。
//
// victim 穿 thorns level=10 钻石胸甲（level=10 时 shouldTrigger 恒真，消除概率不确定性）。
// attacker 以 mobAttack 攻击 victim → victim.actuallyHurt 步骤 9 → applyThornsEnchantments →
// onUserHurt 触发 → attacker 受反伤 getThornsDamage(rng) ∈ [1.0, 5.0)。
//
// 注：onUserHurt 内部 rng seed = victim.id() ^ victim.ticksExisted()，TestLivingEntity 静态构造
// ticksExisted=0、id 固定，seed 确定，故反伤为确定值（非采样分布）。本测试只验证反伤发生在
// [1.0, 5.0) 区间（端到端链路接通），数值范围的统计分布验证由 EnchantmentCallbackTest.GetThornsDamage
// 用独立 rng 采样覆盖（不依赖 onUserHurt 内部 seed）。
//
// 修复前 onUserHurt 反伤用老公式 1-4 整数，本测试 EXPECT_GE(reflected, 1.0f) 仍过，但配合
// ThornsConsumesArmorDurability 共同验证 onUserHurt 完整触发（反伤+耐久）。
TEST_F(ThornsDurabilityTest, ThornsReflectsDamageToAttacker)
{
    TestLivingEntity victim;
    victim.setWorld(&m_world);
    victim.setHealth(victim.maxHealth());

    // 穿 thorns level=10 钻石胸甲（level=10 确保每次触发）
    ItemStack chest(Items::DIAMOND_CHESTPLATE, 1);
    chest.addEnchantment("minecraft:thorns", 10);
    victim.setEquipment(EquipmentSlot::Chest, chest);

    TestLivingEntity attacker;
    attacker.setWorld(&m_world);
    attacker.setHealth(attacker.maxHealth());

    auto source = DamageSources::mobAttack(&attacker);
    victim.hurt(source, 1.0f);

    f32 reflected = attacker.maxHealth() - attacker.health();
    EXPECT_GE(reflected, 1.0f) << "thorns should reflect >= 1.0 damage (vanilla DamageEntity min)";
    EXPECT_LT(reflected, 5.0f) << "thorns should reflect < 5.0 damage (vanilla DamageEntity max exclusive)";
}

// 触发荆棘后 victim 胸甲耐久损耗 +2（验证 ChangeItemDamage(constant 2.0) 接入）。
//
// victim 穿 thorns level=10 钻石胸甲（初始 damage=0），attacker 攻击 victim 触发荆棘 →
// onUserHurt 调 hurtAndBreak(chest, 2, victim, Chest) → 胸甲 damage 0→2。
//
// 修复前 onUserHurt 不扣耐久（注释承认未接入），chest.getDamage() 仍 0 → 本测试 FAIL。
TEST_F(ThornsDurabilityTest, ThornsConsumesArmorDurability)
{
    TestLivingEntity victim;
    victim.setWorld(&m_world);
    victim.setHealth(victim.maxHealth());

    ItemStack chest(Items::DIAMOND_CHESTPLATE, 1);
    chest.addEnchantment("minecraft:thorns", 10);
    victim.setEquipment(EquipmentSlot::Chest, chest);

    EXPECT_EQ(victim.getEquipment(EquipmentSlot::Chest).getDamage(), 0);

    TestLivingEntity attacker;
    attacker.setWorld(&m_world);
    attacker.setHealth(attacker.maxHealth());

    auto source = DamageSources::mobAttack(&attacker);
    victim.hurt(source, 1.0f);

    // 触发荆棘后胸甲耐久损耗 +2（对齐 vanilla ChangeItemDamage(constant 2.0)）
    EXPECT_EQ(victim.getEquipment(EquipmentSlot::Chest).getDamage(), 2)
        << "thorns trigger should consume 2 durability from the triggering armor "
        << "(vanilla ChangeItemDamage constant 2.0)";
}

// 无荆棘护甲时无耐久消耗 + 无反伤（确定性负向对照，防上述测试假通过）。
//
// victim 穿无附魔钻石胸甲，attacker 攻击 victim → applyThornsEnchantments 遍历护甲无荆棘 →
// 不触发 onUserHurt → attacker 无反伤（HP 不变）+ 胸甲耐久不变。
//
// 若 applyThornsEnchantments 误对无附魔护甲触发（如 getEnchantmentLevel 判定失效），attacker 会
// 受反伤 / 胸甲扣耐久 → 本测试 FAIL。与上述正向上测试交叉验证：穿荆棘反伤+扣耐久 vs 无荆棘无变化。
TEST_F(ThornsDurabilityTest, NoThornsNoDurabilityConsumptionNoReflection)
{
    TestLivingEntity victim;
    victim.setWorld(&m_world);
    victim.setHealth(victim.maxHealth());

    ItemStack chest(Items::DIAMOND_CHESTPLATE, 1); // 无附魔
    victim.setEquipment(EquipmentSlot::Chest, chest);

    TestLivingEntity attacker;
    attacker.setWorld(&m_world);
    attacker.setHealth(attacker.maxHealth());

    auto source = DamageSources::mobAttack(&attacker);
    victim.hurt(source, 1.0f);

    // 无荆棘：attacker 无反伤（HP 不变），胸甲耐久不变
    EXPECT_FLOAT_EQ(attacker.health(), attacker.maxHealth());
    EXPECT_EQ(victim.getEquipment(EquipmentSlot::Chest).getDamage(), 0);
}
