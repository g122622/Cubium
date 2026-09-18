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

// 盔甲架四标签（BURNS_ARMOR_STANDS / IGNITES_ARMOR_STANDS / CAN_BREAK_ARMOR_STAND /
// ALWAYS_KILLS_ARMOR_STANDS）伤害语义测试。
//
// 验证 ArmorStandEntity::hurt（EffectEntities.cpp）对齐 vanilla ArmorStand.hurtServer:266-318：
//   - BYPASSES_INVULNERABILITY → kill（remove）
//   - IS_EXPLOSION → brokenByAnything + kill（remove）
//   - IGNITES_ARMOR_STANDS：isOnFire → causeDamage(0.15) else igniteForSeconds(5)
//   - BURNS_ARMOR_STANDS && health>0.5 → causeDamage(4.0)
//   - CAN_BREAK/ALWAYS_KILLS：5tick 节流记录 lastHit，二次（节流内）或 ALWAYS_KILLS → brokenByPlayer + kill
//
// 标签成员集（DamageTypeTags.cpp:755-781）：
//   ALWAYS_KILLS_ARMOR_STANDS = {Arrow, Trident, Fireball, WitherSkull, WindBurst}
//   BURNS_ARMOR_STANDS        = {OnFire}
//   CAN_BREAK_ARMOR_STAND     = {ExplosionPlayer, PlayerAttack, Spear, MaceSmash}
//   IGNITES_ARMOR_STANDS      = {InFire, Campfire}
//
// 此前缺陷：Cubium ArmorStandEntity 继承 Entity（非 LivingEntity），无 hurt override，所有伤害源
// 走 Entity::hurt 基类（仅 isInvulnerableTo + markHurt + indicateDamage），完全不实现盔甲架特有
// 的标签分支语义——箭射不碎、岩浆点点火不生效、着火不掉血，偏离 vanilla。
//
// 架构差异处理：vanilla ArmorStand 继承 LivingEntity（maxHealth=2、causeDamage/kill/brokenByAnybody
// 装备掉落体系）。Cubium ArmorStandEntity 继承 Entity 无 health/装备体系，实现用自带 m_health
// （默认 2.0）模拟 causeDamage 扣血 + remove() 销毁。装备掉落/mobGriefing Mob 守卫/creativePlayer/
// 5tick 节流的完整 gameTime 体系留 TODO。
//
// 测试设计（六例覆盖四标签全分支 + BYPASSES/IS_EXPLOSION）：
//   - BurnsArmorStandsKillsStand：onFire()（OnFire 在 BURNS）health 2.0>0.5 → causeDamage(4.0)
//     → health=0 销毁。证明 BURNS 分支 + causeDamage 扣至 0 销毁。
//   - IgnitesArmorStandsIgnitesWhenNotBurning：inFire()（InFire 在 IGNITES）未着火 →
//     igniteForSeconds(5)，isOnFire 转 true，health 不变。证明 IGNITES 点燃分支。
//   - IgnitesArmorStandsDamagesWhenBurning：先 setFire(40) 着火，再 campfire()（Campfire 在 IGNITES）
//     → causeDamage(0.15)，health 2.0→1.85。证明 IGNITES 已着火分支。
//   - AlwaysKillsArmorStandsBreaksImmediately：arrow(proj, nullptr)（Arrow 在 ALWAYS_KILLS，
//     非 CAN_BREAK）→ 跳过节流直接销毁。证明 ALWAYS_KILLS 跳节流。
//   - CanBreakThrottlesFirstHitThenBreaksSecondHit：playerAttack(attacker)（PlayerAttack 在 CAN_BREAK，
//     非 ALWAYS_KILLS）首次 → 记录 lastHit return true 不销毁；同 tick 二次 → 节流内 brokenByPlayer
//     销毁。证明 5tick 节流 + 二次破坏。attacker 非 Player（dynamic_cast 返 nullptr）跳过 mayBuild/
//     creative 守卫直达节流。
//   - BypassesInvulnerabilityKillsStand：genericKill()（GenericKill 在 BYPASSES_INVULNERABILITY）
//     → 直接 remove。证明 BYPASSES 分支（最高优先级，先于 isInvulnerableTo/invisible/marker）。
//
// 注：ArmorStandEntity 构造单参 registry，setWorld(&m_world) 绑定测试世界（getGameTime/absorb
// playSound）。m_world 的 getGameTime 默认 currentTick()，测试无 tick 推进则恒定值，节流判定
// currentTick - lastHit > 5 首次成立（lastHit 初值 -6）。
//
// Ref: vanilla ArmorStand.java:266-318（hurtServer 完整分支链）
// Ref: EffectEntities.cpp（ArmorStandEntity::hurt + causeDamage 实现）
// Ref: DamageTypeTags.cpp:755-781（四标签成员集）
// Ref: DamageSource.hpp:796（DamageSources 工厂）

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/entity/entities/effect/EffectEntities.hpp"
#include "common/item/Items.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"

