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

// NO_IMPACT / NO_KNOCKBACK 标签运行时查询测试。
//
// 验证 LivingEntity::hurt（LivingEntity.cpp）的 markHurt 与 indicateDamage 门控对齐 vanilla
// LivingEntity.hurtServer:1218-1238：
//   if (!source.is(DamageTypeTags.NO_IMPACT) && (!flag || amount > 0.0F)) markHurt();      // :1218
//   if (!source.is(DamageTypeTags.NO_KNOCKBACK)) { ... indicateDamage(d0, d1); }          // :1222,1236
// 即 markHurt 受 NO_IMPACT 门控（溺水不触发受击标记/速度同步），indicateDamage 受 NO_KNOCKBACK
// 门控（魔法/火焰/摔落/爆炸等不产生受击倾斜）。
//
// 标签成员集（均已对齐 vanilla 1.21.11 数据包）：
//   - NO_IMPACT = {Drown}（DamageTypeTags.cpp:649）：溺水是渐进缺氧，无受击反馈。
//   - NO_KNOCKBACK = {Explosion, InFire, OnFire, Lava, HotFloor, InWall, Cramming, Drown, Starve,
//     Cactus, Fall, EnderPearl, FlyIntoWall, OutOfWorld, Generic, Magic, Wither, DragonBreath,
//     Dryout, SweetBerryBush, Freeze, Stalagmite, OutsideBorder, GenericKill, Campfire, Spear,
//     BadRespawnPoint, LightningBolt, ExplosionPlayer}（DamageTypeTags.cpp:654，近 30 类型）。
//
// 此前缺陷：Cubium hurt 无条件 markHurt() + 无条件 indicateDamage()（仅 m_world/amount 门控），
// 不查 NO_IMPACT/NO_KNOCKBACK。后果：
//   1. 溺水伤害触发受击标记（vanilla 不应触发，溺水无受击动画）。
//   2. 魔法/火焰等 NO_KNOCKBACK 伤害触发 indicateDamage 设 hurtDir（vanilla 不应触发受击倾斜）。
//
// 修复：markHurt 加 !source.is(NO_IMPACT) 门控；indicateDamage 加 !source.is(NO_KNOCKBACK) 门控。
//
// 测试设计（三例，TestLivingEntity 桩 + TestWorld，观察 isHurtMarked/getHurtDir）：
//   - MeleeAttackMarksHurt：mobAttack（不在 NO_IMPACT）→ isHurtMarked()==true（基线，证明 markHurt
//     链路工作）。
//   - DrownDoesNotMarkHurt：drown（NO_IMPACT 唯一成员）→ isHurtMarked()==false（门控生效）。
//   - MagicDoesNotIndicateDamage：magic（NO_KNOCKBACK 成员）→ indicateDamage 不调用，hurtDir 保持
//     预设值；对照 mobAttack（不在 NO_KNOCKBACK，attacker 异位）→ indicateDamage 调用，hurtDir 改变。
//     证明 NO_KNOCKBACK 门控精确（非"任何伤害都不倾斜"的过宽实现）。
//
// 注：victim maxHealth=20，drown/magic bypassesArmor，5 点伤害不死。hurt 内 actuallyHurt 路径用
// BaseTestWorld 吸收 playSound。drown/magic 无 trueSource，actuallyHurt 步骤8/9 跳过。mobAttack 的
// attacker 与 victim 异位使 sourcePosition 非空，indicateDamage 计算 hurtDir=atan2(d1,d0)*RAD-yaw。
//
// Ref: vanilla LivingEntity.java:1218-1220（NO_IMPACT markHurt 门控）
// Ref: vanilla LivingEntity.java:1222-1238（NO_KNOCKBACK knockback+indicateDamage 门控）
// Ref: LivingEntity.cpp（hurt 的 markHurt/indicateDamage 门控）
// Ref: DamageTypeTags.cpp:649（NO_IMPACT={Drown}）/ :654（NO_KNOCKBACK 近 30 类型）
// Ref: Entity.hpp:1938（isHurtMarked）/ LivingEntity.hpp:863（getHurtDir）/ :889（animateHurt 设 hurtDir）

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/item/Items.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"

using namespace mc;

