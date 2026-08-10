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
#include "common/entity/ai/brain/sensor/Sensors.hpp"
#include "common/entity/ai/brain/task/tasks/interact/InteractTasks.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/GlobalPos.hpp"
#include "common/world/block/BlockPos.hpp"

using mc::BlockPos;
using mc::EntityInstanceId;
using mc::GlobalPos;
using mc::i64;
using mc::ItemStack;
using mc::Vector3;
using mc::entity::VillagerEntity;
using mc::test::BaseTestWorld;
namespace interact = mc::entity::ai::brain::task::interact;
namespace memory = mc::entity::ai::brain::memory;
using memory::MemoryModuleStatus;
using memory::MemoryModuleTypes;
using memory::WalkTarget;

namespace {

// ============================================================================
// InteractTasks 记忆模块需求测试
// ============================================================================

class InteractTaskMemoryTest : public ::testing::Test {
protected:
    void SetUp() override { MemoryModuleTypes::initialize(); }
};

// 测试：InteractWithDoorTask 的记忆需求
TEST_F(InteractTaskMemoryTest, InteractWithDoorTaskMemoryRequirements)
{
    // InteractWithDoorTask 需要:
    // - INTERACTABLE_DOORS = VALUE_PRESENT
    // - OPENED_DOORS = REGISTERED
    // - WALK_TARGET = VALUE_PRESENT
    mc::entity::ai::brain::Brain<int> brain;
    brain.registerMemory(MemoryModuleTypes::INTERACTABLE_DOORS);
    brain.registerMemory(MemoryModuleTypes::OPENED_DOORS);
    brain.registerMemory(MemoryModuleTypes::WALK_TARGET);

    // 初始状态：INTERACTABLE_DOORS 缺失
    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::INTERACTABLE_DOORS, MemoryModuleStatus::VALUE_PRESENT));
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::OPENED_DOORS, MemoryModuleStatus::REGISTERED));
    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::WALK_TARGET, MemoryModuleStatus::VALUE_PRESENT));

    // 设置 INTERACTABLE_DOORS 和 WALK_TARGET 后满足条件
    brain.setMemory(MemoryModuleTypes::INTERACTABLE_DOORS, std::vector<GlobalPos>{});
    brain.setMemory(MemoryModuleTypes::WALK_TARGET, WalkTarget(BlockPos(10, 64, 20), 1.0f, 1));
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::INTERACTABLE_DOORS, MemoryModuleStatus::VALUE_PRESENT));
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::WALK_TARGET, MemoryModuleStatus::VALUE_PRESENT));
}

// 测试：VillagerInteractTask 的记忆需求
TEST_F(InteractTaskMemoryTest, VillagerInteractTaskMemoryRequirements)
{
    // VillagerInteractTask 需要:
    // - INTERACTION_TARGET = VALUE_PRESENT
    // - WALK_TARGET = REGISTERED
    mc::entity::ai::brain::Brain<int> brain;
    brain.registerMemory(MemoryModuleTypes::INTERACTION_TARGET);
    brain.registerMemory(MemoryModuleTypes::WALK_TARGET);

    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::INTERACTION_TARGET, MemoryModuleStatus::VALUE_PRESENT));
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::WALK_TARGET, MemoryModuleStatus::REGISTERED));
}

// 测试：ProtectOwnerTask 的记忆需求
TEST_F(InteractTaskMemoryTest, ProtectOwnerTaskMemoryRequirements)
{
    // ProtectOwnerTask 需要:
    // - OWNER_HURT_BY = VALUE_PRESENT
    // - ATTACK_TARGET = REGISTERED
    mc::entity::ai::brain::Brain<int> brain;
    brain.registerMemory(MemoryModuleTypes::OWNER_HURT_BY);
    brain.registerMemory(MemoryModuleTypes::ATTACK_TARGET);

    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::OWNER_HURT_BY, MemoryModuleStatus::VALUE_PRESENT));
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::ATTACK_TARGET, MemoryModuleStatus::REGISTERED));
}

// 测试：FollowParentTask 的记忆需求
TEST_F(InteractTaskMemoryTest, FollowParentTaskMemoryRequirements)
{
    // FollowParentTask 需要:
    // - NEAREST_VISIBLE_ADULT = VALUE_PRESENT
    mc::entity::ai::brain::Brain<int> brain;
    brain.registerMemory(MemoryModuleTypes::NEAREST_VISIBLE_ADULT);

    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::NEAREST_VISIBLE_ADULT, MemoryModuleStatus::VALUE_PRESENT));
}

