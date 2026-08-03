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

#include <functional>
#include <optional>
#include <vector>

namespace mc {

namespace server {
class ServerWorld; // 前向声明
} // namespace server

namespace gameevent {

/**
 * @brief 监听器访问器
 *
 * 用于在 visitInRangeListeners 中回调访问匹配的监听器。
 * 参数为监听器引用和其在世界中的位置。
 *
 */
using ListenerVisitor = std::function<void(GameEventListener& listener, const Vector3d& listenerPos)>;

/**
 * @brief 游戏事件监听器注册表接口
 *
 * 管理一个区域（通常是区块段）内的游戏事件监听器。
 * 提供注册、注销和范围查询功能。
 *
 */
class GameEventListenerRegistry {
public:
    virtual ~GameEventListenerRegistry() = default;

    /**
     * @brief 检查注册表是否为空
     * @return 如果没有监听器返回 true
     */
    [[nodiscard]] virtual bool isEmpty() const = 0;

    /**
     * @brief 注册监听器
     * @param listener 要注册的监听器
     */
    virtual void registerListener(GameEventListener& listener) = 0;

    /**
     * @brief 注销监听器
     * @param listener 要注销的监听器
     */
    virtual void unregisterListener(GameEventListener& listener) = 0;

    /**
     * @brief 访问在事件范围内的所有监听器
     *
     * 遍历注册表中的监听器，对每个在事件通知半径内的监听器
     * 调用 visitor。支持在遍历过程中安全地添加和移除监听器
     * （延迟到遍历结束后处理）。
     *
     * @param event 游戏事件
     * @param pos 事件位置（世界坐标，方块中心）
     * @param context 事件上下文
     * @param visitor 访问器回调
     * @return 是否找到了任何在范围内的监听器
     */
    virtual bool visitInRangeListeners(const GameEvent& event,
        const Vector3d& pos,
        const GameEvent::Context& context,
        const ListenerVisitor& visitor) = 0;
};

/**
 * @brief 空注册表（无操作单例）
 *
 * 对于未完全加载的区块或客户端世界返回此单例，
 * 避免不必要的注册表创建。
 *
 */
class NoopGameEventListenerRegistry final : public GameEventListenerRegistry {
public:
    [[nodiscard]] bool isEmpty() const override { return true; }
    void registerListener(GameEventListener& /*listener*/) override {}
    void unregisterListener(GameEventListener& /*listener*/) override {}
    bool visitInRangeListeners(const GameEvent& /*event*/,
        const Vector3d& /*pos*/,
        const GameEvent::Context& /*context*/,
        const ListenerVisitor& /*visitor*/) override
    {
        return false;
    }

    /**
     * @brief 获取全局空注册表实例
     */
    [[nodiscard]] static NoopGameEventListenerRegistry& instance()
    {
        static NoopGameEventListenerRegistry s_instance;
        return s_instance;
    }
};

/**
 * @brief 基于欧几里得距离的监听器注册表
 *
 * 每个区块段拥有一个注册表实例，管理该段内的所有游戏事件监听器。
 * 使用欧几里得距离判断监听器是否在事件的通知半径内。
 *
 * 支持遍历期间的安全增删操作（延迟到遍历结束后处理），
 * 防止迭代器失效。
 *
 * 当注册表为空时，通过 OnEmptyAction 回调通知上层移除该注册表，
 * 节省内存。
 *
 */
class EuclideanGameEventListenerRegistry final : public GameEventListenerRegistry {
public:
    /**
     * @brief 注册表为空时的回调
     * @param sectionY 段Y坐标
     */
    using OnEmptyAction = std::function<void(i32 sectionY)>;

    /**
     * @brief 构造注册表
     * @param world 服务端世界引用
     * @param sectionY 此注册表所属的段Y坐标
     * @param onEmpty 当注册表变为空时的回调
     */
    EuclideanGameEventListenerRegistry(server::ServerWorld& world, i32 sectionY, OnEmptyAction onEmpty);

    [[nodiscard]] bool isEmpty() const override;
    void registerListener(GameEventListener& listener) override;
    void unregisterListener(GameEventListener& listener) override;
    bool visitInRangeListeners(const GameEvent& event,
        const Vector3d& pos,
        const GameEvent::Context& context,
        const ListenerVisitor& visitor) override;

private:
    /**
     * @brief 检查监听器是否在事件的通知半径内，并返回其位置
     *
     * 通过监听器的 PositionSource 获取其位置，然后计算与事件位置的距离。
     * 使用整数坐标的方块距离。
     *
     * @param eventPos 事件位置
     * @param listener 监听器
     * @return 如果在范围内返回监听器位置，否则返回空
     */
    [[nodiscard]] std::optional<Vector3d> getPostableListenerPosition(
        const Vector3d& eventPos, GameEventListener& listener) const;

    server::ServerWorld& m_world;
    i32 m_sectionY;
    OnEmptyAction m_onEmpty;

    // 当前注册的监听器列表
    std::vector<GameEventListener*> m_listeners;

    // 遍历期间待添加的监听器（延迟处理，防止迭代器失效）
    std::vector<GameEventListener*> m_listenersToAdd;

    // 遍历期间待移除的监听器（延迟处理，防止迭代器失效）
    std::vector<GameEventListener*> m_listenersToRemove;

    // 是否正在遍历（重入保护）
    bool m_processing = false;
};

} // namespace gameevent

} // namespace mc
