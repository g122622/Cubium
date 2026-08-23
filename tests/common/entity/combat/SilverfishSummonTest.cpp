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

// ALWAYS_TRIGGERS_SILVERFISH 标签运行时查询测试。
//
// 验证 SilverfishEntity::hurt（EndermiteEntity.cpp）对齐 vanilla Silverfish.hurtServer:87：
//   if ((source.getEntity() != null || source.is(ALWAYS_TRIGGERS_SILVERFISH)) && friendsGoal != null)
//       friendsGoal.notifyHurt();
// 即蠹虫受实体攻击（有攻击者）或 ALWAYS_TRIGGERS_SILVERFISH 标签伤害（成员={Magic}）时，
// 通知召唤同伴目标（notifyHurt 设 m_lookForFriends=20）。纯魔法伤害（无攻击实体）也能唤醒同伴。
//
// 此前缺陷：Cubium SilverfishEntity::hurt 硬编码 source.isMagic() 代标签，且用 source.isEntitySource()
// 代 getEntity()!=null。标签查询更正确且支持数据包扩展，getEntity() 语义对齐 vanilla。
//
// 观察手段：notifyHurt 设 m_summonGoal->m_lookForFriends=20，shouldExecute() 返 m_lookForFriends>0。
// 通过 getSummonGoal()->shouldExecute() 间接观察 notifyHurt 是否被触发（无需访问 private 成员）。
//
// 测试设计（三例交叉验证标签门控）：
//   - MagicTriggersSummon：magic()（getEntity()=nullptr，ALWAYS_TRIGGERS_SILVERFISH 成员）→ notifyHurt
//     → shouldExecute true。证明纯魔法伤害触发召唤（标签门控，非 isMagic 硬编码）。
//   - FallDoesNotTriggerSummon：fall()（getEntity()=nullptr，不在标签，非实体源）→ 不 notifyHurt
//     → shouldExecute false。对照证明门控区分标签内外。
//   - EntityAttackTriggersSummon：mobAttack(attacker)（getEntity()=attacker≠null）→ notifyHurt
//     → shouldExecute true。证明实体攻击触发召唤（getEntity()!=null 分支）。
//
// 注：蠹虫 maxHealth 经 registerAttributes 设置（通常 8），受 1.0 伤害不死。hurt 走 MonsterEntity::hurt
// 扣血但不影响 shouldExecute 观察（notifyHurt 在 hurt 开头同步调用）。BaseTestWorld 吸收 playSound。
//
// Ref: vanilla Silverfish.java:87（hurtServer ALWAYS_TRIGGERS_SILVERFISH 门控）
// Ref: EndermiteEntity.cpp（SilverfishEntity::hurt 标签查询实现）
// Ref: DamageTypeTags.cpp:789（ALWAYS_TRIGGERS_SILVERFISH={Magic}）

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/ai/goal/goals/special/SilverfishGoals.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/entity/entities/monster/arthropod/EndermiteEntity.hpp"
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

class SilverfishSummonTest : public ::testing::Test {
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

// 纯魔法伤害（无攻击实体，ALWAYS_TRIGGERS_SILVERFISH）触发召唤同伴。
TEST_F(SilverfishSummonTest, MagicTriggersSummon)
{
    SilverfishEntity silverfish(EntityInstanceId(2), mc::test::testEcsRegistry());
    silverfish.setWorld(&m_world);
    silverfish.setHealth(silverfish.maxHealth());

    ASSERT_NE(silverfish.getSummonGoal(), nullptr);
    EXPECT_FALSE(silverfish.getSummonGoal()->shouldExecute()); // 初始未触发

    auto source = DamageSources::magic(); // getEntity()=nullptr，Magic 在 ALWAYS_TRIGGERS_SILVERFISH
    silverfish.hurt(source, 1.0f);

    // notifyHurt 设 m_lookForFriends=20，shouldExecute 返 true
    EXPECT_TRUE(silverfish.getSummonGoal()->shouldExecute());
}

// 摔落伤害（无攻击实体，不在标签）不触发召唤同伴。
TEST_F(SilverfishSummonTest, FallDoesNotTriggerSummon)
{
    SilverfishEntity silverfish(EntityInstanceId(2), mc::test::testEcsRegistry());
    silverfish.setWorld(&m_world);
    silverfish.setHealth(silverfish.maxHealth());

    ASSERT_NE(silverfish.getSummonGoal(), nullptr);
    EXPECT_FALSE(silverfish.getSummonGoal()->shouldExecute());

    auto source = DamageSources::fall(); // getEntity()=nullptr，Fall 不在 ALWAYS_TRIGGERS_SILVERFISH
    silverfish.hurt(source, 1.0f);

    // 不 notifyHurt，shouldExecute 仍 false
    EXPECT_FALSE(silverfish.getSummonGoal()->shouldExecute());
}

// 实体攻击（getEntity()!=null）触发召唤同伴。
TEST_F(SilverfishSummonTest, EntityAttackTriggersSummon)
{
    SilverfishEntity silverfish(EntityInstanceId(2), mc::test::testEcsRegistry());
    silverfish.setWorld(&m_world);
    silverfish.setHealth(silverfish.maxHealth());

    TestLivingEntity attacker;
    attacker.setWorld(&m_world);

    ASSERT_NE(silverfish.getSummonGoal(), nullptr);
    EXPECT_FALSE(silverfish.getSummonGoal()->shouldExecute());

    auto source = DamageSources::mobAttack(&attacker); // getEntity()=attacker≠null
    silverfish.hurt(source, 1.0f);

    // notifyHurt 触发，shouldExecute 返 true
    EXPECT_TRUE(silverfish.getSummonGoal()->shouldExecute());
}
