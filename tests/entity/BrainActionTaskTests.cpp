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
18 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include <memory>

#include "common/TestWorldHelper.hpp"
#include "common/entity/ai/brain/Brain.hpp"
#include "common/entity/ai/brain/memory/MemoryModuleStatus.hpp"
#include "common/entity/ai/brain/memory/MemoryModuleType.hpp"
#include "common/entity/ai/brain/schedule/Schedule.hpp"
#include "common/entity/ai/brain/task/tasks/action/ActionTasks.hpp"
#include "common/entity/entities/monster/undead/ZombieEntity.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"

using mc::BlockPos;
using mc::EntityInstanceId;
using mc::i64;
using mc::LivingEntity;
using mc::Vector3;
using mc::ZombieEntity;
using mc::entity::VillagerEntity;
using mc::test::BaseTestWorld;
namespace action = mc::entity::ai::brain::task::action;
namespace memory = mc::entity::ai::brain::memory;
using memory::MemoryModuleStatus;
using memory::MemoryModuleTypes;

namespace {

// ============================================================================
// AttackTask 记忆模块需求测试
// ============================================================================

class AttackTaskMemoryTest : public ::testing::Test {
protected:
    void SetUp() override { MemoryModuleTypes::initialize(); }
};

// 测试：AttackTask 的记忆模块需求 - ATTACK_TARGET 必须存在
TEST_F(AttackTaskMemoryTest, RequiresAttackTargetPresent)
{
    mc::entity::ai::brain::Brain<int> brain;
    brain.registerMemory(MemoryModuleTypes::ATTACK_TARGET);
    brain.registerMemory(MemoryModuleTypes::ATTACK_COOLING_DOWN);
    brain.registerMemory(MemoryModuleTypes::LOOK_TARGET);

    // 初始状态：ATTACK_TARGET 缺失
    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::ATTACK_TARGET, MemoryModuleStatus::VALUE_PRESENT));
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::ATTACK_TARGET, MemoryModuleStatus::VALUE_ABSENT));
}

// 测试：AttackTask 的记忆模块需求 - ATTACK_COOLING_DOWN 必须不存在
TEST_F(AttackTaskMemoryTest, RequiresCoolingDownAbsent)
{
    mc::entity::ai::brain::Brain<int> brain;
    brain.registerMemory(MemoryModuleTypes::ATTACK_COOLING_DOWN);

    // 初始状态：ATTACK_COOLING_DOWN 不存在 → VALUE_ABSENT 成立
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::ATTACK_COOLING_DOWN, MemoryModuleStatus::VALUE_ABSENT));

    // 设置冷却后 → VALUE_ABSENT 不成立
    brain.setMemoryWithTTL<bool>(MemoryModuleTypes::ATTACK_COOLING_DOWN, true, 20);
    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::ATTACK_COOLING_DOWN, MemoryModuleStatus::VALUE_ABSENT));
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::ATTACK_COOLING_DOWN, MemoryModuleStatus::VALUE_PRESENT));
}

// 测试：AttackTask 的记忆模块需求 - LOOK_TARGET 必须已注册
TEST_F(AttackTaskMemoryTest, RequiresLookTargetRegistered)
{
    mc::entity::ai::brain::Brain<int> brain;
    brain.registerMemory(MemoryModuleTypes::LOOK_TARGET);

    // LOOK_TARGET 注册即可（REGISTERED），不需要有值
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::LOOK_TARGET, MemoryModuleStatus::REGISTERED));
}

// ============================================================================
// 测试用世界子类
// ============================================================================

class ActionTaskTestWorld : public BaseTestWorld {
public:
    ActionTaskTestWorld() = default;
};

// ============================================================================
// AttackTask 核心逻辑测试
// ============================================================================

class AttackTaskTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        MemoryModuleTypes::initialize();
        mc::entity::ai::brain::schedule::Schedule::initialize();
        m_world = std::make_unique<ActionTaskTestWorld>();
    }

    std::unique_ptr<ActionTaskTestWorld> m_world;
};

// 测试：AttackTask 在没有 ATTACK_TARGET 记忆时不应执行
TEST_F(AttackTaskTest, ShouldNotExecuteWithoutAttackTarget)
{
    VillagerEntity villager(EntityInstanceId(1), mc::test::testEcsRegistry());
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    // 注册记忆但未设置 ATTACK_TARGET
    auto& brain = villager.brain();
    brain.registerMemory(MemoryModuleTypes::ATTACK_TARGET);
    brain.registerMemory(MemoryModuleTypes::ATTACK_COOLING_DOWN);
    brain.registerMemory(MemoryModuleTypes::LOOK_TARGET);

    action::AttackTask<VillagerEntity> task;
    mc::math::Random rng(42);

    // 没有 ATTACK_TARGET → shouldExecute 不满足 → start() 返回 false
    EXPECT_FALSE(task.start(m_world.get(), &villager, 0, rng));
}

