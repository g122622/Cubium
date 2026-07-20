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
#include "common/entity/ai/brain/memory/BlockPosTarget.hpp"
#include "common/entity/ai/brain/memory/MemoryModuleStatus.hpp"
#include "common/entity/ai/brain/memory/MemoryModuleType.hpp"
#include "common/entity/ai/brain/memory/WalkTarget.hpp"
#include "common/entity/ai/brain/schedule/Schedule.hpp"
#include "common/entity/ai/brain/task/tasks/movement/MovementTasks.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/GlobalPos.hpp"
#include "common/world/block/BlockPos.hpp"

using mc::BlockPos;
using mc::EntityInstanceId;
using mc::GlobalPos;
using mc::i64;
using mc::Vector3;
using mc::entity::VillagerEntity;
using mc::test::BaseTestWorld;
namespace movement = mc::entity::ai::brain::task::movement;
namespace memory = mc::entity::ai::brain::memory;
using memory::MemoryModuleStatus;
using memory::MemoryModuleTypes;
using memory::WalkTarget;

namespace {

// ========== WalkTarget 构造测试 ==========

TEST(WalkTargetVariantsTest, Vector3Constructor)
{
    WalkTarget wt(Vector3(10.5f, 64.0f, 20.5f), 1.0f, 1);
    ASSERT_NE(wt.getTarget(), nullptr);
    EXPECT_FLOAT_EQ(wt.getSpeed(), 1.0f);
    EXPECT_EQ(wt.getDistance(), 1);
}

TEST(WalkTargetVariantsTest, BlockPosConstructor)
{
    WalkTarget wt(BlockPos(10, 64, 20), 0.8f, 3);
    ASSERT_NE(wt.getTarget(), nullptr);
    EXPECT_EQ(wt.getTarget()->getBlockPos(), BlockPos(10, 64, 20));
    EXPECT_FLOAT_EQ(wt.getSpeed(), 0.8f);
    EXPECT_EQ(wt.getDistance(), 3);
}

TEST(WalkTargetVariantsTest, BlockPosTargetCenterPosition)
{
    BlockPos bp(3, 70, -5);
    WalkTarget wt(bp, 1.0f, 1);
    auto target = wt.getTarget();
    ASSERT_NE(target, nullptr);
    EXPECT_FLOAT_EQ(target->getPosition().x, 3.5f);
    EXPECT_FLOAT_EQ(target->getPosition().y, 70.5f);
    EXPECT_FLOAT_EQ(target->getPosition().z, -4.5f);
}

TEST(WalkTargetVariantsTest, Vector3ConstructorConvertsToBlockCenter)
{
    Vector3 exactPos(10.7f, 64.3f, 20.9f);
    WalkTarget wt(exactPos, 0.5f, 2);
    auto target = wt.getTarget();
    ASSERT_NE(target, nullptr);
    EXPECT_FLOAT_EQ(target->getPosition().x, 10.5f);
    EXPECT_FLOAT_EQ(target->getPosition().y, 64.5f);
    EXPECT_FLOAT_EQ(target->getPosition().z, 20.5f);
    EXPECT_FLOAT_EQ(wt.getSpeed(), 0.5f);
    EXPECT_EQ(wt.getDistance(), 2);
}

TEST(WalkTargetVariantsTest, SharedPositionTargetIsNotCopied)
{
    auto sharedTarget = std::make_shared<memory::BlockPosTarget>(BlockPos(1, 2, 3));
    WalkTarget wt1(sharedTarget, 1.0f, 1);
    WalkTarget wt2(sharedTarget, 0.5f, 2);
    EXPECT_EQ(wt1.getTarget(), wt2.getTarget());
}

TEST(WalkTargetVariantsTest, PositionTargetConstructor)
{
    auto target = std::make_shared<memory::BlockPosTarget>(BlockPos(5, 70, 8));
    WalkTarget wt(target, 1.2f, 2);
    ASSERT_NE(wt.getTarget(), nullptr);
    EXPECT_EQ(wt.getTarget()->getBlockPos(), BlockPos(5, 70, 8));
    EXPECT_FLOAT_EQ(wt.getSpeed(), 1.2f);
    EXPECT_EQ(wt.getDistance(), 2);
}

// ========== 记忆状态需求测试 ==========

TEST(MovementTasksMemoryTest, WalkTargetMemoryIntegration)
{
    MemoryModuleTypes::initialize();

    mc::entity::ai::brain::Brain<int> brain;
    brain.registerMemory(MemoryModuleTypes::WALK_TARGET);
    brain.registerMemory(MemoryModuleTypes::PATH);
    brain.registerMemory(MemoryModuleTypes::CANT_REACH_WALK_TARGET_SINCE);

    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::WALK_TARGET));

    WalkTarget walkTarget(BlockPos(10, 64, 20), 1.0f, 2);
    brain.setMemory(MemoryModuleTypes::WALK_TARGET, walkTarget);
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::WALK_TARGET));

    auto stored = brain.getMemory(MemoryModuleTypes::WALK_TARGET);
    ASSERT_TRUE(stored.has_value());
    EXPECT_EQ(stored->getTarget()->getBlockPos(), BlockPos(10, 64, 20));

    brain.removeMemory(MemoryModuleTypes::WALK_TARGET);
    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::WALK_TARGET));
}

