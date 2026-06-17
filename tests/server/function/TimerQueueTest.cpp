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

#include "server/function/TimerQueue.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::function;

/**
 * @brief TimerQueue 单元测试
 */
class TimerQueueTest : public ::testing::Test {
protected:
    TimerQueue queue;
};

// ========== 基本调度 ==========

TEST_F(TimerQueueTest, ScheduleFunction_SingleEvent)
{
    ResourceLocation loc("minecraft", "test");
    queue.scheduleFunction("minecraft:test", loc, 100);

    EXPECT_FALSE(queue.isEmpty());
    EXPECT_EQ(queue.size(), 1u);
}

TEST_F(TimerQueueTest, ScheduleFunction_MultipleEvents)
{
    queue.scheduleFunction("minecraft:a", ResourceLocation("minecraft", "a"), 100);
    queue.scheduleFunction("minecraft:b", ResourceLocation("minecraft", "b"), 200);
    queue.scheduleFunction("minecraft:c", ResourceLocation("minecraft", "c"), 300);

    EXPECT_EQ(queue.size(), 3u);
}

TEST_F(TimerQueueTest, ScheduleFunctionTag)
{
    queue.scheduleFunctionTag("#minecraft:tick", ResourceLocation("minecraft", "tick"), 100);

    EXPECT_EQ(queue.size(), 1u);

    auto due = queue.tick(100);
    ASSERT_EQ(due.size(), 1u);
    EXPECT_EQ(due[0].type, TimerQueue::EventType::FunctionTag);
    EXPECT_EQ(due[0].id, "#minecraft:tick");
}

// ========== 去重 ==========

TEST_F(TimerQueueTest, Deduplication_SameIdAndTime)
{
    ResourceLocation loc("minecraft", "test");
    queue.scheduleFunction("minecraft:test", loc, 100);
    queue.scheduleFunction("minecraft:test", loc, 100); // 重复

    EXPECT_EQ(queue.size(), 1u);
}

TEST_F(TimerQueueTest, NoDeduplication_SameIdDifferentTime)
{
    ResourceLocation loc("minecraft", "test");
    queue.scheduleFunction("minecraft:test", loc, 100);
    queue.scheduleFunction("minecraft:test", loc, 200); // 不同时间，不重复

    EXPECT_EQ(queue.size(), 2u);
}

TEST_F(TimerQueueTest, NoDeduplication_DifferentIdSameTime)
{
    queue.scheduleFunction("minecraft:a", ResourceLocation("minecraft", "a"), 100);
    queue.scheduleFunction("minecraft:b", ResourceLocation("minecraft", "b"), 100); // 不同 ID，不重复

    EXPECT_EQ(queue.size(), 2u);
}

// ========== Tick 处理 ==========

TEST_F(TimerQueueTest, Tick_NoDueEvents)
{
    queue.scheduleFunction("minecraft:test", ResourceLocation("minecraft", "test"), 100);

    auto due = queue.tick(50); // 当前 tick 50，事件在 tick 100
    EXPECT_TRUE(due.empty());
    EXPECT_EQ(queue.size(), 1u); // 事件仍在队列中
}

TEST_F(TimerQueueTest, Tick_SingleDueEvent)
{
    queue.scheduleFunction("minecraft:test", ResourceLocation("minecraft", "test"), 100);

    auto due = queue.tick(100);
    ASSERT_EQ(due.size(), 1u);
    EXPECT_EQ(due[0].id, "minecraft:test");
    EXPECT_EQ(due[0].loc, ResourceLocation("minecraft", "test"));
    EXPECT_EQ(due[0].type, TimerQueue::EventType::Function);
    EXPECT_TRUE(queue.isEmpty());
}

TEST_F(TimerQueueTest, Tick_MultipleDueEvents)
{
    queue.scheduleFunction("minecraft:a", ResourceLocation("minecraft", "a"), 50);
    queue.scheduleFunction("minecraft:b", ResourceLocation("minecraft", "b"), 100);
    queue.scheduleFunction("minecraft:c", ResourceLocation("minecraft", "c"), 150);

    auto due = queue.tick(100);
    ASSERT_EQ(due.size(), 2u);
    // 优先队列按触发时间排序，先到期的先返回
    EXPECT_EQ(due[0].id, "minecraft:a");
    EXPECT_EQ(due[1].id, "minecraft:b");

    // tick 150 时还有剩余事件
    EXPECT_EQ(queue.size(), 1u);
    auto due2 = queue.tick(150);
    ASSERT_EQ(due2.size(), 1u);
    EXPECT_EQ(due2[0].id, "minecraft:c");
}

TEST_F(TimerQueueTest, Tick_ExactTimeBoundary)
{
    queue.scheduleFunction("minecraft:test", ResourceLocation("minecraft", "test"), 100);

    // tick 99: 不到期
    auto due = queue.tick(99);
    EXPECT_TRUE(due.empty());

    // tick 100: 到期
    due = queue.tick(100);
    ASSERT_EQ(due.size(), 1u);
}

TEST_F(TimerQueueTest, Tick_FutureEventsUnaffected)
{
    queue.scheduleFunction("minecraft:a", ResourceLocation("minecraft", "a"), 100);
    queue.scheduleFunction("minecraft:b", ResourceLocation("minecraft", "b"), 200);

    auto due = queue.tick(100);
    ASSERT_EQ(due.size(), 1u);
    EXPECT_EQ(due[0].id, "minecraft:a");

    // 剩余事件仍在队列中
    EXPECT_EQ(queue.size(), 1u);
}

TEST_F(TimerQueueTest, Tick_EmptyQueue)
{
    auto due = queue.tick(100);
    EXPECT_TRUE(due.empty());
}

