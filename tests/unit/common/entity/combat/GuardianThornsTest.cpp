/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

// AVOIDS_GUARDIAN_THORNS 标签运行时查询测试。
//
// 验证 GuardianEntity::hurt（GuardianEntity.cpp）对齐 vanilla Guardian.hurtServer:314-326：
//   if (!isMoving() && !source.is(AVOIDS_GUARDIAN_THORNS) && !source.is(THORNS)
//       && source.getDirectEntity() instanceof LivingEntity)
//       directEntity.hurtServer(level, damageSources().thorns(this), 2.0F);
// 即守卫者非移动状态、伤害源不属于 AVOIDS_GUARDIAN_THORNS/THORNS、且直接来源是 LivingEntity 时，
// 对直接攻击者造成 2.0 荆棘反伤。
//
// 标签成员集（DamageTypeTags.cpp:794-802，含 #is_explosion 子标签）：
//   AVOIDS_GUARDIAN_THORNS = {Magic, Thorns, Fireworks, Explosion, ExplosionPlayer, BadRespawnPoint}
// 即守卫者对魔法/荆棘/爆炸伤害不反伤（这些是远程或反伤类来源），仅对近战等直接来源反伤。
//
// 此前缺陷：Cubium GuardianEntity 无 hurt override，守卫者荆棘反伤整体未实现（被近战攻击时攻击者
// 不受反伤），偏离 vanilla。
//
// 修复：override hurt，复制 vanilla hurtServer 开头荆棘分支。荆棘伤害用 DamageSources::thorns(this)
// （this=守卫者为 owner/causer），type=Thorns，会被反伤判定自身的 !source.is(THORNS)（Cubium 用
// source.type()!=DamageType::Thorns）门控挡住，反伤链不递归（对齐 vanilla 防无限循环）。
//
// 测试设计（三例精确交叉验证，TestLivingEntity 作攻击者观察反伤后血量）：
//   - MeleeAttackTriggersThorns：mobAttack（type=MobAttack 不在标签，directSource=attacker 是
//     LivingEntity，guardian 静止）→ 反伤 attacker 2.0（health 20→18）。基线证明荆棘链路工作。
//   - ExplosionDoesNotTriggerThorns：explosion(attacker, attacker)（type=Explosion 在
//     AVOIDS_GUARDIAN_THORNS，directSource=attacker 是 LivingEntity）→ 标签门控挡住不反伤
//     （attacker health 仍 20）。精确证明标签门控生效（非 directSource=nullptr 的混淆）。
//   - ThornsDoesNotRecurse：thorns(attacker)（type=Thorns 在标签 + 显式 type!=Thorns 门控，
//     directSource=attacker）→ 不反伤（attacker health 仍 20）。证明荆棘不递归。
//
// 注：guardian maxHealth=30，受 5 点伤害不死（30→25）；attacker maxHealth=20，受 2.0 反伤不死
// （20→18）。guardian 静态构造（无 AI tick 驱动移动）isMoving() 返 false（moveController 无
// setMoveTo），符合 vanilla 静止场景语义。actuallyHurt 路径用 BaseTestWorld 吸收 playSound。
//
// Ref: vanilla Guardian.java:314-326（hurtServer 荆棘分支）
// Ref: vanilla Guardian.java:101-103（isMoving）/ :451,482-485（GuardianMoveControl 设 DATA_ID_MOVING）
// Ref: GuardianEntity.cpp（hurt override + isMoving 近似）
// Ref: DamageTypeTags.cpp:794（AVOIDS_GUARDIAN_THORNS 成员集）
// Ref: DamageSource.hpp:916（DamageSources::thorns 工厂）

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/entity/entities/monster/ocean/GuardianEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"

using namespace mc;

