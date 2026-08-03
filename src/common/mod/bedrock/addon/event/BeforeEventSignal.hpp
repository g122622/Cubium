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
#include <any>
#include <cstddef>
#include <functional>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace mc::mod::bedrock::addon {

/**
 * @brief 事件订阅句柄
 *
 * 用于取消beforeEvent或afterEvent的订阅。
 */
struct ScriptEventHandler {
    u64 id = 0;
    std::type_index eventType;

    ScriptEventHandler()
        : eventType(typeid(void))
    {}
    ScriptEventHandler(u64 id_, std::type_index type)
        : id(id_)
        , eventType(type)
    {}

    [[nodiscard]] bool valid() const { return id != 0; }
};

/**
 * @brief BeforeEvent信号（同步、可取消）
 *
 * 在游戏逻辑执行之前触发。脚本可以调用cancel()来阻止原始操作。
 * 所有beforeEvent处理器都会被调用（即使事件已被取消），
 * 这与基岩版行为一致。
 *
 * 使用示例：
 * @code
 * auto handle = signal.subscribe<ActorHurtBeforeEvent>(
 *     [](ActorHurtBeforeEvent& event) {
 *         if (event.damage > 100) {
 *             event.cancel();  // 阻止伤害超过100的伤害
 *         }
 *     }
 * );
 * // 取消订阅
 * signal.unsubscribe(handle);
 * @endcode
 */
class BeforeEventSignal {
public:
    using Callback = std::function<void(std::any&)>;

    BeforeEventSignal() = default;
    ~BeforeEventSignal() noexcept = default;

    // 禁止拷贝
    BeforeEventSignal(const BeforeEventSignal&) = delete;
    BeforeEventSignal& operator=(const BeforeEventSignal&) = delete;

    /**
     * @brief 订阅beforeEvent
     *
     * @param eventType 事件类型索引
     * @param callback 回调函数，接收可修改的事件数据
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
     * @brief 触发beforeEvent
     *
     * 调用所有订阅的处理器。即使事件被取消，后续处理器仍然会被调用。
     *
     * @param eventType 事件类型索引
     * @param eventData 事件数据（可修改）
     * @return 是否被取消
     */
    bool fire(std::type_index eventType, std::any& eventData);

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

private:
    struct Subscription {
        u64 id;
        Callback callback;
    };

    std::unordered_map<std::type_index, std::vector<Subscription>> m_handlers;
    u64 m_nextId = 1;
    mutable std::mutex m_mutex;
};

} // namespace mc::mod::bedrock::addon
