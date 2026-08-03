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

namespace mc::server::event {

/**
 * @brief 服务端事件基类
 *
 * 所有服务端事件都必须继承此类。
 * 事件是值类型，应该轻量且易于复制。
 */
struct ServerEvent {
    u64 timestamp;                    ///< 游戏tick
    mutable bool m_cancelled = false; ///< 取消标志（脚本可取消beforeEvent）

    ServerEvent()
        : timestamp(0)
    {}
    explicit ServerEvent(u64 tick)
        : timestamp(tick)
    {}
    virtual ~ServerEvent() = default;

    /**
     * @brief 检查事件是否已被取消
     *
     * 仅对beforeEvent有意义。取消后游戏逻辑将跳过原始操作。
     */
    [[nodiscard]] bool isCancelled() const { return m_cancelled; }

    /**
     * @brief 取消事件
     *
     * 在beforeEvent处理器中调用可阻止游戏执行原始操作。
     * 取消后，后续的beforeEvent处理器仍然会被调用（与基岩版行为一致）。
     */
    void cancel() const { m_cancelled = true; }
};

/**
 * @brief 服务端事件总线
 *
 * 线程安全的事件分发系统，支持：
 * - 类型安全的事件订阅
 * - 自动取消订阅（使用HandlerId或Subscription RAII）
 * - 事件优先级
 * - 事件过滤
 *
 * 设计考虑后续迁移其他系统到事件总线。
 *
 * 使用示例：
 * @code
 * // 订阅事件
 * auto id = ServerEventBus::instance().subscribe<BlockBreakEvent>([](const BlockBreakEvent& e) {
 *     spdlog::info("Block broken at ({}, {}, {})", e.pos.x, e.pos.y, e.pos.z);
 * });
 *
 * // 发布事件
 * BlockBreakEvent event{playerId, pos, state, tool};
 * ServerEventBus::instance().publish(event);
 *
 * // 取消订阅
 * ServerEventBus::instance().unsubscribe(id);
 *
 * // 使用RAII订阅
 * {
 *     auto subscription = ServerEventBus::instance().makeSubscription<BlockBreakEvent>(handler);
 *     // 离开作用域自动取消订阅
 * }
 * @endcode
 */
class ServerEventBus {
public:
    using HandlerId = u64;

    /**
     * @brief 获取单例实例
     */
    static ServerEventBus& instance();

    // 禁止拷贝和移动
    ServerEventBus(const ServerEventBus&) = delete;
    ServerEventBus& operator=(const ServerEventBus&) = delete;

    // ========== 订阅管理 ==========

