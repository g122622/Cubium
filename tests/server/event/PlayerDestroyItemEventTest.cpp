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

#include "common/item/Items.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "server/event/ServerEventBus.hpp"
#include "server/event/events/ServerEvents.hpp"
#include "server/world/ServerWorld.hpp"
#include "common/world/gen/chunk/DebugChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include <atomic>

using namespace mc;
using namespace mc::server;
using namespace mc::server::event;

// ============================================================================
// PlayerDestroyItemEvent 测试
// ============================================================================

class PlayerDestroyItemEventTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化方块注册表
        VanillaBlocks::initialize();
        // 初始化物品注册表
        Items::initialize();

        // 创建 ServerWorld 用于测试
        ServerWorldConfig config;
        config.viewDistance = 10;
        config.dimension = 0;
        m_world = std::make_unique<ServerWorld>(config);
    }

    void TearDown() override
    {
        m_world.reset();
        // 清理事件订阅
        m_subscriptions.clear();
    }

    std::unique_ptr<ServerWorld> m_world;
    std::vector<event::ServerEventBus::HandlerId> m_subscriptions;
};

// ============================================================================
// 基本功能测试
// ============================================================================

TEST_F(PlayerDestroyItemEventTest, EventStructure_HasCorrectFields)
{
    // 测试事件结构是否正确
    ItemStack item(Items::DIAMOND_SWORD, 1);
    PlayerDestroyItemEvent event{100, PlayerId{42}, item, 0, Hand::MainHand};

    EXPECT_EQ(event.timestamp, 100);
    EXPECT_EQ(event.playerId, PlayerId{42});
    EXPECT_EQ(event.slot, 0);
    EXPECT_EQ(event.hand, Hand::MainHand);
    EXPECT_FALSE(event.item.isEmpty());
    EXPECT_EQ(event.item.getItem(), Items::DIAMOND_SWORD);
}

TEST_F(PlayerDestroyItemEventTest, EventStructure_OffHandSlot)
{
    // 测试副手槽位
    ItemStack item(Items::DIAMOND_SWORD, 1);
    PlayerDestroyItemEvent event{100, PlayerId{42}, item, 40, Hand::OffHand};

    EXPECT_EQ(event.slot, 40);
    EXPECT_EQ(event.hand, Hand::OffHand);
}

TEST_F(PlayerDestroyItemEventTest, EventStructure_UnknownSlot)
{
    // 测试未知槽位
    ItemStack item(Items::DIAMOND_SWORD, 1);
    PlayerDestroyItemEvent event{100, PlayerId{42}, item, -1, Hand::MainHand};

    EXPECT_EQ(event.slot, -1);
}

// ============================================================================
// 事件发布测试
// ============================================================================

TEST_F(PlayerDestroyItemEventTest, ServerWorld_OnPlayerDestroyItem_PublishesEvent)
{
    // 订阅事件
    std::atomic<bool> eventReceived{false};
    PlayerDestroyItemEvent receivedEvent{0, PlayerId{0}, ItemStack{}, 0, Hand::MainHand};

    auto subscription = event::ServerEventBus::instance().makeSubscription<PlayerDestroyItemEvent>(
        [&](const PlayerDestroyItemEvent& e) {
            eventReceived = true;
            receivedEvent = e;
        });

    // 发布事件
    ItemStack item(Items::DIAMOND_SWORD, 1);
    m_world->onPlayerDestroyItem(PlayerId{42}, item, 0, Hand::MainHand);

    // 验证事件被接收
    EXPECT_TRUE(eventReceived);
    EXPECT_EQ(receivedEvent.playerId, PlayerId{42});
    EXPECT_EQ(receivedEvent.slot, 0);
    EXPECT_EQ(receivedEvent.hand, Hand::MainHand);

    // 清理订阅
    subscription.unsubscribe();
}

