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
#include <functional>
#include <memory>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace mc::server::event {
struct ServerEvent;
}

namespace mc::mod::bedrock::addon {

class BeforeEventSignal;
class AfterEventSignal;

/**
 * @brief 脚本事件总线
 *
 * 桥接游戏事件到脚本回调。
 * 负责管理beforeEvent和afterEvent信号，
 * 在每tick中驱动事件分发。
 *
 * 生命周期：
 * - initialize(): 在服务器启动时调用
 * - tick(): 每tick调用，处理beforeEvent（同步）和flush afterEvent（延迟）
 * - shutdown(): 在服务器关闭时调用
 */
class ScriptEventBus {
public:
    ScriptEventBus();
    ~ScriptEventBus();

    // 禁止拷贝
    ScriptEventBus(const ScriptEventBus&) = delete;
    ScriptEventBus& operator=(const ScriptEventBus&) = delete;

    // 允许移动
    ScriptEventBus(ScriptEventBus&& other) noexcept;
    ScriptEventBus& operator=(ScriptEventBus&& other) noexcept;

    /**
     * @brief 初始化事件总线
     */
    void initialize();

    /**
     * @brief 关闭事件总线，清除所有订阅
     */
    void shutdown();

    /**
     * @brief 每tick调用
     *
     * 处理beforeEvent（同步、可取消）和flush afterEvent（延迟批量处理）。
     */
    void tick();

    /**
     * @brief 获取全局beforeEvent信号
     *
     * beforeEvent是同步的、可取消的。在游戏逻辑执行之前调用。
     * 脚本可以调用cancel()来阻止原始操作。
     */
    BeforeEventSignal& beforeEvents() { return *m_beforeEvents; }

    /**
     * @brief 获取全局afterEvent信号
     *
     * afterEvent是延迟的、不可取消的。在游戏逻辑执行之后，
     * 于每tick结束时批量处理。
     */
    AfterEventSignal& afterEvents() { return *m_afterEvents; }

    /**
     * @brief 将游戏事件入队到afterEvent队列
     *
     * 游戏逻辑执行完毕后调用此方法将事件数据入队，
     * 在下一个tick时批量分发给脚本。
     *
     * @param eventType 事件类型索引
     * @param eventData 事件数据（std::any包装）
     */
    void enqueueAfterEvent(std::type_index eventType, std::any eventData);

    /**
     * @brief 分发beforeEvent
     *
     * 在游戏逻辑执行之前调用。如果任何脚本处理器取消了事件，
     * 游戏逻辑应跳过原始操作。
     *
     * @param eventType 事件类型索引
     * @param eventData 事件数据（可修改）
     * @return 是否被取消（true表示应跳过原始操作）
     */
    bool dispatchBeforeEvent(std::type_index eventType, std::any& eventData);

    /**
     * @brief 检查是否有afterEvent待处理
     */
    [[nodiscard]] bool hasPendingAfterEvents() const;

private:
    std::unique_ptr<BeforeEventSignal> m_beforeEvents;
    std::unique_ptr<AfterEventSignal> m_afterEvents;
    bool m_initialized = false;
};

} // namespace mc::mod::bedrock::addon
