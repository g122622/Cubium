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

// ========== 序列化/反序列化 ==========

TEST_F(TimerQueueTest, Serialize_EmptyQueue)
{
    auto serialized = queue.serialize();
    ASSERT_NE(serialized, nullptr);
    EXPECT_TRUE(serialized->value.empty());
}

TEST_F(TimerQueueTest, Serialize_SingleFunctionEvent)
{
    queue.scheduleFunction("minecraft:test", ResourceLocation("minecraft", "test"), 100);

    auto serialized = queue.serialize();
    ASSERT_NE(serialized, nullptr);
    ASSERT_EQ(serialized->value.size(), 1u);

    const auto& eventTag = serialized->value[0];
    auto nameIt = eventTag.value.find("Name");
    ASSERT_NE(nameIt, eventTag.value.end());
    ASSERT_EQ(nameIt->second->id(), nbt::TagId::String);
    EXPECT_EQ(dynamic_cast<const nbt::tags::string_tag&>(*nameIt->second).value, "minecraft:test");

    auto triggerTimeIt = eventTag.value.find("TriggerTime");
    ASSERT_NE(triggerTimeIt, eventTag.value.end());
    ASSERT_EQ(triggerTimeIt->second->id(), nbt::TagId::Long);
    EXPECT_EQ(dynamic_cast<const nbt::tags::long_tag&>(*triggerTimeIt->second).value, 100);

    auto callbackIt = eventTag.value.find("Callback");
    ASSERT_NE(callbackIt, eventTag.value.end());
    ASSERT_EQ(callbackIt->second->id(), nbt::TagId::Compound);
    const auto& callback = dynamic_cast<const nbt::tags::compound_tag&>(*callbackIt->second);

    auto typeIt = callback.value.find("Type");
    ASSERT_NE(typeIt, callback.value.end());
    ASSERT_EQ(typeIt->second->id(), nbt::TagId::String);
    EXPECT_EQ(dynamic_cast<const nbt::tags::string_tag&>(*typeIt->second).value, "minecraft:function");

    auto funcNameIt = callback.value.find("Name");
    ASSERT_NE(funcNameIt, callback.value.end());
    ASSERT_EQ(funcNameIt->second->id(), nbt::TagId::String);
    EXPECT_EQ(dynamic_cast<const nbt::tags::string_tag&>(*funcNameIt->second).value, "minecraft:test");
}

TEST_F(TimerQueueTest, Serialize_FunctionTagEvent)
{
    queue.scheduleFunctionTag("#minecraft:tick", ResourceLocation("minecraft", "tick"), 200);

    auto serialized = queue.serialize();
    ASSERT_NE(serialized, nullptr);
    ASSERT_EQ(serialized->value.size(), 1u);

    const auto& eventTag = serialized->value[0];
    auto callbackIt = eventTag.value.find("Callback");
    ASSERT_NE(callbackIt, eventTag.value.end());
    const auto& callback = dynamic_cast<const nbt::tags::compound_tag&>(*callbackIt->second);

    auto typeIt = callback.value.find("Type");
    ASSERT_NE(typeIt, callback.value.end());
    EXPECT_EQ(dynamic_cast<const nbt::tags::string_tag&>(*typeIt->second).value, "minecraft:function_tag");
}

TEST_F(TimerQueueTest, Serialize_MultipleEventsSorted)
{
    // 按非顺序时间添加事件
    queue.scheduleFunction("minecraft:c", ResourceLocation("minecraft", "c"), 300);
    queue.scheduleFunction("minecraft:a", ResourceLocation("minecraft", "a"), 100);
    queue.scheduleFunction("minecraft:b", ResourceLocation("minecraft", "b"), 200);

    auto serialized = queue.serialize();
    ASSERT_NE(serialized, nullptr);
    ASSERT_EQ(serialized->value.size(), 3u);

    // 序列化后按 triggerTime 升序排列
    const auto& event0 = serialized->value[0];
    auto name0 = event0.value.find("Name");
    ASSERT_NE(name0, event0.value.end());
    EXPECT_EQ(dynamic_cast<const nbt::tags::string_tag&>(*name0->second).value, "minecraft:a");

    const auto& event1 = serialized->value[1];
    auto name1 = event1.value.find("Name");
    ASSERT_NE(name1, event1.value.end());
    EXPECT_EQ(dynamic_cast<const nbt::tags::string_tag&>(*name1->second).value, "minecraft:b");

    const auto& event2 = serialized->value[2];
    auto name2 = event2.value.find("Name");
    ASSERT_NE(name2, event2.value.end());
    EXPECT_EQ(dynamic_cast<const nbt::tags::string_tag&>(*name2->second).value, "minecraft:c");
}

