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
#include "common/util/math/Vector3.hpp"
#include "common/world/gameevent/GameEvent.hpp"
#include "common/world/gameevent/GameEventListener.hpp"

#include <vector>

namespace mc {

namespace server {
class ServerWorld; // 前向声明
} // namespace server

namespace gameevent {

/**
 * @brief 游戏事件分发器
 *
 * 负责将游戏事件分发给附近的 GameEventListener。当事件被触发时，
 * 分发器根据事件的通知半径确定受影响的区块段范围，然后遍历
 * 这些段中的注册表，将事件投递给范围内的监听器。
 *
 * 对于 DeliveryMode::BY_DISTANCE 的监听器，事件会先收集到队列中，
 * 按距离排序后再投递，确保最近的监听器优先接收事件。
 *
 */
class GameEventDispatcher {
public:
    /**
     * @brief 构造分发器
     * @param world 服务端世界引用
     */
    explicit GameEventDispatcher(server::ServerWorld& world);

    /**
     * @brief 发布游戏事件
     *
     * 根据事件的通知半径确定受影响的区块段范围，
     * 遍历所有受影响段中的监听器注册表，将事件投递给范围内的监听器。
     *
     * DeliveryMode::Unspecified 的监听器立即接收事件；
     * DeliveryMode::ByDistance 的监听器先收集到队列中，
     * 按距离排序后依次投递。
     *
     * @param event 游戏事件
     * @param pos 事件位置（世界坐标）
     * @param context 事件上下文
     */
    void post(const GameEvent& event, const Vector3d& pos, const GameEvent::Context& context);

private:
    /**
     * @brief 处理按距离排序的事件队列
     * @param queue 待处理的事件监听器信息列表
     */
    void _handleGameEventMessagesInQueue(std::vector<ListenerInfo>& queue);

    server::ServerWorld& m_world;
};

} // namespace gameevent

} // namespace mc
