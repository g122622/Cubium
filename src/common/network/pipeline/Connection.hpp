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
#include "common/network/crypto/Crypt.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/pipeline/CipherHandlers.hpp"
#include "common/network/pipeline/CompressionHandlers.hpp"
#include "common/network/pipeline/ProtocolSwapHandler.hpp"
#include "common/network/pipeline/ProtocolTableSet.hpp"
#include "common/network/pipeline/VarintFraming.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/network/protocol/PacketFlow.hpp"
#include "common/network/transport/ITransport.hpp"
#include "common/network/transport/LocalTransport.hpp"

#include <array>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace mc::network::pipeline {

/**
 * @brief 统一连接门面（持有传输 + 流水线 handler + 当前阶段包表 + 监听器；双模式 wire/local）
 *
 * 游戏逻辑只见 Connection::send(ir::IrPacket) / Connection::onPacket(cb<ir::IrPacket>)，
 * 与 codec 完全解耦。内部按模式分流：
 *
 * Wire 模式（独立服/真 Java），出站：
 *   ir → ProtocolInfo.encode → [packetID+payload]
 *      → CompressionEncoder → [VarInt(数据长度)+data]   （压缩层，threshold<0 跳过）
 *      → VarintFraming        → [VarInt(帧长)+前述]      （帧层）
 *      → CipherEncoder        → AES-CFB8 密文            （加密层，离线模式跳过）
 *      → ITransport.send      → TCP 原始字节
 * 入站反向：ITransport.onBytes → CipherDecoder → VarintFraming(切帧) → CompressionDecoder
 *   → ProtocolInfo.decode → ir → 监听器 → ProtocolSwapHandler(terminal 切阶段)
 *
 * Local 模式（集成服同进程）：send → 直传 ir::Packet 到对端 ILocalTransport
 *   recv → ILocalTransport.onPacket → cb（零序列化、零拷贝，不经任何流水线 handler）
 *
 * 阶段切换（对齐 MC Java Connection 的 setupInboundProtocol/setupOutboundProtocol）：
 * 入站、出站协议表分两个字段独立维护。terminal 包驱动自动切换——
 *   出站：send(terminal) 后 setOutboundPhase(next)（对齐 Java handleOutboundTerminalPacket）；
 *   入站：recv(terminal) 后 setInboundPhase(next)（对齐 Java handleInboundTerminalPacket）。
 * 部分 forward 切换（如服务端收 LoginAcknowledged 后要按 Configuration 出站发 SelectKnownPacks，
 * 但出站尚未由任何 terminal 自动翻）由监听器显式调 setOutboundPhase 补齐，对齐 Java
 * listener 内 setupOutboundProtocol 的显式调用。压缩/加密在登录握手后由
 * setupCompression/setupEncryption 装入。
 *
 * @tparam B 缓冲类型（Java 后端用 buffer::RegistryByteBuf）
 */
template <typename B>
class Connection {
public:
    using PacketListener = std::function<void(const ir::IrPacket&)>;

    /**
     * @brief Wire 模式构造：注入 ITransport + ProtocolTableSet
     *
     * @param localFlow 本端流向（客户端=Serverbound 发出，服务端=Clientbound 发出）
     */
    Connection(std::unique_ptr<transport::ITransport> wireTransport,
        std::shared_ptr<ProtocolTableSet<B>> tables,
        protocol::PacketFlow localFlow)
        : m_wireTransport(std::move(wireTransport))
        , m_tables(std::move(tables))
        , m_flow(localFlow)
        , m_inboundPhase(protocol::ConnectionProtocol::Handshaking)
        , m_outboundPhase(protocol::ConnectionProtocol::Handshaking)
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
        , m_inboundPhase(protocol::ConnectionProtocol::Handshaking)
        , m_outboundPhase(protocol::ConnectionProtocol::Handshaking)
        , m_mode(Mode::Local)
    {
        _installLocalReceive();
    }

    /**
     * @brief 发送一个 IR 包
     *
     * Wire 模式：encode → 压缩 → 帧编码 → 加密 → ITransport。
     * Local 模式：直传 IrPacket 到对端 ILocalTransport。
     */
    [[nodiscard]] Result<void> send(ir::IrPacket packet);

    /**
     * @brief 注册包到达监听器
     */
    void onPacket(PacketListener listener) { m_listener = std::move(listener); }

    [[nodiscard]] protocol::PacketFlow flow() const noexcept { return m_flow; }
    [[nodiscard]] bool isWireMode() const noexcept { return m_mode == Mode::Wire; }
    [[nodiscard]] bool isLocalMode() const noexcept { return m_mode == Mode::Local; }
    [[nodiscard]] bool isConnected() const noexcept;

    /// 入站阶段（对端发来方向的解码表）。disconnect 发对端可解的包时用此。
    [[nodiscard]] protocol::ConnectionProtocol phase() const noexcept { return m_inboundPhase; }
    [[nodiscard]] protocol::ConnectionProtocol inboundPhase() const noexcept { return m_inboundPhase; }
    /// 出站阶段（本端发出方向的编码表）。
    [[nodiscard]] protocol::ConnectionProtocol outboundPhase() const noexcept { return m_outboundPhase; }