TEST_F(TimerQueueTest, Deserialize_EmptyList)
{
    nbt::tags::compound_list_tag emptyList;
    queue.scheduleFunction("minecraft:test", ResourceLocation("minecraft", "test"), 100);
    EXPECT_EQ(queue.size(), 1u);

    queue.deserialize(emptyList);
    EXPECT_TRUE(queue.isEmpty());
    EXPECT_EQ(queue.size(), 0u);
}

TEST_F(TimerQueueTest, Deserialize_SingleFunctionEvent)
{
    nbt::tags::compound_list_tag list;
    nbt::tags::compound_tag eventTag;
    eventTag.put("Name", std::string("minecraft:test"));
    eventTag.put("TriggerTime", static_cast<i64>(100));

    auto callback = std::make_unique<nbt::tags::compound_tag>();
    callback->put("Type", std::string("minecraft:function"));
    callback->put("Name", std::string("minecraft:test"));
    eventTag.value.emplace("Callback", std::move(callback));

    list.value.push_back(std::move(eventTag));

    queue.deserialize(list);
    EXPECT_EQ(queue.size(), 1u);

    auto due = queue.tick(100);
    ASSERT_EQ(due.size(), 1u);
    EXPECT_EQ(due[0].id, "minecraft:test");
    EXPECT_EQ(due[0].type, TimerQueue::EventType::Function);
    EXPECT_EQ(due[0].loc, ResourceLocation("minecraft", "test"));
}

TEST_F(TimerQueueTest, Deserialize_FunctionTagEvent)
{
    nbt::tags::compound_list_tag list;
    nbt::tags::compound_tag eventTag;
    eventTag.put("Name", std::string("#minecraft:tick"));
    eventTag.put("TriggerTime", static_cast<i64>(50));

    auto callback = std::make_unique<nbt::tags::compound_tag>();
    callback->put("Type", std::string("minecraft:function_tag"));
    callback->put("Name", std::string("minecraft:tick"));
    eventTag.value.emplace("Callback", std::move(callback));

    list.value.push_back(std::move(eventTag));

    queue.deserialize(list);
    EXPECT_EQ(queue.size(), 1u);

    auto due = queue.tick(50);
    ASSERT_EQ(due.size(), 1u);
    EXPECT_EQ(due[0].type, TimerQueue::EventType::FunctionTag);
    EXPECT_EQ(due[0].id, "#minecraft:tick");
}

TEST_F(TimerQueueTest, Deserialize_MultipleEvents)
{
    nbt::tags::compound_list_tag list;

    // 事件 1：minecraft:a at tick 100
    {
        nbt::tags::compound_tag eventTag;
        eventTag.put("Name", std::string("minecraft:a"));
        eventTag.put("TriggerTime", static_cast<i64>(100));
        auto callback = std::make_unique<nbt::tags::compound_tag>();
        callback->put("Type", std::string("minecraft:function"));
        callback->put("Name", std::string("minecraft:a"));
        eventTag.value.emplace("Callback", std::move(callback));
        list.value.push_back(std::move(eventTag));
    }

    // 事件 2：minecraft:b at tick 200
    {
        nbt::tags::compound_tag eventTag;
        eventTag.put("Name", std::string("minecraft:b"));
        eventTag.put("TriggerTime", static_cast<i64>(200));
        auto callback = std::make_unique<nbt::tags::compound_tag>();
        callback->put("Type", std::string("minecraft:function"));
        callback->put("Name", std::string("minecraft:b"));
        eventTag.value.emplace("Callback", std::move(callback));
        list.value.push_back(std::move(eventTag));
    }

    queue.deserialize(list);
    EXPECT_EQ(queue.size(), 2u);

    auto due1 = queue.tick(100);
    ASSERT_EQ(due1.size(), 1u);
    EXPECT_EQ(due1[0].id, "minecraft:a");

    auto due2 = queue.tick(200);
    ASSERT_EQ(due2.size(), 1u);
    EXPECT_EQ(due2[0].id, "minecraft:b");
}

