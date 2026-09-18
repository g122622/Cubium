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

// BYPASSES_EFFECTS 标签运行时查询测试。
//
// 验证 LivingEntity::applyPotionDamageCalculations（LivingEntity.cpp）对齐 vanilla
// LivingEntity.getDamageAfterMagicAbsorb:1822-1823：
//   if (source.is(DamageTypeTags.BYPASSES_EFFECTS)) { return damage; }
// 即 BYPASSES_EFFECTS 标签伤害（成员={Starve}）跳过抗性药水与附魔保护减伤，直接返回原值。
// 饥饿伤害不应被抗性药水减免。
//
// 此前缺陷：Cubium applyPotionDamageCalculations 抗性门控用 !source.bypassesInvulnerability()
// （=OutOfWorld+GenericKill），漏 Starve；附魔门控用 !source.isDamageAbsolute()（Starve=true 跳过附魔，
// 此部分正确）。结果饥饿伤害被抗性药水错误减免——偏离 vanilla。
//
// 修复：函数开头加 source.is(BYPASSES_EFFECTS) 提前返回原值。
//
// 测试设计（两例交叉验证）：
//   - StarveBypassesResistance：实体有 Resistance I（减伤20%），actuallyHurt(starve, 4.0)
//     → BYPASSES_EFFECTS 门控不减伤，health 降 4.0。
//   - NormalAttackReducedByResistance：对照，实体有 Resistance I，actuallyHurt(mobAttack, 4.0)
//     → 抗性减伤 20%，health 降 3.2（4.0*0.8）。证明抗性对非标签伤害仍生效（门控仅挡 Starve）。
//
// 注：TestLivingEntity 无护甲（护甲减伤0）、无保护附魔（EPF=0），排除护甲/附魔干扰，仅观察抗性。
// starve 是 isDamageAbsolute+bypassesArmor，护甲/附魔本就不减。BaseTestWorld 吸收 hurtSound。
// actuallyHurt 是 public virtual（LivingEntity.hpp:207），直接调避开 hurt 入口的 isInvulnerableTo/
// 无敌帧门控，精确测 applyPotionDamageCalculations 减伤。
//
// Ref: vanilla LivingEntity.java:1822-1823（getDamageAfterMagicAbsorb BYPASSES_EFFECTS 门控）
// Ref: LivingEntity.cpp（applyPotionDamageCalculations BYPASSES_EFFECTS 提前返回）
// Ref: DamageTypeTags.cpp:550（BYPASSES_EFFECTS={Starve}）

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/combat/CombatRules.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/item/Items.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"

using namespace mc;

namespace {

class TestLivingEntity : public LivingEntity {
public:
    TestLivingEntity()
        : LivingEntity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

class TestWorld final : public mc::test::BaseTestWorld {
public:
    TestWorld() = default;
};

} // namespace

class BypassesEffectsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        item::enchant::EnchantmentRegistry::clear();
        item::enchant::EnchantmentRegistry::initialize();
        Items::initialize();
        DamageTypeTags::initialize();
    }

    void TearDown() override { item::enchant::EnchantmentRegistry::clear(); }

    TestWorld m_world;
};

// 饥饿伤害（BYPASSES_EFFECTS）不被抗性药水减免。
TEST_F(BypassesEffectsTest, StarveBypassesResistance)
{
    TestLivingEntity entity;
    entity.setWorld(&m_world);
    entity.setHealth(20.0f);

    // Resistance I（amplifier=0，减伤 20%）
    entity.addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Resistance, 200, 0));
    ASSERT_TRUE(entity.hasEffect(entity::effect::EffectType::Resistance));
    ASSERT_EQ(entity.getEffectLevel(entity::effect::EffectType::Resistance), 1);

    auto source = DamageSources::starve(); // Starve 在 BYPASSES_EFFECTS
    entity.actuallyHurt(source, 4.0f);

    // BYPASSES_EFFECTS 门控：抗性不减伤，health 降 4.0（20→16）
    EXPECT_FLOAT_EQ(entity.health(), 16.0f);
}

// 普通伤害（非 BYPASSES_EFFECTS）被抗性药水减免（对照）。
TEST_F(BypassesEffectsTest, NormalAttackReducedByResistance)
{
    TestLivingEntity entity;
    entity.setWorld(&m_world);
    entity.setHealth(20.0f);

    entity.addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Resistance, 200, 0));
    ASSERT_EQ(entity.getEffectLevel(entity::effect::EffectType::Resistance), 1);

    TestLivingEntity attacker;
    attacker.setWorld(&m_world);

    auto source = DamageSources::mobAttack(&attacker); // MobAttack 不在 BYPASSES_EFFECTS
    entity.actuallyHurt(source, 4.0f);

    // 抗性 I 减伤 20%：4.0*0.8=3.2，health 降 3.2（20→16.8）
    EXPECT_FLOAT_EQ(entity.health(), 20.0f - 4.0f * 0.8f);
}
