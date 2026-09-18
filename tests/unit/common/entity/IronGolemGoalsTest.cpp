/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction restriction, including without limitation the rights
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

#include <memory>
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/entities/monster/undead/ZombieEntity.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/Items.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "entity/ai/goal/GoalFlag.hpp"
#include "entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "entity/ai/goal/goals/special/IronGolemGoals.hpp"
#include "entity/entities/passive/golem/IronGolemEntity.hpp"

namespace mc {
namespace test {

// ==================== IronGolemGoals 测试夹具 ====================

class IronGolemGoalsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        ironGolem = std::make_unique<IronGolemEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    }

    void TearDown() override { ironGolem.reset(); }

    std::unique_ptr<IronGolemEntity> ironGolem;
};

// ==================== OfferFlowerGoal 测试夹具 ====================

class OfferFlowerGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        ironGolem = std::make_unique<IronGolemEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
        goal = std::make_unique<entity::ai::goal::OfferFlowerGoal>(ironGolem.get());
    }

    void TearDown() override
    {
        goal.reset();
        ironGolem.reset();
    }

    std::unique_ptr<IronGolemEntity> ironGolem;
    std::unique_ptr<entity::ai::goal::OfferFlowerGoal> goal;
};

// ==================== OfferFlowerGoal 基础测试 ====================

TEST_F(OfferFlowerGoalTest, Construction)
{
    EXPECT_NE(goal, nullptr);
}

TEST_F(OfferFlowerGoalTest, GetTypeName)
{
    EXPECT_EQ(goal->getTypeName(), "OfferFlowerGoal");
}

TEST_F(OfferFlowerGoalTest, ShouldExecuteReturnsFalseWithoutWorld)
{
    // 无世界时不应执行
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(OfferFlowerGoalTest, MutexFlags)
{
    // 验证互斥标志包含 Move 和 Look
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(::mc::entity::ai::GoalFlag::Move));
    EXPECT_TRUE(flags.test(::mc::entity::ai::GoalFlag::Look));
}

TEST_F(OfferFlowerGoalTest, StartExecutingSetsHoldingRose)
{
    // 开始执行时应设置持花状态
    EXPECT_FALSE(ironGolem->isHoldingRose());
    goal->startExecuting();
    EXPECT_TRUE(ironGolem->isHoldingRose());
    EXPECT_GT(ironGolem->getHoldRoseTick(), 0);
}

TEST_F(OfferFlowerGoalTest, ResetTaskClearsHoldingRose)
{
    // 重置时应清除持花状态（对应 MC stop() 中 offerFlower(false)）
    goal->startExecuting();
    EXPECT_TRUE(ironGolem->isHoldingRose());
    goal->resetTask();
    EXPECT_FALSE(ironGolem->isHoldingRose());
}

TEST_F(OfferFlowerGoalTest, ShouldContinueExecutingAfterStart)
{
    // 开始执行后 shouldContinueExecuting 应返回 true（m_tick > 0）
    goal->startExecuting();
    EXPECT_TRUE(goal->shouldContinueExecuting());
}

TEST_F(OfferFlowerGoalTest, TickDecrementsTimer)
{
    // startExecuting 后 m_tick = OFFER_TICKS（400），tick() 应递减
    goal->startExecuting();
    i32 before = ironGolem->getHoldRoseTick();
    goal->tick();
    // tick() 只递减 m_tick，不直接递减 m_holdRoseTick（由 IronGolemEntity::tick 处理）
    // 但 shouldContinueExecuting 依赖 m_tick，调用多次后仍应继续执行
    EXPECT_TRUE(goal->shouldContinueExecuting());
}

// ==================== MoveTowardsTargetGoal 测试夹具 ====================

class MoveTowardsTargetGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        ironGolem = std::make_unique<IronGolemEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
        goal = std::make_unique<entity::ai::goal::MoveTowardsTargetGoal>(ironGolem.get(), 0.9, 32.0f);
    }

    void TearDown() override
    {
        goal.reset();
        ironGolem.reset();
    }

    std::unique_ptr<IronGolemEntity> ironGolem;
    std::unique_ptr<entity::ai::goal::MoveTowardsTargetGoal> goal;
};

TEST_F(MoveTowardsTargetGoalTest, Construction)
{
    EXPECT_NE(goal, nullptr);
}

TEST_F(MoveTowardsTargetGoalTest, GetTypeName)
{
    EXPECT_EQ(goal->getTypeName(), "MoveTowardsTargetGoal");
}

TEST_F(MoveTowardsTargetGoalTest, ShouldExecuteReturnsFalseWithoutTarget)
{
    // 无目标时不应执行
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(MoveTowardsTargetGoalTest, MutexFlags)
{
    // 验证互斥标志包含 Move
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(::mc::entity::ai::GoalFlag::Move));
}