TEST(MovementTasksMemoryTest, MemoryStatusCheckForTaskRequirements)
{
    MemoryModuleTypes::initialize();

    mc::entity::ai::brain::Brain<int> brain;
    brain.registerMemory(MemoryModuleTypes::WALK_TARGET);
    brain.registerMemory(MemoryModuleTypes::LOOK_TARGET);
    brain.registerMemory(MemoryModuleTypes::ATTACK_TARGET);
    brain.registerMemory(MemoryModuleTypes::AVOID_TARGET);
    brain.registerMemory(MemoryModuleTypes::HIDING_PLACE);
    brain.registerMemory(MemoryModuleTypes::PATH);
    brain.registerMemory(MemoryModuleTypes::CANT_REACH_WALK_TARGET_SINCE);

    // MoveToTargetTask requires: WALK_TARGET=PRESENT, PATH=REGISTERED
    // 初始状态：WALK_TARGET 缺失 → MoveToTargetTask 不应执行
    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::WALK_TARGET, MemoryModuleStatus::VALUE_PRESENT));
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::WALK_TARGET, MemoryModuleStatus::VALUE_ABSENT));
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::WALK_TARGET, MemoryModuleStatus::REGISTERED));

    // 设置 WALK_TARGET 后 → MoveToTargetTask 可执行
    brain.setMemory(MemoryModuleTypes::WALK_TARGET, WalkTarget(BlockPos(0, 64, 0), 1.0f, 1));
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::WALK_TARGET, MemoryModuleStatus::VALUE_PRESENT));

    // StrollTask requires: WALK_TARGET=ABSENT
    // 设置 WALK_TARGET 后 → StrollTask 不应执行
    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::WALK_TARGET, MemoryModuleStatus::VALUE_ABSENT));

    // ChaseTask requires: ATTACK_TARGET=PRESENT, WALK_TARGET=REGISTERED, LOOK_TARGET=REGISTERED
    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::ATTACK_TARGET, MemoryModuleStatus::VALUE_PRESENT));
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::WALK_TARGET, MemoryModuleStatus::REGISTERED));
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::LOOK_TARGET, MemoryModuleStatus::REGISTERED));

    // FleeTask requires: AVOID_TARGET=PRESENT, WALK_TARGET=ABSENT
    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::AVOID_TARGET, MemoryModuleStatus::VALUE_PRESENT));

    // FindHiddenBlockTask requires: HIDING_PLACE=ABSENT, WALK_TARGET=ABSENT
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::HIDING_PLACE, MemoryModuleStatus::VALUE_ABSENT));
}

TEST(MovementTasksMemoryTest, HomeAndNearestBedMemoryForHiding)
{
    MemoryModuleTypes::initialize();

    mc::entity::ai::brain::Brain<int> brain;
    brain.registerMemory(MemoryModuleTypes::HOME);
    brain.registerMemory(MemoryModuleTypes::NEAREST_BED);

    GlobalPos homePos(mc::DimensionId(0), BlockPos(100, 64, 200));
    brain.setMemory(MemoryModuleTypes::HOME, homePos);

    auto storedHome = brain.getMemory(MemoryModuleTypes::HOME);
    ASSERT_TRUE(storedHome.has_value());
    EXPECT_EQ(storedHome->getPos(), BlockPos(100, 64, 200));

    brain.setMemory(MemoryModuleTypes::NEAREST_BED, BlockPos(50, 65, 100));
    auto storedBed = brain.getMemory(MemoryModuleTypes::NEAREST_BED);
    ASSERT_TRUE(storedBed.has_value());
    EXPECT_EQ(*storedBed, BlockPos(50, 65, 100));
}

