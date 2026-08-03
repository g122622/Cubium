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
#include "common/network/backend/java/handshake/JavaLoginHandshaker.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/configuration/ConfigurationPackets.hpp"
#include "common/network/ir/packets/handshake/HandshakePackets.hpp"
#include "common/network/ir/packets/login/LoginPackets.hpp"
#include "common/network/ir/packets/status/StatusPackets.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "server/network/ServerNetwork.hpp"

#include <array>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace mc::server::net {

/**
 * @brief 服务器列表 ping 状态信息（由应用层从设置/玩家管理器填充）
 *
 * 对齐 MC Java 1.21.11 ServerStatus：description(纯文本 motd)、players(max/online/sample)、
 * version(name/protocol)。favicon/enforcesSecureChat 在离线默认下省略（对齐 Java Codec
 * 的"省略而非 null"规则）。
 */
struct StatusInfo {
    std::string motd;        ///< description.text
    std::string versionName; ///< "1.21.11"
    i32 protocolVersion;     ///< 774
    i32 maxPlayers;          ///< players.max
    i32 onlinePlayers;       ///< players.online
    bool onlineMode;         ///< 是否在线模式（影响 enforcesSecureChat，当前省略）
};

/**
 * @brief 服务端登录握手 + 配置阶段状态机（每连接一个实例）
 *
 * 由 ServerClientConnection 的 IR 监听器驱动：收到任意阶段包时先调 handleInbound，
 * 返回 true 表示握手范围内已消费（handshake/login/configuration），false 表示已进入
 * Play 阶段、应交给 ServerPlayRouter。
 *
 * 流程（离线模式，我方互通必达）：
 *   收 ClientIntention(intent=LOGIN) → 阶段切 Login
 *   收 Hello(username) → 离线 UUID → 发 LoginCompression(集成服 threshold=-1) → 发 LoginFinished
 *   收 LoginAcknowledged → 阶段切 Configuration → 发 SelectKnownPacks
 *   收 SelectKnownPacks(C→S 回命中) → 发 RegistryData×N → 发 UpdateTags →
 *     发 UpdateEnabledFeatures → 发 FinishConfiguration
 *   收 FinishConfiguration(C→S) → 阶段切 Play → 触发 onPlayerReady(username,offlineUuid)
 *
 * 在线模式（真 Java，尽力）：收 Hello 后发 buildHelloBound → 收 Key → handleKey →
 *   setupEncryption → 发 LoginCompression+LoginFinished（其余同离线）。
 *
 * 玩家实体创建不在本类内——onPlayerReady 回调由 IntegratedServer/StandaloneServer
 * 注入，执行 addPlayer/setupInitialPlayerState/createPlayerEntity 等序列（迁自旧
 * handleLoginRequestPacket），并回填 playerId 给 ServerPlayRouter。
 */
class ServerHandshakeStateMachine {
public:
    /// 握手完成（进入 Play）回调：username + 离线 UUID，由应用层创建玩家并返回 playerId
    using PlayerReadyCallback = std::function<void(const std::string& username, const std::array<u8, 16>& offlineUuid)>;

    /**
     * @param conn 所属连接
     * @param isOfflineMode true=离线模式（跳过 RSA）；false=在线模式（真 Java，尽力）
     * @param compressionThreshold 压缩阈值；集成服传 -1 禁用，独立服传正阈值
     */
    ServerHandshakeStateMachine(ServerClientConnection& conn, bool isOfflineMode, i32 compressionThreshold);

    /**
     * @brief 处理一个入站 IR 包
     * @return true=握手范围内已消费；false=Play 包（交 ServerPlayRouter）
     */
    [[nodiscard]] Result<bool> handleInbound(const mc::network::ir::IrPacket& packet);

    void onPlayerReady(PlayerReadyCallback cb) { m_onReady = std::move(cb); }

    /// 注入服务器状态信息提供者（Status 阶段 StatusRequest 时调用，构造 StatusResponse JSON）
    using StatusProvider = std::function<StatusInfo()>;
    void onStatusRequest(StatusProvider cb) { m_statusProvider = std::move(cb); }

    [[nodiscard]] const std::string& username() const noexcept { return m_username; }
    [[nodiscard]] bool playReady() const noexcept { return m_playReady; }

    /**
     * @brief 取客户端在 Configuration 阶段上报的 ClientInformation（C→S id=0）
     *
     * 客户端未上报（连接早于此能力前）返回 nullopt。含 language/viewDistance/
     * chatVisibility/chatColors/mainHand/particleStatus 等设置，供玩家初始化与按客户端
     * 视距收敛区块发送使用（对齐 Java ServerPlayer#clientInformation）。
     */
    [[nodiscard]] const std::optional<mc::network::ir::configuration::ClientInformation>&
    clientInformation() const noexcept
    {
        return m_clientInformation;
    }

private:
    ServerClientConnection& m_conn;
    bool m_isOfflineMode;
    i32 m_compressionThreshold;

    std::string m_username;
    std::array<u8, 16> m_offlineUuid{};
    bool m_loginFinishedSent = false;
    bool m_configurationStarted = false;
    bool m_playReady = false;

    // 在线模式加密握手中间态
    std::vector<u8> m_serverPrivateKeyDer;
    std::vector<u8> m_verifyToken;

    PlayerReadyCallback m_onReady;
    StatusProvider m_statusProvider;
    bool m_hasRequestedStatus = false; ///< StatusRequest 单次守卫（二次请求断连，对齐 Java）

    // 客户端 Configuration 阶段上报的设置（C→S ClientInformation）。未上报时为 nullopt。
    std::optional<mc::network::ir::configuration::ClientInformation> m_clientInformation;

    // === 各阶段处理 ===
    [[nodiscard]] Result<void> _handleHandshake(const mc::network::ir::handshake::ClientIntention& intention);
    [[nodiscard]] Result<void> _handleLoginPacket(const mc::network::ir::LoginPacket& pkt);
    [[nodiscard]] Result<void> _handleConfigurationPacket(const mc::network::ir::ConfigurationPacket& pkt);
    /// Status 阶段：处理 StatusRequest（回 StatusResponse）/ PingRequest（回 Pong 后断连）
    [[nodiscard]] Result<void> _handleStatusPacket(const mc::network::ir::StatusPacket& pkt);
    /// 构造对齐 Java ServerStatus 的状态 JSON 字符串
    [[nodiscard]] static std::string _buildStatusJson(const StatusInfo& info);

    /// Login 阶段：收到 Hello 后的离线模式分支
    [[nodiscard]] Result<void> _advanceAfterHelloOffline();
    /// Login 阶段：收到 Hello 后的在线模式分支（发 HelloBound）
    [[nodiscard]] Result<void> _advanceAfterHelloOnline();
    /// 收到 Key 后装加密层 + 发 LoginCompression+LoginFinished
    [[nodiscard]] Result<void> _handleKey(const mc::network::ir::login::Key& key);

    /// Configuration：发 SelectKnownPacks
    [[nodiscard]] Result<void> _beginConfiguration();
    /// Configuration：收到 C→S SelectKnownPacks 后推送剩余配置数据
    [[nodiscard]] Result<void> _pushConfigurationData();

    [[nodiscard]] Result<void> _send(mc::network::ir::IrPacket packet);
};

} // namespace mc::server::net
