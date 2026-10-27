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

// lastHurtByPlayer 实体引用归属测试。
//
// 验证 LivingEntity::hurt 的"记录最近攻击者"段对齐 vanilla
// LivingEntity.resolvePlayerResponsibleForDamage（LivingEntity.java:1334-1348）：
//   - 真凶为 Player → setLastHurtByPlayer(player, 100)；
//   - 真凶为已驯服的 Wolf → 归属其主人玩家；主人引用解析不到时清空归属并归零记忆窗口；
//   - 其他实体 → 不改动玩家归属。
// 并验证该引用经 getKillCredit（LivingEntity.java:1886-1892）优先于 lastHurtByMob 生效，
// 以及记忆窗口到期（aiStep:466-470）时引用被清空。

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/entity/entities/passive/tamable/WolfEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"

using namespace mc;

namespace {

/// 普通测试生物：用于观察玩家归属
class TestLivingEntity : public LivingEntity {
public:
    TestLivingEntity()
        : LivingEntity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

/// 测试玩家：注册属性并补满生命值
class TestPlayer final : public Player {
public:
    TestPlayer(EntityInstanceId id, const std::string& name)
        : Player(id, name, mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

/// 测试狼：允许指定实例 id，便于同夹具内多只狼
class TestWolf final : public WolfEntity {
public:
    explicit TestWolf(EntityInstanceId id)
        : WolfEntity(id, mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

/// 测试世界：只覆写玩家列表，供 TameableEntity::getOwner 按 UUID 解析主人
class TestWorld final : public mc::test::BaseTestWorld {
public:
    TestWorld() = default;

    [[nodiscard]] std::vector<Entity*> getPlayers() const override
    {
        return m_players;
    }

    std::vector<Entity*> m_players;
};

} // namespace

class HurtByPlayerAttributionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::ensureInitialized();
        DamageTypeTags::initialize();
    }

    /// 让狼已驯服并绑定主人 UUID（同 vanilla TamableAnimal.setOwnerUUID + setTame）
    static void _makeTamedBy(WolfEntity& wolf, const Player& owner)
    {
        wolf.setOwnerId(owner.uuidBytes());
        wolf.setTamed(true);
    }

    TestWorld m_world;
};

// 玩家直接攻击：记录玩家归属，getKillCredit 返回该玩家。
TEST_F(HurtByPlayerAttributionTest, PlayerAttackSetsLastHurtByPlayer)
{
    TestLivingEntity victim;
    victim.setWorld(&m_world);
    TestPlayer attacker(EntityInstanceId(2), "Attacker");
    attacker.setWorld(&m_world);
    // 世界列表不注册攻击者：玩家归属是直接指针，不依赖世界查找

    auto source = DamageSources::playerAttack(&attacker);
    victim.hurt(source, 1.0f);

    EXPECT_EQ(victim.getLastHurtByPlayer(), &attacker);
    EXPECT_EQ(victim.lastHurtByPlayerMemoryTime(), 100);
    EXPECT_EQ(victim.getKillCredit(), &attacker);
}

// 驯服狼代攻：功劳归属其主人玩家（vanilla Wolf 分支）。
TEST_F(HurtByPlayerAttributionTest, TamedWolfAttackCreditsOwner)
{
    TestPlayer owner(EntityInstanceId(2), "Owner");
    owner.setUuid("00000000-0000-0000-0000-000000000002");
    owner.setWorld(&m_world);
    m_world.m_players = {&owner};

    TestWolf wolf(EntityInstanceId(3));
    wolf.setWorld(&m_world);
    _makeTamedBy(wolf, owner);

    TestLivingEntity victim;
    victim.setWorld(&m_world);

    auto source = DamageSources::mobAttack(&wolf);
    victim.hurt(source, 1.0f);

    EXPECT_EQ(victim.getLastHurtByPlayer(), &owner);
    EXPECT_EQ(victim.getKillCredit(), &owner);
    // 狼本身仍记为最近攻击生物（HurtByTargetGoal 反击目标）
    EXPECT_EQ(victim.getLastHurtBy(), &wolf);
}

// 已驯服但主人解析不到（主人 UUID 缺失）：清空归属并归零记忆窗口（vanilla else 分支）。
TEST_F(HurtByPlayerAttributionTest, TamedWolfWithoutResolvableOwnerClearsAttribution)
{
    TestPlayer owner(EntityInstanceId(2), "Owner");
    owner.setUuid("00000000-0000-0000-0000-000000000002");
    owner.setWorld(&m_world);
    m_world.m_players = {&owner};

    TestWolf wolf(EntityInstanceId(3));
    wolf.setWorld(&m_world);
    _makeTamedBy(wolf, owner);

    TestLivingEntity victim;
    victim.setWorld(&m_world);
    // 先留下玩家的既有归属。注：同一实体的第二次 hurt 会命中 hurtTime 无敌帧门控而
    // 不进入 resolve 段，故这里用 setter 直接构造既有归属状态，独立验证本次改动分支。
    victim.setLastHurtByPlayer(&owner, 100);
    ASSERT_EQ(victim.getLastHurtByPlayer(), &owner);

    // 主人离开世界后狼再代攻：解析不到主人 → 归属清空
    m_world.m_players.clear();
    auto source = DamageSources::mobAttack(&wolf);
    victim.hurt(source, 1.0f);

    EXPECT_EQ(victim.getLastHurtByPlayer(), nullptr);
    EXPECT_EQ(victim.lastHurtByPlayerMemoryTime(), 0);
}

// 未驯服的狼代攻：不产生玩家归属。
TEST_F(HurtByPlayerAttributionTest, WildWolfDoesNotCreditPlayer)
{
    TestPlayer owner(EntityInstanceId(2), "Owner");
    owner.setUuid("00000000-0000-0000-0000-000000000002");
    owner.setWorld(&m_world);
    m_world.m_players = {&owner};

    TestWolf wolf(EntityInstanceId(3));
    wolf.setWorld(&m_world);
    wolf.setOwnerId(owner.uuidBytes());
    // 不设为驯服：野生狼

    TestLivingEntity victim;
    victim.setWorld(&m_world);
    auto source = DamageSources::mobAttack(&wolf);
    victim.hurt(source, 1.0f);

    EXPECT_EQ(victim.getLastHurtByPlayer(), nullptr);
    EXPECT_EQ(victim.lastHurtByPlayerMemoryTime(), 0);
}

// 记忆窗口到期后清空玩家引用（vanilla aiStep:466-470 的 else 分支）。
TEST_F(HurtByPlayerAttributionTest, MemoryWindowExpiryClearsReference)
{
    TestLivingEntity victim;
    victim.setWorld(&m_world);
    TestPlayer attacker(EntityInstanceId(2), "Attacker");
    attacker.setWorld(&m_world);

    auto source = DamageSources::playerAttack(&attacker);
    victim.hurt(source, 1.0f);
    ASSERT_EQ(victim.getLastHurtByPlayer(), &attacker);
    ASSERT_EQ(victim.lastHurtByPlayerMemoryTime(), 100);

    victim.setLastHurtByPlayerMemoryTime(1);
    victim.tick(); // 100 → 1（本 tick 递减到 0 之前）
    EXPECT_EQ(victim.lastHurtByPlayerMemoryTime(), 0);
    // 递减到 0 的同一 tick 不清空；下一 tick 走 else 分支清空
    EXPECT_EQ(victim.getLastHurtByPlayer(), &attacker);

    victim.tick();
    EXPECT_EQ(victim.getLastHurtByPlayer(), nullptr);
}

// 非玩家、非狼的攻击者不改动既有玩家归属（例如玩家打残后由僵尸补刀，仍归属玩家）。
TEST_F(HurtByPlayerAttributionTest, NonPlayerAttackerKeepsExistingAttribution)
{
    TestLivingEntity victim;
    victim.setWorld(&m_world);
    TestPlayer attacker(EntityInstanceId(2), "Attacker");
    attacker.setWorld(&m_world);

    // 先留下玩家归属（直接 setter，绕过 hurt 的无敌帧门控）
    victim.setLastHurtByPlayer(&attacker, 100);
    ASSERT_EQ(victim.getLastHurtByPlayer(), &attacker);

    TestLivingEntity zombieLike;
    auto mobSource = DamageSources::mobAttack(&zombieLike);
    victim.hurt(mobSource, 1.0f);

    EXPECT_EQ(victim.getLastHurtByPlayer(), &attacker);
    // 最近攻击生物更新为补刀者，但击杀归属仍优先玩家
    EXPECT_EQ(victim.getLastHurtBy(), &zombieLike);
    EXPECT_EQ(victim.getKillCredit(), &attacker);
}