TEST(MovementTasksMemoryTest, CantReachWalkTargetMemoryWithTTL)
{
    MemoryModuleTypes::initialize();

    mc::entity::ai::brain::Brain<int> brain;
    brain.registerMemory(MemoryModuleTypes::CANT_REACH_WALK_TARGET_SINCE);

    brain.setMemoryWithTTL(MemoryModuleTypes::CANT_REACH_WALK_TARGET_SINCE, static_cast<i64>(100), 200);
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::CANT_REACH_WALK_TARGET_SINCE));

    auto value = brain.getMemory(MemoryModuleTypes::CANT_REACH_WALK_TARGET_SINCE);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, 100);
}

// ========== 测试用世界子类 ==========

class MovementTaskTestWorld : public BaseTestWorld {
public:
    MovementTaskTestWorld() = default;
};

// ========== 任务核心逻辑测试（使用 VillagerEntity）==========

class MovementTaskTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        MemoryModuleTypes::initialize();
        mc::entity::ai::brain::schedule::Schedule::initialize();
        m_world = std::make_unique<MovementTaskTestWorld>();
    }

    std::unique_ptr<MovementTaskTestWorld> m_world;
};

TEST_F(MovementTaskTest, MoveToTargetTaskShouldNotExecuteWithoutWalkTarget)
{
    VillagerEntity villager(EntityInstanceId(1));
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    // MoveToTargetTask 需要 WALK_TARGET=PRESENT 才能执行
    // 没有 WALK_TARGET 时，Task 基类的 _hasRequiredMemories 检查会失败
    movement::MoveToTargetTask<VillagerEntity> task;

    // shouldExecute 要求 WALK_TARGET 存在，但 _hasRequiredMemories 先检查
    // WALK_TARGET 不存在 → _hasRequiredMemories 返回 false → start() 不会调用 shouldExecute
    mc::math::Random rng(42);
    EXPECT_FALSE(task.start(m_world.get(), &villager, 0, rng));
}

TEST_F(MovementTaskTest, MoveToTargetTaskShouldExecuteWithWalkTarget)
{
    VillagerEntity villager(EntityInstanceId(1));
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    // 设置 WALK_TARGET 到远处
    auto& brain = villager.brain();
    brain.registerMemory(MemoryModuleTypes::WALK_TARGET);
    brain.registerMemory(MemoryModuleTypes::PATH);
    brain.registerMemory(MemoryModuleTypes::CANT_REACH_WALK_TARGET_SINCE);
    brain.setMemory(MemoryModuleTypes::WALK_TARGET, WalkTarget(BlockPos(20, 64, 20), 1.0f, 1));

    movement::MoveToTargetTask<VillagerEntity> task;

    // WALK_TARGET 存在且距离足够远 → shouldExecute 返回 true
    mc::math::Random rng(42);
    EXPECT_TRUE(task.start(m_world.get(), &villager, 0, rng));
    EXPECT_EQ(task.getStatus(), mc::entity::ai::brain::task::TaskStatus::RUNNING);
}

TEST_F(MovementTaskTest, MoveToTargetTaskShouldNotExecuteWhenAlreadyClose)
{
    VillagerEntity villager(EntityInstanceId(1));
    villager.setWorld(m_world.get());
    villager.setPosition(10.5f, 64.5f, 10.5f);

    // 设置 WALK_TARGET 到当前位置附近
    auto& brain = villager.brain();
    brain.registerMemory(MemoryModuleTypes::WALK_TARGET);
    brain.registerMemory(MemoryModuleTypes::PATH);
    brain.registerMemory(MemoryModuleTypes::CANT_REACH_WALK_TARGET_SINCE);
    brain.setMemory(MemoryModuleTypes::WALK_TARGET, WalkTarget(BlockPos(10, 64, 10), 1.0f, 2));

    movement::MoveToTargetTask<VillagerEntity> task;

    // WALK_TARGET 存在但已经在完成范围内 → shouldExecute 返回 false
    mc::math::Random rng(42);
    EXPECT_FALSE(task.start(m_world.get(), &villager, 0, rng));
}

