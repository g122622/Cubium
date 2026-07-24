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

namespace mc::server {
class MinecraftServer;
}

namespace mc::server::net {

/**
 * @brief 入站 Play 包分发器：std::visit over ir::PlayPacket，按分支调 MinecraftServer 既有处理逻辑
 *
 * 替代旧 MinecraftServer::dispatchPacket 的 12 用例 switch + 死 PacketHandler::handlePacket 的
 * 10 用例。每个分支把 ir::play::* 字段转换为既有处理函数参数后调用；处理体保留，只替换分发。
 *
 * 本类在 Step1 仅以骨架存在（变体分发 + 各分支 TODO 占位），Step3 原子切换时填充各分支的
 * 处理调用（含从死 PacketHandler 迁入的 MoveVehicle/UseEntity/SteerBoat/EntityAction/PlayerInput
 * 五个独占处理体 + VehicleMove 纠正发送）。
 */
class ServerPlayRouter {
public:
    ServerPlayRouter(MinecraftServer& server, PlayerId playerId, u32 sessionId)
        : m_server(server)
        , m_playerId(playerId)
        , m_sessionId(sessionId)
    {}

    /// 分发一个入站 Play 包
    [[nodiscard]] Result<void> handle(const mc::network::ir::IrPacket& packet);

    /// 握手完成后回填本地玩家ID（构造时 playerId 可能尚为 0）
    void setPlayerId(PlayerId playerId) noexcept { m_playerId = playerId; }

private:
    MinecraftServer& m_server;
    PlayerId m_playerId;
    u32 m_sessionId;
};

} // namespace mc::server::net
