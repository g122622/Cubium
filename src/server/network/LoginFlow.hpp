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

#pragma once

#include "common/core/Types.hpp"
#include "server/network/ServerNetwork.hpp" // ServerClientConnection
#include <array>
#include <string>

namespace mc::server {
class MinecraftServer;
struct ServerPlayerData;
} // namespace mc::server

namespace mc::server::net {

/**
 * @brief 登录流程门面（批6 下沉自 MinecraftServer）
 *
 * 承载握手完成（onPlayerReady）后的玩家创建与初始游戏状态推送整簇逻辑：
 *  - createPlayerForConnection：分配 playerId、addPlayer、setupInitialPlayerState、
 *    createPlayerEntity、playerJoinDimension、OP 权限、存档加载、play::Login、
 *    sendPermissionLevelChange、sendInitialGameState。
 *  - setupInitialPlayerState：设置出生点位置与游戏模式。
 *  - sendLoginResponseForConnection：发送 play::Login（post-Configuration S→C）。
 *  - sendPermissionLevelChange：EntityEvent 通知权限等级 + 连带发命令树。
 *  - sendCommandTreePacket：编码并发送 ClientboundCommandsPacket。
 *  - sendInitialGameState：传送 + 时间 + 出生点 + 天气 + 难度 + 区块加载起始信号 +
 *    实体追踪建立 + 玩家区块追踪建立。
 *  - sendInitialDifficultyToPlayer：发送 ChangeDifficulty 包。
 *
 * 调用方：IntegratedServer::_onClientPlayerReady（本地客户端，sessionId=0）、
 * RemoteSessionManager::onPlayerReady（远程 TCP/LAN 玩家）。两路径共用本门面，
 * hardcore/seed/isFlat 三参由调用方按自身 settings/params 提供。
 *
 * 经 MinecraftServer& 的 public 访问器调用各 manager（playerManager/dimensionManager/
 * teleportManager/timeManager/sharedStorage/weatherSyncService/settings/playerEntityManager/
 * getPlayerWorld/resolveOpLevel/sendPacketToPlayer/updateEntityTrackingForPlayer/
 * serializeDifficultyPacket），无需 friend。
 */
class LoginFlow {
public:
    /// 玩家创建结果（createPlayerForConnection 返回）。
    struct PlayerCreationResult {
        PlayerId playerId{};
        EntityInstanceId entityId{};
        bool success{false};
    };

    explicit LoginFlow(MinecraftServer& server)
        : m_server(server)
    {}

    // 不可拷贝/移动（持引用）。
    LoginFlow(const LoginFlow&) = delete;
    LoginFlow& operator=(const LoginFlow&) = delete;
    LoginFlow(LoginFlow&&) = delete;
    LoginFlow& operator=(LoginFlow&&) = delete;

    /// 握手完成后创建玩家并初始化游戏状态（本地客户端 + 远程 TCP 共用）。
    /// 不含路由器 setPlayerId 与物品栏初始化——调用方在返回后自行处理。
    PlayerCreationResult createPlayerForConnection(mc::server::net::ServerClientConnection& connection,
        const std::string& username,
        const std::array<u8, 16>& offlineUuid,
        bool hardcore,
        i64 seed,
        bool isFlat);

private:
    void setupInitialPlayerState(ServerPlayerData* player, GameMode gameMode);
    void sendLoginResponseForConnection(PlayerId playerId, bool hardcore, i64 seed, bool isFlat);
    void sendPermissionLevelChange(PlayerId playerId, i32 permissionLevel);
    void sendCommandTreePacket(PlayerId playerId);
    void sendInitialGameState(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch);
    void sendInitialDifficultyToPlayer(PlayerId playerId);

    MinecraftServer& m_server;
};

} // namespace mc::server::net
