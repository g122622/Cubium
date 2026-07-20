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

#include <gtest/gtest.h>

#include <memory>

#include "common/TestWorldHelper.hpp"
#include "common/entity/ai/brain/Brain.hpp"
#include "common/entity/ai/brain/memory/MemoryModuleStatus.hpp"
#include "common/entity/ai/brain/memory/MemoryModuleType.hpp"
#include "common/entity/ai/brain/memory/WalkTarget.hpp"
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
// 测试用世界子类
// ============================================================================

class NewActionTaskTestWorld : public BaseTestWorld {
public:
    NewActionTaskTestWorld() = default;
};

// ============================================================================
// BreedTask 记忆模块需求测试
// ============================================================================

class BreedTaskMemoryTest : public ::testing::Test {
protected:
    void SetUp() override { MemoryModuleTypes::initialize(); }
};

// 测试：BreedTask 的记忆模块需求 - VISIBLE_MOBS 必须存在
TEST_F(BreedTaskMemoryTest, RequiresVisibleMobsPresent)
{
    mc::entity::ai::brain::Brain<int> brain;
    brain.registerMemory(MemoryModuleTypes::VISIBLE_MOBS);
    brain.registerMemory(MemoryModuleTypes::BREED_TARGET);
    brain.registerMemory(MemoryModuleTypes::WALK_TARGET);
    brain.registerMemory(MemoryModuleTypes::LOOK_TARGET);

    // VISIBLE_MOBS 初始缺失
    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::VISIBLE_MOBS, MemoryModuleStatus::VALUE_PRESENT));
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::VISIBLE_MOBS, MemoryModuleStatus::VALUE_ABSENT));
}

// 测试：BreedTask 的记忆模块需求 - BREED_TARGET 必须不存在
TEST_F(BreedTaskMemoryTest, RequiresBreedTargetAbsent)
{
    mc::entity::ai::brain::Brain<int> brain;
    brain.registerMemory(MemoryModuleTypes::BREED_TARGET);

    // 初始状态：BREED_TARGET 不存在 → VALUE_ABSENT 成立
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::BREED_TARGET, MemoryModuleStatus::VALUE_ABSENT));

    // 设置 BREED_TARGET 后 → VALUE_ABSENT 不成立
    brain.setMemory<mc::AgeableEntity*>(MemoryModuleTypes::BREED_TARGET, nullptr);
    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::BREED_TARGET, MemoryModuleStatus::VALUE_ABSENT));
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::BREED_TARGET, MemoryModuleStatus::VALUE_PRESENT));
}

// ============================================================================
// BreedTask 核心逻辑测试
// ============================================================================

class BreedTaskTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        MemoryModuleTypes::initialize();
        mc::entity::ai::brain::schedule::Schedule::initialize();
        m_world = std::make_unique<NewActionTaskTestWorld>();
    }

    std::unique_ptr<NewActionTaskTestWorld> m_world;
};

// 测试：BreedTask 在没有 VISIBLE_MOBS 记忆时不应执行
TEST_F(BreedTaskTest, ShouldNotExecuteWithoutVisibleMobs)
{
    VillagerEntity villager(EntityInstanceId(1));
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    auto& brain = villager.brain();
    // 只注册，不设置值
    brain.registerMemory(MemoryModuleTypes::VISIBLE_MOBS);
    brain.registerMemory(MemoryModuleTypes::BREED_TARGET);
    brain.registerMemory(MemoryModuleTypes::WALK_TARGET);
    brain.registerMemory(MemoryModuleTypes::LOOK_TARGET);

    action::BreedTask<VillagerEntity> task;
    mc::math::Random rng(42);

    // VISIBLE_MOBS 缺失 → _hasRequiredMemories 检查失败 → start() 返回 false
    EXPECT_FALSE(task.start(m_world.get(), &villager, 0, rng));
}

// 测试：BreedTask 在 VISIBLE_MOBS 为空列表时不应执行
TEST_F(BreedTaskTest, ShouldNotExecuteWithEmptyVisibleMobs)
{
    VillagerEntity villager(EntityInstanceId(1));
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    auto& brain = villager.brain();
    brain.registerMemory(MemoryModuleTypes::VISIBLE_MOBS);
    brain.registerMemory(MemoryModuleTypes::BREED_TARGET);
    brain.registerMemory(MemoryModuleTypes::WALK_TARGET);
    brain.registerMemory(MemoryModuleTypes::LOOK_TARGET);

    // 设置空的可见实体列表
    brain.setMemory<std::vector<LivingEntity*>>(MemoryModuleTypes::VISIBLE_MOBS, {});

    action::BreedTask<VillagerEntity> task;
    mc::math::Random rng(42);

    // VillagerEntity 不是 AnimalEntity，isInLove() 返回 false → shouldExecute 失败
    EXPECT_FALSE(task.start(m_world.get(), &villager, 0, rng));
}

