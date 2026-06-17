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

    // 检查重复：同一 id + triggerTime 不会重复添加（与 MC Java 一致）
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

} // namespace function
} // namespace mc
