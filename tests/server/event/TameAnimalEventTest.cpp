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

#include "common/world/block/registry/VanillaBlocks.hpp"
#include "server/event/ServerEventBus.hpp"
#include "server/event/events/ServerEvents.hpp"
#include "server/world/ServerWorld.hpp"
#include <atomic>

using namespace mc;
using namespace mc::server;
using namespace mc::server::event;

// ============================================================================
// TameAnimalEvent 测试
// ============================================================================

class TameAnimalEventTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        ServerWorldConfig config;
        config.viewDistance = 10;
        config.dimension = 0;
        m_world = std::make_unique<ServerWorld>(config);
    }

    void TearDown() override
    {
        m_world.reset();
        m_subscriptions.clear();
    }

    std::unique_ptr<ServerWorld> m_world;
    std::vector<event::ServerEventBus::HandlerId> m_subscriptions;
};

// ============================================================================
// 事件结构测试
// ============================================================================

TEST_F(TameAnimalEventTest, EventStructure_HasCorrectFields)
{
    // 测试事件结构是否正确
    PlayerId playerId{42};
    Entity* animal = reinterpret_cast<Entity*>(0x1); // 测试用空指针

    TameAnimalEvent event{100, playerId, animal};

    EXPECT_EQ(event.timestamp, 100);
    EXPECT_EQ(event.playerId, PlayerId{42});
    EXPECT_EQ(event.animal, animal);
}

TEST_F(TameAnimalEventTest, EventStructure_NullAnimal)
{
    // 测试动物指针为空的情况
    PlayerId playerId{10};
    TameAnimalEvent event{200, playerId, nullptr};

    EXPECT_EQ(event.timestamp, 200);
    EXPECT_EQ(event.playerId, PlayerId{10});
    EXPECT_EQ(event.animal, nullptr);
}

TEST_F(TameAnimalEventTest, EventStructure_ZeroPlayerId)
{
    // 测试玩家ID为0的情况（非玩家驯服场景，不应出现但需健壮）
    Entity* animal = reinterpret_cast<Entity*>(0x2);
    TameAnimalEvent event{300, PlayerId{0}, animal};

    EXPECT_EQ(event.playerId, PlayerId{0});
    EXPECT_EQ(event.animal, animal);
}

// ============================================================================
// 事件发布测试
// ============================================================================

TEST_F(TameAnimalEventTest, ServerWorld_OnTameAnimal_PublishesEvent)
{
    // 订阅事件
    std::atomic<bool> eventReceived{false};
    TameAnimalEvent receivedEvent{0, PlayerId{0}, nullptr};

    auto subscription =
        event::ServerEventBus::instance().makeSubscription<TameAnimalEvent>([&](const TameAnimalEvent& e) {
            eventReceived = true;
            receivedEvent = e;
        });

    // 发布事件
    PlayerId playerId{42};
    Entity* animal = reinterpret_cast<Entity*>(0x100);
    m_world->onTameAnimal(playerId, animal);

    // 验证事件被接收
    EXPECT_TRUE(eventReceived);
    EXPECT_EQ(receivedEvent.playerId, PlayerId{42});
    EXPECT_EQ(receivedEvent.animal, animal);

    // 清理订阅
    subscription.unsubscribe();
}

TEST_F(TameAnimalEventTest, ServerWorld_OnTameAnimal_MultipleEvents)
{
    // 订阅事件并计数
    std::atomic<i32> eventCount{0};

    auto subscription = event::ServerEventBus::instance().makeSubscription<TameAnimalEvent>(
        [&](const TameAnimalEvent&) { eventCount++; });

    // 发布多个事件（模拟多个玩家驯服不同动物）
    m_world->onTameAnimal(PlayerId{1}, reinterpret_cast<Entity*>(0x10));
    m_world->onTameAnimal(PlayerId{2}, reinterpret_cast<Entity*>(0x20));
    m_world->onTameAnimal(PlayerId{3}, reinterpret_cast<Entity*>(0x30));

    // 验证收到了所有事件
    EXPECT_EQ(eventCount.load(), 3);

    // 清理订阅
    subscription.unsubscribe();
}

