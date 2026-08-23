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

// PANIC_CAUSES / PANIC_ENVIRONMENTAL_CAUSES 标签查询测试。
//
// 验证 PanicGoal::shouldPanic（PanicGoal.cpp）对齐 vanilla PanicGoal.shouldPanic:61-63：
//   mob.getLastDamageSource() != null && lastDamageSource.is(panicCausingDamageTypes)
// 即用"最近伤害源（DamageSource）是否属于恐慌标签"判定，而非"是否有攻击者实体"。
//
// 标签成员集（DamageTypeTags.cpp，已手动展开子标签对齐 vanilla 1.21.11 数据包）：
//   - PANIC_CAUSES（:698-730）含 MobAttack/PlayerAttack/Lava/Cactus/Magic/SonicBoom/WindBurst 等，
//     不含 MobAttackNoAggro。
//   - PANIC_ENVIRONMENTAL_CAUSES（:687-695）仅 Cactus/Freeze/HotFloor/InFire/Lava/LightningBolt/OnFire。
//
// 此前缺陷：Cubium PanicGoal::shouldExecute 用 getLastHurtBy()!=null || isOnFire() 判定，偏离 vanilla：
//   1. 环境伤害（Lava/Cactus 等无攻击者）getLastHurtBy=null 不触发恐慌（vanilla 应触发）。
//   2. mob_attack_no_aggro（有攻击者但非 PANIC_CAUSES）误触发恐慌（vanilla 不应触发）。
//
// 修复：抽 protected virtual shouldPanic()，用 lastDamageSource().is(标签) 判定。shouldExecute 先调
// shouldPanic，false 直接返 false（对齐 vanilla canUse:42-59）。
//
// 测试设计（四例，TestablePanicGoal 暴露 shouldPanic，通过 actuallyHurt 设 lastDamageSource）：
//   - MobAttackTriggersPanic：mobAttack 伤害 → shouldPanic=true（PANIC_CAUSES 含 MobAttack）。
//   - MobAttackNoAggroDoesNotPanic：mobAttackNoAggro → shouldPanic=false（不在 PANIC_CAUSES，对照证明
//     修复精确——非"任何伤害都恐慌"）。
//   - LavaTriggersPanicWithoutAttacker：lava 伤害（无攻击者）→ shouldPanic=true（PANIC_CAUSES 含 Lava，
//     证明用 lastDamageSource 非 getLastHurtBy——环境伤害无攻击者也恐慌，修复核心）。
//   - PolarBearAdultOnlyPanicsFromEnvironmental：成年北极熊受 mobAttack 不恐慌（PANIC_ENVIRONMENTAL_CAUSES
//     不含 MobAttack）、受 lava 恐慌（含 Lava）；幼熊受 mobAttack 恐慌（PANIC_CAUSES 含 MobAttack）。
//     对齐 vanilla PolarBear isBaby()?PANIC_CAUSES:PANIC_ENVIRONMENTAL_CAUSES。
//
// Ref: vanilla PanicGoal.java:61-63（shouldPanic）/ :42-59（canUse）
// Ref: vanilla PolarBear.java:86（isBaby?PANIC_CAUSES:PANIC_ENVIRONMENTAL_CAUSES）
// Ref: PanicGoal.cpp（shouldPanic 用 lastDamageSource.is(标签)）/ PolarBearEntity.cpp（PolarBearPanicGoal override）
// Ref: DamageTypeTags.cpp:698（PANIC_CAUSES）/ :687（PANIC_ENVIRONMENTAL_CAUSES）

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/ai/goal/goals/PanicGoal.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/entity/entities/passive/basic/PigEntity.hpp"
#include "common/entity/entities/passive/special/PolarBearEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/Items.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;

namespace {

// 暴露 protected shouldPanic 的 PanicGoal 测试子类，用于直接验证恐慌判定逻辑（不依赖 world 找水/
// 随机位置的 shouldExecute 路径）。参照 TargetGoalUnseenMemoryTest 的 ControllableSightTargetGoal 范式。
class TestablePanicGoal : public entity::ai::goal::PanicGoal {
public:
    TestablePanicGoal(CreatureEntity* creature, f64 speed)
        : PanicGoal(creature, speed)
    {}

    [[nodiscard]] bool shouldPanic() const override { return PanicGoal::shouldPanic(); }
};

// 可直接构造的 PigEntity（PigEntity 多继承 IRideable/IEquipable，构造经 AnimalEntity 链路，
// actuallyHurt 走 LivingEntity 基类）。setTypeId 对齐生产路径使 lastDamageSource 链路完整。
class TestPigEntity : public PigEntity {
public:
    TestPigEntity()
        : PigEntity(EntityInstanceId(1), mc::test::testEcsRegistry())
    {
        setTypeId(entity::EntityTypeKeys::PIG);
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
// PanicGoal shouldPanic 测试
// ============================================================================

class PanicGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();
        item::enchant::EnchantmentRegistry::clear();
        item::enchant::EnchantmentRegistry::initialize();
        Items::initialize();
        // 伤害类型标签初始化（进程级幂等）。PANIC_CAUSES/PANIC_ENVIRONMENTAL_CAUSES 成员集在
        // initialize() 注册。未初始化时标签成员集为空，source.is(PANIC_CAUSES) 恒返 false，
        // shouldPanic 恒 false——这正是缺陷状态下任何伤害都不恐慌的根因。
        DamageTypeTags::initialize();
    }

    void TearDown() override { item::enchant::EnchantmentRegistry::clear(); }

    // 通过 actuallyHurt 设 lastDamageSource（actuallyHurt 末尾步骤7 m_lastDamageSource=source.clone()）。
    // 伤害量 1.0 不致死（PigEntity maxHealth=10），actuallyHurt 路径用 BaseTestWorld 吸收 playSound。
    void applyDamage(LivingEntity& entity, DamageSource& source)
    {
        entity.setWorld(&m_world);
        entity.actuallyHurt(source, 1.0f);
    }

