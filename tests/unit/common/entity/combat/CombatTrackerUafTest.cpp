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

// CombatTracker 的 UAF 回归测试。
//
// 【背景】CI 的集成测试里服务端稳定崩溃，符号化后的调用栈为：
//     __dynamic_cast
//     mc::CombatTracker::getBestAttacker() const        CombatTracker.cpp:176
//     mc::LivingEntity::getKillCredit()                 LivingEntity.cpp:758
//     mc::LivingEntity::die(mc::DamageSource&)          LivingEntity.cpp:784
//     mc::LivingEntity::actuallyHurt(...)               LivingEntity.cpp:567
//     mc::ecs::FireTickSystem::tick(...)                FireTickSystem.cpp:39
//   Reason: SIGSEGV at address 0x38
//
// 根因：CombatEntry 里的 `m_source` 是 DamageSource 的 clone，其中的实体指针是**裸指针**。
// vanilla 靠 Java GC 保证 `getLastDamageSource().getEntity()` 引用安全；Cubium 无 GC，
// 真凶实体析构后该指针即悬垂。getBestAttacker() 对其做 `dynamic_cast<LivingEntity*>`
// 会读取悬垂对象的虚表指针（vptr），拿到野指针后解引用 → SIGSEGV。
//
// 修复：在 trackDamage 的**同步上下文**（伤害发生瞬间，来源实体必然存活）捕获
// trueSourceId / directSourceId，所有后续访问一律经 `IWorld::getEntity(id)` 反查。
// id 永不悬垂（EntityInstanceId 单调递增、不复用），真凶析构后 getEntity 返回 nullptr。
//
// 本文件用「销毁真凶后仍调用 getBestAttacker / getLastAttacker / getDeathMessage」
// 来复现原缺陷路径——修复前必段错误，修复后安全返回 nullptr / 环境伤害文案。

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/CombatTracker.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/player/Player.hpp"

#include <memory>
#include <string>

using namespace mc;

namespace {

// ============================================================================
// 测试辅助
// ============================================================================

/// 支持实体 id 反查的测试世界（复用 BaseTestWorld 的 registerEntityForLookup / getEntity）。
class CombatTrackerTestWorld : public mc::test::BaseTestWorld {};

/// 可直接构造、可被销毁的测试用 LivingEntity。
class TestMob : public LivingEntity {
public:
    explicit TestMob(EntityInstanceId id)
        : LivingEntity(id, nullptr, mc::test::testEcsRegistry())
    {
        registerAttributes();
        attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
        setHealth(20.0f);
    }
};

/// 玩家（用于验证 getBestAttacker 的「玩家优先」分支）。
class TestPlayerEntity : public Player {
public:
    explicit TestPlayerEntity(EntityInstanceId id)
        : Player(id, "TrackerTestPlayer", mc::test::testEcsRegistry())
    {
        registerAttributes();
        attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
        setHealth(20.0f);
    }
};

/// 夹具：世界 + 受击者（owner）。
class CombatTrackerUafTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_world = std::make_unique<CombatTrackerTestWorld>();
        m_victim = std::make_unique<TestMob>(EntityInstanceId(100));
        m_victim->setWorld(m_world.get());
        m_victim->setPosition(0.0f, 64.0f, 0.0f);
        // 受击者自身也要可反查（部分路径会取 owner）。
        m_world->registerEntityForLookup(m_victim.get());
    }

    void TearDown() override
    {
        m_victim.reset();
        m_world.reset();
    }

    /// 造一个「真凶已登记、可反查」的攻击者，并记录一次伤害。
    /// @param attacker 攻击者（调用方持有所有权）
    /// @param damage   伤害值
    /// @return 攻击者 id，供销毁后调用 `destroyAttacker()` 取消登记
    EntityInstanceId recordDamageFrom(LivingEntity& attacker, f32 damage)
    {
        m_world->registerEntityForLookup(&attacker);
        // EntityDamageSource 的 source/directSource/getTrueSource 都是同一个实体（近战语义）。
        auto source = DamageSources::mobAttack(&attacker);
        m_victim->combatTracker().trackDamage(source, m_victim->health(), damage);
        return attacker.id();
    }

    /// 模拟「真凶实体被销毁」：从反查表中摘除。
    ///
    /// 生产代码里 EntityManager 销毁实体时会把它从 m_entities 中 erase，此后
    /// `IWorld::getEntity(id)` 返回 nullptr。测试中实体对象析构不会自动更新反查表
    /// （表里存的是裸指针），必须显式摘除，否则 getEntity 会返回悬垂指针，
    /// 复现不出「真凶已消失」这一前提。
    void destroyAttacker(EntityInstanceId id) { m_world->unregisterEntityForLookup(id); }

    std::unique_ptr<CombatTrackerTestWorld> m_world;
    std::unique_ptr<TestMob> m_victim;
};

} // namespace

// ============================================================================
// 核心回归：真凶析构后不得段错误
// ============================================================================