TEST_F(PlayerDestroyItemEventTest, ServerWorld_OnPlayerDestroyItem_MultipleEvents)
{
    // 订阅事件并计数
    std::atomic<i32> eventCount{0};

    auto subscription = event::ServerEventBus::instance().makeSubscription<PlayerDestroyItemEvent>(
        [&](const PlayerDestroyItemEvent&) {
            eventCount++;
        });

    // 发布多个事件
    ItemStack item1(Items::DIAMOND_SWORD, 1);
    ItemStack item2(Items::DIAMOND_PICKAXE, 1);
    ItemStack item3(Items::DIAMOND_AXE, 1);

    m_world->onPlayerDestroyItem(PlayerId{1}, item1, 0, Hand::MainHand);
    m_world->onPlayerDestroyItem(PlayerId{2}, item2, 40, Hand::OffHand);
    m_world->onPlayerDestroyItem(PlayerId{3}, item3, 5, Hand::MainHand);

    // 验证收到了所有事件
    EXPECT_EQ(eventCount.load(), 3);

    // 清理订阅
    subscription.unsubscribe();
}

// ============================================================================
// 物品类型测试
// ============================================================================

TEST_F(PlayerDestroyItemEventTest, DifferentItemTypes)
{
    // 测试不同物品类型的事件
    std::vector<ItemStack> receivedItems;

    auto subscription = event::ServerEventBus::instance().makeSubscription<PlayerDestroyItemEvent>(
        [&](const PlayerDestroyItemEvent& e) {
            receivedItems.push_back(e.item);
        });

    // 发布不同物品的事件
    ItemStack sword(Items::DIAMOND_SWORD, 1);
    ItemStack pickaxe(Items::DIAMOND_PICKAXE, 1);
    ItemStack axe(Items::DIAMOND_AXE, 1);
    ItemStack shovel(Items::DIAMOND_SHOVEL, 1);
    ItemStack hoe(Items::DIAMOND_HOE, 1);

    m_world->onPlayerDestroyItem(PlayerId{1}, sword, 0, Hand::MainHand);
    m_world->onPlayerDestroyItem(PlayerId{2}, pickaxe, 1, Hand::MainHand);
    m_world->onPlayerDestroyItem(PlayerId{3}, axe, 2, Hand::MainHand);
    m_world->onPlayerDestroyItem(PlayerId{4}, shovel, 3, Hand::MainHand);
    m_world->onPlayerDestroyItem(PlayerId{5}, hoe, 4, Hand::MainHand);

    // 验证收到了所有物品
    EXPECT_EQ(receivedItems.size(), 5);
    EXPECT_EQ(receivedItems[0].getItem(), Items::DIAMOND_SWORD);
    EXPECT_EQ(receivedItems[1].getItem(), Items::DIAMOND_PICKAXE);
    EXPECT_EQ(receivedItems[2].getItem(), Items::DIAMOND_AXE);
    EXPECT_EQ(receivedItems[3].getItem(), Items::DIAMOND_SHOVEL);
    EXPECT_EQ(receivedItems[4].getItem(), Items::DIAMOND_HOE);

    // 清理订阅
    subscription.unsubscribe();
}

// ============================================================================
// 时间戳测试
// ============================================================================

TEST_F(PlayerDestroyItemEventTest, Timestamp_UsesWorldTick)
{
    // 获取世界的当前 tick
    u64 worldTick = m_world->currentTick();

    // 订阅事件
    u64 receivedTick = 0;
    auto subscription = event::ServerEventBus::instance().makeSubscription<PlayerDestroyItemEvent>(
        [&](const PlayerDestroyItemEvent& e) {
            receivedTick = e.timestamp;
        });

    // 发布事件
    ItemStack item(Items::DIAMOND_SWORD, 1);
    m_world->onPlayerDestroyItem(PlayerId{1}, item, 0, Hand::MainHand);

    // 验证时间戳正确
    EXPECT_EQ(receivedTick, worldTick);

    // 清理订阅
    subscription.unsubscribe();
}

// ============================================================================
// 继承关系测试
// ============================================================================

TEST_F(PlayerDestroyItemEventTest, EventInheritsFromServerEvent)
{
    // 验证事件继承自 ServerEvent
    PlayerDestroyItemEvent event{100, PlayerId{42}, ItemStack{}, 0, Hand::MainHand};

    // ServerEvent 基类有 timestamp 字段
    EXPECT_EQ(event.timestamp, 100);
}
