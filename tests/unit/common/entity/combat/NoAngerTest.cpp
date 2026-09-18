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

// NO_ANGER / NO_ANGER_FROM_WIND_CHARGE 标签运行时查询测试。
//
// 验证 LivingEntity::hurt 的"记录最近攻击者"段（LivingEntity.cpp:289 起步骤 2.5）对齐 vanilla
// LivingEntity.hurtServer:1208-1209（actuallyHurt 之后无条件调 resolveMob/PlayerResponsibleForDamage）。
// resolve 内 setLastHurtByMob（Cubium setLastHurtBy）须满足
//   getEntity() instanceof LivingEntity && !source.is(DamageTypeTags.NO_ANGER)
//   && (!source.is(DamageTypes.WIND_CHARGE) || !this.getType().is(EntityTypeTags.NO_ANGER_FROM_WIND_CHARGE))
// 才 setLastHurtByMob。
//
// 重要：setLastHurtBy 的记录逻辑位于 hurt()（步骤 2.5），而非 actuallyHurt()。
// 此前 actuallyHurt 步骤 8 含此逻辑，后为对齐 vanilla hurtServer:1208-1209（resolve 在 actuallyHurt
// 之后无条件执行，不受护甲/药水/吸收抵消影响）迁移到 hurt()。故测试必须经 hurt() 入口触发，
// 直接调 actuallyHurt 会绕过 hurt() 的步骤 2.5，getLastHurtBy() 恒为 nullptr（假失败）。
//
// 标签成员集（均已对齐 vanilla 1.21.11 数据包）：
//   - DamageTypeTags::NO_ANGER = {MobAttackNoAggro}（DamageTypeTags.cpp:644）：铁傀儡等生物的
//     mob_attack_no_aggro 攻击设计为不激怒目标。
//   - EntityTypeTags::NO_ANGER_FROM_WIND_CHARGE = {breeze,skeleton,bogged,stray,zombie,husk,spider,
//     cave_spider,slime}（EntityTypeTags.cpp:724）：风弹击中这些生物不激怒。
//
// 此前缺陷：Cubium actuallyHurt 步骤 8 无条件 setLastHurtBy（getTrueSource() instanceof LivingEntity
// 即记录），不查 NO_ANGER 与 NO_ANGER_FROM_WIND_CHARGE。后果：
//   1. mob_attack_no_aggro 误激怒目标（vanilla 设计为不激怒）。
//   2. 风弹击中骷髅/僵尸/蜘蛛等 9 类生物误激怒（vanilla 中这些生物不因风弹激怒）。
//
// 修复：步骤 8a 的 setLastHurtBy 加门控 shouldAnger = !source.is(NO_ANGER) &&
// (!isWindCharge || !NO_ANGER_FROM_WIND_CHARGE.contains(getTypeId()))。步骤 8b 的 setLastHurtByPlayer
// 不加门控（对齐 vanilla resolvePlayerResponsibleForDamage 无 NO_ANGER 门控——mob_attack_no_aggro
// 由玩家造成时仍记 lastHurtByPlayer 用于经验掉落，但不激怒）。
//
// 测试设计（四例交叉验证，TestLivingEntity 桩经 hurt() 入口观察 getLastHurtBy）：
//   - MobAttackSetsLastHurtBy：普通 mob_attack 攻击 → getLastHurtBy=attacker（基线，证明激怒链路工作）。
//   - MobAttackNoAggroDoesNotAnger：mob_attack_no_aggro 攻击 → getLastHurtBy=nullptr（NO_ANGER 门控）。
//   - WindChargeDoesNotAngerSkeleton：风弹击中骷髅（NO_ANGER_FROM_WIND_CHARGE 成员）→ getLastHurtBy=nullptr。
//   - WindChargeAngersNonListedMob：风弹击中牛（非标签成员）→ getLastHurtBy=attacker（对照证明风弹本身
//     可激怒，仅标签生物例外，排除"风弹一律不激怒"的过宽实现）。
//
// 注：actuallyHurt 步骤 9 荆棘分支在 victim 无附魔护甲时早退（applyThornsEnchantments 遍历空槽），
// 不影响 attacker。步骤 11 health>0（maxHealth=20，伤害 5 不死）走 playHurtSound，typeId 非空时会
// 播音效——测试用 BaseTestWorld（playSound 默认空实现）吸收，安全。
//
// Ref: vanilla LivingEntity.java:1326-1332（resolveMobResponsibleForDamage）
// Ref: vanilla LivingEntity.java:1334-1348（resolvePlayerResponsibleForDamage，无 NO_ANGER 门控）
// Ref: LivingEntity.cpp:389（步骤 8 更新最近攻击者）/ :NO_ANGER 与 WIND_CHARGE 门控
// Ref: DamageTypeTags.cpp:644（NO_ANGER = {MobAttackNoAggro}）
// Ref: EntityTypeTags.cpp:724（NO_ANGER_FROM_WIND_CHARGE 9 成员）
// Ref: DamageSource.hpp:1086（windBurst 工厂）/ :1124（mobAttackNoAggro 工厂）

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/item/Items.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"

using namespace mc;

namespace {

// 测试用 LivingEntity 子类：参照 DamagesHelmetTest.cpp / BypassesEnchantmentsTest.cpp 的 TestLivingEntity
// 范式。hurt() 为伤害处理正确入口（含步骤 2.5 setLastHurtBy 记录），actuallyHurt 仅为其子步骤。
// setTypeId/getLastHurtBy
// 均为 public。走基类 EquipmentComponent 链路，无附魔护甲使荆棘分支早退。
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
// 直接构造。派生并公开默认构造，使测试可在 SetUp 中创建实例（playSound 默认空实现吸收音效）。
class TestWorld final : public mc::test::BaseTestWorld {
public:
    TestWorld() = default;
};

} // namespace