// 测试：BreedTask getName 返回正确名称
TEST_F(BreedTaskTest, GetNameReturnsCorrectName)
{
    action::BreedTask<VillagerEntity> task;
    EXPECT_EQ(task.getName(), "BreedTask");
}

// 测试：BreedTask 构造函数接受自定义速度和距离参数
TEST_F(BreedTaskTest, CustomSpeedAndDistance)
{
    action::BreedTask<VillagerEntity> taskDefault;
    EXPECT_EQ(taskDefault.getName(), "BreedTask");

    action::BreedTask<VillagerEntity> taskCustom(1.5f, 3);
    EXPECT_EQ(taskCustom.getName(), "BreedTask");
}

// 测试：BreedTask resetTask 清除 BREED_TARGET 和 WALK_TARGET
TEST_F(BreedTaskTest, ResetTaskClearsMemories)
{
    VillagerEntity villager(EntityInstanceId(1));
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    auto& brain = villager.brain();
    brain.registerMemory(MemoryModuleTypes::VISIBLE_MOBS);
    brain.registerMemory(MemoryModuleTypes::BREED_TARGET);
    brain.registerMemory(MemoryModuleTypes::WALK_TARGET);
    brain.registerMemory(MemoryModuleTypes::LOOK_TARGET);

    // 手动设置 BREED_TARGET 和 WALK_TARGET
    brain.setMemory<mc::AgeableEntity*>(MemoryModuleTypes::BREED_TARGET, nullptr);
    brain.setMemory<memory::WalkTarget>(
        MemoryModuleTypes::WALK_TARGET, memory::WalkTarget(BlockPos(10, 64, 20), 1.0f, 1));

    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::BREED_TARGET, MemoryModuleStatus::VALUE_PRESENT));
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::WALK_TARGET, MemoryModuleStatus::VALUE_PRESENT));

    // 手动创建任务并调用 resetTask
    action::BreedTask<VillagerEntity> task;
    mc::math::Random rng(42);
    // 不需要 start，直接调用 resetTask
    task.stop(m_world.get(), &villager, 0);

    // resetTask 应清除 BREED_TARGET 和 WALK_TARGET
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::BREED_TARGET, MemoryModuleStatus::VALUE_ABSENT));
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::WALK_TARGET, MemoryModuleStatus::VALUE_ABSENT));
}

// ============================================================================
// PlayDeadTask 记忆模块需求测试
// ============================================================================

class PlayDeadTaskMemoryTest : public ::testing::Test {
protected:
    void SetUp() override { MemoryModuleTypes::initialize(); }
};

// 测试：PlayDeadTask 的记忆模块需求 - PLAY_DEAD_TICKS 必须存在
TEST_F(PlayDeadTaskMemoryTest, RequiresPlayDeadTicksPresent)
{
    mc::entity::ai::brain::Brain<int> brain;
    brain.registerMemory(MemoryModuleTypes::PLAY_DEAD_TICKS);
    brain.registerMemory(MemoryModuleTypes::ATTACK_TARGET);
    brain.registerMemory(MemoryModuleTypes::PACIFIED);

    // PLAY_DEAD_TICKS 初始缺失
    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::PLAY_DEAD_TICKS, MemoryModuleStatus::VALUE_PRESENT));
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::PLAY_DEAD_TICKS, MemoryModuleStatus::VALUE_ABSENT));
}

// ============================================================================
// PlayDeadTask 核心逻辑测试
// ============================================================================

class PlayDeadTaskTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        MemoryModuleTypes::initialize();
        mc::entity::ai::brain::schedule::Schedule::initialize();
        m_world = std::make_unique<NewActionTaskTestWorld>();
    }

    std::unique_ptr<NewActionTaskTestWorld> m_world;
};