// 测试：AttackTask 在有 ATTACK_COOLING_DOWN 记忆时不应执行
TEST_F(AttackTaskTest, ShouldNotExecuteWhenCoolingDown)
{
    VillagerEntity villager(EntityInstanceId(1), mc::test::testEcsRegistry());
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    auto& brain = villager.brain();
    brain.registerMemory(MemoryModuleTypes::ATTACK_TARGET);
    brain.registerMemory(MemoryModuleTypes::ATTACK_COOLING_DOWN);
    brain.registerMemory(MemoryModuleTypes::LOOK_TARGET);

    // 设置攻击目标（僵尸靠近村民）
    ZombieEntity zombie(EntityInstanceId(2), mc::test::testEcsRegistry());
    zombie.setWorld(m_world.get());
    zombie.setPosition(1.0f, 64.0f, 0.0f);
    m_world->registerEntityForLookup(&zombie);
    brain.setMemory<EntityInstanceId>(MemoryModuleTypes::ATTACK_TARGET, zombie.id());

    // 设置冷却记忆
    brain.setMemoryWithTTL<bool>(MemoryModuleTypes::ATTACK_COOLING_DOWN, true, 20);

    action::AttackTask<VillagerEntity> task;
    mc::math::Random rng(42);

    // ATTACK_COOLING_DOWN 存在 → _hasRequiredMemories 检查失败 → start() 返回 false
    EXPECT_FALSE(task.start(m_world.get(), &villager, 0, rng));
}

// 测试：AttackTask 在所有条件满足时应执行
TEST_F(AttackTaskTest, ShouldExecuteWhenAllConditionsMet)
{
    VillagerEntity villager(EntityInstanceId(1), mc::test::testEcsRegistry());
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    auto& brain = villager.brain();
    brain.registerMemory(MemoryModuleTypes::ATTACK_TARGET);
    brain.registerMemory(MemoryModuleTypes::ATTACK_COOLING_DOWN);
    brain.registerMemory(MemoryModuleTypes::LOOK_TARGET);

    // 设置攻击目标（僵尸紧挨村民，在近战攻击范围内）
    ZombieEntity zombie(EntityInstanceId(2), mc::test::testEcsRegistry());
    zombie.setWorld(m_world.get());
    zombie.setPosition(0.5f, 64.0f, 0.0f);
    m_world->registerEntityForLookup(&zombie);
    brain.setMemory<EntityInstanceId>(MemoryModuleTypes::ATTACK_TARGET, zombie.id());

    action::AttackTask<VillagerEntity> task;
    mc::math::Random rng(42);

    // ATTACK_TARGET 存在 + ATTACK_COOLING_DOWN 不存在 + LOOK_TARGET 已注册 + 在攻击范围内 → 应执行
    EXPECT_TRUE(task.start(m_world.get(), &villager, 0, rng));
    EXPECT_EQ(task.getStatus(), mc::entity::ai::brain::task::TaskStatus::RUNNING);
}

// 测试：AttackTask 执行后设置 ATTACK_COOLING_DOWN 记忆
TEST_F(AttackTaskTest, SetsCoolingDownAfterAttack)
{
    VillagerEntity villager(EntityInstanceId(1), mc::test::testEcsRegistry());
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    auto& brain = villager.brain();
    brain.registerMemory(MemoryModuleTypes::ATTACK_TARGET);
    brain.registerMemory(MemoryModuleTypes::ATTACK_COOLING_DOWN);
    brain.registerMemory(MemoryModuleTypes::LOOK_TARGET);

    // 设置攻击目标
    ZombieEntity zombie(EntityInstanceId(2), mc::test::testEcsRegistry());
    zombie.setWorld(m_world.get());
    zombie.setPosition(0.5f, 64.0f, 0.0f);
    m_world->registerEntityForLookup(&zombie);
    brain.setMemory<EntityInstanceId>(MemoryModuleTypes::ATTACK_TARGET, zombie.id());

    action::AttackTask<VillagerEntity> task(20); // 20 tick 冷却
    mc::math::Random rng(42);

    EXPECT_TRUE(task.start(m_world.get(), &villager, 0, rng));

    // 攻击后应设置 ATTACK_COOLING_DOWN 记忆
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::ATTACK_COOLING_DOWN, MemoryModuleStatus::VALUE_PRESENT));

    auto coolingDown = brain.getMemory<bool>(MemoryModuleTypes::ATTACK_COOLING_DOWN);
    ASSERT_TRUE(coolingDown.has_value());
    EXPECT_TRUE(*coolingDown);
}

