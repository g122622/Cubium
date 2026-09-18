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

// BreezeEntity::isInvulnerableTo 对齐 MC Java 1.21.11 测试。
//
// 验证 BreezeEntity::isInvulnerableTo（BreezeEntity.cpp）对齐 vanilla
// Breeze.isInvulnerableTo（Breeze.java:267-269）：
//   return p_312691_.getEntity() instanceof Breeze || super.isInvulnerableTo(p_376278_, p_312691_);
// 当伤害来源实体（getEntity()，对弹射物是射击者，对近战是攻击者）是旋风人时，目标旋风人免疫。
// 这保障旋风人之间的风弹互不伤害——旋风人 A 发射的风弹命中旋风人 B 时，伤害源 getEntity()=A（Breeze），
// B 的 isInvulnerableTo 返回 true，不受伤害。
//
// 此前缺陷：Cubium BreezeEntity 未重写 isInvulnerableTo，旋风人风弹会正常伤害其他旋风人，
// 与 vanilla 直接冲突（vanilla 旋风人互不伤害）。修复：重写 isInvulnerableTo 检查
// source.getEntity()->entityType() == BREEZE → 返回 true，否则委托基类 MonsterEntity::isInvulnerableTo。
//
// 测试设计（两例交叉验证 getEntity() instanceof Breeze 门控）：
//   - BreezeSource_ImmuneToBreeze：旋风人 A 作 attacker，mobAttack(&breezeA) → getEntity()=Breeze →
//     目标 Breeze 免疫（hurt 返 false，health 不变）。证明 Breeze 来源门控生效。
//   - NonBreezeSource_DamagesBreeze：非旋风人 LivingEntity 作 attacker，mobAttack(&nonBreeze) →
//     getEntity() 非 Breeze → 委托基类 → 目标 Breeze 受伤（hurt 返 true，health 下降）。
//     对照证明门控区分 Breeze 来源与非 Breeze 来源。
//   两测试交叉验证：Breeze 来源免疫 vs 非 Breeze 来源受伤 = isInvulnerableTo 门控正确。
//   - 若 isInvulnerableTo override 回退为缺省（委托基类）：BreezeSource 免疫失效 → hurt 返 true → 正向 FAIL。
//   - 若 isInvulnerableTo 误对所有来源免疫：NonBreezeSource 受伤失效 → health 不变 → 正向 FAIL。
//
// 注：mobAttack(&attacker) 构造 EntityDamageSource，getEntity() 返回 attacker（直接源=causingEntity），
//     与 Java DamageSource.getEntity() 语义一致（近战攻击 causingEntity=attacker）。
//     风弹伤害源 windBurst(windCharge, shooter) 的 getEntity() 返回 shooter，与 mobAttack 的
//     getEntity()=attacker 同语义（都是 causingEntity），故用 mobAttack 验证 getEntity() instanceof Breeze
//     门控等价覆盖风弹场景（风弹 shooter 是 Breeze 时同样触发免疫）。
//
// 实体类型初始化：直接构造的 BreezeEntity/TestLivingEntity 不经过 EntityType::create()，typeId 默认空，
//   entityType() 返回 nullptr。 SetUp 中 VanillaEntities::registerAll() 初始化注册表后，对 attacker
//   setTypeId(BREEZE) 使其 entityType() 懒查表返回 VanillaEntityTypeKeys::BREEZE，对齐生产路径
//   （注册表工厂 create() 会 setTypeId(m_name)）。目标 Breeze 的 isInvulnerableTo 只查 attacker 的
//   entityType()，故目标自身无需 setTypeId。
//
// Ref: vanilla Breeze.java:267-269（isInvulnerableTo：getEntity() instanceof Breeze → 免疫）
// Ref: BreezeEntity.cpp（isInvulnerableTo 重写实现）
// Ref: DamageSource.hpp:922（mobAttack 工厂，getEntity()=attacker）

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/monster/breeze/BreezeEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;

namespace {

// 测试用 LivingEntity 子类：作非旋风人 attacker（验证非 Breeze 来源不触发免疫）。
// 继承 LivingEntity 并 setTypeId(PLAYER)，entityType() 返回 PLAYER（非 BREEZE）。
class TestLivingEntity : public LivingEntity {
public:
    TestLivingEntity()
        : LivingEntity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

// BaseTestWorld 默认构造为 protected，派生公开以作 fixture 成员（吸收 hurt 链路的 playSound）。
class TestWorld final : public mc::test::BaseTestWorld {
public:
    TestWorld() = default;
};

} // namespace

class BreezeIsInvulnerableToTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化方块和实体注册表，确保 VanillaEntityTypeKeys::BREEZE 有正确的 typeId
        // （VanillaEntities::registerAll 内部 set VanillaEntityTypeKeys::BREEZE = registry.getType(BREEZE)）。
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();
    }

    TestWorld m_world;
};

// 旋风人来源的伤害被目标旋风人免疫（对齐 vanilla Breeze.isInvulnerableTo: getEntity() instanceof Breeze）。
TEST_F(BreezeIsInvulnerableToTest, BreezeSource_ImmuneToBreeze)
{
    // 攻击者旋风人 A：setTypeId(BREEZE) 使 entityType()==VanillaEntityTypeKeys::BREEZE。
    BreezeEntity attacker(EntityInstanceId(1), mc::test::testEcsRegistry());
    attacker.setTypeId(entity::EntityTypeKeys::BREEZE);
    attacker.setWorld(&m_world);
    attacker.setHealth(attacker.maxHealth());

    // 目标旋风人 B：受攻击者，isInvulnerableTo 检查 attacker 的 entityType()。
    BreezeEntity target(EntityInstanceId(2), mc::test::testEcsRegistry());
    target.setWorld(&m_world);
    target.setHealth(target.maxHealth()); // 30 满血

    // mobAttack(&attacker)：getEntity()=attacker（Breeze）→ isInvulnerableTo 返回 true → 免疫。
    auto source = DamageSources::mobAttack(&attacker);
    const bool result = target.hurt(source, 5.0f);

    // Breeze 来源门控免疫：hurt 返 false，target health 不变（30）。
    EXPECT_FALSE(result) << "breeze should be immune to damage from another breeze";
    EXPECT_FLOAT_EQ(target.health(), target.maxHealth());
}

// 非旋风人来源的伤害正常造成伤害（对照：门控区分 Breeze 来源与非 Breeze 来源）。
TEST_F(BreezeIsInvulnerableToTest, NonBreezeSource_DamagesBreeze)
{
    // 攻击者：非旋风人 LivingEntity，setTypeId(PLAYER) 使 entityType()==PLAYER（非 BREEZE）。
    TestLivingEntity attacker;
    attacker.setTypeId(entity::EntityTypeKeys::PLAYER);
    attacker.setWorld(&m_world);
    attacker.setHealth(attacker.maxHealth());

    // 目标旋风人 B。
    BreezeEntity target(EntityInstanceId(2), mc::test::testEcsRegistry());
    target.setWorld(&m_world);
    target.setHealth(target.maxHealth()); // 30 满血

    // mobAttack(&attacker)：getEntity()=attacker（PLAYER，非 Breeze）→ 委托基类 → 不免疫 → 受伤。
    auto source = DamageSources::mobAttack(&attacker);
    const bool result = target.hurt(source, 5.0f);

    // 非 Breeze 来源正常受伤：hurt 返 true，target health 下降（30 - 5 = 25）。
    EXPECT_TRUE(result) << "breeze should take damage from non-breeze source";
    EXPECT_FLOAT_EQ(target.health(), target.maxHealth() - 5.0f);
}