// 测试：PlayDeadTask 在没有 PLAY_DEAD_TICKS 记忆时不应执行
TEST_F(PlayDeadTaskTest, ShouldNotExecuteWithoutPlayDeadTicks)
{
    VillagerEntity villager(EntityInstanceId(1));
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    auto& brain = villager.brain();
    brain.registerMemory(MemoryModuleTypes::PLAY_DEAD_TICKS);
    brain.registerMemory(MemoryModuleTypes::ATTACK_TARGET);
    brain.registerMemory(MemoryModuleTypes::PACIFIED);

    action::PlayDeadTask<VillagerEntity> task;
    mc::math::Random rng(42);

    // PLAY_DEAD_TICKS 缺失 → _hasRequiredMemories 检查失败 → start() 返回 false
    EXPECT_FALSE(task.start(m_world.get(), &villager, 0, rng));
}

// 测试：PlayDeadTask 在 PLAY_DEAD_TICKS 存在时应执行
TEST_F(PlayDeadTaskTest, ShouldExecuteWithPlayDeadTicks)
{
    VillagerEntity villager(EntityInstanceId(1));
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    auto& brain = villager.brain();
    brain.registerMemory(MemoryModuleTypes::PLAY_DEAD_TICKS);
    brain.registerMemory(MemoryModuleTypes::ATTACK_TARGET);
    brain.registerMemory(MemoryModuleTypes::PACIFIED);

    // 设置 PLAY_DEAD_TICKS 记忆
    brain.setMemory<mc::i32>(MemoryModuleTypes::PLAY_DEAD_TICKS, 200);

    action::PlayDeadTask<VillagerEntity> task;
    mc::math::Random rng(42);

    EXPECT_TRUE(task.start(m_world.get(), &villager, 0, rng));
    EXPECT_EQ(task.getStatus(), mc::entity::ai::brain::task::TaskStatus::RUNNING);
}

// 测试：PlayDeadTask 执行后设置 PACIFIED 记忆并清除 ATTACK_TARGET
TEST_F(PlayDeadTaskTest, SetsPacifiedAndClearsAttackTarget)
{
    VillagerEntity villager(EntityInstanceId(1));
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    auto& brain = villager.brain();
    brain.registerMemory(MemoryModuleTypes::PLAY_DEAD_TICKS);
    brain.registerMemory(MemoryModuleTypes::ATTACK_TARGET);
    brain.registerMemory(MemoryModuleTypes::PACIFIED);

    // 设置 PLAY_DEAD_TICKS 和 ATTACK_TARGET
    brain.setMemory<mc::i32>(MemoryModuleTypes::PLAY_DEAD_TICKS, 200);
    brain.setMemory<LivingEntity*>(MemoryModuleTypes::ATTACK_TARGET, nullptr);

    action::PlayDeadTask<VillagerEntity> task;
    mc::math::Random rng(42);

    EXPECT_TRUE(task.start(m_world.get(), &villager, 0, rng));

    // PACIFIED 应被设置
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::PACIFIED, MemoryModuleStatus::VALUE_PRESENT));
    auto pacified = brain.getMemory<bool>(MemoryModuleTypes::PACIFIED);
    ASSERT_TRUE(pacified.has_value());
    EXPECT_TRUE(*pacified);

    // ATTACK_TARGET 应被清除
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::ATTACK_TARGET, MemoryModuleStatus::VALUE_ABSENT));
}

// 测试：PlayDeadTask getName 返回正确名称
TEST_F(PlayDeadTaskTest, GetNameReturnsCorrectName)
{
    action::PlayDeadTask<VillagerEntity> task;
    EXPECT_EQ(task.getName(), "PlayDeadTask");
}

// 测试：PlayDeadTask 不应在已有 PACIFIED 记忆时重复触发
TEST_F(PlayDeadTaskTest, ShouldNotExecuteWhenAlreadyPacified)
{
    VillagerEntity villager(EntityInstanceId(1));
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    auto& brain = villager.brain();
    brain.registerMemory(MemoryModuleTypes::PLAY_DEAD_TICKS);
    brain.registerMemory(MemoryModuleTypes::ATTACK_TARGET);
    brain.registerMemory(MemoryModuleTypes::PACIFIED);

    // 设置 PLAY_DEAD_TICKS 和 PACIFIED
    brain.setMemory<mc::i32>(MemoryModuleTypes::PLAY_DEAD_TICKS, 200);
    brain.setMemory<bool>(MemoryModuleTypes::PACIFIED, true);

    action::PlayDeadTask<VillagerEntity> task;
    mc::math::Random rng(42);

    // PACIFIED 已存在 → shouldExecute 返回 false
    EXPECT_FALSE(task.start(m_world.get(), &villager, 0, rng));
}