// 测试：AttackTask 是单次触发型任务，shouldContinueExecuting 返回 false
TEST_F(AttackTaskTest, IsSingleShotTask)
{
    VillagerEntity villager(EntityInstanceId(1), mc::test::testEcsRegistry());
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    auto& brain = villager.brain();
    brain.registerMemory(MemoryModuleTypes::ATTACK_TARGET);
    brain.registerMemory(MemoryModuleTypes::ATTACK_COOLING_DOWN);
    brain.registerMemory(MemoryModuleTypes::LOOK_TARGET);

    // 设置攻击目标
    ZombieEntity zombie(EntityInstanceId(2), mc::test::testEcsRegistry());
    zombie.setWorld(m_world.get());
    zombie.setPosition(0.5f, 64.0f, 0.0f);
    m_world->registerEntityForLookup(&zombie);
    brain.setMemory<EntityInstanceId>(MemoryModuleTypes::ATTACK_TARGET, zombie.id());

    action::AttackTask<VillagerEntity> task(20);
    mc::math::Random rng(42);

    EXPECT_TRUE(task.start(m_world.get(), &villager, 0, rng));

    // AttackTask 是单次触发型，tick 后应停止
    // shouldContinueExecuting 返回 false → stop() 被调用
    task.tick(m_world.get(), &villager, 1);
    EXPECT_EQ(task.getStatus(), mc::entity::ai::brain::task::TaskStatus::STOPPED);
}

// 测试：AttackTask 在目标太远时不应执行
TEST_F(AttackTaskTest, ShouldNotExecuteWhenTargetOutOfRange)
{
    VillagerEntity villager(EntityInstanceId(1), mc::test::testEcsRegistry());
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    auto& brain = villager.brain();
    brain.registerMemory(MemoryModuleTypes::ATTACK_TARGET);
    brain.registerMemory(MemoryModuleTypes::ATTACK_COOLING_DOWN);
    brain.registerMemory(MemoryModuleTypes::LOOK_TARGET);

    // 设置攻击目标（僵尸距离较远，超出近战攻击范围）
    ZombieEntity zombie(EntityInstanceId(2), mc::test::testEcsRegistry());
    zombie.setWorld(m_world.get());
    zombie.setPosition(10.0f, 64.0f, 0.0f); // 距离 10 格，超出近战范围
    m_world->registerEntityForLookup(&zombie);
    brain.setMemory<EntityInstanceId>(MemoryModuleTypes::ATTACK_TARGET, zombie.id());

    action::AttackTask<VillagerEntity> task;
    mc::math::Random rng(42);

    // 超出攻击范围 → shouldExecute 返回 false
    EXPECT_FALSE(task.start(m_world.get(), &villager, 0, rng));
}

// 测试：AttackTask 攻击范围计算
// 攻击范围 = (attackerWidth * 2)^2 + targetWidth
// VillagerEntity width = 0.6 (from MobEntity default), ZombieEntity width = 0.6
// 攻击范围 = (0.6 * 2)^2 + 0.6 = 1.44 + 0.6 = 2.04
// 距离 < sqrt(2.04) ≈ 1.428 时可攻击
TEST_F(AttackTaskTest, AttackRangeCalculation)
{
    VillagerEntity villager(EntityInstanceId(1), mc::test::testEcsRegistry());
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    auto& brain = villager.brain();
    brain.registerMemory(MemoryModuleTypes::ATTACK_TARGET);
    brain.registerMemory(MemoryModuleTypes::ATTACK_COOLING_DOWN);
    brain.registerMemory(MemoryModuleTypes::LOOK_TARGET);

    // 计算攻击范围：(0.6*2)^2 + 0.6 = 1.44 + 0.6 = 2.04
    // sqrt(2.04) ≈ 1.428
    mc::f32 attackReachSq = (0.6f * 2.0f) * (0.6f * 2.0f) + 0.6f; // = 2.04

    // 测试1: 在攻击范围内（距离 1.0）
    {
        ZombieEntity zombie(EntityInstanceId(2), mc::test::testEcsRegistry());
        zombie.setWorld(m_world.get());
        zombie.setPosition(1.0f, 64.0f, 0.0f);
        m_world->registerEntityForLookup(&zombie);
        brain.setMemory<EntityInstanceId>(MemoryModuleTypes::ATTACK_TARGET, zombie.id());

        action::AttackTask<VillagerEntity> task;
        mc::math::Random rng(42);
        EXPECT_TRUE(task.start(m_world.get(), &villager, 0, rng));
    }

    // 清除冷却记忆，以便下一次测试
    brain.removeMemory(MemoryModuleTypes::ATTACK_COOLING_DOWN);

    // 测试2: 刚好在攻击范围外（距离 1.5，距离平方 = 2.25 > 2.04）
    {
        ZombieEntity zombie3(EntityInstanceId(3), mc::test::testEcsRegistry());
        zombie3.setWorld(m_world.get());
        zombie3.setPosition(1.5f, 64.0f, 0.0f);
        m_world->registerEntityForLookup(&zombie3);
        brain.setMemory<EntityInstanceId>(MemoryModuleTypes::ATTACK_TARGET, zombie3.id());

        action::AttackTask<VillagerEntity> task;
        mc::math::Random rng(42);
        EXPECT_FALSE(task.start(m_world.get(), &villager, 0, rng));
    }
}

