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
#include "common/world/block/BlockPos.hpp"
#include "common/world/gameevent/GameEvent.hpp"
#include "common/world/gameevent/PositionSource.hpp"

#include <functional>
#include <optional>

namespace mc {

namespace server {
class ServerWorld; // 前向声明
} // namespace server

namespace gameevent {

/**
 * @brief 游戏事件监听器接口
 *
 * 可以接收游戏事件的监听器。由需要响应振动信号的方块实体
 * （如幽匿感测体、幽匿尖啸体）或实体（如监守者、悦灵）实现。
 *
 * 参考: net.minecraft.world.level.gameevent.GameEventListener
 */
class GameEventListener {
public:
    virtual ~GameEventListener() = default;

    /**
     * @brief 事件投递模式
     *
     * UNSPECIFIED: 事件到达时立即投递，不排序
     * BY_DISTANCE: 多个监听器按距离排序，最近的优先投递
     */
    enum class DeliveryMode { Unspecified, ByDistance };

    /**
     * @brief 获取监听器的位置源
     * @return 位置源，用于确定监听器在世界中的位置
     */
    [[nodiscard]] virtual PositionSource& getListenerSource() = 0;
    [[nodiscard]] virtual const PositionSource& getListenerSource() const = 0;

    /**
     * @brief 获取监听器的检测半径
     * @return 监听器能检测到事件的最大距离（格）
     */
    [[nodiscard]] virtual i32 getListenerRadius() const = 0;

    /**
     * @brief 处理接收到的游戏事件
     *
     * 当事件在监听器半径内被检测到时调用。
     *
     * @param world 服务端世界引用
     * @param event 游戏事件
     * @param context 事件上下文
     * @param pos 事件发生位置（世界坐标）
     * @return 如果事件被消费/处理返回 true，否则返回 false
     */
    virtual bool handleGameEvent(
        server::ServerWorld& world, const GameEvent& event, const GameEvent::Context& context, const Vector3d& pos) = 0;

    /**
     * @brief 获取事件投递模式
     * @return 默认为 Unspecified
     */
    [[nodiscard]] virtual DeliveryMode getDeliveryMode() const { return DeliveryMode::Unspecified; }

    /**
     * @brief 监听器提供者接口
     *
     * 方块实体实现此接口以暴露其内部的游戏事件监听器。
     * 方块实体本身不直接继承 GameEventListener，而是通过此接口
     * 间接提供监听器实例。
     *
     * 参考: net.minecraft.world.level.gameevent.GameEventListener.Provider
     */
    template <typename T>
    class Provider {
    public:
        virtual ~Provider() = default;

        /**
         * @brief 获取此提供者的游戏事件监听器
         * @return 监听器指针，如果没有监听器返回 nullptr
         */
        [[nodiscard]] virtual T* getGameEventListener() = 0;
        [[nodiscard]] virtual const T* getGameEventListener() const = 0;
    };
};

/**
 * @brief 游戏事件监听器信息
 *
 * 用于 BY_DISTANCE 投递模式时排序和延迟投递事件。
 * 按照监听器到事件源的距离排序，距离近的优先投递。
 *
 * 参考: net.minecraft.world.level.gameevent.GameEvent.ListenerInfo
 */
struct ListenerInfo {
    const GameEvent* event;
    Vector3d sourcePos;
    GameEvent::Context context;
    GameEventListener* recipient;
    f64 distanceToRecipient;

    /**
     * @brief 按距离排序（近的在前）
     */
    [[nodiscard]] bool operator<(const ListenerInfo& other) const
    {
        return distanceToRecipient < other.distanceToRecipient;
    }
};

} // namespace gameevent

} // namespace mc