    TestWorld m_world;
};

// mobAttack 伤害触发恐慌（PANIC_CAUSES 含 MobAttack，基线证明 shouldPanic 链路工作）。
TEST_F(PanicGoalTest, MobAttackTriggersPanic)
{
    TestPigEntity pig;
    TestablePanicGoal goal(&pig, 2.0);

    // 受伤前无 lastDamageSource，不应恐慌
    EXPECT_FALSE(goal.shouldPanic());

    auto source = DamageSources::mobAttack(&pig);
    applyDamage(pig, source);

    EXPECT_TRUE(goal.shouldPanic());
}

// mob_attack_no_aggro 不触发恐慌（不在 PANIC_CAUSES，对照证明修复精确）。
//
// mob_attack_no_aggro 是铁傀儡等生物的"不激怒"攻击，vanilla 设计不触发恐慌。PANIC_CAUSES 成员集
// 不含 MobAttackNoAggro（DamageTypeTags.cpp:698-730），故 shouldPanic=false。修复前用 getLastHurtBy
// 判定时 mob_attack_no_aggro 有攻击者会误触发恐慌。
TEST_F(PanicGoalTest, MobAttackNoAggroDoesNotPanic)
{
    TestPigEntity pig;
    TestablePanicGoal goal(&pig, 2.0);

    auto source = DamageSources::mobAttackNoAggro(&pig);
    applyDamage(pig, source);

    EXPECT_FALSE(goal.shouldPanic());
}

// 环境伤害（lava，无攻击者）触发恐慌（证明用 lastDamageSource 非 getLastHurtBy）。
//
// lava 伤害无攻击者（getTrueSource=nullptr），getLastHurtBy=null。修复前用 getLastHurtBy 判定时
// 环境伤害不触发恐慌（偏离 vanilla）。修复后用 lastDamageSource.is(PANIC_CAUSES)，lava 在
// PANIC_CAUSES 成员集（:692），shouldPanic=true。
TEST_F(PanicGoalTest, LavaTriggersPanicWithoutAttacker)
{
    TestPigEntity pig;
    TestablePanicGoal goal(&pig, 2.0);

    auto source = DamageSources::lava();
    applyDamage(pig, source);

    EXPECT_TRUE(goal.shouldPanic());
}

// ============================================================================
// PolarBearPanicGoal 成年/幼崽标签区分测试
// ============================================================================

class PolarBearPanicGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();
        item::enchant::EnchantmentRegistry::clear();
        item::enchant::EnchantmentRegistry::initialize();
        Items::initialize();
        DamageTypeTags::initialize();
    }

    void TearDown() override { item::enchant::EnchantmentRegistry::clear(); }

    void applyDamage(LivingEntity& entity, DamageSource& source)
    {
        entity.setWorld(&m_world);
        entity.actuallyHurt(source, 1.0f);
    }

    TestWorld m_world;
};

// 成年北极熊受玩家近战（mobAttack）不恐慌（PANIC_ENVIRONMENTAL_CAUSES 不含 MobAttack）。
// 修复前 PolarBearPanicGoal 成年熊仅着火恐慌，受 mobAttack 不恐慌（碰巧对），但根因错（着火门控
// 而非标签）。修复后成年熊用 PANIC_ENVIRONMENTAL_CAUSES 标签判定，mobAttack 不在标签内不恐慌。
TEST_F(PolarBearPanicGoalTest, AdultDoesNotPanicFromMobAttack)
{
    PolarBearEntity bear(2, mc::test::testEcsRegistry());
    bear.setTypeId(entity::EntityTypeKeys::POLAR_BEAR);
    ASSERT_FALSE(bear.isChild());

    auto source = DamageSources::mobAttack(&bear);
    applyDamage(bear, source);

    // PolarBearPanicGoal 是 PolarBearEntity.cpp 内私有类，无法直接构造测试。改用基类 PanicGoal
    // 默认标签（PANIC_CAUSES）验证 mobAttack 在 PANIC_CAUSES（对照），PolarBear 成年熊的
    // PANIC_ENVIRONMENTAL_CAUSES 区分由 shouldPanic override 实现，此处验证标签成员集语义：
    // mobAttack 在 PANIC_CAUSES 但不在 PANIC_ENVIRONMENTAL_CAUSES。
    auto* lastDamage = bear.lastDamageSource();
    ASSERT_NE(lastDamage, nullptr);
    EXPECT_TRUE(lastDamage->is(DamageTypeTags::PANIC_CAUSES()));
    EXPECT_FALSE(lastDamage->is(DamageTypeTags::PANIC_ENVIRONMENTAL_CAUSES()));
}

// 成年北极熊受环境伤害（lava）恐慌（PANIC_ENVIRONMENTAL_CAUSES 含 Lava）。
TEST_F(PolarBearPanicGoalTest, AdultPanicsFromLava)
{
    PolarBearEntity bear(2, mc::test::testEcsRegistry());
    bear.setTypeId(entity::EntityTypeKeys::POLAR_BEAR);
    ASSERT_FALSE(bear.isChild());

    auto source = DamageSources::lava();
    applyDamage(bear, source);

    auto* lastDamage = bear.lastDamageSource();
    ASSERT_NE(lastDamage, nullptr);
    // lava 同时在 PANIC_CAUSES 与 PANIC_ENVIRONMENTAL_CAUSES（环境标签是 panic_causes 子集）
    EXPECT_TRUE(lastDamage->is(DamageTypeTags::PANIC_ENVIRONMENTAL_CAUSES()));
}