TEST_F(TimerQueueTest, Tick_SequentialIdOrdering)
{
    // 同一 tick 的事件按插入顺序处理
    queue.scheduleFunction("minecraft:a", ResourceLocation("minecraft", "a"), 100);
    queue.scheduleFunction("minecraft:b", ResourceLocation("minecraft", "b"), 100);
    queue.scheduleFunction("minecraft:c", ResourceLocation("minecraft", "c"), 100);

    auto due = queue.tick(100);
    ASSERT_EQ(due.size(), 3u);
    EXPECT_EQ(due[0].id, "minecraft:a");
    EXPECT_EQ(due[1].id, "minecraft:b");
    EXPECT_EQ(due[2].id, "minecraft:c");
}

// ========== 移除 ==========

TEST_F(TimerQueueTest, RemoveById)
{
    queue.scheduleFunction("minecraft:a", ResourceLocation("minecraft", "a"), 100);
    queue.scheduleFunction("minecraft:b", ResourceLocation("minecraft", "b"), 200);
    queue.scheduleFunction("minecraft:a", ResourceLocation("minecraft", "a"), 300); // 同 ID 不同时间

    i32 removed = queue.remove("minecraft:a");
    EXPECT_EQ(removed, 2);
    EXPECT_EQ(queue.size(), 1u);

    // 只剩下 minecraft:b
    auto due = queue.tick(300);
    ASSERT_EQ(due.size(), 1u);
    EXPECT_EQ(due[0].id, "minecraft:b");
}

TEST_F(TimerQueueTest, RemoveById_NotFound)
{
    queue.scheduleFunction("minecraft:a", ResourceLocation("minecraft", "a"), 100);
    i32 removed = queue.remove("minecraft:nonexistent");
    EXPECT_EQ(removed, 0);
    EXPECT_EQ(queue.size(), 1u);
}

TEST_F(TimerQueueTest, RemoveById_EmptyQueue)
{
    i32 removed = queue.remove("minecraft:test");
    EXPECT_EQ(removed, 0);
}

// ========== getEventIds ==========

TEST_F(TimerQueueTest, GetEventIds)
{
    queue.scheduleFunction("minecraft:a", ResourceLocation("minecraft", "a"), 100);
    queue.scheduleFunction("minecraft:b", ResourceLocation("minecraft", "b"), 200);
    queue.scheduleFunction("minecraft:a", ResourceLocation("minecraft", "a"), 300); // 同 ID 不同时间

    auto ids = queue.getEventIds();
    // 应该只有 2 个唯一 ID（minecraft:a 和 minecraft:b）
    EXPECT_EQ(ids.size(), 2u);

    // 验证两个 ID 都存在（顺序不确定，使用 set 比较）
    std::unordered_set<std::string> idSet(ids.begin(), ids.end());
    EXPECT_TRUE(idSet.count("minecraft:a") > 0);
    EXPECT_TRUE(idSet.count("minecraft:b") > 0);
}

// ========== isEmpty / size ==========

TEST_F(TimerQueueTest, IsEmpty_Initially)
{
    EXPECT_TRUE(queue.isEmpty());
    EXPECT_EQ(queue.size(), 0u);
}

TEST_F(TimerQueueTest, IsEmpty_AfterProcessing)
{
    queue.scheduleFunction("minecraft:test", ResourceLocation("minecraft", "test"), 100);
    EXPECT_FALSE(queue.isEmpty());

    queue.tick(100);
    EXPECT_TRUE(queue.isEmpty());
}

TEST_F(TimerQueueTest, Size_AfterClear)
{
    queue.scheduleFunction("minecraft:a", ResourceLocation("minecraft", "a"), 100);
    queue.scheduleFunction("minecraft:b", ResourceLocation("minecraft", "b"), 200);
    EXPECT_EQ(queue.size(), 2u);

    queue.clear();
    EXPECT_EQ(queue.size(), 0u);
    EXPECT_TRUE(queue.isEmpty());
}

// ========== clear ==========

TEST_F(TimerQueueTest, Clear)
{
    queue.scheduleFunction("minecraft:a", ResourceLocation("minecraft", "a"), 100);
    queue.scheduleFunction("minecraft:b", ResourceLocation("minecraft", "b"), 200);

    queue.clear();
    EXPECT_TRUE(queue.isEmpty());
    EXPECT_EQ(queue.size(), 0u);

    // 确认清空后可以重新添加
    queue.scheduleFunction("minecraft:c", ResourceLocation("minecraft", "c"), 300);
    EXPECT_EQ(queue.size(), 1u);

    auto due = queue.tick(300);
    ASSERT_EQ(due.size(), 1u);
    EXPECT_EQ(due[0].id, "minecraft:c");
}

// ========== DueEvent 字段 ==========

TEST_F(TimerQueueTest, DueEvent_FunctionType)
{
    queue.scheduleFunction("minecraft:test", ResourceLocation("minecraft", "test"), 50);

    auto due = queue.tick(50);
    ASSERT_EQ(due.size(), 1u);
    EXPECT_EQ(due[0].type, TimerQueue::EventType::Function);
    EXPECT_EQ(due[0].id, "minecraft:test");
    EXPECT_EQ(due[0].loc, ResourceLocation("minecraft", "test"));
}

TEST_F(TimerQueueTest, DueEvent_FunctionTagType)
{
    queue.scheduleFunctionTag("#minecraft:load", ResourceLocation("minecraft", "load"), 50);

    auto due = queue.tick(50);
    ASSERT_EQ(due.size(), 1u);
    EXPECT_EQ(due[0].type, TimerQueue::EventType::FunctionTag);
    EXPECT_EQ(due[0].id, "#minecraft:load");
    EXPECT_EQ(due[0].loc, ResourceLocation("minecraft", "load"));
}