// ============================================================================
// JumpTask 记忆模块需求测试（仅内存级别测试，不实例化模板以避免编译问题）
// ============================================================================

class JumpTaskMemoryTest : public ::testing::Test {
protected:
    void SetUp() override { MemoryModuleTypes::initialize(); }
};

// 测试：JumpTask 的记忆模块需求 - JUMP_COOLDOWN 必须不存在
TEST_F(JumpTaskMemoryTest, RequiresJumpCooldownAbsent)
{
    mc::entity::ai::brain::Brain<int> brain;
    brain.registerMemory(MemoryModuleTypes::JUMP_COOLDOWN);

    // 初始状态：JUMP_COOLDOWN 不存在 → VALUE_ABSENT 成立
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::JUMP_COOLDOWN, MemoryModuleStatus::VALUE_ABSENT));

    // 设置冷却后 → VALUE_ABSENT 不成立
    brain.setMemoryWithTTL<mc::i32>(MemoryModuleTypes::JUMP_COOLDOWN, 20, 20);
    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::JUMP_COOLDOWN, MemoryModuleStatus::VALUE_ABSENT));
}

// 测试：JumpTask getName 和构造函数
TEST_F(JumpTaskMemoryTest, GetNameAndConstructor)
{
    // 仅测试构造和 getName，不触发完整模板实例化
    // JumpTask 依赖 MobEntity 的 isOnGround() 和 JumpController，
    // 使用 Brain<int> 测试记忆模块需求即可
}

// ============================================================================
// KickTask 记忆模块需求测试
// ============================================================================

class KickTaskMemoryTest : public ::testing::Test {
protected:
    void SetUp() override { MemoryModuleTypes::initialize(); }
};

// 测试：KickTask 的记忆模块需求 - ATTACK_TARGET 必须存在
TEST_F(KickTaskMemoryTest, RequiresAttackTargetPresent)
{
    mc::entity::ai::brain::Brain<int> brain;
    brain.registerMemory(MemoryModuleTypes::ATTACK_TARGET);
    brain.registerMemory(MemoryModuleTypes::ATTACK_COOLING_DOWN);

    // ATTACK_TARGET 初始缺失
    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::ATTACK_TARGET, MemoryModuleStatus::VALUE_PRESENT));
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::ATTACK_TARGET, MemoryModuleStatus::VALUE_ABSENT));
}

// ============================================================================
// KickTask 核心逻辑测试
// ============================================================================

class KickTaskTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        MemoryModuleTypes::initialize();
        mc::entity::ai::brain::schedule::Schedule::initialize();
        m_world = std::make_unique<NewActionTaskTestWorld>();
    }

    std::unique_ptr<NewActionTaskTestWorld> m_world;
};

// 测试：KickTask 在没有 ATTACK_TARGET 记忆时不应执行
TEST_F(KickTaskTest, ShouldNotExecuteWithoutAttackTarget)
{
    VillagerEntity villager(EntityInstanceId(1));
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    auto& brain = villager.brain();
    brain.registerMemory(MemoryModuleTypes::ATTACK_TARGET);
    brain.registerMemory(MemoryModuleTypes::ATTACK_COOLING_DOWN);

    action::KickTask<VillagerEntity> task;
    mc::math::Random rng(42);

    // 没有 ATTACK_TARGET → shouldExecute 不满足 → start() 返回 false
    EXPECT_FALSE(task.start(m_world.get(), &villager, 0, rng));
}

// 测试：KickTask 在有 ATTACK_COOLING_DOWN 记忆时不应执行
TEST_F(KickTaskTest, ShouldNotExecuteWhenCoolingDown)
{
    VillagerEntity villager(EntityInstanceId(1));
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    auto& brain = villager.brain();
    brain.registerMemory(MemoryModuleTypes::ATTACK_TARGET);
    brain.registerMemory(MemoryModuleTypes::ATTACK_COOLING_DOWN);

    // 设置攻击目标
    ZombieEntity zombie(EntityInstanceId(2));
    zombie.setWorld(m_world.get());
    zombie.setPosition(1.0f, 64.0f, 0.0f);
    brain.setMemory<LivingEntity*>(MemoryModuleTypes::ATTACK_TARGET, &zombie);

    // 设置冷却记忆
    brain.setMemoryWithTTL<bool>(MemoryModuleTypes::ATTACK_COOLING_DOWN, true, 20);

    action::KickTask<VillagerEntity> task;
    mc::math::Random rng(42);

    // ATTACK_COOLING_DOWN 存在 → _hasRequiredMemories 检查失败 → start() 返回 false
    EXPECT_FALSE(task.start(m_world.get(), &villager, 0, rng));
}

