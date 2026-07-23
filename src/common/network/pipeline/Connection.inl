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

// 出站表选择：本端发出方向。客户端(Serverbound 发) / 服务端(Clientbound 发)。
template <typename B>
protocol::ProtocolInfo<B, ir::HandshakePacket>* Connection<B>::_outboundHandshake() const
{
    return protocol::isClientbound(m_flow) ? m_tables->handshakeCb.get() : m_tables->handshakeSb.get();
}
template <typename B>
protocol::ProtocolInfo<B, ir::StatusPacket>* Connection<B>::_outboundStatus() const
{
    return protocol::isClientbound(m_flow) ? m_tables->statusCb.get() : m_tables->statusSb.get();
}
template <typename B>
protocol::ProtocolInfo<B, ir::LoginPacket>* Connection<B>::_outboundLogin() const
{
    return protocol::isClientbound(m_flow) ? m_tables->loginCb.get() : m_tables->loginSb.get();
}
template <typename B>
protocol::ProtocolInfo<B, ir::ConfigurationPacket>* Connection<B>::_outboundConfiguration() const
{
    return protocol::isClientbound(m_flow) ? m_tables->configurationCb.get() : m_tables->configurationSb.get();
}
template <typename B>
protocol::ProtocolInfo<B, ir::PlayPacket>* Connection<B>::_outboundPlay() const
{
    return protocol::isClientbound(m_flow) ? m_tables->playCb.get() : m_tables->playSb.get();
}

// 入站表选择：对端发来的方向。客户端收 Clientbound；服务端收 Serverbound。
template <typename B>
protocol::ProtocolInfo<B, ir::HandshakePacket>* Connection<B>::_inboundHandshake() const
{
    return protocol::isClientbound(m_flow) ? m_tables->handshakeSb.get() : m_tables->handshakeCb.get();
}
template <typename B>
protocol::ProtocolInfo<B, ir::StatusPacket>* Connection<B>::_inboundStatus() const
{
    return protocol::isClientbound(m_flow) ? m_tables->statusSb.get() : m_tables->statusCb.get();
}
template <typename B>
protocol::ProtocolInfo<B, ir::LoginPacket>* Connection<B>::_inboundLogin() const
{
    return protocol::isClientbound(m_flow) ? m_tables->loginSb.get() : m_tables->loginCb.get();
}
template <typename B>
protocol::ProtocolInfo<B, ir::ConfigurationPacket>* Connection<B>::_inboundConfiguration() const
{
    return protocol::isClientbound(m_flow) ? m_tables->configurationSb.get() : m_tables->configurationCb.get();
}
template <typename B>
protocol::ProtocolInfo<B, ir::PlayPacket>* Connection<B>::_inboundPlay() const
{
    return protocol::isClientbound(m_flow) ? m_tables->playSb.get() : m_tables->playCb.get();
}

template <typename B>
Result<void> Connection<B>::send(ir::IrPacket packet)
{
    if (m_mode == Mode::Local) {
        if (m_localTransport == nullptr) {
            return Error(ErrorCode::InvalidState, "Local 传输缺失", "Connection::send");
        }
        // Local 模式：直传 IrPacket，不经序列化（零拷贝）。terminal 切换由对端收包时处理。
        return m_localTransport->send(std::move(packet));
    }

    // Wire 模式：encode → 压缩 → 帧编码 → 加密 → ITransport。
    B buf;
    Result<void> encResult = Result<void>::ok();
    switch (m_phase) {
        case protocol::ConnectionProtocol::Handshaking:
            if (auto* p = _outboundHandshake()) {
                encResult = p->encode(buf, std::get<ir::HandshakePacket>(packet.packet));
            }
            break;
        case protocol::ConnectionProtocol::Status:
            if (auto* p = _outboundStatus()) {
                encResult = p->encode(buf, std::get<ir::StatusPacket>(packet.packet));
            }
            break;
        case protocol::ConnectionProtocol::Login:
            if (auto* p = _outboundLogin()) {
                encResult = p->encode(buf, std::get<ir::LoginPacket>(packet.packet));
            }
            break;
        case protocol::ConnectionProtocol::Configuration:
            if (auto* p = _outboundConfiguration()) {
                encResult = p->encode(buf, std::get<ir::ConfigurationPacket>(packet.packet));
            }
            break;
        case protocol::ConnectionProtocol::Play:
            if (auto* p = _outboundPlay()) {
                encResult = p->encode(buf, std::get<ir::PlayPacket>(packet.packet));
            }
            break;
    }
    if (!encResult.success()) {
        return encResult;
    }
    if (m_wireTransport == nullptr) {
        return Error(ErrorCode::InvalidState, "Wire 传输缺失", "Connection::send");
    }

    // packetID+payload（buf.bytes()）→ 压缩层 → 帧层 → 加密层。
    std::vector<u8> payload = buf.bytes();

    std::vector<u8> compressed;
    if (m_compressionActive && m_compressionEncoder != nullptr) {
        auto r = m_compressionEncoder->encode(payload, compressed);
        if (!r.success()) {
            return r;
        }
    } else {
        // 未启用压缩：压缩层格式仍要求 VarInt(0) 前缀？Java 中 threshold<0 时压缩层不装，
        // payload 直接进帧层。故未激活时 payload 即帧内容，不加前缀。
        compressed = payload;
    }

    std::vector<u8> framed;
    framed.reserve(compressed.size() + 5);
    VarintFraming::encodeFrame(compressed.data(), compressed.size(), framed);

    std::vector<u8> encrypted;
    auto cipherResult = m_cipherEncoder.encode(framed, encrypted);
    if (!cipherResult.success()) {
        return cipherResult;
    }

    // 发送侧 terminal 包：发送后切阶段（对齐 Java 出站 terminal 即 setupOutboundProtocol）。
    const auto swap = ProtocolSwapHandler::check(packet, m_flow);
    auto sendResult =
        m_wireTransport->send(encrypted.data(), encrypted.size(), transport::DeliveryHint::ReliableOrdered);
    if (!sendResult.success()) {
        return sendResult;
    }
    if (swap.isTerminal) {
        setPhase(swap.nextPhase);
    }
    return Result<void>::ok();
}

