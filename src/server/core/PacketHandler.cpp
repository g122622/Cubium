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

// 注意：旧 PacketHandler 是 1.16.5 字节协议的入站 switch，已被新网络层
// MinecraftServer::routeInboundPlayPacket（按 ir::PlayPacket 变体分发）取代。
// 本文件保留类壳以维持 IServer::packetHandler() 接口与 m_packetHandler 成员，
// 所有处理体降级为 TODO(Step4/Phase6) 占位：
//   - MoveVehicle/UseEntity/SteerBoat/EntityAction/PlayerInput 的独占处理逻辑
//     待迁入 ServerPlayRouter（对应 ir::play::ServerboundMoveVehicle/Interact/
//     PaddleBoat/PlayerCommand/PlayerInput）。
//   - 登录/移动/心跳/聊天/传送确认 已由 routeInboundPlayPacket 覆盖。
// 删除整个 PacketHandler 体系在 Step5（删旧体系）完成。

#include "PacketHandler.hpp"
#include "ConnectionManager.hpp"
#include "KeepAliveManager.hpp"
#include "PlayerManager.hpp"
#include "PositionTracker.hpp"
#include "TeleportManager.hpp"
#include "TimeManager.hpp"
#include <spdlog/spdlog.h>

namespace mc::server::core {

PacketHandler::PacketHandler(PlayerManager& playerManager,
    ConnectionManager& connectionManager,
    TeleportManager& teleportManager,
    KeepAliveManager& keepAliveManager,
    PositionTracker& positionTracker,
    TimeManager& timeManager,
    GameMode defaultGameMode)
    : m_playerManager(playerManager)
    , m_connectionManager(connectionManager)
    , m_teleportManager(teleportManager)
    , m_keepAliveManager(keepAliveManager)
    , m_positionTracker(positionTracker)
    , m_timeManager(timeManager)
    , m_defaultGameMode(defaultGameMode)
{
    (void)m_server; // 由 setServer 注入，占位壳暂不使用
}

PacketHandleResult PacketHandler::handlePacket(u32 sessionId, const u8* data, size_t size)
{
    // 旧字节协议入站 switch 已废，新网络层经 routeInboundPlayPacket 分发。
    (void)sessionId;
    (void)data;
    (void)size;
    return PacketHandleResult::Ignore;
}

LoginResult PacketHandler::handleLoginRequest(u32 sessionId, network::ConnectionPtr connection, const u8* data, size_t size)
{
    // 登录全由 ServerHandshakeStateMachine 驱动，此处不再可达。
    (void)sessionId;
    (void)connection;
    (void)data;
    (void)size;
    LoginResult result;
    result.message = "Legacy login path disabled (use ServerHandshake)";
    return result;
}

PacketHandleResult PacketHandler::handlePlayerMove(u32 sessionId, const u8* data, size_t size)
{
    // 已由 routeInboundPlayPacket 的 MovePlayer 分支覆盖。
    (void)sessionId;
    (void)data;
    (void)size;
    return PacketHandleResult::Ignore;
}

PacketHandleResult PacketHandler::handlePlayerInput(u32 sessionId, const u8* data, size_t size)
{
    // TODO(Step4): 迁入 ServerPlayRouter，对应 ir::play::PlayerInput（u8 bitmask）。
    (void)sessionId;
    (void)data;
    (void)size;
    return PacketHandleResult::Ignore;
}

PacketHandleResult PacketHandler::handleMoveVehicle(u32 sessionId, const u8* data, size_t size)
{
    // TODO(Step4): 迁入 ServerPlayRouter，对应 ir::play::ServerboundMoveVehicle。
    //   含 moved wrongly / 碰撞回退 + ClientboundMoveVehicle 校正发送。
    (void)sessionId;
    (void)data;
    (void)size;
    return PacketHandleResult::Ignore;
}

PacketHandleResult PacketHandler::handleEntityAction(u32 sessionId, const u8* data, size_t size)
{
    // TODO(Step4): 迁入 ServerPlayRouter，对应 ir::play::PlayerCommand。
    (void)sessionId;
    (void)data;
    (void)size;
    return PacketHandleResult::Ignore;
}

PacketHandleResult PacketHandler::handleSteerBoat(u32 sessionId, const u8* data, size_t size)
{
    // TODO(Step4): 迁入 ServerPlayRouter，对应 ir::play::PaddleBoat。
    (void)sessionId;
    (void)data;
    (void)size;
    return PacketHandleResult::Ignore;
}

PacketHandleResult PacketHandler::handleTeleportConfirm(u32 sessionId, const u8* data, size_t size)
{
    // 已由 routeInboundPlayPacket 的 AcceptTeleportation 分支覆盖。
    (void)sessionId;
    (void)data;
    (void)size;
    return PacketHandleResult::Ignore;
}

PacketHandleResult PacketHandler::handleKeepAlive(u32 sessionId, const u8* data, size_t size, u64 currentTimeMs)
{
    // 已由 routeInboundPlayPacket 的 KeepAlive 分支覆盖。
    (void)sessionId;
    (void)data;
    (void)size;
    (void)currentTimeMs;
    return PacketHandleResult::Ignore;
}

PacketHandleResult PacketHandler::handleChatMessage(u32 sessionId, const u8* data, size_t size)
{
    // 已由 routeInboundPlayPacket 的 Chat 分支覆盖。
    (void)sessionId;
    (void)data;
    (void)size;
    return PacketHandleResult::Ignore;
}

PacketHandleResult PacketHandler::handleUseEntity(u32 sessionId, const u8* data, size_t size)
{
    // TODO(Step4): 迁入 ServerPlayRouter，对应 ir::play::Interact。
    //   含 player_interacted_with_entity 成就触发。
    (void)sessionId;
    (void)data;
    (void)size;
    return PacketHandleResult::Ignore;
}

} // namespace mc::server::core