// 测试：TemptTask 的记忆需求
TEST_F(InteractTaskMemoryTest, TemptTaskMemoryRequirements)
{
    // TemptTask 需要:
    // - TEMPTING_PLAYER = VALUE_PRESENT
    mc::entity::ai::brain::Brain<int> brain;
    brain.registerMemory(MemoryModuleTypes::TEMPTING_PLAYER);

    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::TEMPTING_PLAYER, MemoryModuleStatus::VALUE_PRESENT));
}

// 测试：PickupItemTask 的记忆需求
TEST_F(InteractTaskMemoryTest, PickupItemTaskMemoryRequirements)
{
    // PickupItemTask 需要:
    // - NEAREST_VISIBLE_WANTED_ITEM = VALUE_PRESENT
    mc::entity::ai::brain::Brain<int> brain;
    brain.registerMemory(MemoryModuleTypes::NEAREST_VISIBLE_WANTED_ITEM);

    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::NEAREST_VISIBLE_WANTED_ITEM, MemoryModuleStatus::VALUE_PRESENT));
}

// ============================================================================
// Sensor getUsedMemories 测试
// ============================================================================

class SensorUsedMemoriesTest : public ::testing::Test {
protected:
    void SetUp() override { MemoryModuleTypes::initialize(); }
};

// 测试：TemptingPlayerSensor 报告的记忆需求
TEST_F(SensorUsedMemoriesTest, TemptingPlayerSensorUsedMemories)
{
    using namespace mc::entity::ai::brain::sensor;
    TemptingPlayerSensor<VillagerEntity> sensor([](const ItemStack&) { return false; });
    auto usedMemories = sensor.getUsedMemories();

    bool hasTemptingPlayer = false;
    for (auto* mem : usedMemories) {
        if (mem == MemoryModuleTypes::TEMPTING_PLAYER) hasTemptingPlayer = true;
    }

    EXPECT_TRUE(hasTemptingPlayer) << "TemptingPlayerSensor 应使用 TEMPTING_PLAYER 记忆";
}

// 测试：InteractableDoorsSensor 报告的记忆需求
TEST_F(SensorUsedMemoriesTest, InteractableDoorsSensorUsedMemories)
{
    using namespace mc::entity::ai::brain::sensor;
    InteractableDoorsSensor<VillagerEntity> sensor;
    auto usedMemories = sensor.getUsedMemories();

    bool hasDoors = false;
    bool hasOpenedDoors = false;
    for (auto* mem : usedMemories) {
        if (mem == MemoryModuleTypes::INTERACTABLE_DOORS) hasDoors = true;
        if (mem == MemoryModuleTypes::OPENED_DOORS) hasOpenedDoors = true;
    }

    EXPECT_TRUE(hasDoors) << "InteractableDoorsSensor 应使用 INTERACTABLE_DOORS 记忆";
    EXPECT_TRUE(hasOpenedDoors) << "InteractableDoorsSensor 应使用 OPENED_DOORS 记忆";
}

// 测试：OwnerHurtBySensor 报告的记忆需求
TEST_F(SensorUsedMemoriesTest, OwnerHurtBySensorUsedMemories)
{
    using namespace mc::entity::ai::brain::sensor;
    OwnerHurtBySensor<VillagerEntity> sensor;
    auto usedMemories = sensor.getUsedMemories();

    bool hasOwnerHurtBy = false;
    for (auto* mem : usedMemories) {
        if (mem == MemoryModuleTypes::OWNER_HURT_BY) hasOwnerHurtBy = true;
    }

    EXPECT_TRUE(hasOwnerHurtBy) << "OwnerHurtBySensor 应使用 OWNER_HURT_BY 记忆";
}

// 测试：TemptingPlayerSensor 可自定义范围和间隔
TEST_F(SensorUsedMemoriesTest, TemptingPlayerSensorCustomConfig)
{
    using namespace mc::entity::ai::brain::sensor;
    // 自定义范围=15.0f，间隔=40
    TemptingPlayerSensor<VillagerEntity> sensor([](const ItemStack&) { return false; }, 15.0f, 40);
    auto usedMemories = sensor.getUsedMemories();

    bool hasTemptingPlayer = false;
    for (auto* mem : usedMemories) {
        if (mem == MemoryModuleTypes::TEMPTING_PLAYER) hasTemptingPlayer = true;
    }
    EXPECT_TRUE(hasTemptingPlayer);
}