// 真凶被销毁后调用 getBestAttacker()——修复前在此段错误（悬垂指针上的 dynamic_cast）。
// 修复后应安全返回 nullptr（id 反查不到已析构的实体）。
TEST_F(CombatTrackerUafTest, GetBestAttackerAfterAttackerDestroyedDoesNotCrash)
{
    EntityInstanceId attackerId = INVALID_ENTITY_ID;
    {
        auto attacker = std::make_unique<TestMob>(EntityInstanceId(101));
        attacker->setWorld(m_world.get());
        attackerId = recordDamageFrom(*attacker, 5.0f);
        // attacker 在作用域结束时析构：此时 CombatEntry 内的裸实体指针已悬垂。
    }
    destroyAttacker(attackerId);

    // 原实现会在这里 SIGSEGV（栈顶 __dynamic_cast）。修复后安全返回 nullptr。
    Entity* best = m_victim->combatTracker().getBestAttacker();
    EXPECT_EQ(best, nullptr);

    LivingEntity* bestLiving = m_victim->combatTracker().getBestAttackerLiving();
    EXPECT_EQ(bestLiving, nullptr);
}

// getLastAttacker() 同样走实体反查，真凶析构后应返回 nullptr 而非悬垂指针。
TEST_F(CombatTrackerUafTest, GetLastAttackerAfterAttackerDestroyedDoesNotCrash)
{
    EntityInstanceId attackerId = INVALID_ENTITY_ID;
    {
        auto attacker = std::make_unique<TestMob>(EntityInstanceId(102));
        attacker->setWorld(m_world.get());
        attackerId = recordDamageFrom(*attacker, 3.0f);
    }
    destroyAttacker(attackerId);

    EXPECT_EQ(m_victim->combatTracker().getLastAttacker(), nullptr);
}

// getDeathMessage() 会经真凶取显示名。真凶已析构时应退化为环境伤害文案而非崩溃。
TEST_F(CombatTrackerUafTest, GetDeathMessageAfterAttackerDestroyedDoesNotCrash)
{
    EntityInstanceId attackerId = INVALID_ENTITY_ID;
    {
        auto attacker = std::make_unique<TestMob>(EntityInstanceId(103));
        attacker->setWorld(m_world.get());
        attackerId = recordDamageFrom(*attacker, 7.0f);
    }
    destroyAttacker(attackerId);

    std::string message = m_victim->combatTracker().getDeathMessage();
    EXPECT_FALSE(message.empty());
    // 不应出现「被 XX 击杀」——真凶已不存在，取不到名字。
    EXPECT_EQ(message.find("slain"), std::string::npos);
}

// ============================================================================
// 存活真凶：修复后行为必须与修复前一致（不能把功能一起改坏）
// ============================================================================

// 真凶存活时 getBestAttacker() 应正常返回该实体。
TEST_F(CombatTrackerUafTest, GetBestAttackerReturnsLiveAttacker)
{
    auto attacker = std::make_unique<TestMob>(EntityInstanceId(104));
    attacker->setWorld(m_world.get());
    recordDamageFrom(*attacker, 5.0f);

    EXPECT_EQ(m_victim->combatTracker().getBestAttacker(), attacker.get());
    EXPECT_EQ(m_victim->combatTracker().getBestAttackerLiving(), attacker.get());
}

// 未登记到世界反查表的实体（模拟「真凶在别的维度/未登记」）应被安全跳过而非崩溃。
TEST_F(CombatTrackerUafTest, GetBestAttackerSkipsUnregisteredAttacker)
{
    auto attacker = std::make_unique<TestMob>(EntityInstanceId(105));
    // 故意不调用 registerEntityForLookup：getEntity(id) 查不到。
    auto source = DamageSources::mobAttack(attacker.get());
    m_victim->combatTracker().trackDamage(source, m_victim->health(), 5.0f);

    EXPECT_EQ(m_victim->combatTracker().getBestAttacker(), nullptr);
}

// 玩家优先规则：玩家伤害 >= 生物伤害的 1/3 时返回玩家。
// 这条规则依赖 isPlayerSource()（DamageType 上的静态属性），改走 id 反查后必须保持成立。
TEST_F(CombatTrackerUafTest, GetBestAttackerPrefersPlayerWhenDamageIsSignificant)
{
    auto mob = std::make_unique<TestMob>(EntityInstanceId(106));
    mob->setWorld(m_world.get());
    auto player = std::make_unique<TestPlayerEntity>(EntityInstanceId(107));
    player->setWorld(m_world.get());

    m_world->registerEntityForLookup(mob.get());
    m_world->registerEntityForLookup(player.get());

    // 生物造成 3 点，玩家造成 5 点（5 >= 3/3，满足玩家优先条件）。
    {
        auto mobSource = DamageSources::mobAttack(mob.get());
        m_victim->combatTracker().trackDamage(mobSource, m_victim->health(), 3.0f);
    }
    {
        auto playerSource = DamageSources::playerAttack(player.get());
        m_victim->combatTracker().trackDamage(playerSource, m_victim->health(), 5.0f);
    }

    EXPECT_EQ(m_victim->combatTracker().getBestAttacker(), static_cast<Entity*>(player.get()));
}

// 无实体来源（环境伤害）不应崩溃，且不返回任何攻击者。
TEST_F(CombatTrackerUafTest, GetBestAttackerWithEnvironmentalDamageReturnsNull)
{
    auto source = DamageSources::outOfWorld();
    m_victim->combatTracker().trackDamage(source, m_victim->health(), 4.0f);

    EXPECT_EQ(m_victim->combatTracker().getBestAttacker(), nullptr);
    EXPECT_EQ(m_victim->combatTracker().getLastAttacker(), nullptr);
    EXPECT_FALSE(m_victim->combatTracker().getDeathMessage().empty());
}