// ============================================================================
// NO_ANGER / NO_ANGER_FROM_WIND_CHARGE 测试
// ============================================================================

class NoAngerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 附魔注册表初始化使 applyThornsEnchantments 内 EnchantmentRegistry::get("minecraft:thorns")
        // 可解析（victim 无附魔护甲早退，但初始化避免空表查询异常）。Items::initialize 供 ItemStack
        // 解析（本测试不直接用物品，但保持环境一致）。
        item::enchant::EnchantmentRegistry::clear();
        item::enchant::EnchantmentRegistry::initialize();
        Items::initialize();
        // 伤害类型标签初始化（进程级单例，s_initialized 守卫幂等）。NO_ANGER 成员集（= {MobAttackNoAggro}）
        // 在 initialize() 中注册（DamageTypeTags.cpp:644）。未初始化时标签成员集为空，source.is(NO_ANGER)
        // 恒返 false，修复门控形同虚设——这正是缺陷状态下 mob_attack_no_aggro 误激怒的根因。
        DamageTypeTags::initialize();
        // 实体类型标签初始化（进程级单例，s_initialized 守卫幂等）。NO_ANGER_FROM_WIND_CHARGE 9 成员
        // 在 initialize() 中注册（EntityTypeTags.cpp:724）。未初始化时 contains() 恒返 false，致风弹击中
        // 骷髅/僵尸等标签生物时 shouldAnger 误判为 true（误激怒）——与 NO_ANGER 同属"标签未初始化→
        // 门控失效"缺陷。两个标签须同时初始化，缺任一均致对应门控形同虚设。
        EntityTypeTags::initialize();
    }

    void TearDown() override { item::enchant::EnchantmentRegistry::clear(); }

    TestWorld m_world;
};

// 普通 mob_attack 攻击激怒目标（基线，证明 setLastHurtBy 链路本身工作）。
//
// victim 受 attacker 的 mob_attack 5 点伤害：非 NO_ANGER、非风弹 → shouldAnger=true → setLastHurtBy。
// 若本测试失败（getLastHurtBy=nullptr）说明 setLastHurtBy 链路本身损坏，后续三例的"不激怒"无法归因于
// 门控（假通过）。
TEST_F(NoAngerTest, MobAttackSetsLastHurtBy)
{
    TestLivingEntity victim;
    victim.setWorld(&m_world);
    TestLivingEntity attacker;

    auto source = DamageSources::mobAttack(&attacker);
    victim.hurt(source, 5.0f);

    EXPECT_EQ(victim.getLastHurtBy(), &attacker);
}

// mob_attack_no_aggro 攻击不激怒目标（NO_ANGER 门控生效）。
//
// victim 受 attacker 的 mob_attack_no_aggro 5 点伤害：source.is(NO_ANGER)=true → shouldAnger=false
// → 不 setLastHurtBy。修复前此处会 setLastHurtBy（误激怒）。注：铁傀儡等生物用 mob_attack_no_aggro
// 设计为不激怒目标，对齐 vanilla。
TEST_F(NoAngerTest, MobAttackNoAggroDoesNotAnger)
{
    TestLivingEntity victim;
    victim.setWorld(&m_world);
    TestLivingEntity attacker;

    auto source = DamageSources::mobAttackNoAggro(&attacker);
    victim.hurt(source, 5.0f);

    EXPECT_EQ(victim.getLastHurtBy(), nullptr);
}

// 风弹击中骷髅（NO_ANGER_FROM_WIND_CHARGE 成员）不激怒（NO_ANGER_FROM_WIND_CHARGE 门控生效）。
//
// victim 设为 minecraft:skeleton（NO_ANGER_FROM_WIND_CHARGE 成员），受 attacker 发射的风弹 5 点伤害：
// isWindCharge=true 且 NO_ANGER_FROM_WIND_CHARGE.contains("minecraft:skeleton")=true → shouldAnger=false
// → 不 setLastHurtBy。修复前此处会 setLastHurtBy（误激怒，骷髅会攻击风弹发射者）。
TEST_F(NoAngerTest, WindChargeDoesNotAngerSkeleton)
{
    TestLivingEntity victim;
    victim.setWorld(&m_world);
    victim.setTypeId("minecraft:skeleton");
    TestLivingEntity attacker;

    // windBurst(windCharge, shooter)：shooter=attacker 是 getTrueSource()（vanilla getEntity 真凶），
    // windCharge=attacker 占位（directSource，本测试不查 directSource）。
    auto source = DamageSources::windBurst(&attacker, &attacker);
    victim.hurt(source, 5.0f);

    EXPECT_EQ(victim.getLastHurtBy(), nullptr);
}

// 对照：风弹击中牛（非 NO_ANGER_FROM_WIND_CHARGE 成员）激怒（证明风弹本身可激怒，仅标签生物例外）。
//
// victim 设为 minecraft:cow（非 NO_ANGER_FROM_WIND_CHARGE 成员），受风弹 5 点伤害：isWindCharge=true
// 但 NO_ANGER_FROM_WIND_CHARGE.contains("minecraft:cow")=false → shouldAnger=true → setLastHurtBy。
// 若本测试失败（getLastHurtBy=nullptr）说明风弹一律不激怒（过宽实现），偏离 vanilla 仅 9 类标签生物
// 例外的语义。
TEST_F(NoAngerTest, WindChargeAngersNonListedMob)
{
    TestLivingEntity victim;
    victim.setWorld(&m_world);
    victim.setTypeId("minecraft:cow");
    TestLivingEntity attacker;

    auto source = DamageSources::windBurst(&attacker, &attacker);
    victim.hurt(source, 5.0f);

    EXPECT_EQ(victim.getLastHurtBy(), &attacker);
}
