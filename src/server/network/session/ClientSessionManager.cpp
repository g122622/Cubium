/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permitted persons to whom the Software is
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

#include "server/network/session/ClientSessionManager.hpp"

#include "common/core/Types.hpp"
#include "common/network/backend/java/JavaBackend.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/util/UuidUtils.hpp"
#include "server/application/MinecraftServer.hpp"
#include "server/network/base/ServerClientConnection.hpp"
#include "server/network/handshake/LoginFlow.hpp"
#include "server/network/handshake/ServerHandshake.hpp"
#include "server/network/session/ClientSession.hpp"
#include <array>
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <spdlog/spdlog.h>

namespace mc::server::net {

void ClientSessionManager::onClientConnect(ServerClientConnection& conn)
{
    const u32 sessionId = conn.sessionId();
    spdlog::info("{}: remote client connected: sessionId={}", m_logPrefix, sessionId);

    // 建会话簿记：握手状态机（在线模式按 server.properties online-mode 决定 + 压缩阈值）
    // + Play 路由器（playerId 占位 0，在 onPlayerReady 握手完成后回填）。sessionId 隔离各连接的
    // 握手/路由器实例。
    // 批7：Play 路由器改持 ServerPlayHandler& 单例门面（routeInboundPlayPacket 整簇下沉）。
    // 在线模式默认 false（离线基线保持），置 true 时握手走 RSA+AES 加密链路（真 Java 互通）。
    const bool isOfflineMode = !m_server.settings().onlineMode.get();
    auto session = std::make_unique<ClientSession>(
        conn, isOfflineMode, m_compressionThreshold, m_server.playHandler(), /*playerId=*/0, sessionId);

    // 握手完成回调：进入 Play 时创建玩家实体并回填 playerId。
    session->handshake().onPlayerReady(
        [this, sessionId](const std::string& username, const std::array<u8, 16>& offlineUuid) {
            onPlayerReady(sessionId, username, offlineUuid);
        });

    // Status（服务器列表 ping）信息提供者：两子类均读基类 m_settings，此处统一取值。
    session->handshake().onStatusRequest([this]() -> StatusInfo {
        auto& s = m_server.settings();
        return StatusInfo{s.motd.get(),
            std::string("1.21.11"),
            mc::network::backend::java::kJavaProtocolVersion,
            s.maxPlayers.get(),
            static_cast<i32>(m_server.playerManager().playerCount()),
            s.onlineMode.get()};
    });

    // 装配 Wire 入站派发分支（主线程 drainInbound 调用）：会话自行完成
    // 「握手状态机 → phase/playerId 守卫 → Play 处理器」的整条派发链。
    ClientSession* sessionRaw = session.get();
    conn.setInboundHandler([this, sessionRaw](const mc::network::ir::IrPacket& packet) {
        auto result = sessionRaw->handleInbound(packet);
        if (!result.success()) {
            spdlog::error("{}: inbound dispatch failed: {}", m_logPrefix, result.error().toString());
        }
    });

    // 接收线程仅入队，不跑游戏逻辑（跨线程安全）。
    ServerClientConnection* connPtr = &conn;
    conn.onPacket([connPtr](const mc::network::ir::IrPacket& packet) { connPtr->enqueueInbound(packet); });

    {
        std::lock_guard<std::mutex> lock(m_sessionsMutex);
        m_sessions[sessionId] = std::move(session);
    }
}

void ClientSessionManager::onPlayerReady(
    u32 sessionId, const std::string& username, const std::array<u8, 16>& offlineUuid)
{
    // 查 session 取连接；session 在 onClientConnect 已登记。
    ServerClientConnection* conn = nullptr;
    ClientSession* session = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_sessionsMutex);
        auto it = m_sessions.find(sessionId);
        if (it == m_sessions.end()) {
            spdlog::warn("{}: onPlayerReady for unknown sessionId={}", m_logPrefix, sessionId);
            return;
        }
        session = it->second.get();
        conn = session->connection();
    }
    if (conn == nullptr) {
        return;
    }

    // 远程玩家世界参数由 provider 注入（StandaloneServer 取 m_settings，IntegratedServer 取 m_params）。
    const SessionWorldParams wp = m_worldParamsProvider();
    auto creation =
        m_server.loginFlow().createPlayerForConnection(*conn, username, offlineUuid, wp.hardcore, wp.seed, wp.isFlat);
    if (!creation.success) {
        spdlog::warn("{}: failed to create remote player entity for '{}'", m_logPrefix, username);
        return;
    }
    // 回填会话 playerId（构造时占位 0），此后 Play 包按真实 playerId 派发。
    session->setPlayerId(creation.playerId);
}

void ClientSessionManager::onClientDisconnect(ServerClientConnection& conn)
{
    const u32 sessionId = conn.sessionId();
    spdlog::info("{}: remote client disconnected: sessionId={}", m_logPrefix, sessionId);

    // 移除会话簿记
    {
        std::lock_guard<std::mutex> lock(m_sessionsMutex);
        m_sessions.erase(sessionId);
    }

    // 移除远程玩家（若已创建实体）：按 sessionId 查 playerId。
    const PlayerId playerId = m_server.playerManager().getPlayerIdBySession(sessionId);
    if (playerId != 0) {
        // 离场广播用的 UUID 必须在 removePlayer 之前取——之后 ServerPlayerData 即被销毁。
        // 判据用 uuid 非空而**不用** hasConnection()：本回调触发时该连接的 socket 已经关闭，
        // hasConnection()（= connection && connection->isConnected()）此刻恒为 false，
        // 用它会让离场广播永不发出。
        std::optional<std::array<u8, 16>> departedUuid;
        if (const auto* playerData = m_server.playerManager().getPlayer(playerId);
            playerData != nullptr && !playerData->uuid.empty()) {
            departedUuid = util::uuidFromString(playerData->uuid);
        }

        // 清理玩家实体
        auto* world = m_server.getPlayerWorld(playerId);
        if (world != nullptr) {
            m_server.playerEntityManager().removePlayerEntity(playerId, *world);
        }

        // 移除玩家会话信息。物品栏无需另行清理——它随上面移除的玩家实体一起消失，
        // InventoryManager 不持有任何需要释放的副本。
        m_server.playerManager().removePlayer(playerId);

        // 广播离场（ClientboundPlayerInfoRemove，cb 67），否则其余客户端的 Tab 列表会永久
        // 残留该玩家。顺序与语义一致于 vanilla：先把玩家移出在线列表，再向剩余玩家广播；
        // 离开者本人此时已不在接收集合内，不会收到自己的移除包。
        if (departedUuid.has_value()) {
            mc::network::ir::play::PlayerInfoRemove removal;
            removal.uuids.push_back(*departedUuid);
            m_server.broadcastPacket(mc::network::ir::IrPacket{
                mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{removal}});
        }
    }
}

void ClientSessionManager::clear() noexcept
{
    std::lock_guard<std::mutex> lock(m_sessionsMutex);
    m_sessions.clear();
}

std::size_t ClientSessionManager::sessionCount() const noexcept
{
    std::lock_guard<std::mutex> lock(m_sessionsMutex);
    return m_sessions.size();
}

} // namespace mc::server::net