template <typename B>
void Connection<B>::_installWireReceive()
{
    if (m_wireTransport == nullptr) {
        return;
    }
    m_wireTransport->onBytes([this](const u8* data, usize size) { _handleWireBytes(data, size); });
}

template <typename B>
void Connection<B>::_installLocalReceive()
{
    if (m_localTransport == nullptr) {
        return;
    }
    m_localTransport->onPacket([this](ir::IrPacket packet) {
        // Local 模式收到包：terminal 切换 + 回调监听器。
        const auto swap = ProtocolSwapHandler::check(packet, m_flow);
        if (m_listener) {
            m_listener(packet);
        }
        if (swap.isTerminal) {
            setPhase(swap.nextPhase);
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
    if (data == nullptr || size == 0) {
        return;
    }
    // 入站：累积原始字节 → 解密 → 切帧 → 解压 → decode → 监听器。
    // CFB8 流式：加密字节须按到达顺序逐字节喂入解密器，故先全部累积到 m_encryptedIn
    // 再整体解密；切帧后的明文残留留在 m_plainIn，不与下次的加密字节混存。
    m_encryptedIn.insert(m_encryptedIn.end(), data, data + size);

    // 解密整段缓冲（CFB8 流式，跨包/跨帧连续）。未激活加密时直通（明文）。
    std::vector<u8> decrypted;
    auto decResult = m_cipherDecoder.decode(m_encryptedIn, decrypted);
    if (!decResult.success()) {
        // 解密失败：清缓冲断开（CFB8 状态已损坏，无法恢复）。
        m_encryptedIn.clear();
        m_plainIn.clear();
        return;
    }
    m_encryptedIn.clear();
    m_plainIn.insert(m_plainIn.end(), decrypted.begin(), decrypted.end());

    // 切帧：m_plainIn 可能含多个完整帧 + 残留。残留留待下次。
    std::vector<u8> frame;
    while (VarintFraming::tryDecodeFrame(m_plainIn, frame)) {
        std::vector<u8> decompressed;
        if (m_compressionActive && m_compressionDecoder != nullptr) {
            auto r = m_compressionDecoder->decode(frame, decompressed);
            if (!r.success()) {
                continue; // 单包解压失败：跳过该包，保留连接
            }
        } else {
            decompressed = frame;
        }
        auto dispatchResult = _decodeAndDispatch(decompressed);
        (void)dispatchResult;
    }
}

template <typename B>
Result<void> Connection<B>::_decodeAndDispatch(const std::vector<u8>& frameBytes)
{
    // 压缩层解出后的 frameBytes = packetID + payload。按当前 (phase, 入站方向) decode。
    B buf(frameBytes.data(), frameBytes.size());

    ir::IrPacket packet;
    packet.phase = m_phase;
    switch (m_phase) {
        case protocol::ConnectionProtocol::Handshaking: {
            if (auto* p = _inboundHandshake()) {
                auto r = p->decode(buf);
                if (r.success()) {
                    packet.packet = std::move(r).value();
                } else {
                    return r.error();
                }
            } else {
                return Error(ErrorCode::InvalidState, "入站握手表缺失", "Connection::_decodeAndDispatch");
            }
            break;
        }
        case protocol::ConnectionProtocol::Status: {
            if (auto* p = _inboundStatus()) {
                auto r = p->decode(buf);
                if (r.success()) {
                    packet.packet = std::move(r).value();
                } else {
                    return r.error();
                }
            } else {
                return Error(ErrorCode::InvalidState, "入站状态表缺失", "Connection::_decodeAndDispatch");
            }
            break;
        }
        case protocol::ConnectionProtocol::Login: {
            if (auto* p = _inboundLogin()) {
                auto r = p->decode(buf);
                if (r.success()) {
                    packet.packet = std::move(r).value();
                } else {
                    return r.error();
                }
            } else {
                return Error(ErrorCode::InvalidState, "入站登录表缺失", "Connection::_decodeAndDispatch");
            }
            break;
        }
        case protocol::ConnectionProtocol::Configuration: {
            if (auto* p = _inboundConfiguration()) {
                auto r = p->decode(buf);
                if (r.success()) {
                    packet.packet = std::move(r).value();
                } else {
                    return r.error();
                }
            } else {
                return Error(ErrorCode::InvalidState, "入站配置表缺失", "Connection::_decodeAndDispatch");
            }
            break;
        }
        case protocol::ConnectionProtocol::Play: {
            if (auto* p = _inboundPlay()) {
                auto r = p->decode(buf);
                if (r.success()) {
                    packet.packet = std::move(r).value();
                } else {
                    return r.error();
                }
            } else {
                return Error(ErrorCode::InvalidState, "入站游戏表缺失", "Connection::_decodeAndDispatch");
            }
            break;
        }
    }

    // terminal 切换 + 回调监听器。
    const auto swap = ProtocolSwapHandler::check(packet, m_flow);
    if (m_listener) {
        m_listener(packet);
    }
    if (swap.isTerminal) {
        setPhase(swap.nextPhase);
    }
    return Result<void>::ok();
}

} // namespace mc::network::pipeline
