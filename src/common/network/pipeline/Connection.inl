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

#include <spdlog/spdlog.h>

namespace mc::network::pipeline {

template <typename B>
bool Connection<B>::State::isConnected() const noexcept
{
    if (m_mode == Mode::Wire) {
        return m_wireTransport != nullptr && m_wireTransport->isConnected();
    }
    return m_localTransport != nullptr && m_localTransport->isConnected();
}

// 出站表选择：本端发出方向。客户端(Serverbound 发) / 服务端(Clientbound 发)。
template <typename B>
protocol::ProtocolInfo<B, ir::HandshakePacket>* Connection<B>::State::_outboundHandshake() const
{
    return protocol::isClientbound(m_flow) ? m_tables->handshakeCb.get() : m_tables->handshakeSb.get();
}
template <typename B>
protocol::ProtocolInfo<B, ir::StatusPacket>* Connection<B>::State::_outboundStatus() const
{
    return protocol::isClientbound(m_flow) ? m_tables->statusCb.get() : m_tables->statusSb.get();
}
template <typename B>
protocol::ProtocolInfo<B, ir::LoginPacket>* Connection<B>::State::_outboundLogin() const
{
    return protocol::isClientbound(m_flow) ? m_tables->loginCb.get() : m_tables->loginSb.get();
}
template <typename B>
protocol::ProtocolInfo<B, ir::ConfigurationPacket>* Connection<B>::State::_outboundConfiguration() const
{
    return protocol::isClientbound(m_flow) ? m_tables->configurationCb.get() : m_tables->configurationSb.get();
}
template <typename B>
protocol::ProtocolInfo<B, ir::PlayPacket>* Connection<B>::State::_outboundPlay() const
{
    return protocol::isClientbound(m_flow) ? m_tables->playCb.get() : m_tables->playSb.get();
}

// 入站表选择：对端发来的方向。客户端收 Clientbound；服务端收 Serverbound。
template <typename B>
protocol::ProtocolInfo<B, ir::HandshakePacket>* Connection<B>::State::_inboundHandshake() const
{
    return protocol::isClientbound(m_flow) ? m_tables->handshakeSb.get() : m_tables->handshakeCb.get();
}
template <typename B>
protocol::ProtocolInfo<B, ir::StatusPacket>* Connection<B>::State::_inboundStatus() const
{
    return protocol::isClientbound(m_flow) ? m_tables->statusSb.get() : m_tables->statusCb.get();
}
template <typename B>
protocol::ProtocolInfo<B, ir::LoginPacket>* Connection<B>::State::_inboundLogin() const
{
    return protocol::isClientbound(m_flow) ? m_tables->loginSb.get() : m_tables->loginCb.get();
}
template <typename B>
protocol::ProtocolInfo<B, ir::ConfigurationPacket>* Connection<B>::State::_inboundConfiguration() const
{
    return protocol::isClientbound(m_flow) ? m_tables->configurationSb.get() : m_tables->configurationCb.get();
}
template <typename B>
protocol::ProtocolInfo<B, ir::PlayPacket>* Connection<B>::State::_inboundPlay() const
{
    return protocol::isClientbound(m_flow) ? m_tables->playSb.get() : m_tables->playCb.get();
}

template <typename B>
Result<std::vector<u8>> Connection<B>::State::encodeFrame(const ir::IrPacket& packet)
{
    if (packet.packet.index() != static_cast<usize>(packet.phase)) {
        return Error(ErrorCode::ProtocolError, "Packet phase does not match its variant", "Connection::encodeFrame");
    }
    B buf;
    Result<void> encResult = Result<void>::ok();
    switch (packet.phase) {
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
        return encResult.error();
    }
    if (m_wireTransport == nullptr) {
        return Error(ErrorCode::InvalidState, "Wire transport missing", "Connection::send");
    }

    // packetID+payload（buf.bytes()）→ 压缩层 → 帧层 → 加密层。
    std::vector<u8> payload = buf.takeBytes();

    std::vector<u8> compressed;
    if (m_compressionActive && m_compressionEncoder != nullptr) {
        auto r = m_compressionEncoder->encode(payload, compressed);
        if (!r.success()) {
            return r.error();
        }
    } else {
        // 压缩未启用时直接组帧，不添加压缩层长度字段。
        compressed = std::move(payload);
    }

    if (compressed.size() > VarintFraming::kMaxFramePayloadSize) {
        return Error(ErrorCode::ResourceExhausted, "Outbound frame exceeds size limit", "Connection::encodeFrame");
    }
    std::vector<u8> framed;
    framed.reserve(compressed.size() + 5);
    VarintFraming::encodeFrame(compressed.data(), compressed.size(), framed);

    return framed;
}

template <typename B>
Result<void> Connection<B>::State::send(const OutboundPacket& outbound)
{
    const auto& packet = outbound.packet();
    if (m_mode == Mode::Local) return m_localTransport->send(packet);
    if (packet.phase != m_outboundPhase.load()) {
        return Error(ErrorCode::ProtocolError, "Outbound packet does not match connection phase", "Connection::send");
    }
    const i32 threshold = m_compressionActive ? m_compressionEncoder->threshold() : -1;
    auto prepared =
        outbound.prepare(m_tables.get(), m_flow, threshold, [this, &packet] { return encodeFrame(packet); });
    if (!prepared.success()) return prepared.error();
    std::vector<u8> encrypted;
    auto encryptedResult = m_cipherEncoder.encode(*prepared.value(), encrypted);
    if (!encryptedResult.success()) return encryptedResult;
    auto result = m_wireTransport->send(encrypted.data(), encrypted.size(), transport::DeliveryHint::ReliableOrdered);
    if (!result.success()) return result;
    const auto swap = ProtocolSwapHandler::check(packet, m_flow);
    if (swap.isTerminal) setOutboundPhase(swap.nextPhase);
    return Result<void>::ok();
}

template <typename B>
void Connection<B>::State::_installWireReceive()
{
    if (m_wireTransport == nullptr) {
        return;
    }
    m_wireTransport->onBytes([weak = this->weak_from_this()](const u8* data, usize size) {
        if (auto state = weak.lock()) state->_handleWireBytes(data, size);
    });
}

template <typename B>
void Connection<B>::State::_installLocalReceive()
{
    if (m_localTransport == nullptr) {
        return;
    }
    m_localTransport->onPacket([weak = this->weak_from_this()](ir::IrPacket packet) {
        auto self = weak.lock();
        if (!self) return;
        auto* state = self.get();
        // Local 模式收到包：入站 terminal 切换 + 回调监听器。出站阶段由对端 send 侧
        // terminal 切换维护（Local send 直传 IrPacket 不经出站表，阶段仅影响解码侧 inbound）。
        const auto swap = ProtocolSwapHandler::check(packet, state->m_flow);
        if (state->m_listener) {
            state->m_listener(packet);
        }
        if (swap.isTerminal) {
            state->setInboundPhase(swap.nextPhase);
        }
    });
}

template <typename B>
void Connection<B>::State::pumpLocal()
{
    if (m_localTransport != nullptr) {
        m_localTransport->pump();
    }
}

template <typename B>
void Connection<B>::State::close()
{
    if (m_wireTransport != nullptr) {
        m_wireTransport->close();
    }
    if (m_localTransport != nullptr) {
        m_localTransport->close();
    }
}

template <typename B>
void Connection<B>::State::_handleWireBytes(const u8* data, usize size)
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
    while (true) {
        auto frameResult = VarintFraming::tryDecodeFrame(m_plainIn, frame);
        if (!frameResult.success()) {
            // 帧结构非法（长度前缀超 5 字节，或声明帧长超上限）：流已无法重新同步，
            // 且继续累积会让 m_plainIn 无上限增长（畸形长度前缀永远凑不满），
            // 故在此断开连接并清空入站缓冲。
            spdlog::warn("Connection: invalid frame from peer, closing connection: {}", frameResult.error().toString());
            m_plainIn.clear();
            m_encryptedIn.clear();
            if (m_wireTransport != nullptr) {
                m_wireTransport->close();
            }
            return;
        }
        if (!frameResult.value()) {
            break; // 数据不足，等下一批字节
        }

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
        if (!dispatchResult.success()) {
            // 未登记的 packet id、payload 越界、非法 NBT 等：按设计跳过该包并保留连接
            // （对齐 IdDispatchCodec「调用方按需跳过」的约定），但必须留痕——静默丢弃
            // 会让协议错误在服务端完全不可观测。
            spdlog::warn("Connection: dropping undecodable inbound packet: {}", dispatchResult.error().toString());
        }
    }
}

