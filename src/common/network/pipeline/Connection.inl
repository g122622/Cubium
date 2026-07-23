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

// Connection<B> 模板实现。由 Connection.hpp 末尾 include。

namespace mc::network::pipeline {

template <typename B>
bool Connection<B>::isConnected() const noexcept
{
    if (m_mode == Mode::Wire) {
        return m_wireTransport != nullptr && m_wireTransport->isConnected();
    }
    return m_localTransport != nullptr && m_localTransport->isConnected();
}

template <typename B>
Result<void> Connection<B>::send(ir::IrPacket packet)
{
    if (m_mode == Mode::Local) {
        if (m_localTransport == nullptr) {
            return Error(ErrorCode::InvalidState, "Local 传输缺失", "Connection::send");
        }
        // Local 模式：直传 IrPacket，不经序列化（零拷贝）。
        return m_localTransport->send(std::move(packet));
    }

    // Wire 模式：按当前阶段选 ProtocolInfo encode。
    // TODO(Phase3): 各阶段 codec 表填充后启用；当前骨架阶段未接入真实 codec，
    //               先按阶段分发到对应 ProtocolInfo.encode。
    B buf;
    Result<void> encResult = Result<void>::ok();

    switch (m_phase) {
        case protocol::ConnectionProtocol::Handshaking:
            if (auto* p = protocol::isClientbound(m_flow) ? m_tables->handshakeCb.get() : m_tables->handshakeSb.get()) {
                encResult = p->encode(buf, std::get<ir::HandshakePacket>(packet.packet));
            }
            break;
        case protocol::ConnectionProtocol::Status:
            if (auto* p = protocol::isClientbound(m_flow) ? m_tables->statusCb.get() : m_tables->statusSb.get()) {
                encResult = p->encode(buf, std::get<ir::StatusPacket>(packet.packet));
            }
            break;
        case protocol::ConnectionProtocol::Login:
            if (auto* p = protocol::isClientbound(m_flow) ? m_tables->loginCb.get() : m_tables->loginSb.get()) {
                encResult = p->encode(buf, std::get<ir::LoginPacket>(packet.packet));
            }
            break;
        case protocol::ConnectionProtocol::Configuration:
            if (auto* p = protocol::isClientbound(m_flow) ? m_tables->configurationCb.get()
                                                          : m_tables->configurationSb.get()) {
                encResult = p->encode(buf, std::get<ir::ConfigurationPacket>(packet.packet));
            }
            break;
        case protocol::ConnectionProtocol::Play:
            if (auto* p = protocol::isClientbound(m_flow) ? m_tables->playCb.get() : m_tables->playSb.get()) {
                encResult = p->encode(buf, std::get<ir::PlayPacket>(packet.packet));
            }
            break;
    }
    if (!encResult.success()) {
        return encResult;
    }

    // TODO(Phase2): encode 后的字节经压缩/加密 pipeline handler 处理再交给 transport。
    if (m_wireTransport == nullptr) {
        return Error(ErrorCode::InvalidState, "Wire 传输缺失", "Connection::send");
    }
    return m_wireTransport->send(buf.bytes(), transport::DeliveryHint::ReliableOrdered);
}

template <typename B>
void Connection<B>::_installWireReceive()
{
    if (m_wireTransport == nullptr) {
        return;
    }
    // 注册字节到达回调：解帧后的 payload → 按 (phase,flow) decode 成阶段变体 → 包成 IrPacket → 监听器
    m_wireTransport->onMessage([this](const u8* data, usize size) { _handleWireBytes(data, size); });
}

template <typename B>
void Connection<B>::_installLocalReceive()
{
    if (m_localTransport == nullptr) {
        return;
    }
    // Local 模式：对端 send 进来的 IrPacket 直接回调监听器（零序列化）。
    m_localTransport->onPacket([this](ir::IrPacket packet) {
        if (m_listener) {
            m_listener(packet);
        }
    });
}

template <typename B>
void Connection<B>::pumpLocal()
{
    if (m_localTransport != nullptr) {
        m_localTransport->pump();
    }
}

template <typename B>
void Connection<B>::close()
{
    if (m_wireTransport != nullptr) {
        m_wireTransport->close();
    }
    if (m_localTransport != nullptr) {
        m_localTransport->close();
    }
}

template <typename B>
void Connection<B>::_handleWireBytes(const u8* data, usize size)
{
    // TODO(Phase2): 字节先经解密/解压 pipeline handler 还原成 payload。

    // TODO(Phase3): 各阶段 codec 表填充后，按 (phase, flow) decode 成阶段变体，包成 IrPacket
    //               交给监听器；terminal 包触发 setPhase 切阶段。当前骨架仅占位。
    (void)data;
    (void)size;
    if (m_listener) {
        // 骨架阶段：无真实 decode，暂不回调（Phase3 接入 codec 后补全）。
    }
}

} // namespace mc::network::pipeline
