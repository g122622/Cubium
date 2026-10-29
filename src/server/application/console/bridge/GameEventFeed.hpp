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
#include "server/application/console/buffer/EventRingBuffer.hpp"
#include "server/event/ServerEventBus.hpp"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace mc::server::console {

/**
 * @brief 把服务端事件总线上的游戏事件转成终端事件
 *
 * 订阅的动作发生在事件的发布线程（多数在 tick 线程，聊天在入站包处理线程），
 * 因此这里只做"取快照 → 拼文本 → 写环形缓冲 → 唤醒 UI"，不访问任何可变世界状态：
 * 事件载荷中的实体指针只用于读取名字/类型，不缓存。
 */
class GameEventFeed {
public:
    /**
     * @brief 创建事件订阅
     * @param events 事件区缓冲（须比本对象存活更久）
     * @param onEventAvailable 新事件到达时的唤醒回调（须线程安全，通常为 App::Post）
     */
    GameEventFeed(EventRingBuffer& events, std::function<void()> onEventAvailable);

    ~GameEventFeed();

    GameEventFeed(const GameEventFeed&) = delete;
    GameEventFeed& operator=(const GameEventFeed&) = delete;

private:
    /**
     * @brief 写一条事件并唤醒 UI
     */
    void _emit(TerminalEventKind kind, std::string text);

    EventRingBuffer& m_events;
    std::function<void()> m_onEventAvailable;
    std::vector<mc::server::event::ServerEventBus::HandlerId> m_subscriptions;
};

} // namespace mc::server::console