using namespace mc;
using mc::entity::ArmorStandEntity;

namespace {

// 测试用 LivingEntity 子类：作 CAN_BREAK 分支的伤害来源实体（playerAttack 接受 Entity*）。
// 非 Player，dynamic_cast<Player*> 返 nullptr，跳过 mayBuild/creative 守卫直达 5tick 节流逻辑。
class TestLivingEntity : public LivingEntity {
public:
    TestLivingEntity()
        : LivingEntity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

// BaseTestWorld 默认构造为 protected（TestWorldHelper.hpp:158），派生公开以作 fixture 成员。
class TestWorld final : public mc::test::BaseTestWorld {
public:
    TestWorld() = default;
};

} // namespace

// ============================================================================
// ArmorStandEntity 四标签伤害语义测试
// ============================================================================

class ArmorStandDamageTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        item::enchant::EnchantmentRegistry::clear();
        item::enchant::EnchantmentRegistry::initialize();
        Items::initialize();
        // 伤害类型标签初始化（进程级幂等）。四标签成员集在 initialize() 注册
        // （DamageTypeTags.cpp:755-781）。未初始化时标签成员集为空，source.is(标签) 恒返 false，
        // 所有伤害源落基类 Entity::hurt，四标签分支形同虚设——这正是缺陷状态下盔甲架对箭/火免疫的根因。
        DamageTypeTags::initialize();
    }

    void TearDown() override { item::enchant::EnchantmentRegistry::clear(); }

    TestWorld m_world;
};

// BURNS_ARMOR_STANDS（on_fire）：health 2.0>0.5 → causeDamage(4.0) → health=0 销毁。
TEST_F(ArmorStandDamageTest, BurnsArmorStandsKillsStand)
{
    ArmorStandEntity armorStand(mc::test::testEcsRegistry());
    armorStand.setWorld(&m_world);

    EXPECT_FLOAT_EQ(armorStand.health(), 2.0f);
    EXPECT_FALSE(armorStand.isRemoved());

    auto source = DamageSources::onFire();
    bool result = armorStand.hurt(source, 1.0f);

    // BURNS 分支 return false（vanilla :290），health 扣至 0 销毁
    EXPECT_FALSE(result);
    EXPECT_FLOAT_EQ(armorStand.health(), 0.0f);
    EXPECT_TRUE(armorStand.isRemoved());
}

// IGNITES_ARMOR_STANDS（in_fire）未着火：igniteForSeconds(5)，isOnFire 转 true，health 不变。
TEST_F(ArmorStandDamageTest, IgnitesArmorStandsIgnitesWhenNotBurning)
{
    ArmorStandEntity armorStand(mc::test::testEcsRegistry());
    armorStand.setWorld(&m_world);

    EXPECT_FALSE(armorStand.isOnFire());

    auto source = DamageSources::inFire();
    bool result = armorStand.hurt(source, 1.0f);

    // IGNITES 分支 return false（vanilla :287），未着火→点燃 5 秒
    EXPECT_FALSE(result);
    EXPECT_TRUE(armorStand.isOnFire());
    EXPECT_FLOAT_EQ(armorStand.health(), 2.0f);
    EXPECT_FALSE(armorStand.isRemoved());
}

