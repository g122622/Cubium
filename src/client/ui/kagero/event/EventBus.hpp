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

#pragma once

#include "Event.hpp"
#include "client/ui/kagero/Types.hpp"
#include "common/core/Types.hpp"
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc::client::ui::kagero::event {

/**
 * @brief 事件总线
 *
 * 全局事件分发系统，支持：
 * - 类型安全的事件订阅
 * - 自动取消订阅（使用HandlerId）
 * - 事件优先级
 * - 事件过滤
 *
 * 使用示例：
 * @code
 * // 订阅事件
 * auto id = EventBus::instance().subscribe<ClickEvent>([](const ClickEvent& e) {
 *     // 处理点击事件
 * });
 *
 * // 发布事件
 * ClickEvent event(100, 200, 0);
 * EventBus::instance().publish(event);
 *
 * // 取消订阅
 * EventBus::instance().unsubscribe(id);
 * @endcode
 */
class EventBus {
public:
    using HandlerId = u64;

    /**
     * @brief 获取单例实例
     */
    static EventBus& instance()
    {
        static EventBus instance;
        return instance;
    }

    // 禁止拷贝和移动
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    /**
     * @brief 订阅特定类型的事件
     *
     * @tparam EventT 事件类型
     * @param handler 事件处理函数
     * @param priority 优先级（数值越大越先执行）
     * @return HandlerId 用于取消订阅
     */
    template <typename EventT>
    HandlerId subscribe(std::function<void(const EventT&)> handler, i32 priority = 0)
    {
        static_assert(std::is_base_of_v<Event, EventT>, "EventT must derive from Event");

        std::lock_guard<std::mutex> lock(m_mutex);

        HandlerId id = _nextId();
        TypeInfo typeInfo = _getTypeInfo<EventT>();

        HandlerEntry entry(
            id, priority, [handler](const Event& e) { handler(static_cast<const EventT&>(e)); }, typeInfo);

        m_handlers[typeInfo].push_back(std::move(entry));
        _sortHandlers(typeInfo);

        m_handlerToType.emplace(id, typeInfo);

        return id;
    }

    /**
     * @brief 取消订阅
     *
     * @param id 订阅时返回的HandlerId
     * @return 是否成功取消
     */
    bool unsubscribe(HandlerId id)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto typeIt = m_handlerToType.find(id);
        if (typeIt == m_handlerToType.end()) {
            return false;
        }

        TypeInfo typeInfo = typeIt->second;
        m_handlerToType.erase(typeIt);

        auto& handlers = m_handlers[typeInfo];
        auto it =
            std::find_if(handlers.begin(), handlers.end(), [id](const HandlerEntry& entry) { return entry.id == id; });

        if (it != handlers.end()) {
            handlers.erase(it);
            return true;
        }

        return false;
    }

    /**
     * @brief 发布事件
     *
     * @tparam EventT 事件类型
     * @param event 事件实例
     */
    template <typename EventT>
    void publish(const EventT& event)
    {
        static_assert(std::is_base_of_v<Event, EventT>, "EventT must derive from Event");

        std::lock_guard<std::mutex> lock(m_mutex);

        TypeInfo typeInfo = _getTypeInfo<EventT>();

        auto it = m_handlers.find(typeInfo);
        if (it == m_handlers.end()) {
            return;
        }

        // 复制处理器列表以避免迭代时修改
        auto handlers = it->second;

        for (const auto& entry : handlers) {
            if (event.isCancelled()) {
                break;
            }

            // 应用过滤器：仅应用全局过滤器或匹配事件类型的过滤器
            bool shouldHandle = true;
            for (const auto& filter : m_filters) {
                if (filter.second.eventType != std::type_index(typeid(void)) && filter.second.eventType != typeInfo) {
                    continue; // 过滤器仅应用于特定类型，且不匹配当前事件类型
                }
                if (!filter.second.filter(event)) {
                    shouldHandle = false;
                    break;
                }
            }

            if (shouldHandle) {
                entry.handler(event);
            }
        }
    }

    /**
     * @brief 添加事件过滤器（自动生成ID，全局过滤）
     *
     * @param filter 过滤函数，返回 true 继续处理，返回 false 阻止处理
     * @return HandlerId 过滤器ID，用于移除过滤器
     */
    HandlerId addFilter(EventFilter filter)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        HandlerId id = _nextId();
        m_filters.insert_or_assign(id, FilterEntry{TypeInfo(typeid(void)), std::move(filter)});
        return id;
    }

    /**
     * @brief 添加类型化事件过滤器（仅过滤指定类型的事件）
     *
     * @tparam EventT 要过滤的事件类型
     * @param filter 过滤函数
     * @return HandlerId 过滤器ID
     */
    template <typename EventT>
    HandlerId addFilterForType(EventFilter filter)
    {
        static_assert(std::is_base_of_v<Event, EventT>, "EventT must derive from Event");
        std::lock_guard<std::mutex> lock(m_mutex);
        HandlerId id = _nextId();
        m_filters.insert_or_assign(id, FilterEntry{_getTypeInfo<EventT>(), std::move(filter)});
        return id;
    }

    /**
     * @brief 添加事件过滤器（指定ID）
     *
     * @param id 过滤器ID（用于移除）
     * @param filter 过滤函数
     * @deprecated 使用 addFilter(EventFilter) 替代，该方法自动生成ID
     */
    void addFilterWithId(HandlerId id, EventFilter filter)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_filters.insert_or_assign(id, FilterEntry{TypeInfo(typeid(void)), std::move(filter)});
    }

    /**
     * @brief 移除事件过滤器
     *
     * @param id 过滤器ID
     * @return 是否成功移除
     */
    bool removeFilter(HandlerId id)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_filters.erase(id) > 0;
    }

    /**
     * @brief 清除所有处理器
     */
    void clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_handlers.clear();
        m_handlerToType.clear();
        m_filters.clear();
    }

    /**
     * @brief 获取特定类型事件的处理器数量
     */
    template <typename EventT>
    size_t handlerCount() const
    {
        TypeInfo typeInfo = _getTypeInfo<EventT>();
        auto it = m_handlers.find(typeInfo);
        return it != m_handlers.end() ? it->second.size() : 0;
    }

