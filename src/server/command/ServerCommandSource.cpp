#include "ServerCommandSource.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include "server/application/IServer.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace mc {
namespace command {

ServerCommandSource::ServerCommandSource(server::IServer* server,
    ServerPlayer* player,
    server::ServerWorld* world,
    const Vector3d& position,
    const Vector2f& rotation,
    i32 permissionLevel,
    PlayerId playerId,
    std::string playerName)
    : m_server(server)
    , m_player(player)
    , m_playerId(player ? player->playerId() : playerId)
    , m_world(world)
    , m_position(position)
    , m_rotation(rotation)
    , m_permissionLevel(permissionLevel)
    , m_feedbackDisabled(false)
{
    // 设置显示名称
    if (player) {
        m_name = player->username();
    } else if (!playerName.empty()) {
        m_name = std::move(playerName);
    } else {
        m_name = "Console";
    }
}

void ServerCommandSource::sendMessage(const std::string& message, const std::optional<Uuid>& /*senderUuid*/
)
{
    if (m_player) {
        m_player->sendSystemMessage(message);
        return;
    } else if (m_playerId != 0) {
        if (m_server != nullptr) {
            network::ChatMessagePacket chatPacket(message, 0);
            network::PacketSerializer payload;
            chatPacket.serialize(payload);

            if (m_server->connectionManager().sendPacketToPlayer(
                    m_playerId, network::PacketType::ChatBroadcast, payload.buffer())) {
                return;
            }
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

bool ServerCommandSource::shouldReceiveFeedback() const
{
    return !m_feedbackDisabled;
}

bool ServerCommandSource::shouldReceiveErrors() const
{
    return true;
}

bool ServerCommandSource::allowLogging() const
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

ServerCommandSource ServerCommandSource::withPlayer(ServerPlayer* player) const
{
    ServerCommandSource source(*this);
    source.m_player = player;
    if (player) {
        source.m_playerId = player->playerId();
        source.m_name = player->username();
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

ServerCommandSource ServerCommandSource::withWorld(server::ServerWorld* world) const
{
    ServerCommandSource source(*this);
    source.m_world = world;
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
        nullptr, // 无世界
        Vector3d(0, 0, 0),
        Vector2f(0, 0),
        4,
        0,
        "Console");
}

} // namespace command
} // namespace mc