// ==================== IronGolemEntity 集成测试 ====================

class IronGolemEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 注册原版实体类型，使 VanillaEntityTypeKeys 指针非空，可解引用传入 canAttackType。
        // registerAll() 幂等且线程安全，多次调用无副作用。
        entity::VanillaEntities::registerAll();
        ironGolem = std::make_unique<IronGolemEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    }

    void TearDown() override { ironGolem.reset(); }

    std::unique_ptr<IronGolemEntity> ironGolem;
};

TEST_F(IronGolemEntityTest, Construction)
{
    EXPECT_NE(ironGolem, nullptr);
    EXPECT_FALSE(ironGolem->isArmsRaised());
    EXPECT_FALSE(ironGolem->isHoldingRose());
    EXPECT_FALSE(ironGolem->isPlayerCreated());
}

TEST_F(IronGolemEntityTest, ArmsRaisedState)
{
    EXPECT_FALSE(ironGolem->isArmsRaised());
    ironGolem->setArmsRaised(true);
    EXPECT_TRUE(ironGolem->isArmsRaised());
    ironGolem->setArmsRaised(false);
    EXPECT_FALSE(ironGolem->isArmsRaised());
}

TEST_F(IronGolemEntityTest, HoldingRoseState)
{
    EXPECT_FALSE(ironGolem->isHoldingRose());
    ironGolem->setHoldingRose(true);
    EXPECT_TRUE(ironGolem->isHoldingRose());
    EXPECT_GT(ironGolem->getHoldRoseTick(), 0);
    ironGolem->setHoldingRose(false);
    EXPECT_FALSE(ironGolem->isHoldingRose());
    EXPECT_EQ(ironGolem->getHoldRoseTick(), 0);
}

TEST_F(IronGolemEntityTest, PlayerCreatedState)
{
    EXPECT_FALSE(ironGolem->isPlayerCreated());
    ironGolem->setPlayerCreated(true);
    EXPECT_TRUE(ironGolem->isPlayerCreated());
}

TEST_F(IronGolemEntityTest, CanAttackType)
{
    // 玩家创建的铁傀儡不攻击玩家
    ironGolem->setPlayerCreated(true);
    EXPECT_FALSE(ironGolem->canAttackType(*entity::VanillaEntityTypeKeys::PLAYER));

    // 铁傀儡不攻击苦力怕
    EXPECT_FALSE(ironGolem->canAttackType(*entity::VanillaEntityTypeKeys::CREEPER));

    // 非玩家创建的铁傀儡默认允许攻击（除苦力怕和玩家创建者外的类型）
    // 使用 EntityType::UNKNOWN 测试默认行为：UNKNOWN 不等于 PLAYER/CREEPER/GHAST，应允许攻击
    ironGolem->setPlayerCreated(false);
    EXPECT_TRUE(ironGolem->canAttackType(entity::EntityType::UNKNOWN));
}

TEST_F(IronGolemEntityTest, IAngerableInterface)
{
    // 测试愤怒接口
    EXPECT_FALSE(ironGolem->isAngry());
    ironGolem->setAngry(true);
    EXPECT_TRUE(ironGolem->isAngry());
    EXPECT_GT(ironGolem->getAngerTime(), 0);

    ironGolem->setAngry(false);
    EXPECT_FALSE(ironGolem->isAngry());
    EXPECT_EQ(ironGolem->getAngerTime(), 0);
}

TEST_F(IronGolemEntityTest, EyeHeight)
{
    EXPECT_FLOAT_EQ(ironGolem->eyeHeight(), 2.1f);
}

TEST_F(IronGolemEntityTest, Dimensions)
{
    EXPECT_FLOAT_EQ(ironGolem->width(), 1.4f);
    EXPECT_FLOAT_EQ(ironGolem->height(), 2.7f);
}

// ==================== GolemEntity 测试 ====================

class GolemEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        golem = std::make_unique<IronGolemEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    }

    void TearDown() override { golem.reset(); }

    std::unique_ptr<IronGolemEntity> golem;
};

TEST_F(GolemEntityTest, IAngerableGetRevengeTarget)
{
    // 无复仇目标时返回 nullptr
    EXPECT_EQ(golem->getRevengeTarget(), nullptr);
    EXPECT_EQ(golem->getRevengeTimer(), 0);
}

TEST_F(GolemEntityTest, IAngerableSetRevengeTarget)
{
    // 设置复仇目标（但目标实体不在世界中，所以 getRevengeTarget 仍返回 nullptr）
    // 这里只测试设置不崩溃
    golem->setRevengeTarget(nullptr);
    EXPECT_EQ(golem->getRevengeTarget(), nullptr);
}

