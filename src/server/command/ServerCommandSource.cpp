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
#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include "server/application/IServer.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include <algorithm>
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
    // 设置显示名称（优先级：player 名称 > playerName 参数 > entity 名称 > "Console"）
    if (player) {
        m_name = player->username();
    } else if (!playerName.empty()) {
        m_name = std::move(playerName);
    } else {
        m_name = "Console";
    }
}

server::ServerWorld* ServerCommandSource::world() const noexcept
{
    if (m_server == nullptr) {
        return nullptr;
    }

    auto* dimension = m_server->dimensionManager().getDimension(m_dimensionId);
    return dimension != nullptr ? dimension->world() : nullptr;
}

void ServerCommandSource::sendMessage(const std::string& message, const std::optional<Uuid>& /*senderUuid*/
)
{
    if (m_player) {
        m_player->sendSystemMessage(message);
        return;
    } else if (m_playerId != 0) {
        if (m_server != nullptr) {
            // TODO(Phase6): 新 IR 暂无 S→C 系统/聊天消息包（SystemChat/DisguisedChat）。
            //   旧 ChatBroadcast 字节包已删除，无实体 player 时的系统消息当前无法下发，
            //   降级为日志输出。
            spdlog::info("[System -> {}] {}", m_name, message);
            return;
        }

        spdlog::info("[System -> {}] {}", m_name, message);
        return;
    }

    spdlog::info("{}", message);
}

void ServerCommandSource::sendError(const std::string& message)
{
    // 错误消息使用红色格式（对于支持的客户端）
    std::string formattedMessage = "§c" + message;
    sendMessage(formattedMessage);
}

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

ServerPlayer& ServerCommandSource::assertPlayer() const
{
    if (!m_player) {
        throw CommandException(CommandErrorType::PermissionDenied, "commands.requires.player");
    }
    return *m_player;
}

Entity& ServerCommandSource::entityOrException() const
{
    if (m_entity == nullptr) {
        throw CommandException(CommandErrorType::PermissionDenied, "commands.requires.entity");
    }
    return *m_entity;
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
        "Console");
}

} // namespace command
} // namespace mc
