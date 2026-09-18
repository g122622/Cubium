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
#include "common/network/ir/IrPacket.hpp"
#include "server/network/play/BlockActionHandler.hpp"
#include "server/network/play/ChatHandler.hpp"
#include "server/network/play/EntityActionHandler.hpp"
#include "server/network/play/MovementHandler.hpp"
#include "server/network/play/PlayerStateHandler.hpp"
#include "server/network/play/SessionSignalHandler.hpp"
#include "server/network/play/base/PlayHandlerBase.hpp"

namespace mc::server {
class MinecraftServer;
} // namespace mc::server

namespace mc::server::net {

/**
 * @brief Play 包处理聚合门面
 *
 * 只做两件事：
 *  - `route`：24 路 `std::holds_alternative` 分发表，把包转给对应包族处理器；4 个分支
 *    回调 `MinecraftServer` 的纯虚（handleHotbarSelect / handleContainerClick /
 *    handleCloseContainer / SetCreativeModeSlot），保留子类 override 的多态分发。
 *  - `updateEntityTrackingForPlayer`：登录序列与维度切换的对外入口，转调 MovementHandler。
 *
 * 各包族的实现体在 `MovementHandler` / `BlockActionHandler` / `EntityActionHandler` /
 * `ChatHandler` / `PlayerStateHandler` / `SessionSignalHandler` 中。分发表**刻意集中在本文件
 * 一份**而不下沉到各处理器：否则每个包要依次问遍 6 个处理器，"某变体无人认领" 也无法集中
 * 发现（现有 else 分支的告警会被静默化）。代价是新增 C→S 包要改两处（route 分支 + 处理器），
 * 详见 play/README.md。
 */
class ServerPlayHandler : public PlayHandlerBase {
public:
    explicit ServerPlayHandler(MinecraftServer& server);

    /// 路由入站 Play IR 包到对应包族处理器（ClientSession::handleInbound 唯一入口）。
    void route(PlayerId playerId, const mc::network::ir::IrPacket& packet);

    /// 刷新指定玩家的实体追踪范围（移动/传送确认/维度切换/登录序列调用）。
    void updateEntityTrackingForPlayer(PlayerId playerId, f64 x, f64 y, f64 z);

private:
    MovementHandler m_movement;
    BlockActionHandler m_blockAction;
    EntityActionHandler m_entityAction;
    ChatHandler m_chat;
    PlayerStateHandler m_playerState;
    SessionSignalHandler m_sessionSignal;
};

} // namespace mc::server::net
