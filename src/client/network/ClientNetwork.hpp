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
#include "common/network/buffer/RegistryByteBuf.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/pipeline/Connection.hpp"
#include "common/network/pipeline/ProtocolTableSet.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/network/protocol/PacketFlow.hpp"
#include "common/network/transport/LocalTransport.hpp"
#include "common/network/transport/TcpTransport.hpp"

#include <array>
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc::client::net {

class ClientPlayVisitor;

/// Java 后端客户端连接实例类型
using ClientConn = mc::network::pipeline::Connection<mc::network::buffer::RegistryByteBuf>;
using ProtocolTables = mc::network::pipeline::ProtocolTableSet<mc::network::buffer::RegistryByteBuf>;

/// 客户端连接状态（供 UI/调试读取）
enum class ClientConnState : u8 {
    Disconnected,
    Connecting,
    LoggingIn, // Handshake/Login/Configuration 阶段
    Playing,
    Disconnecting
};

/// 登录完成（收到 play::Login，进入 Play 稳态）回调：携带本地玩家身份
/// Step3 时由 ClientApplication 注入，触发本地玩家实体生成 + predictor 初始化
using LoginReadyCallback =
    std::function<void(i32 playerId, const std::string& dimension, const std::array<u8, 16>& uuid)>;

/**
 * @brief 客户端网络门面：持 Connection<RegistryByteBuf>，驱动握手状态机 + 出站统一 send
 *
 * 游戏逻辑只见 connectLocal/connectTcp/send/tick/onPacket；握手由本类内部状态机驱动，
 * Play 阶段入站包委托 ClientPlayVisitor::handle，Configuration 阶段委托
 * ClientPlayVisitor::handleConfiguration。
 *
 * 客户端握手流程（离线模式，与 ServerHandshake 对端）：
 *   connectLocal/connectTcp → 发 ClientIntention(LOGIN) → 发 Hello(username) →
 *   收 LoginCompression(装压缩层) → 收 LoginFinished(存身份) → 发 LoginAcknowledged →
 *   Configuration：收 SelectKnownPacks → 回 SelectKnownPacks{core}、收 RegistryData/
 *     UpdateTags/UpdateEnabledFeatures 忽略、收 FinishConfiguration → 回 FinishConfiguration →
 *   Play：收 play::Login → 调 onLoginReady(playerId,dimension,uuid) → 稳态
 *
 * 离线模式（集成服/我方独立服）跳过 RSA：不调 setupEncryption（Connection 明文 wire passthrough）。
 * 在线模式（真 Java）：收 HelloBound → handleHelloBound → 发 Key → setupEncryption 装加密层。
 *
 * tick() pump Local 模式对端投递的包（集成服）；Wire 模式由 TcpTransport 接收线程异步驱动。
 * 重要：handler 内禁止递归 tick/pump。
 */
class ClientNetwork {
public:
    ClientNetwork();
    ~ClientNetwork();

    ClientNetwork(const ClientNetwork&) = delete;
    ClientNetwork& operator=(const ClientNetwork&) = delete;

    /**
     * @brief Local 模式连接（集成服）：注入 ServerNetwork::createLocalClientSide 取出的客户端侧 transport
     * @param serverSideTransport 服务端侧投递过来的 ILocalTransport（客户端持其对端）
     * @param username 玩家用户名（生成离线 UUID + Hello.username）
     */
    [[nodiscard]] Result<void> connectLocal(
        std::unique_ptr<mc::network::transport::ILocalTransport> serverSideTransport, const std::string& username);

    /**
     * @brief Wire 模式连接（独立服/真 Java）：TCP 连接到 host:port
     */
    [[nodiscard]] Result<void> connectTcp(const std::string& host, u16 port, const std::string& username);

    void disconnect(const std::string& reason = "Client disconnect");

    /// 出站统一发送口（Play 阶段由游戏逻辑调用，构造 ir::play::* 后 send）
    [[nodiscard]] Result<void> send(mc::network::ir::IrPacket packet);

    /// tick：pump Local 模式对端投递的包；Wire 模式由接收线程异步驱动
    void tick();

    /// 注册入站 Play/Configuration 包 visitor（Step3 注入 ClientPlayVisitor）
    void setPlayVisitor(ClientPlayVisitor* visitor) { m_visitor = visitor; }

    /// 登录完成回调（收到 play::Login 时触发）
    void onLoginReady(LoginReadyCallback cb) { m_onLoginReady = std::move(cb); }

    // === 状态/统计（DebugScreenWidget 读取）===
    [[nodiscard]] ClientConnState state() const noexcept { return m_state; }
    [[nodiscard]] bool isConnected() const noexcept;
    [[nodiscard]] bool isPlaying() const noexcept { return m_state == ClientConnState::Playing; }
    [[nodiscard]] u64 packetsSent() const noexcept { return m_packetsSent.load(); }
    [[nodiscard]] u64 packetsReceived() const noexcept { return m_packetsReceived.load(); }
    [[nodiscard]] i64 pingMs() const noexcept { return m_pingMs.load(); }

    /**
     * @brief 更新客户端到服务端的往返延迟（毫秒）。
     *
     * 由 PongResponse(cb:60) 往返计算结果写入：RTT = 收包时刻 - 出站 ping 的 time 字段。
     * 对齐 Java PingDebugMonitor.onPongReceived 的 Util.getMillis() - p.time()。
     */
    void setPingMs(i64 pingMs) noexcept { m_pingMs.store(pingMs); }

    [[nodiscard]] i32 playerId() const noexcept { return m_playerId; }
    [[nodiscard]] const std::string& username() const noexcept { return m_username; }
    [[nodiscard]] const std::array<u8, 16>& uuid() const noexcept { return m_uuid; }

private:
    /// 入站包分发：握手/Configuration 自处理，Play 委托 visitor
    void _handleInbound(const mc::network::ir::IrPacket& packet);

    // === 握手状态机 ===
    [[nodiscard]] Result<void> _sendHandshakeHello(); // 发 ClientIntention + Hello
    [[nodiscard]] Result<void> _handleLoginPacket(const mc::network::ir::LoginPacket& pkt);
    [[nodiscard]] Result<void> _handleConfigurationPacket(const mc::network::ir::ConfigurationPacket& pkt);
    [[nodiscard]] Result<void> _handlePlayPacket(const mc::network::ir::PlayPacket& pkt);

    void _setState(ClientConnState s) noexcept { m_state = s; }

    std::shared_ptr<ProtocolTables> m_tables;
    std::unique_ptr<ClientConn> m_conn;
    ClientPlayVisitor* m_visitor = nullptr; // 非拥有，由 ClientApplication 持有
    LoginReadyCallback m_onLoginReady;

    ClientConnState m_state = ClientConnState::Disconnected;
    std::string m_username;
    std::array<u8, 16> m_uuid{}; // LoginFinished 给的离线 UUID
    i32 m_playerId = 0;          // play::Login 给的本地玩家 id

    // 统计
    std::atomic<u64> m_packetsSent{0};
    std::atomic<u64> m_packetsReceived{0};
    std::atomic<i64> m_pingMs{0};

    // 握手中间态
    bool m_loginFinishedReceived = false;
    bool m_configurationFinished = false;
};

} // namespace mc::client::net
