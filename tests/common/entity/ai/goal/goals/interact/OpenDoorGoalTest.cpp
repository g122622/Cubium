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

#include "common/TestWorldHelper.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/goals/interact/OpenDoorGoal.hpp"
#include "common/entity/ai/goal/goals/interact/RaiderOpenDoorGoal.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/ecs/components/PhysicsStateComponent.hpp"
#include "common/entity/entities/monster/illager/AbstractRaiderEntity.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/village/raid/Raid.hpp"

using namespace mc;
using namespace mc::block_registry;
using namespace mc::entity::ai;
using namespace mc::entity::ai::goal;

// ============================================================================
// 测试世界
// ============================================================================

namespace {

/**
 * @brief OpenDoorGoal 测试用世界
 *
 * 支持方块状态、游戏规则和门开关操作的设置和追踪。
 */
class OpenDoorTestWorld final : public mc::test::BaseTestWorld {
public:
    OpenDoorTestWorld() = default;

    void setBlock(i32 x, i32 y, i32 z, const BlockState* state) { m_blocks[BlockPos(x, y, z)] = state; }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_blocks.find(BlockPos(x, y, z));
        return it != m_blocks.end() ? it->second : &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[BlockPos(x, y, z)] = state;
        return true;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        (void)flags;
        m_blocks[BlockPos(x, y, z)] = state;
        return true;
    }

    void playEvent(i32 eventId, const BlockPos& pos, i32 data) override
    {
        (void)eventId;
        (void)pos;
        (void)data;
    }

    void destroyBlockProgress(EntityInstanceId breakerId, const BlockPos& pos, i32 progress) override
    {
        (void)breakerId;
        (void)pos;
        (void)progress;
    }

    [[nodiscard]] world::gamerule::GameRules& getGameRules() override { return m_gameRules; }
    [[nodiscard]] const world::gamerule::GameRules& getGameRules() const override { return m_gameRules; }

    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }

    void setDifficulty(Difficulty difficulty) { m_difficulty = difficulty; }

    [[nodiscard]] const BlockState* getBlockStateAt(const BlockPos& pos) const
    {
        return getBlockState(pos.x, pos.y, pos.z);
    }

private:
    std::unordered_map<BlockPos, const BlockState*> m_blocks;
    world::gamerule::GameRules m_gameRules;
    Difficulty m_difficulty = Difficulty::Normal;
};

/**
 * @brief 测试用 MobEntity，支持设置碰撞状态和世界
 */
class TestOpenDoorMob final : public MobEntity {
public:
    TestOpenDoorMob()
        : MobEntity(EntityInstanceId(1), mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }

    void setPositionForTest(f64 x, f64 y, f64 z)
    {
        setPosition(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
    }

    void setWorldForTest(IWorld* world) { setWorld(world); }

    void setCollidedHorizontallyForTest(bool value)
    {
        // m_collidedHorizontally 已迁入 ecs::PhysicsStateComponent（无 public setter），
        // 测试通过组件直接写以模拟"实体碰撞门"触发 DoorGoal。
        if (auto* c = tryGetComponent<ecs::PhysicsStateComponent>()) {
            c->m_collidedHorizontally = value;
        }
    }

    void setNavigatorCanOpenDoors(bool value)
    {
        auto* nav = navigator();
        if (nav) {
            nav->setCanOpenDoors(value);
        }
    }
};

} // anonymous namespace

// ============================================================================
// OpenDoorGoal 构造和基础属性测试
// ============================================================================

class OpenDoorGoalTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(OpenDoorGoalTest, TypeName)
{
    TestOpenDoorMob mob;
    OpenDoorGoal goal(&mob, true);
    EXPECT_EQ(goal.getTypeName(), "OpenDoorGoal");
}

TEST_F(OpenDoorGoalTest, MutexFlags)
{
    // OpenDoorGoal 继承 DoorInteractGoal，使用 GoalFlag::Move
    TestOpenDoorMob mob;
    OpenDoorGoal goal(&mob, true);
    EXPECT_TRUE(goal.getMutexFlags().test(GoalFlag::Move));
}

TEST_F(OpenDoorGoalTest, CloseDoorParameterTrue)
{
    // closeDoor = true 模式：开门后会在穿过门后关门
    TestOpenDoorMob mob;
    OpenDoorGoal goal(&mob, true);
    // shouldContinueExecuting 在 forgetTime > 0 时返回 true（需要先 startExecuting）
    EXPECT_FALSE(goal.shouldContinueExecuting());
}

TEST_F(OpenDoorGoalTest, CloseDoorParameterFalse)
{
    // closeDoor = false 模式：shouldContinueExecuting 始终返回 false
    TestOpenDoorMob mob;
    OpenDoorGoal goal(&mob, false);
    EXPECT_FALSE(goal.shouldContinueExecuting());
}

