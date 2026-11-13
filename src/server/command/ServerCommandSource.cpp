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

#include "ServerCommandSource.hpp"
#include "common/command/CommandSource.hpp"
#include "common/command/ICommandSource.hpp"
#include "common/command/errors/CommandErrors.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/text/ComponentNbtSerialization.hpp"
#include "server/access/AccessManager.hpp"
#include "server/access/base/OpListManagerTypes.hpp"
#include "server/application/IServer.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/network/outbound/PacketSender.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/player/lifecycle/PlayerManager.hpp"
#include "server/player/runtime/ServerPlayerData.hpp"
#include "server/world/ServerWorld.hpp"
#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <spdlog/spdlog.h>

namespace mc {
namespace command {

ServerCommandSource::ServerCommandSource(server::IServer* server,
    ServerPlayer* player,
    DimensionId dimensionId,
    const Vector3d& position,
    const Vector2f& rotation,
    i32 permissionLevel,
    PlayerId playerId,
    std::string playerName,
    Entity* entity)
    : m_server(server)
    , m_player(player)
    , m_entity(entity ? entity : static_cast<Entity*>(player))
    , m_playerId(player ? player->playerId() : playerId)
    , m_dimensionId(dimensionId)
    , m_position(position)
    , m_rotation(rotation)
    , m_permissionLevel(permissionLevel)
    , m_feedbackDisabled(false)
{
    // 设置显示名称（优先级：player 名称 > playerName 参数 > 默认 "Server"）。
    // 原版非玩家命令源（控制台、调度器）的显示名就是 "Server"。
    if (player) {
        m_name = player->username();
    } else if (!playerName.empty()) {
        m_name = std::move(playerName);
    } else {
        m_name = "Server";
    }
}

ServerCommandSource::ServerCommandSource(server::IServer* server,
    ServerPlayer* player,
    Entity* entity,
    PlayerId playerId,
    DimensionId dimensionId,
    const Vector3d& position,
    const Vector2f& rotation,
    i32 permissionLevel,
    std::string name,
    bool feedbackDisabled,
    EntityAnchorType anchor)
    : m_server(server)
    , m_player(player)
    , m_entity(entity)
    , m_playerId(playerId)
    , m_dimensionId(dimensionId)
    , m_position(position)
    , m_rotation(rotation)
    , m_permissionLevel(permissionLevel)
    , m_name(std::move(name))
    , m_feedbackDisabled(feedbackDisabled)
    , m_anchor(anchor)
{}

server::ServerWorld* ServerCommandSource::world() const noexcept
{
    if (m_server == nullptr) {
        return nullptr;
    }

    auto* dimension = m_server->dimensionManager().getDimension(m_dimensionId);
    return dimension != nullptr ? dimension->world() : nullptr;
}

void ServerCommandSource::sendSuccess(const text::ITextComponent& message, bool broadcastToAdmins)
{
    if (shouldReceiveFeedback()) {
        _deliver(message);
    }

    if (!broadcastToAdmins || m_server == nullptr) {
        return;
    }

    // 广播给其他权限 >= GameMaster 的玩家。内容只序列化一次，避免每个接收者重复编码。
    const std::vector<u8> content = ::mc::text::componentToNbtBytes(&message);
    const PlayerId selfPlayerId = m_playerId;
    const server::AccessManager& access = m_server->accessManager();

    m_server->playerManager().forEachPlayer([&content, &access, selfPlayerId](server::ServerPlayerData& data) {
        if (data.playerId == selfPlayerId) {
            return;
        }
        if (static_cast<u8>(access.operatorGetLevel(data.uuid)) < static_cast<u8>(server::OpLevel::GameMaster)) {
            return;
        }

        mc::network::ir::play::SystemChat pkt;
        pkt.content = content;
        pkt.overlay = false;
        static_cast<void>(data.send(mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(pkt)}}));
    });
}

void ServerCommandSource::sendFailure(const text::ITextComponent& message)
{
    if (!shouldReceiveErrors()) {
        return;
    }

    // 原版 sendFailure 在组件上叠加红色样式；deepCopy 避免修改调用方持有的组件。
    auto styled = message.deepCopy();
    styled->setStyle(styled->getStyle().applyFormat(text::TextFormatting::Red));
    _deliver(*styled);
}