    /**
     * @brief 主动切换阶段。语义为切换【入站】阶段（供 disconnect 等按对端可解阶段发包）。
     * 出站阶段切换用 setOutboundPhase。
     */
    void setPhase(protocol::ConnectionProtocol phase) noexcept { m_inboundPhase = phase; }
    void setInboundPhase(protocol::ConnectionProtocol phase) noexcept { m_inboundPhase = phase; }
    void setOutboundPhase(protocol::ConnectionProtocol phase) noexcept { m_outboundPhase = phase; }

    /**
     * @brief 装入压缩层（收到 LoginCompression 后调用）
     *
     * threshold < 0 表示禁用压缩（移除压缩层）。Wire 模式专用；Local 模式无压缩。
     */
    void setupCompression(i32 threshold)
    {
        if (threshold < 0) {
            m_compressionActive = false;
            m_compressionEncoder.reset();
            m_compressionDecoder.reset();
        } else {
            m_compressionActive = true;
            m_compressionEncoder = std::make_unique<CompressionEncoder>(threshold);
            m_compressionDecoder = std::make_unique<CompressionDecoder>(threshold);
        }
    }

    /**
     * @brief 装入加密层（登录握手双方确认共享密钥后调用）
     *
     * 离线模式默认不调本方法（明文 wire）。真 Java 在线互通时握手后装入。
     */
    [[nodiscard]] Result<void> setupEncryption(const std::array<u8, crypto::kSharedSecretBytes>& sharedSecret)
    {
        auto enc = m_cipherEncoder.init(sharedSecret);
        if (!enc.success()) {
            return enc;
        }
        auto dec = m_cipherDecoder.init(sharedSecret);
        if (!dec.success()) {
            return dec;
        }
        return Result<void>::ok();
    }

    /**
     * @brief Local 模式驱动：pump 对端投递的包回调监听器
     *
     * Wire 模式由 ITransport.onBytes 异步驱动，无需调用本方法。
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
    [[nodiscard]] Result<void> _decodeAndDispatch(const std::vector<u8>& frameBytes);

    // 按当前 (phase, flow) 取出站 ProtocolInfo（本端发出方向的表）。
    // 客户端发出 = Serverbound 表；服务端发出 = Clientbound 表。
    [[nodiscard]] protocol::ProtocolInfo<B, ir::HandshakePacket>* _outboundHandshake() const;
    [[nodiscard]] protocol::ProtocolInfo<B, ir::StatusPacket>* _outboundStatus() const;
    [[nodiscard]] protocol::ProtocolInfo<B, ir::LoginPacket>* _outboundLogin() const;
    [[nodiscard]] protocol::ProtocolInfo<B, ir::ConfigurationPacket>* _outboundConfiguration() const;
    [[nodiscard]] protocol::ProtocolInfo<B, ir::PlayPacket>* _outboundPlay() const;

    // 按当前 (phase, flow) 取入站 ProtocolInfo（对端发来的方向的表）。
    [[nodiscard]] protocol::ProtocolInfo<B, ir::HandshakePacket>* _inboundHandshake() const;
    [[nodiscard]] protocol::ProtocolInfo<B, ir::StatusPacket>* _inboundStatus() const;
    [[nodiscard]] protocol::ProtocolInfo<B, ir::LoginPacket>* _inboundLogin() const;
    [[nodiscard]] protocol::ProtocolInfo<B, ir::ConfigurationPacket>* _inboundConfiguration() const;
    [[nodiscard]] protocol::ProtocolInfo<B, ir::PlayPacket>* _inboundPlay() const;

    std::unique_ptr<transport::ITransport> m_wireTransport;
    std::unique_ptr<transport::ILocalTransport> m_localTransport;
    std::shared_ptr<ProtocolTableSet<B>> m_tables;
    PacketListener m_listener;
    protocol::PacketFlow m_flow;
    // 入站/出站阶段分离（对齐 MC Java setupInboundProtocol/setupOutboundProtocol）：
    // 入站阶段驱动解码表选择，出站阶段驱动编码表选择。terminal 包分别触发各自的自动切换，
    // forward 切换（出站需领先入站的场景）由监听器显式调 setOutboundPhase 补齐。
    protocol::ConnectionProtocol m_inboundPhase;
    protocol::ConnectionProtocol m_outboundPhase;
    Mode m_mode;

    // 流水线 handler（仅 Wire 模式用）
    CipherEncoder m_cipherEncoder;
    CipherDecoder m_cipherDecoder;
    std::unique_ptr<CompressionEncoder> m_compressionEncoder;
    std::unique_ptr<CompressionDecoder> m_compressionDecoder;
    bool m_compressionActive = false;

    // 入站缓冲：
    // - m_encryptedIn：从 ITransport 收到的原始（可能加密）字节，喂给 CipherDecoder。
    // - m_plainIn：解密后的明文字节，喂给 VarintFraming 切帧。
    // 两缓冲分离：CFB8 流式状态要求加密字节按到达顺序逐字节喂入，而切帧后的明文残留
    // 不能与新到的加密字节混存（否则解密器二次处理明文导致数据损坏）。
    std::vector<u8> m_encryptedIn;
    std::vector<u8> m_plainIn;
};

} // namespace mc::network::pipeline

#include "common/core/Types.hpp"
#include "common/network/protocol/ProtocolInfo.hpp"
#include "common/network/pipeline/Connection.inl"
