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

#include "TimerQueue.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <algorithm>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc {
namespace function {

void TimerQueue::scheduleFunction(const std::string& id, ResourceLocation loc, u64 triggerTime)
{
    scheduleInternal(id, std::move(loc), triggerTime, EventType::Function);
}

void TimerQueue::scheduleFunctionTag(const std::string& id, ResourceLocation loc, u64 triggerTime)
{
    scheduleInternal(id, std::move(loc), triggerTime, EventType::FunctionTag);
}

void TimerQueue::scheduleInternal(const std::string& id, ResourceLocation loc, u64 triggerTime, EventType type)
{
    EventKey key{id, triggerTime};

    // 检查重复：同一 id + triggerTime 不会重复添加
    if (m_scheduledKeys.count(key) > 0) {
        return;
    }

    m_scheduledKeys.insert(key);

    Event event;
    event.triggerTime = triggerTime;
    event.sequentialId = m_nextSequentialId++;
    event.id = id;
    event.type = type;
    event.loc = std::move(loc);

    m_queue.push(std::move(event));
}

i32 TimerQueue::remove(const std::string& id)
{
    // 收集要移除的事件
    std::vector<Event> remaining;
    i32 removedCount = 0;

    while (!m_queue.empty()) {
        Event event = std::move(const_cast<Event&>(m_queue.top()));
        m_queue.pop();

        if (event.id == id) {
            // 移除对应的 key
            EventKey key{event.id, event.triggerTime};
            m_scheduledKeys.erase(key);
            ++removedCount;
        } else {
            remaining.push_back(std::move(event));
        }
    }

    // 重新插入保留的事件
    for (auto& event : remaining) {
        m_queue.push(std::move(event));
    }

    return removedCount;
}

std::vector<TimerQueue::DueEvent> TimerQueue::tick(u64 currentTick)
{
    std::vector<DueEvent> dueEvents;

    while (!m_queue.empty()) {
        const Event& topEvent = m_queue.top();
        if (topEvent.triggerTime > currentTick) {
            break;
        }

        // 取出事件
        Event event = std::move(const_cast<Event&>(m_queue.top()));
        m_queue.pop();

        // 移除对应的 key
        EventKey key{event.id, event.triggerTime};
        m_scheduledKeys.erase(key);

        // 收集到期事件
        dueEvents.push_back(DueEvent{std::move(event.id), event.type, std::move(event.loc)});
    }

    return dueEvents;
}

std::vector<std::string> TimerQueue::getEventIds() const
{
    // 从 scheduled keys 中收集唯一的 ID
    std::unordered_set<std::string> idSet;
    for (const auto& key : m_scheduledKeys) {
        idSet.insert(key.id);
    }
    return std::vector<std::string>(idSet.begin(), idSet.end());
}

bool TimerQueue::isEmpty() const noexcept
{
    return m_queue.empty();
}

Size TimerQueue::size() const noexcept
{
    return m_queue.size();
}

void TimerQueue::clear()
{
    while (!m_queue.empty()) {
        m_queue.pop();
    }
    m_scheduledKeys.clear();
}

std::unique_ptr<nbt::tags::compound_list_tag> TimerQueue::serialize() const
{
    auto list = std::make_unique<nbt::tags::compound_list_tag>();

    // 从优先队列中提取所有事件并排序（优先队列的迭代器不保证顺序）
    std::vector<Event> events;
    events.reserve(m_queue.size());

    // 复制队列中的事件（priority_queue 不提供迭代器，需要逐个取出再放回）
    // 使用 const_cast 访问底层容器是 priority_queue 的常见做法，
    // 但这里采用安全方式：创建临时队列来拷贝
    auto tempQueue = m_queue;
    while (!tempQueue.empty()) {
        events.push_back(std::move(const_cast<Event&>(tempQueue.top())));
        tempQueue.pop();
    }

    // 按 triggerTime 升序排列，同 tick 按 sequentialId 升序
    std::sort(events.begin(), events.end(), [](const Event& a, const Event& b) {
        if (a.triggerTime != b.triggerTime) {
            return a.triggerTime < b.triggerTime;
        }
        return a.sequentialId < b.sequentialId;
    });

    for (const auto& event : events) {
        nbt::tags::compound_tag eventTag;

        // 事件标识符
        eventTag.put("Name", event.id);

        // 触发时间
        eventTag.put("TriggerTime", static_cast<i64>(event.triggerTime));

        // 回调信息（与 MC Java 格式兼容）
        auto callback = std::make_unique<nbt::tags::compound_tag>();
        switch (event.type) {
            case EventType::Function:
                callback->put("Type", std::string("minecraft:function"));
                break;
            case EventType::FunctionTag:
                callback->put("Type", std::string("minecraft:function_tag"));
                break;
        }
        callback->put("Name", event.loc.toString());
        eventTag.value.emplace("Callback", std::move(callback));

        list->value.push_back(std::move(eventTag));
    }

    return list;
}

void TimerQueue::deserialize(const nbt::tags::compound_list_tag& eventsList)
{
    // 清空当前队列
    clear();

    for (const auto& eventTag : eventsList.value) {
        // 读取事件标识符
        auto nameIt = eventTag.value.find("Name");
        if (nameIt == eventTag.value.end() || nameIt->second->id() != nbt::TagId::String) {
            spdlog::warn("TimerQueue::deserialize: skipping event missing Name field");
            continue;
        }
        std::string name = dynamic_cast<const nbt::tags::string_tag&>(*nameIt->second).value;

        // 读取触发时间
        auto triggerTimeIt = eventTag.value.find("TriggerTime");
        if (triggerTimeIt == eventTag.value.end() || triggerTimeIt->second->id() != nbt::TagId::Long) {
            spdlog::warn("TimerQueue::deserialize: skipping event missing TriggerTime field: {}", name);
            continue;
        }
        u64 triggerTime = static_cast<u64>(dynamic_cast<const nbt::tags::long_tag&>(*triggerTimeIt->second).value);

        // 读取回调信息
        auto callbackIt = eventTag.value.find("Callback");
        if (callbackIt == eventTag.value.end() || callbackIt->second->id() != nbt::TagId::Compound) {
            spdlog::warn("TimerQueue::deserialize: skipping event missing Callback field: {}", name);
            continue;
        }
        const auto& callback = dynamic_cast<const nbt::tags::compound_tag&>(*callbackIt->second);

        // 读取回调类型
        auto typeIt = callback.value.find("Type");
        if (typeIt == callback.value.end() || typeIt->second->id() != nbt::TagId::String) {
            spdlog::warn("TimerQueue::deserialize: skipping event missing Callback.Type field: {}", name);
            continue;
        }
        std::string typeStr = dynamic_cast<const nbt::tags::string_tag&>(*typeIt->second).value;

        // 读取回调中的函数/标签 ResourceLocation
        auto funcNameIt = callback.value.find("Name");
        if (funcNameIt == callback.value.end() || funcNameIt->second->id() != nbt::TagId::String) {
            spdlog::warn("TimerQueue::deserialize: skipping event missing Callback.Name field: {}", name);
            continue;
        }
        std::string funcName = dynamic_cast<const nbt::tags::string_tag&>(*funcNameIt->second).value;
        ResourceLocation loc = ResourceLocation::parse(funcName);

        // 根据回调类型调度事件
        if (typeStr == "minecraft:function") {
            scheduleFunction(name, std::move(loc), triggerTime);
        } else if (typeStr == "minecraft:function_tag") {
            scheduleFunctionTag(name, std::move(loc), triggerTime);
        } else {
            spdlog::warn(
                "TimerQueue::deserialize: skipping event with unknown callback type: type={}, name={}", typeStr, name);
        }
    }
}

} // namespace function
} // namespace mc
