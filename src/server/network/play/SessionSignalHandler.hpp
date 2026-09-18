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
#include "server/network/play/base/PlayHandlerBase.hpp"

namespace mc::server::net {

/**
 * @brief 连接存活信号与 batch 反馈包族
 *
 * 覆盖心跳回声（KeepAlive）、ping 通道（ServerboundPingRequest / ServerboundPong）、
 * 阶段重协商确认（ConfigurationAcknowledged）与区块批次流速反馈（ChunkBatchReceived）。
 * 这一族都不改变世界状态，只维持连接与流量控制。
 */
class SessionSignalHandler : public PlayHandlerBase {
public:
    explicit SessionSignalHandler(MinecraftServer& server)
        : PlayHandlerBase(server)
    {}

    /// ServerboundPingRequest：回 PongResponse（同 time）
    void handlePingRequestPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);

    /// ServerboundPong：common 通道 ping 回声，vanilla 不据此计算 RTT，仅记录
    void handlePongPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);

    /// KeepAlive：把心跳回包时间交给 KeepAliveManager
    void handleKeepAlivePacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);

    /// ConfigurationAcknowledged：阶段重协商确认（入站阶段切换由 ProtocolSwapHandler 完成）
    void handleConfigurationAcknowledgedPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);

    /// ChunkBatchReceived：客户端回报的批次接收速率（区块批次流速控尚未实现）
    void handleChunkBatchReceivedPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet);
};

} // namespace mc::server::net
