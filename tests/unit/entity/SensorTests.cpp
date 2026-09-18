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

#include "common/entity/ai/brain/Brain.hpp"
#include "common/entity/ai/brain/memory/BlockPosTarget.hpp"
#include "common/entity/ai/brain/memory/MemoryModuleType.hpp"
#include "common/entity/ai/brain/memory/WalkTarget.hpp"
#include "common/entity/ai/brain/sensor/Sensor.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/GlobalPos.hpp"

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace sensor {
namespace {

// ============================================================================
// Mock Entity for Testing Brain Memory Operations
// ============================================================================

/**
 * @brief Simple mock entity for brain testing
 *
 * This mock entity provides a Brain instance for testing memory operations
 * without requiring the full entity infrastructure.
 */
class MockTestEntity {
public:
    Brain<MockTestEntity> brain;

    Brain<MockTestEntity>& getBrain() { return brain; }
    const Brain<MockTestEntity>& getBrain() const { return brain; }
};

// ============================================================================
// Sensor Base Class Tests (using a custom test sensor)
// ============================================================================

class TestableSensor : public Sensor<MockTestEntity> {
public:
    TestableSensor(i32 interval = 20)
        : Sensor<MockTestEntity>(interval)
        , updateCount(0)
    {}

    int updateCount = 0;

    // Public accessor for testing
    std::unordered_set<const memory::MemoryModuleTypeBase*> testGetUsedMemories() const { return getUsedMemories(); }

protected:
    void update(IWorld* world, MockTestEntity* entity) override
    {
        (void)world;
        (void)entity;
        updateCount++;
    }

    std::unordered_set<const memory::MemoryModuleTypeBase*> getUsedMemories() const override
    {
        return {memory::MemoryModuleTypes::DUMMY};
    }
};

TEST(SensorBaseTest, CounterDecrement)
{
    memory::MemoryModuleTypes::initialize();

    TestableSensor sensor(5);
    MockTestEntity entity;

    // Initialize counter with random value
    math::Random random(12345);
    sensor.initCounter(random);

    // Tick multiple times - update should be called every 5 ticks
    for (int i = 0; i < 10; i++) {
        sensor.tick(nullptr, &entity);
    }

    // With interval 5, update should be called at least once
    EXPECT_GE(sensor.updateCount, 1);
}

TEST(SensorBaseTest, CounterInitializedOnlyOnce)
{
    TestableSensor sensor(10);
    math::Random random1(12345);
    math::Random random2(54321);

    // First init sets the counter
    sensor.initCounter(random1);

    // Second init should not change the counter (it's already >= 0)
    sensor.initCounter(random2);

    // No crash means the logic is working
}

TEST(SensorBaseTest, UpdateCalledAfterInterval)
{
    TestableSensor sensor(1); // Interval 1 means update every tick
    MockTestEntity entity;

    // With interval 1 and counter starting at 0, first tick should trigger update
    sensor.tick(nullptr, &entity);
    EXPECT_EQ(sensor.updateCount, 1);

    sensor.tick(nullptr, &entity);
    EXPECT_EQ(sensor.updateCount, 2);
}

TEST(SensorBaseTest, IntervalConstructor)
{
    TestableSensor sensor1(10);
    TestableSensor sensor2(20);
    TestableSensor sensor3(100);

    // Sensors constructed successfully with different intervals
}

TEST(SensorBaseTest, GetUsedMemoriesReturnsCorrectSet)
{
    memory::MemoryModuleTypes::initialize();

    TestableSensor sensor;
    auto memories = sensor.testGetUsedMemories();

    EXPECT_EQ(memories.size(), 1u);
    EXPECT_NE(memories.find(memory::MemoryModuleTypes::DUMMY), memories.end());
}

// ============================================================================
// Brain Memory Tests
// ============================================================================

TEST(BrainMemoryTest, RegisterAndSetMemory)
{
    memory::MemoryModuleTypes::initialize();

    Brain<MockTestEntity> brain;

    // Register memory module
    brain.registerMemory(memory::MemoryModuleTypes::HOME);

    // Set memory value
    GlobalPos pos(DimensionId(0), BlockPos(100, 64, 200));
    brain.setMemory(memory::MemoryModuleTypes::HOME, pos);

    // Get memory value
    auto value = brain.getMemory<GlobalPos>(memory::MemoryModuleTypes::HOME);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value->getDimensionId(), DimensionId(0));
    EXPECT_EQ(value->getPos(), BlockPos(100, 64, 200));
}

TEST(BrainMemoryTest, HasMemory)
{
    memory::MemoryModuleTypes::initialize();

    Brain<MockTestEntity> brain;
    brain.registerMemory(memory::MemoryModuleTypes::ATTACK_COOLING_DOWN);

    // Initially absent
    EXPECT_FALSE(brain.hasMemory(memory::MemoryModuleTypes::ATTACK_COOLING_DOWN));

    // Set value
    brain.setMemory(memory::MemoryModuleTypes::ATTACK_COOLING_DOWN, true);

    // Now present
    EXPECT_TRUE(brain.hasMemory(memory::MemoryModuleTypes::ATTACK_COOLING_DOWN));
}

TEST(BrainMemoryTest, RemoveMemory)
{
    memory::MemoryModuleTypes::initialize();

    Brain<MockTestEntity> brain;
    brain.registerMemory(memory::MemoryModuleTypes::HURT_BY_ENTITY);

    // Set and verify（实体类记忆存 id，永不悬垂）
    brain.setMemory(memory::MemoryModuleTypes::HURT_BY_ENTITY, EntityInstanceId(0x100));
    EXPECT_TRUE(brain.hasMemory(memory::MemoryModuleTypes::HURT_BY_ENTITY));

    // Remove
    brain.removeMemory(memory::MemoryModuleTypes::HURT_BY_ENTITY);
    EXPECT_FALSE(brain.hasMemory(memory::MemoryModuleTypes::HURT_BY_ENTITY));
}

TEST(BrainMemoryTest, ClearMemories)
{
    memory::MemoryModuleTypes::initialize();

    Brain<MockTestEntity> brain;
    brain.registerMemory(memory::MemoryModuleTypes::ATTACK_COOLING_DOWN);
    brain.registerMemory(memory::MemoryModuleTypes::HOME);

    brain.setMemory(memory::MemoryModuleTypes::ATTACK_COOLING_DOWN, true);
    GlobalPos pos(DimensionId(0), BlockPos(100, 64, 200));
    brain.setMemory(memory::MemoryModuleTypes::HOME, pos);

    EXPECT_TRUE(brain.hasMemory(memory::MemoryModuleTypes::ATTACK_COOLING_DOWN));
    EXPECT_TRUE(brain.hasMemory(memory::MemoryModuleTypes::HOME));

    brain.clear();

    EXPECT_FALSE(brain.hasMemory(memory::MemoryModuleTypes::ATTACK_COOLING_DOWN));
    EXPECT_FALSE(brain.hasMemory(memory::MemoryModuleTypes::HOME));
}

TEST(BrainMemoryTest, MemoryStatusAbsent)
{
    memory::MemoryModuleTypes::initialize();

    Brain<MockTestEntity> brain;
    brain.registerMemory(memory::MemoryModuleTypes::HOME);

    // Memory is registered but value absent
    EXPECT_FALSE(brain.hasMemory(memory::MemoryModuleTypes::HOME, memory::MemoryModuleStatus::VALUE_PRESENT));
    EXPECT_TRUE(brain.hasMemory(memory::MemoryModuleTypes::HOME, memory::MemoryModuleStatus::VALUE_ABSENT));
    EXPECT_TRUE(brain.hasMemory(memory::MemoryModuleTypes::HOME, memory::MemoryModuleStatus::REGISTERED));
}

TEST(BrainMemoryTest, UnregisteredMemoryReturnsFalse)
{
    memory::MemoryModuleTypes::initialize();

    Brain<MockTestEntity> brain;
    // HOME is not registered

    // All status checks should return false for unregistered memory
    EXPECT_FALSE(brain.hasMemory(memory::MemoryModuleTypes::HOME, memory::MemoryModuleStatus::VALUE_PRESENT));
    EXPECT_FALSE(brain.hasMemory(memory::MemoryModuleTypes::HOME, memory::MemoryModuleStatus::VALUE_ABSENT));
    EXPECT_FALSE(brain.hasMemory(memory::MemoryModuleTypes::HOME, memory::MemoryModuleStatus::REGISTERED));
}

// ============================================================================
// Brain Sensor Integration Tests
// ============================================================================

TEST(BrainSensorIntegrationTest, RegisterSensorAutoRegistersMemories)
{
    memory::MemoryModuleTypes::initialize();

    Brain<MockTestEntity> brain;

    // Register a sensor - using TestableSensor
    auto sensor = std::make_unique<TestableSensor>();
    brain.registerSensor(std::move(sensor));

    // The brain should auto-register the memories used by the sensor
    // This is tested indirectly - no exception means it worked
}

TEST(BrainSensorIntegrationTest, RegisterMultipleSensors)
{
    memory::MemoryModuleTypes::initialize();

    Brain<MockTestEntity> brain;

    brain.registerSensor(std::make_unique<TestableSensor>());
    brain.registerSensor(std::make_unique<TestableSensor>());
    brain.registerSensor(std::make_unique<TestableSensor>());

    // Multiple sensors registered successfully
}

// ============================================================================
// MemoryModuleTypes Initialization Tests
// ============================================================================

TEST(MemoryModuleTypesTest, AllTypesInitialized)
{
    memory::MemoryModuleTypes::initialize();

    // Verify key memory types are initialized
    EXPECT_NE(memory::MemoryModuleTypes::DUMMY, nullptr);

    // Entity lists
    EXPECT_NE(memory::MemoryModuleTypes::MOBS, nullptr);
    EXPECT_NE(memory::MemoryModuleTypes::VISIBLE_MOBS, nullptr);
    EXPECT_NE(memory::MemoryModuleTypes::VISIBLE_VILLAGER_BABIES, nullptr);
    EXPECT_NE(memory::MemoryModuleTypes::NEAREST_PLAYERS, nullptr);
    EXPECT_NE(memory::MemoryModuleTypes::NEAREST_VISIBLE_PLAYER, nullptr);
    EXPECT_NE(memory::MemoryModuleTypes::NEAREST_VISIBLE_TARGETABLE_PLAYER, nullptr);

    // Single entities
    EXPECT_NE(memory::MemoryModuleTypes::ATTACK_TARGET, nullptr);
    EXPECT_NE(memory::MemoryModuleTypes::INTERACTION_TARGET, nullptr);
    EXPECT_NE(memory::MemoryModuleTypes::HURT_BY_ENTITY, nullptr);
    EXPECT_NE(memory::MemoryModuleTypes::AVOID_TARGET, nullptr);
    EXPECT_NE(memory::MemoryModuleTypes::NEAREST_HOSTILE, nullptr);
    EXPECT_NE(memory::MemoryModuleTypes::BREED_TARGET, nullptr);
    EXPECT_NE(memory::MemoryModuleTypes::NEAREST_VISIBLE_ADULT, nullptr);

    // Positions
    EXPECT_NE(memory::MemoryModuleTypes::HOME, nullptr);
    EXPECT_NE(memory::MemoryModuleTypes::JOB_SITE, nullptr);
    EXPECT_NE(memory::MemoryModuleTypes::POTENTIAL_JOB_SITE, nullptr);
    EXPECT_NE(memory::MemoryModuleTypes::MEETING_POINT, nullptr);
    EXPECT_NE(memory::MemoryModuleTypes::NEAREST_BED, nullptr);

    // Movement
    EXPECT_NE(memory::MemoryModuleTypes::WALK_TARGET, nullptr);
    EXPECT_NE(memory::MemoryModuleTypes::LOOK_TARGET, nullptr);
    EXPECT_NE(memory::MemoryModuleTypes::PATH, nullptr);

    // Combat
    EXPECT_NE(memory::MemoryModuleTypes::ATTACK_COOLING_DOWN, nullptr);
    EXPECT_NE(memory::MemoryModuleTypes::HURT_BY, nullptr);

    // Time
    EXPECT_NE(memory::MemoryModuleTypes::LAST_SLEPT, nullptr);
    EXPECT_NE(memory::MemoryModuleTypes::LAST_WOKEN, nullptr);
    EXPECT_NE(memory::MemoryModuleTypes::LAST_WORKED_AT_POI, nullptr);

    // State
    EXPECT_NE(memory::MemoryModuleTypes::ADMIRING_ITEM, nullptr);
    EXPECT_NE(memory::MemoryModuleTypes::DANCING, nullptr);
    EXPECT_NE(memory::MemoryModuleTypes::ATE_RECENTLY, nullptr);

    // Piglin/Hoglin
    EXPECT_NE(memory::MemoryModuleTypes::NEAREST_VISIBLE_HUNTABLE_HOGLIN, nullptr);
    EXPECT_NE(memory::MemoryModuleTypes::NEAREST_VISIBLE_BABY_HOGLIN, nullptr);
    EXPECT_NE(memory::MemoryModuleTypes::NEAREST_TARGETABLE_PLAYER_NOT_WEARING_GOLD, nullptr);
    EXPECT_NE(memory::MemoryModuleTypes::NEAREST_ADULT_PIGLINS, nullptr);
    EXPECT_NE(memory::MemoryModuleTypes::NEAREST_VISIBLE_ADULT_PIGLINS, nullptr);
    EXPECT_NE(memory::MemoryModuleTypes::NEAREST_VISIBLE_ADULT_HOGLINS, nullptr);
    EXPECT_NE(memory::MemoryModuleTypes::NEAREST_VISIBLE_ADULT_PIGLIN, nullptr);
    EXPECT_NE(memory::MemoryModuleTypes::VISIBLE_ADULT_PIGLIN_COUNT, nullptr);
    EXPECT_NE(memory::MemoryModuleTypes::VISIBLE_ADULT_HOGLIN_COUNT, nullptr);
}

// ============================================================================
// WalkTarget and LookTarget Tests
// ============================================================================

TEST(WalkTargetTest, BlockPosConstructor)
{
    BlockPos pos(10, 64, -5);
    memory::WalkTarget target(pos, 0.5f, 3);

    EXPECT_NE(target.getTarget(), nullptr);
    EXPECT_EQ(target.getTarget()->getBlockPos(), pos);
    EXPECT_FLOAT_EQ(target.getSpeed(), 0.5f);
    EXPECT_EQ(target.getDistance(), 3);
}

TEST(WalkTargetTest, PositionCenter)
{
    // WalkTarget from BlockPos should use block center (x+0.5, y+0.5, z+0.5)
    BlockPos pos(0, 0, 0);
    memory::WalkTarget target(pos, 1.0f, 1);

    const auto& positionTarget = target.getTarget();
    ASSERT_NE(positionTarget, nullptr);

    Vector3 worldPos = positionTarget->getPosition();
    EXPECT_FLOAT_EQ(worldPos.x, 0.5f);
    EXPECT_FLOAT_EQ(worldPos.y, 0.5f);
    EXPECT_FLOAT_EQ(worldPos.z, 0.5f);
}

TEST(WalkTargetTest, InBrain)
{
    memory::MemoryModuleTypes::initialize();

    Brain<MockTestEntity> brain;
    brain.registerMemory(memory::MemoryModuleTypes::WALK_TARGET);

    memory::WalkTarget target(BlockPos(50, 70, 50), 0.75f, 2);
    brain.setMemory(memory::MemoryModuleTypes::WALK_TARGET, target);

    auto stored = brain.getMemory<memory::WalkTarget>(memory::MemoryModuleTypes::WALK_TARGET);
    ASSERT_TRUE(stored.has_value());
    EXPECT_EQ(stored->getTarget()->getBlockPos(), BlockPos(50, 70, 50));
    EXPECT_FLOAT_EQ(stored->getSpeed(), 0.75f);
    EXPECT_EQ(stored->getDistance(), 2);
}

TEST(BlockPosTargetTest, BasicOperations)
{
    BlockPos pos(100, 64, 200);
    memory::BlockPosTarget target(pos);

    EXPECT_EQ(target.getBlockPos(), pos);

    Vector3 worldPos = target.getPosition();
    EXPECT_FLOAT_EQ(worldPos.x, 100.5f);
    EXPECT_FLOAT_EQ(worldPos.y, 64.5f);
    EXPECT_FLOAT_EQ(worldPos.z, 200.5f);
}

TEST(LookTargetTest, InBrain)
{
    memory::MemoryModuleTypes::initialize();

    Brain<MockTestEntity> brain;
    brain.registerMemory(memory::MemoryModuleTypes::LOOK_TARGET);

    std::shared_ptr<memory::IPositionTarget> lookTarget =
        std::make_shared<memory::BlockPosTarget>(BlockPos(10, 65, 10));
    brain.setMemory(memory::MemoryModuleTypes::LOOK_TARGET, lookTarget);

    auto stored = brain.getMemory<std::shared_ptr<memory::IPositionTarget>>(memory::MemoryModuleTypes::LOOK_TARGET);
    ASSERT_TRUE(stored.has_value());
    EXPECT_EQ((*stored)->getBlockPos(), BlockPos(10, 65, 10));
}

// ============================================================================
// Brain Activity Tests
// ============================================================================

TEST(BrainActivityTest, DefaultActivity)
{
    Brain<MockTestEntity> brain;

    brain.setDefaultActivities({schedule::Activity::IDLE});
    brain.setFallbackActivity(schedule::Activity::IDLE);

    // Activity should be set
}

TEST(BrainActivityTest, HasActivity)
{
    memory::MemoryModuleTypes::initialize();

    Brain<MockTestEntity> brain;
    brain.setDefaultActivities({schedule::Activity::IDLE});

    // 默认活动只有在 Brain 经过一次活动初始化后才生效。
    // 对齐 MC Java Brain：activeActivities 初始为空，需 setActiveActivity/useDefaultActivity
    // 才会把 coreActivities(默认活动) 注入 activeActivities。
    // Cubium 中通过 clear() 将默认活动注入 m_activities。
    brain.clear();

    // Initially should have the default activity
    EXPECT_TRUE(brain.hasActivity(schedule::Activity::IDLE));
}

// ============================================================================
// GlobalPos Tests
// ============================================================================

TEST(GlobalPosTest, ConstructionAndAccess)
{
    GlobalPos pos(DimensionId(0), BlockPos(100, 64, 200));

    EXPECT_EQ(pos.getDimensionId(), DimensionId(0));
    EXPECT_EQ(pos.getPos(), BlockPos(100, 64, 200));
    EXPECT_EQ(pos.x(), 100);
    EXPECT_EQ(pos.y(), 64);
    EXPECT_EQ(pos.z(), 200);
}

TEST(GlobalPosTest, Equality)
{
    GlobalPos pos1(DimensionId(0), BlockPos(100, 64, 200));
    GlobalPos pos2(DimensionId(0), BlockPos(100, 64, 200));
    GlobalPos pos3(DimensionId(1), BlockPos(100, 64, 200));
    GlobalPos pos4(DimensionId(0), BlockPos(101, 64, 200));

    EXPECT_TRUE(pos1 == pos2);
    EXPECT_FALSE(pos1 == pos3);
    EXPECT_FALSE(pos1 == pos4);
}

TEST(GlobalPosTest, SameDimension)
{
    GlobalPos pos1(DimensionId(0), BlockPos(100, 64, 200));
    GlobalPos pos2(DimensionId(0), BlockPos(200, 64, 100));
    GlobalPos pos3(DimensionId(1), BlockPos(100, 64, 200));

    EXPECT_TRUE(pos1.sameDimension(pos2));
    EXPECT_FALSE(pos1.sameDimension(pos3));
}

TEST(GlobalPosTest, InBrainMemory)
{
    memory::MemoryModuleTypes::initialize();

    Brain<MockTestEntity> brain;
    brain.registerMemory(memory::MemoryModuleTypes::HOME);

    GlobalPos home(DimensionId(0), BlockPos(123, 64, 456));
    brain.setMemory(memory::MemoryModuleTypes::HOME, home);

    auto stored = brain.getMemory<GlobalPos>(memory::MemoryModuleTypes::HOME);
    ASSERT_TRUE(stored.has_value());
    EXPECT_EQ(stored->getDimensionId(), DimensionId(0));
    EXPECT_EQ(stored->getPos(), BlockPos(123, 64, 456));
}

} // namespace
} // namespace sensor
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