TEST_F(TimerQueueTest, Deserialize_SkipsInvalidEvents)
{
    nbt::tags::compound_list_tag list;

    // 缺少 Name 的事件
    {
        nbt::tags::compound_tag eventTag;
        eventTag.put("TriggerTime", static_cast<i64>(100));
        auto callback = std::make_unique<nbt::tags::compound_tag>();
        callback->put("Type", std::string("minecraft:function"));
        callback->put("Name", std::string("minecraft:test"));
        eventTag.value.emplace("Callback", std::move(callback));
        list.value.push_back(std::move(eventTag));
    }

    // 缺少 Callback 的事件
    {
        nbt::tags::compound_tag eventTag;
        eventTag.put("Name", std::string("minecraft:test"));
        eventTag.put("TriggerTime", static_cast<i64>(100));
        list.value.push_back(std::move(eventTag));
    }

    // 未知回调类型的事件
    {
        nbt::tags::compound_tag eventTag;
        eventTag.put("Name", std::string("minecraft:unknown"));
        eventTag.put("TriggerTime", static_cast<i64>(100));
        auto callback = std::make_unique<nbt::tags::compound_tag>();
        callback->put("Type", std::string("minecraft:unknown_type"));
        callback->put("Name", std::string("minecraft:unknown"));
        eventTag.value.emplace("Callback", std::move(callback));
        list.value.push_back(std::move(eventTag));
    }

    // 有效事件
    {
        nbt::tags::compound_tag eventTag;
        eventTag.put("Name", std::string("minecraft:valid"));
        eventTag.put("TriggerTime", static_cast<i64>(50));
        auto callback = std::make_unique<nbt::tags::compound_tag>();
        callback->put("Type", std::string("minecraft:function"));
        callback->put("Name", std::string("minecraft:valid"));
        eventTag.value.emplace("Callback", std::move(callback));
        list.value.push_back(std::move(eventTag));
    }

    queue.deserialize(list);
    // 只有最后一个有效事件应该被加载
    EXPECT_EQ(queue.size(), 1u);

    auto due = queue.tick(50);
    ASSERT_EQ(due.size(), 1u);
    EXPECT_EQ(due[0].id, "minecraft:valid");
}

TEST_F(TimerQueueTest, Deserialize_ClearsExistingEvents)
{
    // 先添加一些事件
    queue.scheduleFunction("minecraft:existing", ResourceLocation("minecraft", "existing"), 100);
    EXPECT_EQ(queue.size(), 1u);

    // 反序列化新事件列表
    nbt::tags::compound_list_tag list;
    nbt::tags::compound_tag eventTag;
    eventTag.put("Name", std::string("minecraft:new"));
    eventTag.put("TriggerTime", static_cast<i64>(200));
    auto callback = std::make_unique<nbt::tags::compound_tag>();
    callback->put("Type", std::string("minecraft:function"));
    callback->put("Name", std::string("minecraft:new"));
    eventTag.value.emplace("Callback", std::move(callback));
    list.value.push_back(std::move(eventTag));

    queue.deserialize(list);
    EXPECT_EQ(queue.size(), 1u);

    // 原有事件应被清除
    auto due = queue.tick(100);
    EXPECT_TRUE(due.empty());

    // 新事件应正常工作
    due = queue.tick(200);
    ASSERT_EQ(due.size(), 1u);
    EXPECT_EQ(due[0].id, "minecraft:new");
}

TEST_F(TimerQueueTest, RoundTrip_SerializeDeserialize)
{
    // 添加多种类型的事件
    queue.scheduleFunction("minecraft:func1", ResourceLocation("minecraft", "func1"), 100);
    queue.scheduleFunctionTag("#minecraft:tag1", ResourceLocation("minecraft", "tag1"), 200);
    queue.scheduleFunction("custom:func2", ResourceLocation("custom", "func2"), 300);

    // 序列化
    auto serialized = queue.serialize();
    ASSERT_NE(serialized, nullptr);
    ASSERT_EQ(serialized->value.size(), 3u);

    // 反序列化到新队列
    TimerQueue queue2;
    queue2.deserialize(*serialized);
    EXPECT_EQ(queue2.size(), 3u);

    // 验证事件按正确顺序触发
    auto due1 = queue2.tick(100);
    ASSERT_EQ(due1.size(), 1u);
    EXPECT_EQ(due1[0].id, "minecraft:func1");
    EXPECT_EQ(due1[0].type, TimerQueue::EventType::Function);
    EXPECT_EQ(due1[0].loc, ResourceLocation("minecraft", "func1"));

    auto due2 = queue2.tick(200);
    ASSERT_EQ(due2.size(), 1u);
    EXPECT_EQ(due2[0].id, "#minecraft:tag1");
    EXPECT_EQ(due2[0].type, TimerQueue::EventType::FunctionTag);
    EXPECT_EQ(due2[0].loc, ResourceLocation("minecraft", "tag1"));

    auto due3 = queue2.tick(300);
    ASSERT_EQ(due3.size(), 1u);
    EXPECT_EQ(due3[0].id, "custom:func2");
    EXPECT_EQ(due3[0].type, TimerQueue::EventType::Function);
    EXPECT_EQ(due3[0].loc, ResourceLocation("custom", "func2"));
}

TEST_F(TimerQueueTest, RoundTrip_DeduplicationPreserved)
{
    // 添加重复事件（相同 id + triggerTime）
    ResourceLocation loc("minecraft", "test");
    queue.scheduleFunction("minecraft:test", loc, 100);
    queue.scheduleFunction("minecraft:test", loc, 100); // 重复，应该被去重

    EXPECT_EQ(queue.size(), 1u);

    // 序列化 -> 反序列化
    auto serialized = queue.serialize();
    TimerQueue queue2;
    queue2.deserialize(*serialized);

    EXPECT_EQ(queue2.size(), 1u);

    auto due = queue2.tick(100);
    ASSERT_EQ(due.size(), 1u);
}