// ==================== DefendVillageTargetGoal 测试 ====================
//
// 对齐 vanilla 1.21.11 IronGolem.DefendVillageTargetGoal（IronGolem.java:74 注册优先级 1）。
// 修复前：IronGolemEntity::registerGoals 未注册 DefendVillageTargetGoal，goal 类虽在
// IronGolemGoals 完整实现（shouldExecute 扫描 16 格内村民→取村民 getLastHurtBy→isSuitableTarget
// 通过后写 attackTarget）但是死代码——村庄内村民被攻击时铁傀儡不会自动锁敌。本组验证
// 修复后链路激活：村民被攻击者伤害 → 铁傀儡 targetSelector.tick 触发 DefendVillage
// shouldExecute → startExecuting 调 setAttackTarget(攻击者)。
//
// 关键驱动机制：DefendVillageTargetGoal::shouldExecute 经 EntityUtils::findClosestEntity
// <VillagerEntity> 查找村民，其内部调 world->getEntitiesInRange(pos, range, except)（非
// getEntitiesInAABB）。故测试世界须 override getEntitiesInRange 注入预设村民。村民本身不被
// isSuitableTarget 检查（仅经 dynamic_cast<VillagerEntity*> 筛选），但攻击者须经
// isSuitableTarget→canAttackType(*attackerType)，attackerType 由 entityType() 懒查询得到，
// 故攻击者须 setTypeId 对齐工厂路径（详见 target/README.md §13）。

namespace {

/// @brief 保卫村庄测试用世界：override getEntitiesInRange 返回预设村民列表。
///
/// DefendVillageTargetGoal::shouldExecute→findClosestEntity<VillagerEntity> 经此方法取候选，
/// vanilla 搜索半径 16 格。except 为铁傀儡自身（findClosestEntity 传入），此处忽略以简化。
class IronGolemDefendVillageTestWorld final : public mc::test::BaseTestWorld {
public:
    void setNearbyVillagers(std::vector<mc::Entity*> villagers) { m_villagers = std::move(villagers); }

    [[nodiscard]] std::vector<mc::Entity*> getEntitiesInRange(const mc::Vector3&, f32, const mc::Entity*) const override
    {
        return m_villagers;
    }

private:
    std::vector<mc::Entity*> m_villagers;
};

} // namespace

class DefendVillageTargetGoalTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 注册所有实体类型以使 entityType() 查表非 null——DefendVillageTargetGoal::isSuitableTarget
        // 内 canAttackType(*attackerType) 依赖攻击者 entityType() 非 null，未注册时返 false，
        // 保卫村庄链路无法触发。registerAll 进程级幂等。
        mc::entity::VanillaEntities::registerAll();
    }

    void SetUp() override
    {
        mc::VanillaBlocks::initialize();
        mc::Items::initialize();

        // 铁傀儡：村庄保卫者。setTypeId 对齐工厂路径（EntityType::create 会 setTypeId），
        // 直接构造的实体 m_typeId 默认空，entityType() 懒查询返 nullptr。设 targetSelector 每
        // tick 评估（绕 tickRate=2 节流），便于单测稳定触发。
        m_golem = std::make_unique<mc::IronGolemEntity>(mc::EntityInstanceId(1), mc::test::testEcsRegistry());
        m_golem->setTypeId("minecraft:iron_golem");
        m_golem->setWorld(&m_world);
        m_golem->setPosition(0.0, 64.0, 0.0);
        m_golem->targetSelector().setTickRate(1);
    }

    void TearDown() override { m_golem.reset(); }

    IronGolemDefendVillageTestWorld m_world;
    std::unique_ptr<mc::IronGolemEntity> m_golem;
};

TEST_F(DefendVillageTargetGoalTest, DoesNotLockTarget_WhenNoVillagerNearby)
{
    // 无村民时 shouldExecute 找不到村民，返 false，不锁敌。
    m_world.setNearbyVillagers({});
    m_golem->targetSelector().tick();
    EXPECT_EQ(m_golem->attackTarget(), nullptr);
}

TEST_F(DefendVillageTargetGoalTest, DoesNotLockTarget_WhenVillagerHasNoAttacker)
{
    // 村民附近但未被攻击（getLastHurtBy==nullptr）时不应锁敌。
    mc::entity::VillagerEntity villager(mc::EntityInstanceId(2), mc::test::testEcsRegistry());
    villager.setTypeId("minecraft:villager");
    villager.setWorld(&m_world);
    villager.setPosition(2.0, 64.0, 0.0);

    m_world.setNearbyVillagers({&villager});
    m_golem->targetSelector().tick();
    EXPECT_EQ(m_golem->attackTarget(), nullptr);
}