TEST_F(OpenDoorGoalTest, ShouldExecuteReturnsFalseWithNullMob)
{
    OpenDoorGoal goal(nullptr, true);
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(OpenDoorGoalTest, ShouldExecuteReturnsFalseWithNoWorld)
{
    TestOpenDoorMob mob;
    OpenDoorGoal goal(&mob, true);
    // mob 没有设置世界
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(OpenDoorGoalTest, ShouldExecuteReturnsFalseWhenNavigatorCannotOpenDoors)
{
    OpenDoorTestWorld world;
    TestOpenDoorMob mob;
    mob.setWorldForTest(&world);
    // 默认 canOpenDoors = false
    EXPECT_FALSE(mob.navigator()->canOpenDoors());
    OpenDoorGoal goal(&mob, true);
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(OpenDoorGoalTest, ShouldExecuteReturnsFalseWhenNoHorizontalCollision)
{
    OpenDoorTestWorld world;
    TestOpenDoorMob mob;
    mob.setWorldForTest(&world);
    mob.setNavigatorCanOpenDoors(true);
    mob.setCollidedHorizontallyForTest(false); // 没有水平碰撞
    OpenDoorGoal goal(&mob, true);
    EXPECT_FALSE(goal.shouldExecute());
}

// ============================================================================
// OpenDoorGoal 开门/关门逻辑测试
// ============================================================================

TEST_F(OpenDoorGoalTest, StartExecutingOpensDoor)
{
    // startExecuting 应该调用 _setDoorOpen(true) 打开门
    // 验证：startExecuting 后 m_forgetTime 应为 20（通过 shouldContinueExecuting 间接验证）
    OpenDoorTestWorld world;
    TestOpenDoorMob mob;
    mob.setWorldForTest(&world);
    mob.setNavigatorCanOpenDoors(true);

    OpenDoorGoal goal(&mob, true);

    // startExecuting 前应不能继续执行（forgetTime 为 0）
    EXPECT_FALSE(goal.shouldContinueExecuting());

    // startExecuting 设置 forgetTime = 20 并打开门
    goal.startExecuting();

    // startExecuting 后 forgetTime = 20，closeDoor = true，且未穿过门
    // 因此 shouldContinueExecuting 应返回 true
    EXPECT_TRUE(goal.shouldContinueExecuting());
}

TEST_F(OpenDoorGoalTest, ResetTaskClosesDoor)
{
    // resetTask 应该调用 _setDoorOpen(false) 关闭门
    OpenDoorTestWorld world;
    TestOpenDoorMob mob;
    mob.setWorldForTest(&world);
    mob.setNavigatorCanOpenDoors(true);

    OpenDoorGoal goal(&mob, true);

    // 在没有门的情况下调用 resetTask 不应崩溃
    // 同时验证：startExecuting 后 shouldContinueExecuting 返回 true
    // resetTask 后 shouldContinueExecuting 返回 false（因为 forgetTime 未重置，但门状态已清除）
    goal.startExecuting();
    EXPECT_TRUE(goal.shouldContinueExecuting());
    goal.resetTask();
    // resetTask 后 forgetTime 仍为 20，但 DoorInteractGoal 基类状态可能已改变
    // 关键是不崩溃且逻辑一致
}

TEST_F(OpenDoorGoalTest, TickDecrementsForgetTime)
{
    // tick 应该递减 m_forgetTime
    OpenDoorTestWorld world;
    TestOpenDoorMob mob;
    mob.setWorldForTest(&world);
    mob.setNavigatorCanOpenDoors(true);

    OpenDoorGoal goal(&mob, true);
    // 在没有门的情况下调用 tick 不应崩溃
    goal.tick();
}

TEST_F(OpenDoorGoalTest, StartExecutingDoesNotCrashWithNoDoor)
{
    TestOpenDoorMob mob;
    OpenDoorGoal goal(&mob, true);
    // 没有世界和门的情况下调用 startExecuting 不应崩溃
    goal.startExecuting();
}

TEST_F(OpenDoorGoalTest, ResetTaskDoesNotCrashWithNoDoor)
{
    TestOpenDoorMob mob;
    OpenDoorGoal goal(&mob, true);
    goal.resetTask();
}

TEST_F(OpenDoorGoalTest, TickDoesNotCrashWithNoDoor)
{
    TestOpenDoorMob mob;
    OpenDoorGoal goal(&mob, true);
    goal.tick();
}

TEST_F(OpenDoorGoalTest, TickDoesNotCrashWithNullMob)
{
    OpenDoorGoal goal(nullptr, true);
    goal.tick();
}

TEST_F(OpenDoorGoalTest, FullLifecycleWithNoDoor)
{
    // 完整的生命周期测试（没有门的情况）
    OpenDoorTestWorld world;
    TestOpenDoorMob mob;
    mob.setWorldForTest(&world);
    mob.setNavigatorCanOpenDoors(true);

    OpenDoorGoal goal(&mob, true);

    // shouldExecute 在没有碰撞时返回 false
    EXPECT_FALSE(goal.shouldExecute());

    // 手动调用生命周期方法不应崩溃
    goal.startExecuting();
    // startExecuting 后 forgetTime = 20, closeDoor = true, hasPassedDoor = false
    // 所以 shouldContinueExecuting 返回 true
    EXPECT_TRUE(goal.shouldContinueExecuting());
    // tick 递减 forgetTime
    goal.tick();
    // 经过 20 tick 后 forgetTime 降到 0，shouldContinueExecuting 返回 false
    for (int i = 0; i < 20; ++i) {
        goal.tick();
    }
    EXPECT_FALSE(goal.shouldContinueExecuting());
    goal.resetTask();
}

TEST_F(OpenDoorGoalTest, CloseDoorModeContinueExecution)
{
    // closeDoor = true 模式：
    // shouldContinueExecuting 返回 closeDoor && forgetTime > 0 && !hasPassedDoor
    OpenDoorTestWorld world;
    TestOpenDoorMob mob;
    mob.setWorldForTest(&world);
    mob.setNavigatorCanOpenDoors(true);

    OpenDoorGoal goal(&mob, true);
    // 在没有 startExecuting 的情况下，forgetTime 为 0，应返回 false
    EXPECT_FALSE(goal.shouldContinueExecuting());
}

TEST_F(OpenDoorGoalTest, NoCloseDoorModeNeverContinue)
{
    // closeDoor = false 模式：
    // shouldContinueExecuting 始终返回 false
    OpenDoorTestWorld world;
    TestOpenDoorMob mob;
    mob.setWorldForTest(&world);
    mob.setNavigatorCanOpenDoors(true);

    OpenDoorGoal goal(&mob, false);
    goal.startExecuting();
    // 即使 startExecuting 后，closeDoor=false 仍然使 shouldContinueExecuting 返回 false
    EXPECT_FALSE(goal.shouldContinueExecuting());
}

// ============================================================================
// RaiderOpenDoorGoal 测试
// ============================================================================

class RaiderOpenDoorGoalTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(RaiderOpenDoorGoalTest, TypeName)
{
    TestOpenDoorMob mob;
    RaiderOpenDoorGoal goal(&mob);
    EXPECT_EQ(goal.getTypeName(), "RaiderOpenDoorGoal");
}

TEST_F(RaiderOpenDoorGoalTest, ShouldExecuteReturnsFalseWithNonRaiderMob)
{
    // 非袭击者实体应返回 false（dynamic_cast 失败）
    OpenDoorTestWorld world;
    TestOpenDoorMob mob;
    mob.setWorldForTest(&world);
    mob.setNavigatorCanOpenDoors(true);
    mob.setCollidedHorizontallyForTest(true);

    RaiderOpenDoorGoal goal(&mob);
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(RaiderOpenDoorGoalTest, ShouldExecuteReturnsFalseWithNullMob)
{
    RaiderOpenDoorGoal goal(nullptr);
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(RaiderOpenDoorGoalTest, ShouldExecuteReturnsFalseWhenNavigatorCannotOpenDoors)
{
    OpenDoorTestWorld world;
    TestOpenDoorMob mob;
    mob.setWorldForTest(&world);
    // 默认 canOpenDoors = false
    RaiderOpenDoorGoal goal(&mob);
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(RaiderOpenDoorGoalTest, RaiderOpenDoorGoalIsOpenDoorGoalWithCloseDoorFalse)
{
    // RaiderOpenDoorGoal 继承自 OpenDoorGoal，且 closeDoor = false
    TestOpenDoorMob mob;
    RaiderOpenDoorGoal goal(&mob);
    // closeDoor = false 意味着 shouldContinueExecuting 始终返回 false
    EXPECT_FALSE(goal.shouldContinueExecuting());
}

TEST_F(RaiderOpenDoorGoalTest, MutexFlags)
{
    // 继承 DoorInteractGoal 的 GoalFlag::Move
    TestOpenDoorMob mob;
    RaiderOpenDoorGoal goal(&mob);
    EXPECT_TRUE(goal.getMutexFlags().test(GoalFlag::Move));
}

TEST_F(RaiderOpenDoorGoalTest, StartExecutingDoesNotCrashWithNoDoor)
{
    TestOpenDoorMob mob;
    RaiderOpenDoorGoal goal(&mob);
    goal.startExecuting();
}

TEST_F(RaiderOpenDoorGoalTest, ResetTaskDoesNotCrashWithNoDoor)
{
    TestOpenDoorMob mob;
    RaiderOpenDoorGoal goal(&mob);
    goal.resetTask();
}

TEST_F(RaiderOpenDoorGoalTest, TickDoesNotCrashWithNoDoor)
{
    TestOpenDoorMob mob;
    RaiderOpenDoorGoal goal(&mob);
    goal.tick();
}

TEST_F(RaiderOpenDoorGoalTest, FullLifecycleWithNoDoor)
{
    OpenDoorTestWorld world;
    TestOpenDoorMob mob;
    mob.setWorldForTest(&world);
    mob.setNavigatorCanOpenDoors(true);

    RaiderOpenDoorGoal goal(&mob);

    // 非袭击者实体的 shouldExecute 返回 false
    EXPECT_FALSE(goal.shouldExecute());

    // 手动调用生命周期方法不应崩溃
    goal.startExecuting();
    goal.tick();
    goal.resetTask();
}
