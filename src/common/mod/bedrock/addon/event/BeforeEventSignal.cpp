#include "common/mod/bedrock/addon/event/BeforeEventSignal.hpp"

#include <spdlog/spdlog.h>
#include <algorithm>

namespace mc::mod::bedrock::addon {

ScriptEventHandler BeforeEventSignal::subscribe(std::type_index eventType, Callback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    u64 id = m_nextId++;
    m_handlers[eventType].push_back({id, std::move(callback)});
    return ScriptEventHandler{id, eventType};
}

bool BeforeEventSignal::unsubscribe(const ScriptEventHandler& handle) {
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

bool BeforeEventSignal::fire(std::type_index eventType, std::any& eventData) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_handlers.find(eventType);
    if (it == m_handlers.end()) {
        return false;
    }

    // 复制订阅列表以避免迭代时修改
    auto subs = it->second;
    for (const auto& sub : subs) {
        sub.callback(eventData);
        // 所有beforeEvent处理器都会被调用，即使事件已被取消（与基岩版行为一致）
    }

    // 取消状态由调用者通过事件数据本身检查
    return false;
}

void BeforeEventSignal::clearHandlers(std::type_index eventType) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_handlers.erase(eventType);
}

void BeforeEventSignal::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_handlers.clear();
}

size_t BeforeEventSignal::handlerCount(std::type_index eventType) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_handlers.find(eventType);
    return it != m_handlers.end() ? it->second.size() : 0;
}

} // namespace mc::mod::bedrock::addon