TEST_F(MovementTaskTest, ChaseTaskShouldNotExecuteWithoutAttackTarget)
{
    VillagerEntity villager(EntityInstanceId(1));
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    // ChaseTask 需要 ATTACK_TARGET=PRESENT 才能执行
    movement::ChaseTask<VillagerEntity> task;

    mc::math::Random rng(42);
    EXPECT_FALSE(task.start(m_world.get(), &villager, 0, rng));
}

TEST_F(MovementTaskTest, FleeTaskShouldNotExecuteWithoutAvoidTarget)
{
    VillagerEntity villager(EntityInstanceId(1));
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    // FleeTask 需要 AVOID_TARGET=PRESENT 且 WALK_TARGET=ABSENT
    movement::FleeTask<VillagerEntity> task;

    mc::math::Random rng(42);
    EXPECT_FALSE(task.start(m_world.get(), &villager, 0, rng));
}

TEST_F(MovementTaskTest, FindHiddenBlockTaskShouldNotExecuteWithoutHurtOrBell)
{
    VillagerEntity villager(EntityInstanceId(1));
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    // FindHiddenBlockTask 需要 HURT_BY 或 HEARD_BELL_TIME 存在
    movement::FindHiddenBlockTask<VillagerEntity> task;

    mc::math::Random rng(42);
    EXPECT_FALSE(task.start(m_world.get(), &villager, 0, rng));
}

TEST_F(MovementTaskTest, LookAtEntityTaskShouldNotExecuteWhenLookTargetPresent)
{
    VillagerEntity villager(EntityInstanceId(1));
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    // LookAtEntityTask 需要 LOOK_TARGET=ABSENT
    // 设置 LOOK_TARGET 后不应执行
    auto& brain = villager.brain();
    brain.registerMemory(MemoryModuleTypes::LOOK_TARGET);
    auto lookTarget = std::make_shared<memory::BlockPosTarget>(BlockPos(5, 70, 5));
    brain.setMemory(MemoryModuleTypes::LOOK_TARGET, std::static_pointer_cast<memory::IPositionTarget>(lookTarget));

    movement::LookAtEntityTask<VillagerEntity> task;

    mc::math::Random rng(42);
    EXPECT_FALSE(task.start(m_world.get(), &villager, 0, rng));
}

TEST_F(MovementTaskTest, StrollTaskShouldNotExecuteWhenWalkTargetPresent)
{
    VillagerEntity villager(EntityInstanceId(1));
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    // StrollTask 需要 WALK_TARGET=ABSENT
    auto& brain = villager.brain();
    brain.registerMemory(MemoryModuleTypes::WALK_TARGET);
    brain.setMemory(MemoryModuleTypes::WALK_TARGET, WalkTarget(BlockPos(10, 64, 10), 1.0f, 1));

    movement::StrollTask<VillagerEntity> task;

    mc::math::Random rng(42);
    EXPECT_FALSE(task.start(m_world.get(), &villager, 0, rng));
}

TEST_F(MovementTaskTest, TaskGetNameReturnsCorrectName)
{
    // 验证所有任务的 getName() 返回正确名称
    EXPECT_EQ(movement::MoveToTargetTask<VillagerEntity>().getName(), "MoveToTargetTask");
    EXPECT_EQ(movement::StrollTask<VillagerEntity>().getName(), "StrollTask");
    EXPECT_EQ(movement::LookAtEntityTask<VillagerEntity>().getName(), "LookAtEntityTask");
    EXPECT_EQ(movement::FindHiddenBlockTask<VillagerEntity>().getName(), "FindHiddenBlockTask");
    EXPECT_EQ(movement::ChaseTask<VillagerEntity>().getName(), "ChaseTask");
    EXPECT_EQ(movement::FleeTask<VillagerEntity>().getName(), "FleeTask");
}

} // namespace
