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
#include "common/mod/bedrock/addon/event/BeforeEventSignal.hpp"
#include <any>
#include <cstddef>
#include <functional>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace mc::mod::bedrock::addon {

/**
 * @brief AfterEvent信号（延迟、不可取消）
 *
 * 在游戏逻辑执行之后触发。事件在tick结束时批量处理。
 * afterEvent不可取消。
 *
 * 事件处理分为三个阶段：
 * 1. preFlush() - 准备阶段
 * 2. flush() - 分发所有排队事件
 * 3. postFlush() - 清理阶段
 *
 * 使用示例：
 * @code
 * auto handle = signal.subscribe<ActorHurtAfterEvent>(
 *     [](const ActorHurtAfterEvent& event) {
 *         // 处理伤害后的逻辑
 *     }
 * );
 * // 取消订阅
 * signal.unsubscribe(handle);
 * @endcode
 */
class AfterEventSignal {
public:
    using Callback = std::function<void(const std::any&)>;

    AfterEventSignal() = default;
    ~AfterEventSignal() = default;

    // 禁止拷贝
    AfterEventSignal(const AfterEventSignal&) = delete;
    AfterEventSignal& operator=(const AfterEventSignal&) = delete;

    /**
     * @brief 订阅afterEvent
     *
     * @param eventType 事件类型索引
     * @param callback 回调函数，接收只读事件数据
     * @return 订阅句柄
     */
    ScriptEventHandler subscribe(std::type_index eventType, Callback callback);

    /**
     * @brief 取消订阅
     *
     * @param handle 订阅时返回的句柄
     * @return 是否成功取消
     */
    bool unsubscribe(const ScriptEventHandler& handle);

    /**
     * @brief 将事件入队等待处理
     *
     * @param eventType 事件类型索引
     * @param eventData 事件数据
     */
    void enqueue(std::type_index eventType, std::any eventData);

    /**
     * @brief 准备阶段（在flush之前调用）
     */
    void preFlush();

    /**
     * @brief 批量分发所有排队事件
     *
     * @return 是否有事件被处理
     */
    bool flush();

    /**
     * @brief 清理阶段（在flush之后调用）
     */
    void postFlush();

    /**
     * @brief 清除指定事件类型的所有订阅
     */
    void clearHandlers(std::type_index eventType);

    /**
     * @brief 清除所有订阅
     */
    void clear();

    /**
     * @brief 获取指定事件类型的订阅数量
     */
    [[nodiscard]] size_t handlerCount(std::type_index eventType) const;

    /**
     * @brief 获取排队事件数量
     */
    [[nodiscard]] size_t pendingCount() const;

private:
    struct Subscription {
        u64 id;
        Callback callback;
    };

    struct PendingEvent {
        std::type_index eventType;
        std::any data;
    };

    std::unordered_map<std::type_index, std::vector<Subscription>> m_handlers;
    std::vector<PendingEvent> m_pendingEvents;
    u64 m_nextId = 1;
    mutable std::mutex m_mutex;
};

} // namespace mc::mod::bedrock::addon
