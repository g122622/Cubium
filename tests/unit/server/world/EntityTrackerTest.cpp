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

#include <thread>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "server/world/entity/EntityTracker.hpp"

using namespace mc;
using namespace mc::server;

// ============================================================================
// Test Helper: Minimal Entity for testing
// ============================================================================

class TestEntity : public Entity {
public:
    TestEntity(EntityInstanceId id)
        : Entity(id, nullptr, mc::test::testEcsRegistry())
    {
        // 设置位置
        setPosition(0.0f, 64.0f, 0.0f);
    }

    void tick() override {}
};

class TestLivingEntity : public LivingEntity {
public:
    TestLivingEntity(EntityInstanceId id)
        : LivingEntity(id, nullptr, mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }

    void tick() override {}
};

class TestMobEntity : public MobEntity {
public:
    TestMobEntity(EntityInstanceId id)
        : MobEntity(id, mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

// ============================================================================
// EntityTracker Basic Tests
// ============================================================================

class EntityTrackerTest : public ::testing::Test {
protected:
    void SetUp() override { tracker = std::make_unique<EntityTracker>(); }

    void TearDown() override { tracker.reset(); }

    std::unique_ptr<EntityTracker> tracker;
};

TEST_F(EntityTrackerTest, TrackEntity)
{
    TestEntity entity(1);
    tracker->trackEntity(&entity);

    EXPECT_TRUE(tracker->isTracking(1));
    EXPECT_EQ(tracker->trackedEntityCount(), 1);
}

TEST_F(EntityTrackerTest, TrackMultipleEntities)
{
    TestEntity entity1(1);
    TestEntity entity2(2);
    TestEntity entity3(3);

    tracker->trackEntity(&entity1);
    tracker->trackEntity(&entity2);
    tracker->trackEntity(&entity3);

    EXPECT_TRUE(tracker->isTracking(1));
    EXPECT_TRUE(tracker->isTracking(2));
    EXPECT_TRUE(tracker->isTracking(3));
    EXPECT_EQ(tracker->trackedEntityCount(), 3);
}

TEST_F(EntityTrackerTest, TrackSameEntityTwice)
{
    TestEntity entity(1);
    tracker->trackEntity(&entity);
    tracker->trackEntity(&entity); // 重复追踪

    // 应该只追踪一次
    EXPECT_TRUE(tracker->isTracking(1));
    EXPECT_EQ(tracker->trackedEntityCount(), 1);
}

TEST_F(EntityTrackerTest, UntrackEntity)
{
    TestEntity entity(1);
    tracker->trackEntity(&entity);
    EXPECT_TRUE(tracker->isTracking(1));

    tracker->untrackEntity(1);
    EXPECT_FALSE(tracker->isTracking(1));
    EXPECT_EQ(tracker->trackedEntityCount(), 0);
}

TEST_F(EntityTrackerTest, UntrackNonExistentEntity)
{
    // 移除不存在的实体不应崩溃
    tracker->untrackEntity(999);
    EXPECT_EQ(tracker->trackedEntityCount(), 0);
}

TEST_F(EntityTrackerTest, IsTrackingReturnsFalseForUnknownEntity)
{
    EXPECT_FALSE(tracker->isTracking(999));
}

TEST_F(EntityTrackerTest, TrackedEntityCount)
{
    EXPECT_EQ(tracker->trackedEntityCount(), 0);

    TestEntity entity1(1);
    TestEntity entity2(2);

    tracker->trackEntity(&entity1);
    EXPECT_EQ(tracker->trackedEntityCount(), 1);

    tracker->trackEntity(&entity2);
    EXPECT_EQ(tracker->trackedEntityCount(), 2);

    tracker->untrackEntity(1);
    EXPECT_EQ(tracker->trackedEntityCount(), 1);

    tracker->untrackEntity(2);
    EXPECT_EQ(tracker->trackedEntityCount(), 0);
}

TEST_F(EntityTrackerTest, SetTrackingDistance)
{
    tracker->setTrackingDistance(5);
    EXPECT_EQ(tracker->trackingDistance(), 5);

    tracker->setTrackingDistance(15);
    EXPECT_EQ(tracker->trackingDistance(), 15);
}

// ============================================================================
// EntityTracker Player Tracking Tests
// ============================================================================

TEST_F(EntityTrackerTest, RemovePlayerClearsTracking)
{
    // 首先添加一些实体追踪
    TestEntity entity1(1);
    TestEntity entity2(2);
    tracker->trackEntity(&entity1);
    tracker->trackEntity(&entity2);

    // 移除玩家（玩家应该没有被追踪任何实体）
    tracker->removePlayer(100);

    // 实体仍然应该被追踪
    EXPECT_TRUE(tracker->isTracking(1));
    EXPECT_TRUE(tracker->isTracking(2));
}

TEST_F(EntityTrackerTest, GetPlayerTrackedEntitiesEmpty)
{
    // 新玩家没有追踪任何实体
    auto tracked = tracker->getPlayerTrackedEntities(100);
    EXPECT_TRUE(tracked.empty());
}

// ============================================================================
// EntityTracker ShouldTrack Distance Tests
// ============================================================================

TEST_F(EntityTrackerTest, TrackingDistanceCalculation)
{
    // 默认追踪距离是 10 区块 = 160 方块
    EXPECT_EQ(tracker->trackingDistance(), 10);
}

// ============================================================================
// EntityTracker Thread Safety Tests
// ============================================================================

TEST_F(EntityTrackerTest, ConcurrentTrackUntrack)
{
    std::vector<std::thread> threads;

    // 并发追踪和取消追踪
    for (size_t i = 0; i < 10; ++i) {
        threads.emplace_back([this, i]() {
            const EntityInstanceId entityId = static_cast<EntityInstanceId>(i);
            TestEntity entity(entityId);
            tracker->trackEntity(&entity);
            tracker->isTracking(entityId);
            tracker->untrackEntity(entityId);
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // 如果没有崩溃或数据竞争，测试通过
    EXPECT_TRUE(true);
}

TEST_F(EntityTrackerTest, ConcurrentTrackAndCount)
{
    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};

    // 并发追踪和计数
    for (size_t i = 0; i < 10; ++i) {
        threads.emplace_back([this, i, &successCount]() {
            const EntityInstanceId entityId = static_cast<EntityInstanceId>(i);
            TestEntity entity(entityId);
            tracker->trackEntity(&entity);

            if (tracker->isTracking(entityId)) {
                successCount++;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // 所有追踪都应该成功
    EXPECT_EQ(successCount.load(), 10);
    EXPECT_EQ(tracker->trackedEntityCount(), 10);
}

// ============================================================================
// TrackedEntity Structure Tests
// ============================================================================

TEST(TrackedEntityTest, DefaultValues)
{
    TrackedEntity tracked;
    tracked.entityId = 1;

    EXPECT_EQ(tracked.entityId, 1);
    EXPECT_TRUE(tracked.trackingPlayers.empty());
    EXPECT_EQ(tracked.lastPosition, Vector3(0, 0, 0));
    EXPECT_FLOAT_EQ(tracked.lastYaw, 0.0f);
    EXPECT_FLOAT_EQ(tracked.lastPitch, 0.0f);
    EXPECT_EQ(tracked.updateCounter, 0u);
    EXPECT_TRUE(tracked.needsFullUpdate);
}

TEST(TrackedEntityTest, TrackingPlayersSet)
{
    TrackedEntity tracked;
    tracked.trackingPlayers.insert(1);
    tracked.trackingPlayers.insert(2);
    tracked.trackingPlayers.insert(3);

    EXPECT_EQ(tracked.trackingPlayers.size(), 3u);
    EXPECT_TRUE(tracked.trackingPlayers.count(1));
    EXPECT_TRUE(tracked.trackingPlayers.count(2));
    EXPECT_TRUE(tracked.trackingPlayers.count(3));
    EXPECT_FALSE(tracked.trackingPlayers.count(4));
}

// ============================================================================
// EntityTracker Integration with LivingEntity/MobEntity
// ============================================================================

TEST_F(EntityTrackerTest, TrackLivingEntity)
{
    TestLivingEntity entity(1);
    entity.setPosition(100.0, 64.0, 200.0);
    entity.setRotation(45.0f, 30.0f);

    tracker->trackEntity(&entity);

    EXPECT_TRUE(tracker->isTracking(1));
}

TEST_F(EntityTrackerTest, TrackMobEntity)
{
    TestMobEntity entity(1);
    entity.setPosition(50.0, 70.0, -30.0);

    tracker->trackEntity(&entity);

    EXPECT_TRUE(tracker->isTracking(1));
}

TEST_F(EntityTrackerTest, TrackEntityWithPosition)
{
    TestEntity entity(1);
    entity.setPosition(123.0, 64.0, 456.0);

    tracker->trackEntity(&entity);

    EXPECT_TRUE(tracker->isTracking(1));
    // EntityTracker 内部存储了位置用于后续位置变化检测
}