private:
    EventBus() = default;

    /**
     * @brief 类型信息
     */
    using TypeInfo = std::type_index;

    /**
     * @brief 获取类型信息
     */
    template <typename EventT>
    TypeInfo _getTypeInfo() const
    {
        return std::type_index(typeid(EventT));
    }

    /**
     * @brief 处理器条目
     */
    struct HandlerEntry {
        HandlerId id = 0;
        i32 priority = 0;
        std::function<void(const Event&)> handler;
        TypeInfo typeInfo; // type_index 没有默认构造函数，需要显式初始化

        HandlerEntry()
            : typeInfo(typeid(void))
        {}
        HandlerEntry(HandlerId id_, i32 prio, std::function<void(const Event&)> h, TypeInfo ti)
            : id(id_)
            , priority(prio)
            , handler(std::move(h))
            , typeInfo(ti)
        {}
    };

    /**
     * @brief 过滤器条目
     */
    struct FilterEntry {
        TypeInfo eventType = TypeInfo(typeid(void)); ///< 事件类型（typeid(void)表示全局过滤器）
        EventFilter filter;                          ///< 过滤函数
    };

    /**
     * @brief 生成下一个ID
     */
    HandlerId _nextId() { return m_nextId++; }

    /**
     * @brief 按优先级排序处理器
     */
    void _sortHandlers(TypeInfo typeInfo)
    {
        auto& handlers = m_handlers[typeInfo];
        std::stable_sort(handlers.begin(), handlers.end(), [](const HandlerEntry& a, const HandlerEntry& b) {
            return a.priority > b.priority; // 数值越大越先执行
        });
    }

    std::unordered_map<TypeInfo, std::vector<HandlerEntry>> m_handlers;
    std::unordered_map<HandlerId, TypeInfo> m_handlerToType;
    std::unordered_map<HandlerId, FilterEntry> m_filters;
    std::atomic<HandlerId> m_nextId{1};
    mutable std::mutex m_mutex;
};

/**
 * @brief 事件订阅助手类（RAII）
 *
 * 自动管理事件订阅的生命周期
 *
 * 使用示例：
 * @code
 * {
 *     EventSubscription<ClickEvent> subscription([](const ClickEvent& e) {
 *         // 处理点击
 *     });
 *     // 离开作用域时自动取消订阅
 * }
 * @endcode
 */
template <typename EventT>
class EventSubscription {
public:
    using HandlerType = std::function<void(const EventT&)>;

    explicit EventSubscription(HandlerType handler, i32 priority = 0)
        : m_id(EventBus::instance().subscribe<EventT>(std::move(handler), priority))
    {}

    ~EventSubscription()
    {
        if (m_id != 0) {
            EventBus::instance().unsubscribe(m_id);
        }
    }

    // 禁止拷贝
    EventSubscription(const EventSubscription&) = delete;
    EventSubscription& operator=(const EventSubscription&) = delete;

    // 允许移动
    EventSubscription(EventSubscription&& other) noexcept
        : m_id(other.m_id)
    {
        other.m_id = 0;
    }

    EventSubscription& operator=(EventSubscription&& other) noexcept
    {
        if (this != &other) {
            if (m_id != 0) {
                EventBus::instance().unsubscribe(m_id);
            }
            m_id = other.m_id;
            other.m_id = 0;
        }
        return *this;
    }

    /**
     * @brief 获取订阅ID
     */
    [[nodiscard]] EventBus::HandlerId id() const { return m_id; }

    /**
     * @brief 检查是否有效
     */
    [[nodiscard]] bool valid() const { return m_id != 0; }

    /**
     * @brief 手动取消订阅
     */
    void unsubscribe()
    {
        if (m_id != 0) {
            EventBus::instance().unsubscribe(m_id);
            m_id = 0;
        }
    }

private:
    EventBus::HandlerId m_id;
};

} // namespace mc::client::ui::kagero::event