// IGNITES_ARMOR_STANDS（campfire）已着火：causeDamage(0.15)，health 2.0→1.85。
TEST_F(ArmorStandDamageTest, IgnitesArmorStandsDamagesWhenBurning)
{
    ArmorStandEntity armorStand(mc::test::testEcsRegistry());
    armorStand.setWorld(&m_world);
    armorStand.setFire(40); // 先着火

    EXPECT_TRUE(armorStand.isOnFire());

    auto source = DamageSources::campfire();
    bool result = armorStand.hurt(source, 1.0f);

    // IGNITES 分支已着火→causeDamage(0.15)，health 2.0→1.85
    EXPECT_FALSE(result);
    EXPECT_FLOAT_EQ(armorStand.health(), 1.85f);
    EXPECT_FALSE(armorStand.isRemoved());
}

// ALWAYS_KILLS_ARMOR_STANDS（arrow）：跳过节流直接销毁。
TEST_F(ArmorStandDamageTest, AlwaysKillsArmorStandBreaksImmediately)
{
    ArmorStandEntity armorStand(mc::test::testEcsRegistry());
    armorStand.setWorld(&m_world);

    // arrow(arrowEntity, shooter)：Arrow 在 ALWAYS_KILLS，非 CAN_BREAK。
    // shooter=nullptr 无 mayBuild/creative 守卫，flag1=true 跳过节流直接销毁。
    auto source = DamageSources::arrow(&armorStand, nullptr);
    bool result = armorStand.hurt(source, 1.0f);

    // ALWAYS_KILLS 分支走 brokenByPlayer 销毁，return true（vanilla :315）
    EXPECT_TRUE(result);
    EXPECT_TRUE(armorStand.isRemoved());
}

// CAN_BREAK_ARMOR_STAND（player_attack）：首次节流记录 lastHit 不销毁，同 tick 二次节流内销毁。
TEST_F(ArmorStandDamageTest, CanBreakThrottlesFirstHitThenBreaksSecondHit)
{
    ArmorStandEntity armorStand(mc::test::testEcsRegistry());
    armorStand.setWorld(&m_world);

    TestLivingEntity attacker;
    attacker.setWorld(&m_world);

    // PlayerAttack 在 CAN_BREAK，非 ALWAYS_KILLS。attacker 非 Player，跳过 mayBuild/creative 守卫。
    auto source1 = DamageSources::playerAttack(&attacker);
    bool result1 = armorStand.hurt(source1, 1.0f);

    // 首次：currentTick - lastHit(-6) > 5 → 记录 lastHit return true，不销毁
    EXPECT_TRUE(result1);
    EXPECT_FALSE(armorStand.isRemoved());
    EXPECT_FLOAT_EQ(armorStand.health(), 2.0f);

    // 同 tick 二次（m_world 未推进 tick，getGameTime 恒定）：currentTick - lastHit <= 5 →
    // brokenByPlayer 销毁
    auto source2 = DamageSources::playerAttack(&attacker);
    bool result2 = armorStand.hurt(source2, 1.0f);

    EXPECT_TRUE(result2);
    EXPECT_TRUE(armorStand.isRemoved());
}

// BYPASSES_INVULNERABILITY（generic_kill）：最高优先级，先于 isInvulnerableTo/invisible/marker 销毁。
TEST_F(ArmorStandDamageTest, BypassesInvulnerabilityKillsStand)
{
    ArmorStandEntity armorStand(mc::test::testEcsRegistry());
    armorStand.setWorld(&m_world);

    auto source = DamageSources::genericKill();
    bool result = armorStand.hurt(source, 1.0f);

    // BYPASSES_INVULNERABILITY 分支 remove，return false（vanilla :273）
    EXPECT_FALSE(result);
    EXPECT_TRUE(armorStand.isRemoved());
}
