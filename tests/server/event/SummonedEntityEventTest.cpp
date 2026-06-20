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
// SummonedEntityEvent 测试
// ============================================================================

class SummonedEntityEventTest : public ::testing::Test {
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

TEST_F(SummonedEntityEventTest, EventStructure_HasCorrectFields)
{
    // 测试事件结构是否正确
    PlayerId playerId{42};
    Entity* entity = reinterpret_cast<Entity*>(0x1);

    SummonedEntityEvent event{100, playerId, entity};

    EXPECT_EQ(event.timestamp, 100);
    EXPECT_EQ(event.playerId, PlayerId{42});
    EXPECT_EQ(event.entity, entity);
}

TEST_F(SummonedEntityEventTest, EventStructure_NullEntity)
{
    // 测试实体指针为空的情况
    PlayerId playerId{10};
    SummonedEntityEvent event{200, playerId, nullptr};

    EXPECT_EQ(event.timestamp, 200);
    EXPECT_EQ(event.playerId, PlayerId{10});
    EXPECT_EQ(event.entity, nullptr);
}

TEST_F(SummonedEntityEventTest, EventStructure_ZeroPlayerId)
{
    // 测试玩家ID为0的情况（非玩家召唤场景，如铁傀儡自动生成）
    // AdvancementEventHandler 中会检查 playerId 是否为 0 来跳过进度触发
    Entity* entity = reinterpret_cast<Entity*>(0x2);
    SummonedEntityEvent event{300, PlayerId{0}, entity};

    EXPECT_EQ(event.playerId, PlayerId{0});
    EXPECT_EQ(event.entity, entity);
}

// ============================================================================
// 事件发布测试
// ============================================================================

TEST_F(SummonedEntityEventTest, ServerWorld_OnSummonedEntity_PublishesEvent)
{
    // 订阅事件
    std::atomic<bool> eventReceived{false};
    SummonedEntityEvent receivedEvent{0, PlayerId{0}, nullptr};

    auto subscription =
        event::ServerEventBus::instance().makeSubscription<SummonedEntityEvent>([&](const SummonedEntityEvent& e) {
            eventReceived = true;
            receivedEvent = e;
        });

    // 发布事件
    PlayerId playerId{42};
    Entity* entity = reinterpret_cast<Entity*>(0x100);
    m_world->onSummonedEntity(playerId, entity);

    // 验证事件被接收
    EXPECT_TRUE(eventReceived);
    EXPECT_EQ(receivedEvent.playerId, PlayerId{42});
    EXPECT_EQ(receivedEvent.entity, entity);

    // 清理订阅
    subscription.unsubscribe();
}

TEST_F(SummonedEntityEventTest, ServerWorld_OnSummonedEntity_MultipleEvents)
{
    // 订阅事件并计数
    std::atomic<i32> eventCount{0};

    auto subscription = event::ServerEventBus::instance().makeSubscription<SummonedEntityEvent>(
        [&](const SummonedEntityEvent&) { eventCount++; });

    // 发布多个事件（模拟多个玩家召唤不同实体）
    m_world->onSummonedEntity(PlayerId{1}, reinterpret_cast<Entity*>(0x10));
    m_world->onSummonedEntity(PlayerId{2}, reinterpret_cast<Entity*>(0x20));
    m_world->onSummonedEntity(PlayerId{3}, reinterpret_cast<Entity*>(0x30));

    // 验证收到了所有事件
    EXPECT_EQ(eventCount.load(), 3);

    // 清理订阅
    subscription.unsubscribe();
}

TEST_F(SummonedEntityEventTest, ServerWorld_OnSummonedEntity_DifferentPlayers)
{
    // 测试不同玩家召唤实体的事件
    std::vector<PlayerId> receivedPlayerIds;

    auto subscription = event::ServerEventBus::instance().makeSubscription<SummonedEntityEvent>(
        [&](const SummonedEntityEvent& e) { receivedPlayerIds.push_back(e.playerId); });

    // 模拟多个玩家召唤实体
    m_world->onSummonedEntity(PlayerId{100}, reinterpret_cast<Entity*>(0x10));
    m_world->onSummonedEntity(PlayerId{200}, reinterpret_cast<Entity*>(0x20));
    m_world->onSummonedEntity(PlayerId{300}, reinterpret_cast<Entity*>(0x30));

    // 验证不同玩家的ID被正确传递
    EXPECT_EQ(receivedPlayerIds.size(), 3u);
    EXPECT_EQ(receivedPlayerIds[0], PlayerId{100});
    EXPECT_EQ(receivedPlayerIds[1], PlayerId{200});
    EXPECT_EQ(receivedPlayerIds[2], PlayerId{300});

    // 清理订阅
    subscription.unsubscribe();
}

TEST_F(SummonedEntityEventTest, ServerWorld_OnSummonedEntity_ZeroPlayerIdForNonPlayer)
{
    // 测试非玩家召唤实体（如铁傀儡自动生成）时 playerId 为 0
    // AdvancementEventHandler._onSummonedEntity 会检查 playerId == 0 并跳过
    std::atomic<bool> eventReceived{false};
    SummonedEntityEvent receivedEvent{0, PlayerId{1}, nullptr};

    auto subscription =
        event::ServerEventBus::instance().makeSubscription<SummonedEntityEvent>([&](const SummonedEntityEvent& e) {
            eventReceived = true;
            receivedEvent = e;
        });

    // 发布事件，playerId 为 0（非玩家召唤）
    Entity* entity = reinterpret_cast<Entity*>(0x50);
    m_world->onSummonedEntity(PlayerId{0}, entity);

    // 事件仍然应该被发布（事件总线不关心 playerId 是否为 0）
    EXPECT_TRUE(eventReceived);
    EXPECT_EQ(receivedEvent.playerId, PlayerId{0});
    EXPECT_EQ(receivedEvent.entity, entity);

    // 清理订阅
    subscription.unsubscribe();
}

// ============================================================================
// 时间戳测试
// ============================================================================

TEST_F(SummonedEntityEventTest, Timestamp_UsesWorldTick)
{
    // 获取世界的当前 tick
    u64 worldTick = m_world->currentTick();

    // 订阅事件
    u64 receivedTick = 0;
    auto subscription = event::ServerEventBus::instance().makeSubscription<SummonedEntityEvent>(
        [&](const SummonedEntityEvent& e) { receivedTick = e.timestamp; });

    // 发布事件
    m_world->onSummonedEntity(PlayerId{1}, reinterpret_cast<Entity*>(0x10));

    // 验证时间戳正确
    EXPECT_EQ(receivedTick, worldTick);

    // 清理订阅
    subscription.unsubscribe();
}

// ============================================================================
// 继承关系测试
// ============================================================================

TEST_F(SummonedEntityEventTest, EventInheritsFromServerEvent)
{
    // 验证事件继承自 ServerEvent
    SummonedEntityEvent event{100, PlayerId{42}, nullptr};

    // ServerEvent 基类有 timestamp 字段
    EXPECT_EQ(event.timestamp, 100);
}

// ============================================================================
// 取消订阅测试
// ============================================================================

TEST_F(SummonedEntityEventTest, Unsubscribe_StopsReceivingEvents)
{
    // 订阅事件
    std::atomic<i32> eventCount{0};

    auto subscription = event::ServerEventBus::instance().makeSubscription<SummonedEntityEvent>(
        [&](const SummonedEntityEvent&) { eventCount++; });

    // 发布事件 - 应该收到
    m_world->onSummonedEntity(PlayerId{1}, reinterpret_cast<Entity*>(0x10));
    EXPECT_EQ(eventCount.load(), 1);

    // 取消订阅
    subscription.unsubscribe();

    // 再次发布事件 - 不应该收到
    m_world->onSummonedEntity(PlayerId{2}, reinterpret_cast<Entity*>(0x20));
    EXPECT_EQ(eventCount.load(), 1);
}

// ============================================================================
// 多订阅者测试
// ============================================================================

TEST_F(SummonedEntityEventTest, MultipleSubscribers_AllReceiveEvent)
{
    // 多个订阅者
    std::atomic<i32> count1{0};
    std::atomic<i32> count2{0};

    auto sub1 = event::ServerEventBus::instance().makeSubscription<SummonedEntityEvent>(
        [&](const SummonedEntityEvent&) { count1++; });

    auto sub2 = event::ServerEventBus::instance().makeSubscription<SummonedEntityEvent>(
        [&](const SummonedEntityEvent&) { count2++; });

    // 发布事件
    m_world->onSummonedEntity(PlayerId{1}, reinterpret_cast<Entity*>(0x10));

    // 两个订阅者都应该收到
    EXPECT_EQ(count1.load(), 1);
    EXPECT_EQ(count2.load(), 1);

    // 清理
    sub1.unsubscribe();
    sub2.unsubscribe();
}