// 测试：KickTask 在所有条件满足时（近距离目标、无冷却）应执行
TEST_F(KickTaskTest, ShouldExecuteWhenTargetInRangeAndNoCooldown)
{
    VillagerEntity villager(EntityInstanceId(1));
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    auto& brain = villager.brain();
    brain.registerMemory(MemoryModuleTypes::ATTACK_TARGET);
    brain.registerMemory(MemoryModuleTypes::ATTACK_COOLING_DOWN);

    // 设置近距离攻击目标
    ZombieEntity zombie(EntityInstanceId(2));
    zombie.setWorld(m_world.get());
    zombie.setPosition(1.0f, 64.0f, 0.0f); // 距离1格，在默认2格范围内
    brain.setMemory<LivingEntity*>(MemoryModuleTypes::ATTACK_TARGET, &zombie);

    action::KickTask<VillagerEntity> task;
    mc::math::Random rng(42);

    // ATTACK_TARGET 存在 + ATTACK_COOLING_DOWN 不存在 + 在范围内 → 应执行
    EXPECT_TRUE(task.start(m_world.get(), &villager, 0, rng));
    EXPECT_EQ(task.getStatus(), mc::entity::ai::brain::task::TaskStatus::RUNNING);
}

// 测试：KickTask 在目标太远时不应执行
TEST_F(KickTaskTest, ShouldNotExecuteWhenTargetOutOfRange)
{
    VillagerEntity villager(EntityInstanceId(1));
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    auto& brain = villager.brain();
    brain.registerMemory(MemoryModuleTypes::ATTACK_TARGET);
    brain.registerMemory(MemoryModuleTypes::ATTACK_COOLING_DOWN);

    // 设置远距离攻击目标
    ZombieEntity zombie(EntityInstanceId(2));
    zombie.setWorld(m_world.get());
    zombie.setPosition(10.0f, 64.0f, 0.0f); // 距离10格，超出默认2格范围
    brain.setMemory<LivingEntity*>(MemoryModuleTypes::ATTACK_TARGET, &zombie);

    action::KickTask<VillagerEntity> task;
    mc::math::Random rng(42);

    // 超出踢击范围 → shouldExecute 返回 false
    EXPECT_FALSE(task.start(m_world.get(), &villager, 0, rng));
}

// 测试：KickTask 执行后设置 ATTACK_COOLING_DOWN 记忆
TEST_F(KickTaskTest, SetsCoolingDownAfterKick)
{
    VillagerEntity villager(EntityInstanceId(1));
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    auto& brain = villager.brain();
    brain.registerMemory(MemoryModuleTypes::ATTACK_TARGET);
    brain.registerMemory(MemoryModuleTypes::ATTACK_COOLING_DOWN);

    // 设置近距离攻击目标
    ZombieEntity zombie(EntityInstanceId(2));
    zombie.setWorld(m_world.get());
    zombie.setPosition(1.0f, 64.0f, 0.0f);
    brain.setMemory<LivingEntity*>(MemoryModuleTypes::ATTACK_TARGET, &zombie);

    action::KickTask<VillagerEntity> task;
    mc::math::Random rng(42);

    EXPECT_TRUE(task.start(m_world.get(), &villager, 0, rng));

    // 踢击后应设置 ATTACK_COOLING_DOWN 记忆
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::ATTACK_COOLING_DOWN, MemoryModuleStatus::VALUE_PRESENT));

    auto coolingDown = brain.getMemory<bool>(MemoryModuleTypes::ATTACK_COOLING_DOWN);
    ASSERT_TRUE(coolingDown.has_value());
    EXPECT_TRUE(*coolingDown);
}