template <typename B>
Result<void> Connection<B>::State::_decodeAndDispatch(const std::vector<u8>& frameBytes)
{
    // 压缩层解出后的 frameBytes = packetID + payload。按当前 (入站阶段, 入站方向) decode。
    B buf(frameBytes.data(), frameBytes.size());

    ir::IrPacket packet;
    packet.phase = m_inboundPhase.load();
    switch (m_inboundPhase.load()) {
        case protocol::ConnectionProtocol::Handshaking: {
            if (auto* p = _inboundHandshake()) {
                auto r = p->decode(buf);
                if (r.success()) {
                    packet.packet = std::move(r).value();
                } else {
                    return r.error();
                }
            } else {
                return Error(
                    ErrorCode::InvalidState, "Inbound handshake table missing", "Connection::_decodeAndDispatch");
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
                return Error(ErrorCode::InvalidState, "Inbound status table missing", "Connection::_decodeAndDispatch");
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
                return Error(ErrorCode::InvalidState, "Inbound login table missing", "Connection::_decodeAndDispatch");
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
                return Error(
                    ErrorCode::InvalidState, "Inbound configuration table missing", "Connection::_decodeAndDispatch");
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
                return Error(ErrorCode::InvalidState, "Inbound play table missing", "Connection::_decodeAndDispatch");
            }
            break;
        }
    }

    // 回调监听器 → 入站 terminal 自动切换【入站】阶段。
    // 顺序：先回调监听器（监听器内可显式 setOutboundPhase 推进出站阶段并发后续包），
    // 再翻入站阶段（对齐 Java handleInboundTerminalPacket 仅换入站解码器）。监听器内若
    // 需按新入站阶段解码，应显式 setInboundPhase 后再读 m_inboundPhase。
    const auto swap = ProtocolSwapHandler::check(packet, m_flow);
    if (m_listener) {
        m_listener(packet);
    }
    if (swap.isTerminal) {
        setInboundPhase(swap.nextPhase);
    }
    return Result<void>::ok();
}

} // namespace mc::network::pipeline