namespace {

// 测试用 LivingEntity 子类：参照 NoAngerTest.cpp 的 TestLivingEntity 范式。hurt 走基类
// LivingEntity::hurt（markHurt/indicateDamage 门控在此），actuallyHurt public virtual 可直接调。
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
// NO_IMPACT / NO_KNOCKBACK 测试
// ============================================================================

class NoImpactNoKnockbackTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        item::enchant::EnchantmentRegistry::clear();
        item::enchant::EnchantmentRegistry::initialize();
        Items::initialize();
        // 伤害类型标签初始化（进程级幂等）。NO_IMPACT（={Drown}）与 NO_KNOCKBACK（近 30 类型）成员集
        // 在 initialize() 注册（DamageTypeTags.cpp:649/654）。未初始化时标签成员集为空，
        // source.is(NO_IMPACT)/source.is(NO_KNOCKBACK) 恒返 false，门控形同虚设——这正是缺陷状态下
        // 溺水也 markHurt、魔法也 indicateDamage 的根因。
        DamageTypeTags::initialize();
    }

    void TearDown() override { item::enchant::EnchantmentRegistry::clear(); }

    TestWorld m_world;
};

// 近战攻击触发受击标记（基线，证明 markHurt 链路工作）。
//
// victim 受 attacker 的 mobAttack 5 点伤害：MobAttack 不在 NO_IMPACT → markHurt() → isHurtMarked=true。
// 若本测试失败（isHurtMarked=false）说明 markHurt 链路本身损坏，后续"不标记"无法归因于门控。
TEST_F(NoImpactNoKnockbackTest, MeleeAttackMarksHurt)
{
    TestLivingEntity victim;
    victim.setWorld(&m_world);
    TestLivingEntity attacker;

    auto source = DamageSources::mobAttack(&attacker);
    victim.hurt(source, 5.0f);

    EXPECT_TRUE(victim.isHurtMarked());
}

// 溺水伤害不触发受击标记（NO_IMPACT 门控生效）。
//
// victim 受 drown 5 点伤害：Drown 是 NO_IMPACT 唯一成员 → 不 markHurt → isHurtMarked=false。
// 修复前此处会 markHurt（溺水也触发受击标记/速度同步，偏离 vanilla——溺水是渐进缺氧无受击反馈）。
TEST_F(NoImpactNoKnockbackTest, DrownDoesNotMarkHurt)
{
    TestLivingEntity victim;
    victim.setWorld(&m_world);

    auto source = DamageSources::drown();
    victim.hurt(source, 5.0f);

    EXPECT_FALSE(victim.isHurtMarked());
}

// 魔法伤害不触发受击倾斜（NO_KNOCKBACK 门控生效），对照近战触发倾斜。
//
// victimA：预设 hurtDir=1.0（animateHurt），受 magic 5 点伤害。Magic 在 NO_KNOCKBACK → indicateDamage
// 不调用 → hurtDir 保持 1.0。修复前此处会 indicateDamage（设 hurtDir）。
// 对照 victimB：预设 hurtDir=1.0，受 mobAttack（attacker 异位于 (0,0,10)）5 点伤害。MobAttack 不在
// NO_KNOCKBACK → indicateDamage 调用，d0=0、d1=10 → hurtDir=atan2(10,0)*RAD-yaw=90-0=90。证明
// indicateDamage 链路本身工作（非"任何伤害都不倾斜"），仅 NO_KNOCKBACK 成员例外。
TEST_F(NoImpactNoKnockbackTest, MagicDoesNotIndicateDamage)
{
    // victimA：魔法（NO_KNOCKBACK）不倾斜
    TestLivingEntity victimA;
    victimA.setWorld(&m_world);
    victimA.setPosition(0.0f, 0.0f, 0.0f);
    victimA.animateHurt(1.0f); // 预设 hurtDir=1.0
    ASSERT_FLOAT_EQ(victimA.getHurtDir(), 1.0f);

    auto magicSource = DamageSources::magic();
    victimA.hurt(magicSource, 5.0f);

    // 门控生效：indicateDamage 未调用，hurtDir 保持 1.0
    EXPECT_FLOAT_EQ(victimA.getHurtDir(), 1.0f);

    // 对照 victimB：近战（非 NO_KNOCKBACK）倾斜
    TestLivingEntity victimB;
    victimB.setWorld(&m_world);
    victimB.setPosition(0.0f, 0.0f, 0.0f);
    victimB.animateHurt(1.0f); // 预设 hurtDir=1.0
    ASSERT_FLOAT_EQ(victimB.getHurtDir(), 1.0f);

    TestLivingEntity attacker;
    attacker.setWorld(&m_world);
    attacker.setPosition(0.0f, 0.0f, 10.0f); // 异位使 sourcePosition 非空，d0=0、d1=10

    auto mobSource = DamageSources::mobAttack(&attacker);
    victimB.hurt(mobSource, 5.0f);

    // indicateDamage 调用：hurtDir=atan2(10,0)*RAD_TO_DEG - yaw(0) = 90
    EXPECT_FLOAT_EQ(victimB.getHurtDir(), 90.0f);
}