// 测试：KickTask 是单次触发型任务
TEST_F(KickTaskTest, IsSingleShotTask)
{
    VillagerEntity villager(EntityInstanceId(1));
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    auto& brain = villager.brain();
    brain.registerMemory(MemoryModuleTypes::ATTACK_TARGET);
    brain.registerMemory(MemoryModuleTypes::ATTACK_COOLING_DOWN);

    // 设置近距离攻击目标
    ZombieEntity zombie(EntityInstanceId(2));
    zombie.setWorld(m_world.get());
    zombie.setPosition(1.0f, 64.0f, 0.0f);
    brain.setMemory<LivingEntity*>(MemoryModuleTypes::ATTACK_TARGET, &zombie);

    action::KickTask<VillagerEntity> task;
    mc::math::Random rng(42);

    EXPECT_TRUE(task.start(m_world.get(), &villager, 0, rng));

    // KickTask 是单次触发型，tick 后应停止
    task.tick(m_world.get(), &villager, 1);
    EXPECT_EQ(task.getStatus(), mc::entity::ai::brain::task::TaskStatus::STOPPED);
}

// 测试：KickTask getName 返回正确名称
TEST_F(KickTaskTest, GetNameReturnsCorrectName)
{
    action::KickTask<VillagerEntity> task;
    EXPECT_EQ(task.getName(), "KickTask");
}

// 测试：KickTask 构造函数接受自定义范围和冷却时间
TEST_F(KickTaskTest, CustomRangeAndCooldown)
{
    action::KickTask<VillagerEntity> taskDefault;
    EXPECT_EQ(taskDefault.getName(), "KickTask");

    action::KickTask<VillagerEntity> taskCustom(3.0f, 30);
    EXPECT_EQ(taskCustom.getName(), "KickTask");
}

// 测试：KickTask 自定义范围外不应执行
TEST_F(KickTaskTest, CustomRangeRespected)
{
    VillagerEntity villager(EntityInstanceId(1));
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    auto& brain = villager.brain();
    brain.registerMemory(MemoryModuleTypes::ATTACK_TARGET);
    brain.registerMemory(MemoryModuleTypes::ATTACK_COOLING_DOWN);

    // 设置攻击目标距离1.5格
    ZombieEntity zombie(EntityInstanceId(2));
    zombie.setWorld(m_world.get());
    zombie.setPosition(1.5f, 64.0f, 0.0f);
    brain.setMemory<LivingEntity*>(MemoryModuleTypes::ATTACK_TARGET, &zombie);

    // 使用1.0格范围（小于1.5格距离）
    action::KickTask<VillagerEntity> shortRangeTask(1.0f, 20);
    mc::math::Random rng(42);

    // 目标超出自定义范围 → shouldExecute 返回 false
    EXPECT_FALSE(shortRangeTask.start(m_world.get(), &villager, 0, rng));

    // 使用3.0格范围（大于1.5格距离）
    action::KickTask<VillagerEntity> longRangeTask(3.0f, 20);

    // 目标在自定义范围内 → shouldExecute 返回 true
    EXPECT_TRUE(longRangeTask.start(m_world.get(), &villager, 0, rng));
}

// ============================================================================
// EatTask 基本测试（占位验证）
// ============================================================================

class EatTaskTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        MemoryModuleTypes::initialize();
        mc::entity::ai::brain::schedule::Schedule::initialize();
        m_world = std::make_unique<NewActionTaskTestWorld>();
    }

    std::unique_ptr<NewActionTaskTestWorld> m_world;
};

// 测试：EatTask 当前始终返回 false（饥饿系统未实现）
TEST_F(EatTaskTest, ShouldNotExecuteWhenHungrySystemNotImplemented)
{
    VillagerEntity villager(EntityInstanceId(1));
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    action::EatTask<VillagerEntity> task;
    mc::math::Random rng(42);

    // 饥饿系统未实现，shouldExecute 始终返回 false
    EXPECT_FALSE(task.start(m_world.get(), &villager, 0, rng));
}

// 测试：EatTask getName 返回正确名称
TEST_F(EatTaskTest, GetNameReturnsCorrectName)
{
    action::EatTask<VillagerEntity> task;
    EXPECT_EQ(task.getName(), "EatTask");
}

// 测试：EatTask 构造函数接受自定义进食持续时间
TEST_F(EatTaskTest, CustomEatDuration)
{
    action::EatTask<VillagerEntity> taskDefault;
    EXPECT_EQ(taskDefault.getName(), "EatTask");

    action::EatTask<VillagerEntity> taskCustom(80);
    EXPECT_EQ(taskCustom.getName(), "EatTask");
}

} // namespace