// 测试：AttackTask getName 返回正确名称
TEST_F(AttackTaskTest, GetNameReturnsCorrectName)
{
    action::AttackTask<VillagerEntity> task;
    EXPECT_EQ(task.getName(), "AttackTask");
}

// 测试：AttackTask 构造函数接受自定义冷却时间
TEST_F(AttackTaskTest, CustomCooldownTicks)
{
    // 默认冷却 20 tick
    action::AttackTask<VillagerEntity> taskDefault;
    EXPECT_EQ(taskDefault.getName(), "AttackTask"); // 验证构造成功

    // 自定义冷却 40 tick
    action::AttackTask<VillagerEntity> taskCustom(40);
    EXPECT_EQ(taskCustom.getName(), "AttackTask"); // 验证构造成功
}

// ============================================================================
// VillagerEntity Brain 集成测试 - AttackTask 注册验证
// ============================================================================

class AttackTaskIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        MemoryModuleTypes::initialize();
        mc::entity::ai::brain::schedule::Schedule::initialize();
    }
};

// 测试：VillagerEntity Brain 注册了 ATTACK_COOLING_DOWN 记忆模块
TEST_F(AttackTaskIntegrationTest, VillagerBrainRegistersAttackCoolingDownMemory)
{
    VillagerEntity villager(EntityInstanceId(1), mc::test::testEcsRegistry());
    auto& brain = villager.brain();

    // AttackTask 需要 ATTACK_COOLING_DOWN 记忆模块已注册
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::ATTACK_COOLING_DOWN, MemoryModuleStatus::REGISTERED))
        << "VillagerEntity Brain 应注册 ATTACK_COOLING_DOWN 记忆";
}

// 测试：VillagerEntity Brain 注册了 ATTACK_TARGET 记忆模块
TEST_F(AttackTaskIntegrationTest, VillagerBrainRegistersAttackTargetMemory)
{
    VillagerEntity villager(EntityInstanceId(1), mc::test::testEcsRegistry());
    auto& brain = villager.brain();

    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::ATTACK_TARGET, MemoryModuleStatus::REGISTERED))
        << "VillagerEntity Brain 应注册 ATTACK_TARGET 记忆";
}

// 测试：AttackTask 可以在 VillagerEntity Brain 中被创建并执行
TEST_F(AttackTaskIntegrationTest, AttackTaskWithVillagerBrain)
{
    ActionTaskTestWorld world;
    VillagerEntity villager(EntityInstanceId(1), mc::test::testEcsRegistry());
    villager.setWorld(&world);
    villager.setPosition(0.0f, 64.0f, 0.0f);

    auto& brain = villager.brain();

    // 设置攻击目标（僵尸靠近村民）
    ZombieEntity zombie(EntityInstanceId(2), mc::test::testEcsRegistry());
    zombie.setWorld(&world);
    zombie.setPosition(0.5f, 64.0f, 0.0f);
    world.registerEntityForLookup(&zombie);
    brain.setMemory<EntityInstanceId>(MemoryModuleTypes::ATTACK_TARGET, zombie.id());

    // AttackTask 应可以启动（VillagerEntity 的 Brain 已注册所有需要的记忆模块）
    action::AttackTask<VillagerEntity> task;
    mc::math::Random rng(42);

    EXPECT_TRUE(task.start(&world, &villager, 0, rng));

    // 验证冷却记忆被设置
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::ATTACK_COOLING_DOWN, MemoryModuleStatus::VALUE_PRESENT));
}

} // namespace
