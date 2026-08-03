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

/**
 * @file GameEventDispatcher.cpp
 * @brief 游戏事件分发器实现
 */

#include "common/world/gameevent/GameEventDispatcher.hpp"

#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/gameevent/GameEvent.hpp"
#include "common/world/gameevent/GameEventListener.hpp"
#include "common/world/gameevent/GameEventListenerRegistry.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace mc::gameevent {

// ============================================================================
// 常量：区块段坐标转换
// ============================================================================

/**
 * @brief 将方块坐标转换为区块段坐标
 */
static i32 blockToSectionCoord(i32 blockCoord)
{
    return blockCoord >> 4; // blockCoord / 16
}

// ============================================================================
// GameEventDispatcher
// ============================================================================

GameEventDispatcher::GameEventDispatcher(server::ServerWorld& world)
    : m_world(world)
{}

void GameEventDispatcher::post(const GameEvent& event, const Vector3d& pos, const GameEvent::Context& context)
{
    const i32 notificationRadius = event.notificationRadius();

    // 计算受影响的区块段坐标范围
    const BlockPos blockPos(
        static_cast<i32>(std::floor(pos.x)), static_cast<i32>(std::floor(pos.y)), static_cast<i32>(std::floor(pos.z)));

    const i32 minSecX = blockToSectionCoord(blockPos.x - notificationRadius);
    const i32 minSecY = blockToSectionCoord(blockPos.y - notificationRadius);
    const i32 minSecZ = blockToSectionCoord(blockPos.z - notificationRadius);
    const i32 maxSecX = blockToSectionCoord(blockPos.x + notificationRadius);
    const i32 maxSecY = blockToSectionCoord(blockPos.y + notificationRadius);
    const i32 maxSecZ = blockToSectionCoord(blockPos.z + notificationRadius);

    // BY_DISTANCE 模式的延迟队列
    std::vector<ListenerInfo> distanceQueue;

    // 访问器：根据投递模式决定是立即投递还是延迟排序
    auto visitor = [&distanceQueue, &event, &pos, &context, this](
                       GameEventListener& listener, const Vector3d& listenerPos) {
        if (listener.getDeliveryMode() == GameEventListener::DeliveryMode::ByDistance) {
            // BY_DISTANCE: 加入队列，稍后按距离排序
            f64 distSqr = pos.distanceSquared(listenerPos);
            distanceQueue.push_back(ListenerInfo{&event, pos, context, &listener, distSqr});
        } else {
            // UNSPECIFIED: 立即投递
            listener.handleGameEvent(m_world, event, context, pos);
        }
    };

    // 遍历受影响的所有区块段
    for (i32 secX = minSecX; secX <= maxSecX; ++secX) {
        for (i32 secZ = minSecZ; secZ <= maxSecZ; ++secZ) {
            // 获取该区块段所在的区块
            ChunkData* chunk = m_world.chunkManager()->tryToGetChunkInMem(secX, secZ);
            if (chunk == nullptr) {
                continue;
            }

            for (i32 secY = minSecY; secY <= maxSecY; ++secY) {
                // 获取该段的监听器注册表
                GameEventListenerRegistry* registry = chunk->getGameEventListenerRegistry(secY);
                if (registry == nullptr || registry->isEmpty()) {
                    continue;
                }

                // 访问范围内的监听器
                registry->visitInRangeListeners(event, pos, context, visitor);
            }
        }
    }

    // 处理 BY_DISTANCE 队列
    if (!distanceQueue.empty()) {
        _handleGameEventMessagesInQueue(distanceQueue);
    }
}

void GameEventDispatcher::_handleGameEventMessagesInQueue(std::vector<ListenerInfo>& queue)
{
    // 按距离排序（近的优先）
    std::sort(queue.begin(), queue.end());

    // 依次投递
    for (auto& info : queue) {
        if (info.event != nullptr && info.recipient != nullptr) {
            info.recipient->handleGameEvent(m_world, *info.event, info.context, info.sourcePos);
        }
    }
}

} // namespace mc::gameevent
