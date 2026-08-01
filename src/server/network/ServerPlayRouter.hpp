/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, without limitation the rights
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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/network/ir/IrPacket.hpp"

namespace mc::server::net {

class ServerPlayHandler;

/**
 * @brief 入站 Play 包分发器：校验 phase/playerId 后转交 ServerPlayHandler::route 按变体分发
 *
 * 替代旧 MinecraftServer::dispatchPacket 的 12 用例 switch + 已删除的 PacketHandler::handlePacket 的
 * 10 用例。每个连接一个 ServerPlayRouter（持 ServerPlayHandler& 单例门面 + 该连接绑定的 playerId），
 * 握手完成后经 setPlayerId 回填 playerId。
 *
 * 批7：routeInboundPlayPacket 及 13 个非纯虚 handle*Packet 已下沉至 ServerPlayHandler 门面，
 * 本类仅做 phase/playerId 守卫后调 m_playHandler.route。
 */
class ServerPlayRouter {
public:
    ServerPlayRouter(ServerPlayHandler& playHandler, PlayerId playerId, u32 sessionId)
        : m_playHandler(playHandler)
        , m_playerId(playerId)
        , m_sessionId(sessionId)
    {}

    /// 分发一个入站 Play 包
    [[nodiscard]] Result<void> handle(const mc::network::ir::IrPacket& packet);

    /// 握手完成后回填本地玩家ID（构造时 playerId 可能尚为 0）
    void setPlayerId(PlayerId playerId) noexcept { m_playerId = playerId; }

private:
    ServerPlayHandler& m_playHandler;
    PlayerId m_playerId;
    u32 m_sessionId;
};

} // namespace mc::server::net
