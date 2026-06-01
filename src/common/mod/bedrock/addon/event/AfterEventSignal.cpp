#include "common/mod/bedrock/addon/event/AfterEventSignal.hpp"

#include <spdlog/spdlog.h>
#include <algorithm>

namespace mc::mod::bedrock::addon {

ScriptEventHandler AfterEventSignal::subscribe(std::type_index eventType, Callback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    u64 id = m_nextId++;
    m_handlers[eventType].push_back({id, std::move(callback)});
    return ScriptEventHandler{id, eventType};
}

bool AfterEventSignal::unsubscribe(const ScriptEventHandler& handle) {
    if (!handle.valid()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_handlers.find(handle.eventType);
    if (it == m_handlers.end()) {
        return false;
    }

    auto& subs = it->second;
    auto subIt = std::find_if(subs.begin(), subs.end(),
                              [&](const Subscription& sub) { return sub.id == handle.id; });
    if (subIt != subs.end()) {
        subs.erase(subIt);
        return true;
    }
    return false;
}

void AfterEventSignal::enqueue(std::type_index eventType, std::any eventData) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pendingEvents.push_back({eventType, std::move(eventData)});
}

void AfterEventSignal::preFlush() {
    // 准备阶段：当前实现无需特殊处理
}

bool AfterEventSignal::flush() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_pendingEvents.empty()) {
        return false;
    }

    // 交换出待处理事件，避免迭代时入队
    auto events = std::move(m_pendingEvents);
    m_pendingEvents.clear();

    for (const auto& event : events) {
        auto it = m_handlers.find(event.eventType);
        if (it == m_handlers.end()) {
            continue;
        }

        // 复制订阅列表以避免迭代时修改
        auto subs = it->second;
        for (const auto& sub : subs) {
            sub.callback(event.data);
        }
    }

    return !events.empty();
}

void AfterEventSignal::postFlush() {
    // 清理阶段：当前实现无需特殊处理
}

void AfterEventSignal::clearHandlers(std::type_index eventType) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_handlers.erase(eventType);
}

void AfterEventSignal::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_handlers.clear();
    m_pendingEvents.clear();
}

size_t AfterEventSignal::handlerCount(std::type_index eventType) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_handlers.find(eventType);
    return it != m_handlers.end() ? it->second.size() : 0;
}

size_t AfterEventSignal::pendingCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pendingEvents.size();
}

} // namespace mc::mod::bedrock::addon
