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
 * @file GameEventListenerRegistry.cpp
 * @brief 游戏事件监听器注册表实现
 */

#include "common/world/gameevent/GameEventListenerRegistry.hpp"

#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gameevent/GameEvent.hpp"
#include "common/world/gameevent/GameEventListener.hpp"
#include "server/world/ServerWorld.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>

namespace mc::gameevent {

// ============================================================================
// EuclideanGameEventListenerRegistry
// ============================================================================

EuclideanGameEventListenerRegistry::EuclideanGameEventListenerRegistry(
    server::ServerWorld& world, i32 sectionY, OnEmptyAction onEmpty)
    : m_world(world)
    , m_sectionY(sectionY)
    , m_onEmpty(std::move(onEmpty))
{}

bool EuclideanGameEventListenerRegistry::isEmpty() const
{
    return m_listeners.empty();
}

void EuclideanGameEventListenerRegistry::registerListener(GameEventListener& listener)
{
    if (m_processing) {
        // 遍历期间延迟添加
        m_listenersToAdd.push_back(&listener);
    } else {
        // 检查是否已存在，避免重复注册
        auto it = std::find(m_listeners.begin(), m_listeners.end(), &listener);
        if (it == m_listeners.end()) {
            m_listeners.push_back(&listener);
        }
    }
}

void EuclideanGameEventListenerRegistry::unregisterListener(GameEventListener& listener)
{
    if (m_processing) {
        // 遍历期间延迟移除
        m_listenersToRemove.push_back(&listener);
    } else {
        auto it = std::find(m_listeners.begin(), m_listeners.end(), &listener);
        if (it != m_listeners.end()) {
            m_listeners.erase(it);
        }

        // 如果注册表为空，通知上层移除
        // 注意：仅在非遍历状态下调用，避免在 visitInRangeListeners 中销毁自身
        if (m_listeners.empty() && m_onEmpty) {
            m_onEmpty(m_sectionY);
        }
    }
}

bool EuclideanGameEventListenerRegistry::visitInRangeListeners(
    const GameEvent& event, const Vector3d& pos, const GameEvent::Context& context, const ListenerVisitor& visitor)
{
    m_processing = true;
    bool foundAny = false;

    auto it = m_listeners.begin();
    auto endIt = m_listeners.end();
    while (it != endIt) {
        GameEventListener* listener = *it;

        // 处理遍历期间待移除的监听器
        auto removeIt = std::find(m_listenersToRemove.begin(), m_listenersToRemove.end(), listener);
        if (removeIt != m_listenersToRemove.end()) {
            m_listenersToRemove.erase(removeIt);
            it = m_listeners.erase(it);
            endIt = m_listeners.end();
            continue;
        }

        // 获取监听器位置并检查是否在通知半径内
        auto listenerPos = getPostableListenerPosition(pos, *listener);
        if (listenerPos.has_value()) {
            visitor(*listener, listenerPos.value());
            foundAny = true;
        }

        ++it;
    }

    m_processing = false;

    // 处理延迟添加
    if (!m_listenersToAdd.empty()) {
        for (auto* listener : m_listenersToAdd) {
            // 检查是否已存在
            auto existingIt = std::find(m_listeners.begin(), m_listeners.end(), listener);
            if (existingIt == m_listeners.end()) {
                m_listeners.push_back(listener);
            }
        }
        m_listenersToAdd.clear();
    }

    // 处理延迟移除
    if (!m_listenersToRemove.empty()) {
        for (auto* listener : m_listenersToRemove) {
            auto existingIt = std::find(m_listeners.begin(), m_listeners.end(), listener);
            if (existingIt != m_listeners.end()) {
                m_listeners.erase(existingIt);
            }
        }
        m_listenersToRemove.clear();
    }

    // 注意：不在此处调用 m_onEmpty，因为调用者可能在遍历过程中持有对此注册表的引用。
    // 空注册表的清理由调用方（如 GameEventDispatcher）在完成所有遍历后负责检查。
    // DynamicGameEventListener::_ifChunkExists 的 OnEmptyAction 回调会在
    // unregisterListener 的非遍历路径中被调用，确保安全的清理时机。

    return foundAny;
}

std::optional<Vector3d> EuclideanGameEventListenerRegistry::getPostableListenerPosition(
    const Vector3d& eventPos, GameEventListener& listener) const
{
    // 通过 PositionSource 获取监听器位置
    auto listenerPos = listener.getListenerSource().getPosition(m_world);
    if (!listenerPos.has_value()) {
        return std::nullopt;
    }

    // 计算方块级别距离（使用 BlockPos.distanceSq）
    BlockPos eventBlockPos(static_cast<i32>(std::floor(eventPos.x)),
        static_cast<i32>(std::floor(eventPos.y)),
        static_cast<i32>(std::floor(eventPos.z)));
    BlockPos listenerBlockPos(static_cast<i32>(std::floor(listenerPos->x)),
        static_cast<i32>(std::floor(listenerPos->y)),
        static_cast<i32>(std::floor(listenerPos->z)));

    f64 distSqr = static_cast<f64>(eventBlockPos.distanceSq(listenerBlockPos));
    i32 radius = listener.getListenerRadius();
    f64 radiusSqr = static_cast<f64>(radius) * static_cast<f64>(radius);

    if (distSqr > radiusSqr) {
        return std::nullopt;
    }

    return listenerPos;
}

} // namespace mc::gameevent
