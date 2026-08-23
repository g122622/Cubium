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

// IS_PROJECTILE 标签运行时查询测试（盾牌格挡回调门控）。
//
// 验证 LivingEntity::actuallyHurt 格挡回调（LivingEntity.cpp）对齐 vanilla
// LivingEntity.applyItemBlocking / blockUsingItem（LivingEntity.java:1306）：
//   if (f > 0.0F && !source.is(IS_PROJECTILE) && directEntity instanceof LivingEntity)
//       blockUsingItem → attacker.blockedByItem(victim)
// 即仅近战等直接来源（directSource 是 LivingEntity 且非 IS_PROJECTILE 投射物）格挡时回调攻击者，
// 让攻击者执行"被格挡"特殊行为（如 Ravager 50% 眩晕→咆哮）。投射物格挡不回调攻击者。
//
// 此前缺陷：Cubium 用 getTrueSource()（射击者）且缺 !source.is(IS_PROJECTILE) 门控，致箭矢格挡
// 错误回调射击者（射击者被误击退/触发 Ravager 眩晕等）。改为 directSource()（直接来源）+
// IS_PROJECTILE 门控对齐 vanilla。
//
// 测试设计（两例交叉验证）：
//   - ProjectileBlockDoesNotCallbackAttacker：箭矢（Arrow 在 IS_PROJECTILE，directSource=箭矢实体）
//     被格挡 → 门控挡住不回调 attacker（m_blockedFlag==false）。证明 IS_PROJECTILE 门控。
//   - MeleeBlockCallbacksAttacker：近战（MobAttack 非 IS_PROJECTILE，directSource=attacker 是
//     LivingEntity）被格挡 → 回调 attacker（m_blockedFlag==true）。证明近战回调链路工作。
//
// 观察手段：TestBlockingEntity override canBlockDamageSource→true（强制格挡）、damageShield→空
// （避免盾牌物品栏依赖）、blockedByItem→设 m_blockedFlag（观察是否被回调）。attacker 也用
// TestBlockingEntity，读其 m_blockedFlag 判定回调。actuallyHurt 直接调避开 hurt 入口门控。
//
// 注：arrow(arrowEntity, shooter) 的 directSource=arrowEntity、getEntity=shooter。门控
// !source.is(IS_PROJECTILE) 先判，Arrow 在标签→不进回调分支，directSource 不查。mobAttack
// 的 directSource=attacker（EntityDamageSource.source=directSource=m_source）。
//
// Ref: vanilla LivingEntity.java:1306（applyItemBlocking IS_PROJECTILE 门控）
// Ref: LivingEntity.cpp（actuallyHurt 格挡回调 directSource+IS_PROJECTILE 门控）
// Ref: DamageTypeTags.cpp（IS_PROJECTILE 成员集）

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/item/Items.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"

using namespace mc;

namespace {

// 可格挡实体：强制 canBlockDamageSource=true，damageShield 空实现，blockedByItem 设标志。
class TestBlockingEntity : public LivingEntity {
public:
    TestBlockingEntity()
        : LivingEntity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }

    [[nodiscard]] bool canBlockDamageSource(DamageSource& /*source*/) const override { return true; }
    void damageShield(f32 /*amount*/) override {} // 空实现，避免盾牌物品栏依赖
    void blockedByItem(LivingEntity& /*victim*/) override { m_blockedFlag = true; }

    bool m_blockedFlag = false;
};

class TestWorld final : public mc::test::BaseTestWorld {
public:
    TestWorld() = default;
};

} // namespace

class ProjectileBlockCallbackTest : public ::testing::Test {
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

// 投射物（IS_PROJECTILE）格挡不回调攻击者。
TEST_F(ProjectileBlockCallbackTest, ProjectileBlockDoesNotCallbackAttacker)
{
    TestBlockingEntity victim;
    victim.setWorld(&m_world);
    victim.setHealth(20.0f);

    TestBlockingEntity attacker; // 射击者，观察 m_blockedFlag
    attacker.setWorld(&m_world);

    // arrow(arrowEntity, shooter)：Arrow 在 IS_PROJECTILE，directSource=arrowEntity。
    // 用 attacker 近似 arrowEntity（门控先判 IS_PROJECTILE，不查 directSource）。
    auto source = DamageSources::arrow(&attacker, &attacker);
    victim.actuallyHurt(source, 4.0f);

    // IS_PROJECTILE 门控挡住，不回调 attacker（m_blockedFlag==false）
    EXPECT_FALSE(attacker.m_blockedFlag);
    // 格挡成功不扣血
    EXPECT_FLOAT_EQ(victim.health(), 20.0f);
}

// 近战（非 IS_PROJECTILE）格挡回调攻击者。
TEST_F(ProjectileBlockCallbackTest, MeleeBlockCallbacksAttacker)
{
    TestBlockingEntity victim;
    victim.setWorld(&m_world);
    victim.setHealth(20.0f);

    TestBlockingEntity attacker; // 近战攻击者，观察 m_blockedFlag
    attacker.setWorld(&m_world);

    auto source = DamageSources::mobAttack(&attacker); // MobAttack 非 IS_PROJECTILE，directSource=attacker
    victim.actuallyHurt(source, 4.0f);

    // 近战 directSource=attacker 是 LivingEntity 且非 IS_PROJECTILE → 回调 attacker
    EXPECT_TRUE(attacker.m_blockedFlag);
    // 格挡成功不扣血
    EXPECT_FLOAT_EQ(victim.health(), 20.0f);
}
