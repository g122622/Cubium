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
#include "common/network/pipeline/ProtocolTableSet.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/network/protocol/PacketFlow.hpp"
#include "common/network/transport/ITransport.hpp"
#include "common/network/transport/LocalTransport.hpp"

#include <functional>
#include <memory>

namespace mc::network::pipeline {

/**
 * @brief 统一连接门面（持有传输 + 当前阶段包表 + 监听器；双模式 wire/local）
 *
 * 游戏逻辑只见 Connection::send(ir::Packet) / Connection::onPacket(cb<ir::Packet>)，
 * 与 codec 完全解耦。内部按模式分流：
 * - Wire 模式（独立服/真 Java）：send → encode→bytes → pipeline[compress→encrypt] → ITransport
 *   recv → ITransport.onMessage → pipeline[decrypt→decompress] → decode→ir → cb
 * - Local 模式（集成服同进程）：send → 直传 ir::Packet 到对端 ILocalTransport
 *   recv → ILocalTransport.onPacket → cb（零序列化、零拷贝）
 *
 * terminal 包驱动阶段切换：发送/收到 terminal 包后按预设 nextPhase 切 ProtocolInfo。
 *
 * @tparam B 缓冲类型（Java 后端用 buffer::RegistryByteBuf）
 *
 * TODO(Phase2): 接入加密/压缩 pipeline handler（CompressionEncoder/Decoder、CipherEncoder/Decoder）。
 * TODO(Phase3): 接入后端提供的 ProtocolTableSet（5 阶段包表）。
 * TODO(Phase7): Local 模式接入 + 端到端跑通。
 */
template <typename B>
class Connection {
public:
    using PacketListener = std::function<void(const ir::IrPacket&)>;

    /**
     * @brief Wire 模式构造：注入 ITransport + ProtocolTableSet
     */
    Connection(std::unique_ptr<transport::ITransport> wireTransport,
        std::shared_ptr<ProtocolTableSet<B>> tables,
        protocol::PacketFlow localFlow)
        : m_wireTransport(std::move(wireTransport))
        , m_tables(std::move(tables))
        , m_flow(localFlow)
        , m_phase(protocol::ConnectionProtocol::Handshaking)
        , m_mode(Mode::Wire)
    {
        _installWireReceive();
    }

    /**
     * @brief Local 模式构造：注入 ILocalTransport + ProtocolTableSet（tables 仅用于阶段校验）
     */
    Connection(std::unique_ptr<transport::ILocalTransport> localTransport,
        std::shared_ptr<ProtocolTableSet<B>> tables,
        protocol::PacketFlow localFlow)
        : m_localTransport(std::move(localTransport))
        , m_tables(std::move(tables))
        , m_flow(localFlow)
        , m_phase(protocol::ConnectionProtocol::Handshaking)
        , m_mode(Mode::Local)
    {
        _installLocalReceive();
    }

    /**
     * @brief 发送一个 IR 包
     *
     * Wire 模式：encode 成 bytes 经 pipeline 走 ITransport。
     * Local 模式：直传 IrPacket 到对端 ILocalTransport。
     */
    [[nodiscard]] Result<void> send(ir::IrPacket packet);

    /**
     * @brief 注册包到达监听器
     */
    void onPacket(PacketListener listener) { m_listener = std::move(listener); }

    [[nodiscard]] protocol::ConnectionProtocol phase() const noexcept { return m_phase; }
    [[nodiscard]] protocol::PacketFlow flow() const noexcept { return m_flow; }
    [[nodiscard]] bool isWireMode() const noexcept { return m_mode == Mode::Wire; }
    [[nodiscard]] bool isConnected() const noexcept;

    /**
     * @brief 主动切换阶段（terminal 包处理后调用）
     */
    void setPhase(protocol::ConnectionProtocol phase) noexcept { m_phase = phase; }

    /**
     * @brief Local 模式驱动：pump 对端投递的包回调监听器
     *
     * Wire 模式由 ITransport.onMessage 异步驱动，无需调用本方法。
     */
    void pumpLocal();

    void close();

private:
    enum class Mode : u8 {
        Wire,
        Local,
    };

    void _installWireReceive();
    void _installLocalReceive();
    void _handleWireBytes(const u8* data, usize size);

    std::unique_ptr<transport::ITransport> m_wireTransport;
    std::unique_ptr<transport::ILocalTransport> m_localTransport;
    std::shared_ptr<ProtocolTableSet<B>> m_tables;
    PacketListener m_listener;
    protocol::PacketFlow m_flow;
    protocol::ConnectionProtocol m_phase;
    Mode m_mode;
};

} // namespace mc::network::pipeline

#include "common/network/pipeline/Connection.inl"