TEST_F(TameAnimalEventTest, ServerWorld_OnTameAnimal_DifferentPlayers)
{
    // 测试不同玩家驯服动物的事件
    std::vector<PlayerId> receivedPlayerIds;

    auto subscription = event::ServerEventBus::instance().makeSubscription<TameAnimalEvent>(
        [&](const TameAnimalEvent& e) { receivedPlayerIds.push_back(e.playerId); });

    // 模拟多个玩家驯服动物
    m_world->onTameAnimal(PlayerId{100}, reinterpret_cast<Entity*>(0x10));
    m_world->onTameAnimal(PlayerId{200}, reinterpret_cast<Entity*>(0x20));
    m_world->onTameAnimal(PlayerId{300}, reinterpret_cast<Entity*>(0x30));

    // 验证不同玩家的ID被正确传递
    EXPECT_EQ(receivedPlayerIds.size(), 3u);
    EXPECT_EQ(receivedPlayerIds[0], PlayerId{100});
    EXPECT_EQ(receivedPlayerIds[1], PlayerId{200});
    EXPECT_EQ(receivedPlayerIds[2], PlayerId{300});

    // 清理订阅
    subscription.unsubscribe();
}

// ============================================================================
// 时间戳测试
// ============================================================================

TEST_F(TameAnimalEventTest, Timestamp_UsesWorldTick)
{
    // 获取世界的当前 tick
    u64 worldTick = m_world->currentTick();

    // 订阅事件
    u64 receivedTick = 0;
    auto subscription = event::ServerEventBus::instance().makeSubscription<TameAnimalEvent>(
        [&](const TameAnimalEvent& e) { receivedTick = e.timestamp; });

    // 发布事件
    m_world->onTameAnimal(PlayerId{1}, reinterpret_cast<Entity*>(0x10));

    // 验证时间戳正确
    EXPECT_EQ(receivedTick, worldTick);

    // 清理订阅
    subscription.unsubscribe();
}

// ============================================================================
// 继承关系测试
// ============================================================================

TEST_F(TameAnimalEventTest, EventInheritsFromServerEvent)
{
    // 验证事件继承自 ServerEvent
    TameAnimalEvent event{100, PlayerId{42}, nullptr};

    // ServerEvent 基类有 timestamp 字段
    EXPECT_EQ(event.timestamp, 100);
}

// ============================================================================
// 取消订阅测试
// ============================================================================

TEST_F(TameAnimalEventTest, Unsubscribe_StopsReceivingEvents)
{
    // 订阅事件
    std::atomic<i32> eventCount{0};

    auto subscription = event::ServerEventBus::instance().makeSubscription<TameAnimalEvent>(
        [&](const TameAnimalEvent&) { eventCount++; });

    // 发布事件 - 应该收到
    m_world->onTameAnimal(PlayerId{1}, reinterpret_cast<Entity*>(0x10));
    EXPECT_EQ(eventCount.load(), 1);

    // 取消订阅
    subscription.unsubscribe();

    // 再次发布事件 - 不应该收到
    m_world->onTameAnimal(PlayerId{2}, reinterpret_cast<Entity*>(0x20));
    EXPECT_EQ(eventCount.load(), 1);
}

// ============================================================================
// 多订阅者测试
// ============================================================================

TEST_F(TameAnimalEventTest, MultipleSubscribers_AllReceiveEvent)
{
    // 多个订阅者
    std::atomic<i32> count1{0};
    std::atomic<i32> count2{0};

    auto sub1 =
        event::ServerEventBus::instance().makeSubscription<TameAnimalEvent>([&](const TameAnimalEvent&) { count1++; });

    auto sub2 =
        event::ServerEventBus::instance().makeSubscription<TameAnimalEvent>([&](const TameAnimalEvent&) { count2++; });

    // 发布事件
    m_world->onTameAnimal(PlayerId{1}, reinterpret_cast<Entity*>(0x10));

    // 两个订阅者都应该收到
    EXPECT_EQ(count1.load(), 1);
    EXPECT_EQ(count2.load(), 1);

    // 清理
    sub1.unsubscribe();
    sub2.unsubscribe();
}