    /**
     * @brief 订阅事件
     *
     * @tparam EventT 事件类型
     * @param handler 事件处理函数
     * @param priority 优先级（数值越大越先执行）
     * @return HandlerId 用于取消订阅
     */
    template <typename EventT>
    HandlerId subscribe(std::function<void(const EventT&)> handler, i32 priority = 0)
    {
        static_assert(std::is_base_of_v<ServerEvent, EventT>, "EventT must derive from ServerEvent");

        std::lock_guard<std::mutex> lock(m_mutex);

        HandlerId id = _nextId();
        TypeInfo typeInfo = _getTypeInfo<EventT>();

        HandlerEntry entry(
            id,
            priority,
            [handler = std::move(handler)](const ServerEvent& e) { handler(static_cast<const EventT&>(e)); },
            typeInfo);

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
     * @brief 清除特定事件类型的所有处理器
     */
    template <typename EventT>
    void clearHandlers()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        TypeInfo typeInfo = _getTypeInfo<EventT>();

        // 先移除handlerToType映射
        for (auto it = m_handlerToType.begin(); it != m_handlerToType.end();) {
            if (it->second == typeInfo) {
                it = m_handlerToType.erase(it);
            } else {
                ++it;
            }
        }

        m_handlers.erase(typeInfo);
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

    // ========== 事件发布 ==========

    /**
     * @brief 发布事件
     *
     * @tparam EventT 事件类型
     * @param event 事件实例
     */
    template <typename EventT>
    void publish(const EventT& event)
    {
        static_assert(std::is_base_of_v<ServerEvent, EventT>, "EventT must derive from ServerEvent");

        std::lock_guard<std::mutex> lock(m_mutex);

        TypeInfo typeInfo = _getTypeInfo<EventT>();

        auto it = m_handlers.find(typeInfo);
        if (it == m_handlers.end()) {
            return;
        }

        // 复制处理器列表以避免迭代时修改
        auto handlers = it->second;

        for (const auto& entry : handlers) {
            // 应用过滤器
            bool shouldHandle = true;
            for (const auto& [filterId, filter] : m_filters) {
                if (!filter(event)) {
                    shouldHandle = false;
                    break;
                }
            }

            if (shouldHandle) {
                entry.handler(event);
            }
        }
    }

    // ========== 过滤器 ==========

    /**
     * @brief 事件过滤器
     */
    using EventFilter = std::function<bool(const ServerEvent&)>;

    /**
     * @brief 添加事件过滤器
     *
     * @param filter 过滤函数，返回 true 继续处理，返回 false 阻止处理
     * @return HandlerId 过滤器ID，用于移除过滤器
     */
    HandlerId addFilter(EventFilter filter)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        HandlerId id = _nextId();
        m_filters.emplace(id, std::move(filter));
        return id;
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

    // ========== RAII订阅 ==========

    /**
     * @brief 事件订阅助手类（RAII）
     *
     * 自动管理事件订阅的生命周期。
     */
    template <typename EventT>
    class Subscription {
    public:
        using HandlerType = std::function<void(const EventT&)>;

        Subscription()
            : m_id(0)
        {}

        explicit Subscription(HandlerType handler, i32 priority = 0)
            : m_id(ServerEventBus::instance().subscribe<EventT>(std::move(handler), priority))
        {}

        ~Subscription()
        {
            if (m_id != 0) {
                ServerEventBus::instance().unsubscribe(m_id);
            }
        }

        // 禁止拷贝
        Subscription(const Subscription&) = delete;
        Subscription& operator=(const Subscription&) = delete;

        // 允许移动
        Subscription(Subscription&& other) noexcept
            : m_id(other.m_id)
        {
            other.m_id = 0;
        }

        Subscription& operator=(Subscription&& other) noexcept
        {
            if (this != &other) {
                if (m_id != 0) {
                    ServerEventBus::instance().unsubscribe(m_id);
                }
                m_id = other.m_id;
                other.m_id = 0;
            }
            return *this;
        }

        /**
         * @brief 获取订阅ID
         */
        [[nodiscard]] HandlerId id() const { return m_id; }

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
                ServerEventBus::instance().unsubscribe(m_id);
                m_id = 0;
            }
        }

    private:
        HandlerId m_id;
    };

    /**
     * @brief 创建RAII订阅
     */
    template <typename EventT>
    Subscription<EventT> makeSubscription(std::function<void(const EventT&)> handler, i32 priority = 0)
    {
        return Subscription<EventT>(std::move(handler), priority);
    }

    // ========== 统计 ==========

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

    /**
     * @brief 获取总处理器数量
     */
    size_t totalHandlerCount() const
    {
        size_t count = 0;
        for (const auto& [_, handlers] : m_handlers) {
            count += handlers.size();
        }
        return count;
    }

private:
    ServerEventBus() = default;

    using TypeInfo = std::type_index;

    template <typename EventT>
    TypeInfo _getTypeInfo() const noexcept
    {
        return std::type_index(typeid(EventT));
    }

    /**
     * @brief 处理器条目
     */
    struct HandlerEntry {
        HandlerId id = 0;
        i32 priority = 0;
        std::function<void(const ServerEvent&)> handler;
        TypeInfo typeInfo;

        HandlerEntry() noexcept
            : typeInfo(typeid(void))
        {}
        HandlerEntry(HandlerId id_, i32 prio, std::function<void(const ServerEvent&)> h, TypeInfo ti)
            : id(id_)
            , priority(prio)
            , handler(std::move(h))
            , typeInfo(ti)
        {}
    };

    /**
     * @brief 生成下一个ID
     */
    HandlerId _nextId() noexcept { return m_nextId++; }

    /**
     * @brief 按优先级排序处理器
     */
    void _sortHandlers(TypeInfo typeInfo) noexcept
    {
        auto& handlers = m_handlers[typeInfo];
        std::stable_sort(handlers.begin(), handlers.end(), [](const HandlerEntry& a, const HandlerEntry& b) {
            return a.priority > b.priority; // 数值越大越先执行
        });
    }

    std::unordered_map<TypeInfo, std::vector<HandlerEntry>> m_handlers;
    std::unordered_map<HandlerId, TypeInfo> m_handlerToType;
    std::unordered_map<HandlerId, EventFilter> m_filters;
    std::atomic<HandlerId> m_nextId{1};
    mutable std::mutex m_mutex;
};

} // namespace mc::server::event