void ServerCommandSource::_deliver(const text::ITextComponent& message)
{
    if (m_player != nullptr) {
        m_player->sendSystemMessage(message);
        return;
    }

    if (m_playerId != 0) {
        if (m_server != nullptr) {
            // 无实体 ServerPlayer 引用但有 playerId（如远程命令源）：经 PacketSender 下发
            // SystemChat(overlay=false)，保留完整组件结构。
            mc::network::ir::play::SystemChat pkt;
            pkt.content = ::mc::text::componentToNbtBytes(&message);
            pkt.overlay = false;
            m_server->packetSender().sendToPlayer(m_playerId,
                mc::network::ir::IrPacket{
                    mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(pkt)}});
            return;
        }

        spdlog::info("[System -> {}] {}", m_name, message.getUnformattedText());
        return;
    }

    // 非玩家命令源（控制台/RCON/命令方块外部入口）：组件字符串化后写日志，并经虚函数出口
    // 暴露给派生类（终端控制台据此把回执送进自己的事件区）。
    const std::string text = message.getUnformattedText();
    spdlog::info("{}", text);
    onConsoleMessage(text);
}

void ServerCommandSource::onConsoleMessage(const std::string& /*message*/) {}

bool ServerCommandSource::shouldReceiveFeedback() const noexcept
{
    return !m_feedbackDisabled;
}

bool ServerCommandSource::shouldReceiveErrors() const noexcept
{
    return true;
}

bool ServerCommandSource::allowLogging() const noexcept
{
    return true;
}

ServerCommandSource ServerCommandSource::withPlayer(ServerPlayer* player) const
{
    ServerCommandSource source(*this);
    source.m_player = player;
    source.m_entity = static_cast<Entity*>(player);
    if (player) {
        source.m_playerId = player->playerId();
        source.m_name = player->username();
    }
    return source;
}

ServerCommandSource ServerCommandSource::withEntity(Entity& entity) const
{
    ServerCommandSource source(*this);
    source.m_entity = &entity;

    // 如果实体是 ServerPlayer，同时更新 m_player 和 m_playerId
    auto* serverPlayer = dynamic_cast<ServerPlayer*>(&entity);
    if (serverPlayer != nullptr) {
        source.m_player = serverPlayer;
        source.m_playerId = serverPlayer->playerId();
    }
    // else: m_player 和 m_playerId 保持不变，允许非玩家实体作为执行者

    // 更新显示名称为实体名称
    if (serverPlayer != nullptr) {
        source.m_name = serverPlayer->username();
    } else if (entity.hasCustomName()) {
        source.m_name = entity.customNameText();
    } else {
        source.m_name = entity.getTypeId();
    }

    return source;
}

ServerCommandSource ServerCommandSource::withPosition(const Vector3d& pos) const
{
    ServerCommandSource source(*this);
    source.m_position = pos;
    return source;
}

ServerCommandSource ServerCommandSource::withRotation(const Vector2f& rot) const
{
    ServerCommandSource source(*this);
    source.m_rotation = rot;
    return source;
}

ServerCommandSource ServerCommandSource::withDimension(DimensionId dimensionId) const
{
    ServerCommandSource source(*this);
    source.m_dimensionId = dimensionId;
    return source;
}

ServerCommandSource ServerCommandSource::withAnchor(EntityAnchorType anchor) const
{
    ServerCommandSource source(*this);
    source.m_anchor = anchor;
    return source;
}

ServerCommandSource ServerCommandSource::withFeedbackDisabled() const
{
    ServerCommandSource source(*this);
    source.m_feedbackDisabled = true;
    return source;
}

ServerCommandSource ServerCommandSource::withSuppressedOutput() const
{
    return withFeedbackDisabled();
}

ServerCommandSource ServerCommandSource::withPermissionLevel(i32 level) const
{
    ServerCommandSource source(*this);
    source.m_permissionLevel = level;
    return source;
}

ServerCommandSource ServerCommandSource::withMaximumPermission(i32 level) const
{
    ServerCommandSource source(*this);
    source.m_permissionLevel = std::max(source.m_permissionLevel, level);
    return source;
}

ServerCommandSource ServerCommandSource::forConsole(server::IServer* server)
{
    return ServerCommandSource(server,
        nullptr, // 无玩家
        0,
        Vector3d(0, 0, 0),
        Vector2f(0, 0),
        4,
        0,
        "Server");
}

} // namespace command
} // namespace mc