TEST_F(DefendVillageTargetGoalTest, LocksAttacker_WhenVillagerHurtByZombie)
{
    // 核心链路：村民被僵尸攻击 → 铁傀儡 DefendVillage 锁定僵尸。
    // 攻击者用 ZombieEntity（MonsterEntity 子类，canAttackType 仅排除苦力怕/玩家创建者玩家/
    // 凋灵，僵尸可被攻击）。setTypeId 使 entityType() 懒查询非 null，isSuitableTarget 通过。
    mc::ZombieEntity attacker(mc::EntityInstanceId(3), mc::test::testEcsRegistry());
    attacker.setTypeId("minecraft:zombie");
    attacker.setWorld(&m_world);
    attacker.setPosition(4.0, 64.0, 0.0);

    mc::entity::VillagerEntity villager(mc::EntityInstanceId(2), mc::test::testEcsRegistry());
    villager.setTypeId("minecraft:villager");
    villager.setWorld(&m_world);
    villager.setPosition(2.0, 64.0, 0.0);
    // 村民记录攻击者（对齐 vanilla DefendVillage 取 villager.getLastHurtBy()）。
    villager.setLastHurtBy(&attacker);

    m_world.setNearbyVillagers({&villager});
    EXPECT_EQ(m_golem->attackTarget(), nullptr);

    m_golem->targetSelector().tick();

    // DefendVillageTargetGoal::startExecuting 调 setAttackTarget(攻击者)。
    EXPECT_EQ(m_golem->attackTarget(), &attacker);
}

TEST_F(DefendVillageTargetGoalTest, DoesNotLock_WhenAttackerIsDead)
{
    // isSuitableTarget 拒绝已死亡/移除的目标（DefendVillage.shouldExecute 第 441 行
    // `if (!attacker || !attacker->isAlive())`）。村民被攻击者伤害但攻击者已移除时不应锁敌。
    // Entity::isAlive() 基于 m_removed 标志（非 health），discard() 设 m_removed=true。
    mc::ZombieEntity attacker(mc::EntityInstanceId(3), mc::test::testEcsRegistry());
    attacker.setTypeId("minecraft:zombie");
    attacker.setWorld(&m_world);
    attacker.setPosition(4.0, 64.0, 0.0);
    attacker.discard(); // 攻击者已移除（isAlive()==false）

    mc::entity::VillagerEntity villager(mc::EntityInstanceId(2), mc::test::testEcsRegistry());
    villager.setTypeId("minecraft:villager");
    villager.setWorld(&m_world);
    villager.setPosition(2.0, 64.0, 0.0);
    villager.setLastHurtBy(&attacker);

    m_world.setNearbyVillagers({&villager});
    m_golem->targetSelector().tick();
    EXPECT_EQ(m_golem->attackTarget(), nullptr);
}

TEST_F(DefendVillageTargetGoalTest, LocksNearestVillagerAttacker_WhenMultipleVillagers)
{
    // 多个村民被攻击时，DefendVillage 取最近的村民的攻击者（findClosestEntity 选最近）。
    mc::ZombieEntity attackerFar(mc::EntityInstanceId(4), mc::test::testEcsRegistry());
    attackerFar.setTypeId("minecraft:zombie");
    attackerFar.setWorld(&m_world);
    attackerFar.setPosition(8.0, 64.0, 0.0);

    mc::ZombieEntity attackerNear(mc::EntityInstanceId(3), mc::test::testEcsRegistry());
    attackerNear.setTypeId("minecraft:zombie");
    attackerNear.setWorld(&m_world);
    attackerNear.setPosition(4.0, 64.0, 0.0);

    // 远村民（距铁傀儡 6 格）被远僵尸攻击。
    mc::entity::VillagerEntity villagerFar(mc::EntityInstanceId(6), mc::test::testEcsRegistry());
    villagerFar.setTypeId("minecraft:villager");
    villagerFar.setWorld(&m_world);
    villagerFar.setPosition(6.0, 64.0, 0.0);
    villagerFar.setLastHurtBy(&attackerFar);

    // 近村民（距铁傀儡 2 格）被近僵尸攻击。
    mc::entity::VillagerEntity villagerNear(mc::EntityInstanceId(5), mc::test::testEcsRegistry());
    villagerNear.setTypeId("minecraft:villager");
    villagerNear.setWorld(&m_world);
    villagerNear.setPosition(2.0, 64.0, 0.0);
    villagerNear.setLastHurtBy(&attackerNear);

    // 注意：findClosestEntity 遍历顺序取决于 getEntitiesInRange 返回顺序，选距离最近的村民。
    m_world.setNearbyVillagers({&villagerFar, &villagerNear});
    m_golem->targetSelector().tick();

    // 最近村民的攻击者是 attackerNear。
    EXPECT_EQ(m_golem->attackTarget(), &attackerNear);
}

} // namespace test
} // namespace mc
