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

// WITHER_IMMUNE_TO / BYPASSES_INVULNERABILITY 标签运行时查询测试。
//
// 验证 WitherEntity::isInvulnerableTo（WitherEntity.cpp）对齐 vanilla WitherBoss.hurtServer:453-456：
//   - WITHER_IMMUNE_TO（成员={Drown}）→ 免疫（vanilla :453）
//   - getInvulTime()>0 && !BYPASSES_INVULNERABILITY（成员={OutOfWorld, GenericKill}）→ 无敌阶段免疫（vanilla :455）
//
// 此前缺陷：Cubium WitherEntity::isInvulnerableTo 硬编码 source.type()==DamageType::Drown 代 WITHER_IMMUNE_TO
// 标签，且无敌阶段门控硬编码 source.type()!=OutOfWorld 漏了 GenericKill（/kill 命令应绕过无敌阶段）。
// 标签查询修正两处：Drown 走标签，BYPASSES_INVULNERABILITY 全成员（OutOfWorld+GenericKill）均可绕过无敌阶段。
//
// 测试设计（三例交叉验证标签门控）：
//   - DrownIsImmune：drown()（OnFire? 否，Drown 在 WITHER_IMMUNE_TO）→ hurt 返 false，health 不变。
//     证明 WITHER_IMMUNE_TO 标签门控（非硬编码 type 比较）。
//   - GenericKillBypassesInvulnStage：setInvulTime(220) + genericKill()（GenericKill 在 BYPASSES_INVULNERABILITY）
//     → 穿透无敌阶段造成伤害（health 下降）。证明 BYPASSES_INVULNERABILITY 含 GenericKill（原硬编码漏）。
//   - NormalDamageBlockedDuringInvulnStage：setInvulTime(220) + mobAttack（不在 BYPASSES_INVULNERABILITY）
//     → 无敌阶段免疫，health 不变。对照证明门控区分标签内外。
//
// 注：凋灵 maxHealth=300，设 health=300 满血避免 isCharged（health<=150）干扰（充能状态免疫箭矢）。
// BaseTestWorld 吸收 hurt 链路的 playSound。DamageTypeTags::initialize 注册标签成员集（进程级幂等）。
//
// Ref: vanilla WitherBoss.java:453-456（hurtServer WITHER_IMMUNE_TO + BYPASSES_INVULNERABILITY 门控）
// Ref: WitherEntity.cpp（isInvulnerableTo 标签查询实现）
// Ref: DamageTypeTags.cpp:741（WITHER_IMMUNE_TO={Drown}）/ :503（BYPASSES_INVULNERABILITY={OutOfWorld,GenericKill}）

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/entity/entities/boss/WitherEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"

using namespace mc;

namespace {

// 测试用 LivingEntity 子类：作 mobAttack 的攻击者。
class TestLivingEntity : public LivingEntity {
public:
    TestLivingEntity()
        : LivingEntity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

// BaseTestWorld 默认构造为 protected，派生公开以作 fixture 成员（吸收 playSound）。
class TestWorld final : public mc::test::BaseTestWorld {
public:
    TestWorld() = default;
};

} // namespace

class WitherImmuneToTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        item::enchant::EnchantmentRegistry::clear();
        item::enchant::EnchantmentRegistry::initialize();
        Items::initialize();
        // 伤害类型标签初始化（进程级幂等）。WITHER_IMMUNE_TO/BYPASSES_INVULNERABILITY 成员集在
        // initialize() 注册。未初始化时标签成员集为空，source.is(标签) 恒返 false。
        DamageTypeTags::initialize();
    }

    void TearDown() override { item::enchant::EnchantmentRegistry::clear(); }

    TestWorld m_world;
};

// WITHER_IMMUNE_TO（Drown）：凋灵免疫溺水伤害。
TEST_F(WitherImmuneToTest, DrownIsImmune)
{
    entity::WitherEntity wither(EntityInstanceId(2), mc::test::testEcsRegistry());
    wither.setWorld(&m_world);
    wither.setHealth(wither.maxHealth()); // 300 满血，非充能

    auto source = DamageSources::drown();
    bool result = wither.hurt(source, 5.0f);

    // WITHER_IMMUNE_TO 门控免疫，hurt 返 false，health 不变
    EXPECT_FALSE(result);
    EXPECT_FLOAT_EQ(wither.health(), wither.maxHealth());
}

// BYPASSES_INVULNERABILITY（GenericKill）：穿透无敌阶段造成伤害。
TEST_F(WitherImmuneToTest, GenericKillBypassesInvulnStage)
{
    entity::WitherEntity wither(EntityInstanceId(2), mc::test::testEcsRegistry());
    wither.setWorld(&m_world);
    wither.setHealth(wither.maxHealth());
    wither.setInvulTime(220); // 进入无敌阶段

    EXPECT_GT(wither.getInvulTime(), 0);

    auto source = DamageSources::genericKill(); // GenericKill 在 BYPASSES_INVULNERABILITY
    bool result = wither.hurt(source, 5.0f);

    // BYPASSES_INVULNERABILITY 穿透无敌阶段，造成伤害（health 下降）。原硬编码 !=OutOfWorld 漏 GenericKill。
    EXPECT_TRUE(result);
    EXPECT_LT(wither.health(), wither.maxHealth());
}

// 普通伤害（mobAttack）在无敌阶段被免疫（对照）。
TEST_F(WitherImmuneToTest, NormalDamageBlockedDuringInvulnStage)
{
    entity::WitherEntity wither(EntityInstanceId(2), mc::test::testEcsRegistry());
    wither.setWorld(&m_world);
    wither.setHealth(wither.maxHealth());
    wither.setInvulTime(220); // 无敌阶段

    TestLivingEntity attacker;
    attacker.setWorld(&m_world);

    auto source = DamageSources::mobAttack(&attacker); // MobAttack 不在 BYPASSES_INVULNERABILITY
    bool result = wither.hurt(source, 5.0f);

    // 无敌阶段免疫普通伤害（不在 BYPASSES_INVULNERABILITY），health 不变
    EXPECT_FALSE(result);
    EXPECT_FLOAT_EQ(wither.health(), wither.maxHealth());
}
