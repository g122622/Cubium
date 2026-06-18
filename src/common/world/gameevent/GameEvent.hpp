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

namespace mc {

class Entity;     // 前向声明
class BlockState; // 前向声明

namespace gameevent {

/**
 * @brief 游戏事件
 *
 * 代表游戏中发生的可被监听的事件（如方块变化、容器开合、唱片机播放等）。
 * 与 WorldEvents（世界事件/levelEvent，用于广播音效和粒子给客户端）不同，
 * GameEvent 是服务端内部事件分发机制，主要用于幽匿感测体（SculkSensor）
 * 和幽匿尖啸体（SculkShrieker）等方块检测振动信号。
 *
 * GameEvent 不会发送网络包给客户端，而是在服务端通过 GameEventDispatcher
 * 将事件分发给附近的 GameEventListener（如幽匿感测体）。
 */
class GameEvent {
public:
    /**
     * @brief 游戏事件上下文
     *
     * 携带触发事件的源实体和受影响的方块状态。
     * 两个字段均为可空，通过静态工厂方法构造。
     *
     */
    class Context {
    public:
        Context() = default;

        /**
         * @brief 构造上下文
         * @param sourceEntity 触发事件的实体（可为 nullptr）
         * @param affectedState 受影响的方块状态（可为 nullptr）
         */
        Context(const Entity* sourceEntity, const BlockState* affectedState)
            : m_sourceEntity(sourceEntity)
            , m_affectedState(affectedState)
        {}

        /**
         * @brief 从实体构造上下文
         * @param sourceEntity 触发事件的实体
         */
        static Context of(const Entity* sourceEntity) { return Context(sourceEntity, nullptr); }

        /**
         * @brief 从方块状态构造上下文
         * @param affectedState 受影响的方块状态
         */
        static Context of(const BlockState* affectedState) { return Context(nullptr, affectedState); }

        /**
         * @brief 从实体和方块状态构造上下文
         * @param sourceEntity 触发事件的实体
         * @param affectedState 受影响的方块状态
         */
        static Context of(const Entity* sourceEntity, const BlockState* affectedState)
        {
            return Context(sourceEntity, affectedState);
        }

        /** @brief 获取触发事件的实体 */
        [[nodiscard]] const Entity* sourceEntity() const noexcept { return m_sourceEntity; }

        /** @brief 获取受影响的方块状态 */
        [[nodiscard]] const BlockState* affectedState() const noexcept { return m_affectedState; }

    private:
        const Entity* m_sourceEntity = nullptr;
        const BlockState* m_affectedState = nullptr;
    };

    /**
     * @brief 构造游戏事件
     * @param id 事件标识符（如 "block_activate"）
     * @param notificationRadius 通知半径（格），事件在此范围内的监听器可收到通知
     */
    explicit GameEvent(const char* id, i32 notificationRadius = DEFAULT_NOTIFICATION_RADIUS)
        : m_id(id)
        , m_notificationRadius(notificationRadius)
    {}

    /** @brief 获取事件标识符 */
    [[nodiscard]] const char* id() const noexcept { return m_id; }

    /** @brief 获取通知半径（格） */
    [[nodiscard]] i32 notificationRadius() const noexcept { return m_notificationRadius; }

    /** @brief 默认通知半径 */
    static constexpr i32 DEFAULT_NOTIFICATION_RADIUS = 16;

private:
    const char* m_id;
    i32 m_notificationRadius;
};

} // namespace gameevent

} // namespace mc
