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

#include "common/core/Result.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/protocol/PacketFlow.hpp"
#include "common/network/protocol/ProtocolInfo.hpp"

#include <memory>

namespace mc::network::pipeline {

/**
 * @brief 五阶段包表的集合（一个后端提供的完整协议表）
 *
 * Java 后端构建 5 阶段 × 2 流向共 10 张 ProtocolInfo；本结构集中持有，供 Connection
 * 按当前 (phase, flow) 取表编解码。各阶段 Variant 类型不同，故用模板参数 B（缓冲类型）
 * + 显式 5 个 ProtocolInfo 成员。
 *
 * @tparam B 缓冲类型（如 buffer::RegistryByteBuf）
 */
template <typename B>
struct ProtocolTableSet {
    std::unique_ptr<protocol::ProtocolInfo<B, ir::HandshakePacket>> handshakeSb;
    std::unique_ptr<protocol::ProtocolInfo<B, ir::HandshakePacket>> handshakeCb;
    std::unique_ptr<protocol::ProtocolInfo<B, ir::StatusPacket>> statusSb;
    std::unique_ptr<protocol::ProtocolInfo<B, ir::StatusPacket>> statusCb;
    std::unique_ptr<protocol::ProtocolInfo<B, ir::LoginPacket>> loginSb;
    std::unique_ptr<protocol::ProtocolInfo<B, ir::LoginPacket>> loginCb;
    std::unique_ptr<protocol::ProtocolInfo<B, ir::ConfigurationPacket>> configurationSb;
    std::unique_ptr<protocol::ProtocolInfo<B, ir::ConfigurationPacket>> configurationCb;
    std::unique_ptr<protocol::ProtocolInfo<B, ir::PlayPacket>> playSb;
    std::unique_ptr<protocol::ProtocolInfo<B, ir::PlayPacket>> playCb;
};

/**
 * @brief terminal 包判定结果（ProtocolSwapHandler 据此驱动阶段切换）
 */
struct TerminalCheck {
    bool isTerminal = false;
    protocol::ConnectionProtocol nextPhase = protocol::ConnectionProtocol::Play;
};

} // namespace mc::network::pipeline