// ============================================================================
// VillagerEntity Brain 集成测试
// ============================================================================

class VillagerBrainIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        MemoryModuleTypes::initialize();
        mc::entity::ai::brain::schedule::Schedule::initialize();
    }
};

// 测试：VillagerEntity 的 Brain 注册了必要的交互记忆模块
TEST_F(VillagerBrainIntegrationTest, VillagerBrainRegistersDoorMemories)
{
    VillagerEntity villager(EntityInstanceId(1), mc::test::testEcsRegistry());
    auto& brain = villager.brain();

    // InteractWithDoorTask 需要这些记忆模块
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::INTERACTABLE_DOORS, MemoryModuleStatus::REGISTERED))
        << "VillagerEntity Brain 应注册 INTERACTABLE_DOORS 记忆";
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::OPENED_DOORS, MemoryModuleStatus::REGISTERED))
        << "VillagerEntity Brain 应注册 OPENED_DOORS 记忆";
}

// 测试：VillagerEntity 的 Brain 注册了交互目标记忆
TEST_F(VillagerBrainIntegrationTest, VillagerBrainRegistersInteractionTargetMemory)
{
    VillagerEntity villager(EntityInstanceId(1), mc::test::testEcsRegistry());
    auto& brain = villager.brain();

    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::INTERACTION_TARGET, MemoryModuleStatus::REGISTERED))
        << "VillagerEntity Brain 应注册 INTERACTION_TARGET 记忆";
}

// ============================================================================
// 测试用世界子类
// ============================================================================

class InteractTaskTestWorld : public BaseTestWorld {
public:
    InteractTaskTestWorld() = default;
};

// ============================================================================
// InteractTasks TaskStatus 测试
// ============================================================================

class InteractTaskStatusTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        MemoryModuleTypes::initialize();
        mc::entity::ai::brain::schedule::Schedule::initialize();
        m_world = std::make_unique<InteractTaskTestWorld>();
    }

    std::unique_ptr<InteractTaskTestWorld> m_world;
};

// 测试：InteractWithDoorTask 在没有记忆时不应执行
TEST_F(InteractTaskStatusTest, InteractWithDoorTaskNoMemory)
{
    VillagerEntity villager(EntityInstanceId(1), mc::test::testEcsRegistry());
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    interact::InteractWithDoorTask<VillagerEntity> task;
    mc::math::Random rng(42);

    // 没有 INTERACTABLE_DOORS 记忆 → 不应执行
    EXPECT_FALSE(task.start(m_world.get(), &villager, 0, rng));
}

// 测试：VillagerInteractTask 在没有 INTERACTION_TARGET 时不应执行
TEST_F(InteractTaskStatusTest, VillagerInteractTaskNoTarget)
{
    VillagerEntity villager(EntityInstanceId(1), mc::test::testEcsRegistry());
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    interact::VillagerInteractTask<VillagerEntity> task;
    mc::math::Random rng(42);

    // 没有 INTERACTION_TARGET 记忆 → 不应执行
    EXPECT_FALSE(task.start(m_world.get(), &villager, 0, rng));
}

// 测试：TemptTask 在没有 TEMPTING_PLAYER 时不应执行
TEST_F(InteractTaskStatusTest, TemptTaskNoPlayer)
{
    VillagerEntity villager(EntityInstanceId(1), mc::test::testEcsRegistry());
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    interact::TemptTask<VillagerEntity> task;
    mc::math::Random rng(42);

    // 没有 TEMPTING_PLAYER 记忆 → 不应执行
    EXPECT_FALSE(task.start(m_world.get(), &villager, 0, rng));
}

// 测试：FollowParentTask 在没有 NEAREST_VISIBLE_ADULT 时不应执行
TEST_F(InteractTaskStatusTest, FollowParentTaskNoAdult)
{
    VillagerEntity villager(EntityInstanceId(1), mc::test::testEcsRegistry());
    villager.setWorld(m_world.get());
    villager.setPosition(0.0f, 64.0f, 0.0f);

    interact::FollowParentTask<VillagerEntity> task;
    mc::math::Random rng(42);

    // 没有 NEAREST_VISIBLE_ADULT 记忆 → 不应执行
    EXPECT_FALSE(task.start(m_world.get(), &villager, 0, rng));
}

} // namespace
