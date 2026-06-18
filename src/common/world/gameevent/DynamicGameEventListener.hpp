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
#include "common/world/chunk/base/SectionPos.hpp"
#include "common/world/gameevent/GameEventListener.hpp"
#include "common/world/gameevent/GameEventListenerRegistry.hpp"

#include <functional>
#include <optional>

namespace mc {

namespace server {
class ServerWorld; // 前向声明
} // namespace server

namespace gameevent {

/**
 * @brief 动态游戏事件监听器
 *
 * 包装一个位置可能随时间变化的 GameEventListener（如附着在实体上的监听器）。
 * 当实体移动到不同的区块段时，自动从旧段的注册表注销并注册到新段。
 *
 * 适用于监守者、悦灵等基于实体的监听器。方块实体（如幽匿感测体）
 * 的位置固定，不需要此类。
 *
 * 参考: net.minecraft.world.level.gameevent.DynamicGameEventListener
 */
class DynamicGameEventListener final {
public:
    /**
     * @brief 构造动态监听器
     * @param listener 被包装的监听器引用
     */
    explicit DynamicGameEventListener(GameEventListener& listener)
        : m_listener(listener)
    {}

    /**
     * @brief 获取内部监听器
     */
    [[nodiscard]] GameEventListener& getListener() { return m_listener; }
    [[nodiscard]] const GameEventListener& getListener() const { return m_listener; }

    /**
     * @brief 将监听器添加到世界（注册到当前区块段）
     * @param world 服务端世界引用
     */
    void add(server::ServerWorld& world);

    /**
     * @brief 将监听器从世界中移除（从当前区块段注销）
     * @param world 服务端世界引用
     */
    void remove(server::ServerWorld& world);

    /**
     * @brief 更新监听器的区块段注册
     *
     * 当实体位置可能发生变化时调用。如果位置发生了段间移动，
     * 自动从旧段注销并注册到新段。
     *
     * @param world 服务端世界引用
     */
    void move(server::ServerWorld& world);

private:
    /**
     * @brief 如果区块存在，对注册表执行操作
     * @param world 服务端世界引用
     * @param sectionPos 区块段位置（可为空）
     * @param action 要执行的操作
     * @param createIfMissing 如果为 true，注册监听器时创建注册表；如果为 false，仅访问已存在的注册表
     */
    static void _ifChunkExists(server::ServerWorld& world,
        const std::optional<mc::world::chunk::SectionPos>& sectionPos,
        const std::function<void(GameEventListenerRegistry&)>& action,
        bool createIfMissing = false);

    GameEventListener& m_listener;
    std::optional<mc::world::chunk::SectionPos> m_lastSection;
};

} // namespace gameevent

} // namespace mc