namespace {

// 测试用 LivingEntity 子类：作攻击者，受守卫者荆棘反伤后观察血量。参照 NoAngerTest.cpp 的
// TestLivingEntity 范式。actuallyHurt 在 LivingEntity.hpp:207 为 public virtual，hurt 走基类
// LivingEntity::hurt（无荆棘 override，不反伤，无递归）。
class TestLivingEntity : public LivingEntity {
public:
    TestLivingEntity()
        : LivingEntity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

// BaseTestWorld 的默认构造函数为 protected（TestWorldHelper.hpp:158），无法作为测试 fixture 成员
// 直接构造。派生并公开默认构造（playSound 默认空实现吸收 actuallyHurt 的 playHurtSound）。
class TestWorld final : public mc::test::BaseTestWorld {
public:
    TestWorld() = default;
};

} // namespace

// ============================================================================
// GuardianEntity 荆棘反伤（AVOIDS_GUARDIAN_THORNS）测试
// ============================================================================

class GuardianThornsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 附魔注册表初始化使 EnchantmentRegistry::get("minecraft:thorns") 可解析（guardian/attacker
        // 无附魔护甲，actuallyHurt 步骤9 荆棘附魔分支早退，但初始化避免空表查询异常）。
        item::enchant::EnchantmentRegistry::clear();
        item::enchant::EnchantmentRegistry::initialize();
        Items::initialize();
        // 伤害类型标签初始化（进程级幂等）。AVOIDS_GUARDIAN_THORNS 成员集在 initialize() 注册
        // （DamageTypeTags.cpp:794）。未初始化时标签成员集为空，source.is(AVOIDS_GUARDIAN_THORNS)
        // 恒返 false，荆棘门控形同虚设——这正是缺陷状态下任何伤害都反伤的根因（若实现存在）。
        DamageTypeTags::initialize();
    }

    void TearDown() override { item::enchant::EnchantmentRegistry::clear(); }

    TestWorld m_world;
};

// 近战攻击守卫者触发荆棘反伤（基线，证明荆棘链路工作）。
//
// guardian 受 attacker 的 mobAttack 5 点伤害：type=MobAttack 不在 AVOIDS_GUARDIAN_THORNS，
// directSource=attacker 是 LivingEntity，guardian 静止（isMoving=false）→ 对 attacker 反伤 2.0
// （thorns(guardian)）。attacker health 20→18。若本测试失败（attacker health 仍 20）说明荆棘
// 反伤链路本身损坏。
TEST_F(GuardianThornsTest, MeleeAttackTriggersThorns)
{
    GuardianEntity guardian(EntityInstanceId(2), mc::test::testEcsRegistry());
    guardian.setWorld(&m_world);
    guardian.setHealth(guardian.maxHealth());

    TestLivingEntity attacker;
    attacker.setWorld(&m_world);

    auto source = DamageSources::mobAttack(&attacker);
    guardian.hurt(source, 5.0f);

    // attacker 受 2.0 荆棘反伤（health 20→18）
    EXPECT_FLOAT_EQ(attacker.health(), 18.0f);
}

// 爆炸伤害不触发荆棘反伤（AVOIDS_GUARDIAN_THORNS 标签门控生效）。
//
// guardian 受 explosion(attacker, attacker) 5 点伤害：type=Explosion 在 AVOIDS_GUARDIAN_THORNS
// （#is_explosion 子标签成员），directSource=attacker 是 LivingEntity。标签门控挡住 → 不反伤
// （attacker health 仍 20）。此例精确证明标签门控生效——directSource 是 LivingEntity 满足反伤
// 的实体条件，仅因 type=Explosion 在标签而不反伤，排除"directSource=nullptr 才不反伤"的混淆。
TEST_F(GuardianThornsTest, ExplosionDoesNotTriggerThorns)
{
    GuardianEntity guardian(EntityInstanceId(2), mc::test::testEcsRegistry());
    guardian.setWorld(&m_world);
    guardian.setHealth(guardian.maxHealth());

    TestLivingEntity attacker;
    attacker.setWorld(&m_world);

    auto source = DamageSources::explosion(&attacker, &attacker);
    guardian.hurt(source, 5.0f);

    // 不反伤（attacker health 仍 20）
    EXPECT_FLOAT_EQ(attacker.health(), 20.0f);
}

// 荆棘伤害不触发荆棘反伤（防递归，THORNS 门控生效）。
//
// guardian 受 thorns(attacker) 5 点伤害：type=Thorns 在 AVOIDS_GUARDIAN_THORNS 且显式
// source.type()!=DamageType::Thorns 门控，directSource=attacker 是 LivingEntity。双重门控挡住
// → 不反伤（attacker health 仍 20）。对齐 vanilla 防荆棘反伤无限递归（荆棘伤害不再触发荆棘）。
TEST_F(GuardianThornsTest, ThornsDoesNotRecurse)
{
    GuardianEntity guardian(EntityInstanceId(2), mc::test::testEcsRegistry());
    guardian.setWorld(&m_world);
    guardian.setHealth(guardian.maxHealth());

    TestLivingEntity attacker;
    attacker.setWorld(&m_world);

    auto source = DamageSources::thorns(&attacker);
    guardian.hurt(source, 5.0f);

    // 不反伤（attacker health 仍 20）
    EXPECT_FLOAT_EQ(attacker.health(), 20.0f);
}
