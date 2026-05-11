#include "ServerScoreboard.hpp"
#include "../application/MinecraftServer.hpp"
#include "../player/ServerPlayer.hpp"
#include "../core/PlayerManager.hpp"
#include "../core/ServerPlayerData.hpp"
#include "../core/ConnectionManager.hpp"
#include "../../common/network/packet/PacketSerializer.hpp"
#include "../../common/scoreboard/storage/ScoreboardDataManager.hpp"
#include "../../common/util/text/StringTextComponent.hpp"

namespace mc {
namespace server {

ServerScoreboard::ServerScoreboard(MinecraftServer& server)
    : m_server(server)
{
}

ServerScoreboard::~ServerScoreboard() {
    // 析构时自动保存脏数据
    if (m_dirty && m_dataManager) {
        m_dataManager->saveScoreboard(*this);
    }
}

void ServerScoreboard::onPlayerJoin(mc::ServerPlayer& player) {
    // 发送所有已同步的目标
    for (auto* objective : m_addedObjectives) {
        sendObjectiveToPlayer(*objective, player.playerId());

        // 发送该目标的所有分数
        auto scores = getSortedScores(*objective);
        for (auto* score : scores) {
            sendScoreToPlayer(*score, player.playerId());
        }
    }

    // 发送所有显示槽位
    for (size_t i = 0; i < scoreboard::DISPLAY_SLOT_COUNT; ++i) {
        auto slot = static_cast<scoreboard::DisplaySlot>(i);
        if (auto* objective = getObjectiveInDisplaySlot(slot)) {
            sendDisplayObjectiveToPlayer(slot, objective, player.playerId());
        }
    }

    // 发送所有队伍
    auto teams = getTeams();
    for (auto* team : teams) {
        sendTeamToPlayer(*team, player.playerId());
    }
}

void ServerScoreboard::onPlayerLeave(PlayerId playerId, const std::string& playerName) {
    // 移除玩家的所有分数
    removeScore(playerName);
    markDirty();
}

void ServerScoreboard::sendToAllPlayers(network::PacketType type, const std::vector<u8>& payload) {
    auto& playerManager = m_server.playerManager();
    auto& connectionManager = m_server.connectionManager();

    playerManager.forEachPlayer([&](ServerPlayerData& data) {
        if (data.hasConnection()) {
            connectionManager.sendPacketToPlayer(data.playerId, type, payload);
        }
    });
}

void ServerScoreboard::sendToPlayer(PlayerId playerId, network::PacketType type, const std::vector<u8>& payload) {
    m_server.connectionManager().sendPacketToPlayer(playerId, type, payload);
}

void ServerScoreboard::sendObjectiveToPlayer(scoreboard::ScoreObjective& objective, PlayerId playerId) {
    auto packet = createObjectivePacket(objective, network::ObjectiveAction::Add);

    network::PacketSerializer ser;
    packet.serialize(ser);
    sendToPlayer(playerId, network::PacketType::ScoreboardObjective, ser.buffer());
}

void ServerScoreboard::sendRemoveObjectiveToPlayer(scoreboard::ScoreObjective& objective, PlayerId playerId) {
    auto packet = createObjectivePacket(objective, network::ObjectiveAction::Remove);

    network::PacketSerializer ser;
    packet.serialize(ser);
    sendToPlayer(playerId, network::PacketType::ScoreboardObjective, ser.buffer());
}

void ServerScoreboard::sendScoreToPlayer(scoreboard::Score& score, PlayerId playerId) {
    auto packet = createScorePacket(score, network::ScoreAction::Change);

    network::PacketSerializer ser;
    packet.serialize(ser);
    sendToPlayer(playerId, network::PacketType::UpdateScore, ser.buffer());
}

void ServerScoreboard::sendRemoveScoreToPlayer(const std::string& playerName,
                                               const std::string& objectiveName,
                                               PlayerId playerId) {
    network::UpdateScorePacket packet;
    packet.setPlayerName(playerName);
    packet.setObjectiveName(objectiveName);
    packet.setAction(network::ScoreAction::Remove);

    network::PacketSerializer ser;
    packet.serialize(ser);
    sendToPlayer(playerId, network::PacketType::UpdateScore, ser.buffer());
}

void ServerScoreboard::sendDisplayObjectiveToPlayer(scoreboard::DisplaySlot slot,
                                                    scoreboard::ScoreObjective* objective,
                                                    PlayerId playerId) {
    network::DisplayObjectivePacket packet;
    packet.setPosition(static_cast<i32>(slot));
    packet.setObjectiveName(objective ? objective->getName() : "");

    network::PacketSerializer ser;
    packet.serialize(ser);
    sendToPlayer(playerId, network::PacketType::DisplayObjective, ser.buffer());
}

void ServerScoreboard::sendTeamToPlayer(scoreboard::ScorePlayerTeam& team, PlayerId playerId) {
    auto packet = createTeamPacket(team, network::TeamAction::Create);

    network::PacketSerializer ser;
    packet.serialize(ser);
    sendToPlayer(playerId, network::PacketType::Teams, ser.buffer());
}

void ServerScoreboard::sendRemoveTeamToPlayer(scoreboard::ScorePlayerTeam& team, PlayerId playerId) {
    auto packet = createTeamPacket(team, network::TeamAction::Remove);

    network::PacketSerializer ser;
    packet.serialize(ser);
    sendToPlayer(playerId, network::PacketType::Teams, ser.buffer());
}

void ServerScoreboard::save() {
    if (m_dirty && m_dataManager) {
        auto result = m_dataManager->saveScoreboard(*this);
        if (result.success()) {
            m_dirty = false;
        } else {
            // 记录错误但不抛异常
            // TODO: 添加日志
        }
    }
}

void ServerScoreboard::load() {
    if (m_dataManager) {
        auto result = m_dataManager->loadScoreboard(*this);
        if (result.success()) {
            // 加载完成后，将所有目标标记为已同步
            for (auto* objective : getObjectives()) {
                m_addedObjectives.insert(objective);
            }
        } else {
            // 记录错误但不抛异常
            // TODO: 添加日志
        }
    }
}

void ServerScoreboard::onObjectiveAdded(scoreboard::ScoreObjective& objective) {
    m_addedObjectives.insert(&objective);

    // 广播给所有玩家
    auto packet = createObjectivePacket(objective, network::ObjectiveAction::Add);

    network::PacketSerializer ser;
    packet.serialize(ser);
    sendToAllPlayers(network::PacketType::ScoreboardObjective, ser.buffer());

    markDirty();
}

void ServerScoreboard::onObjectiveRemoved(scoreboard::ScoreObjective& objective) {
    m_addedObjectives.erase(&objective);

    // 广播给所有玩家
    auto packet = createObjectivePacket(objective, network::ObjectiveAction::Remove);

    network::PacketSerializer ser;
    packet.serialize(ser);
    sendToAllPlayers(network::PacketType::ScoreboardObjective, ser.buffer());

    markDirty();
}

void ServerScoreboard::onObjectiveChanged(scoreboard::ScoreObjective& objective) {
    // 广播给所有玩家
    auto packet = createObjectivePacket(objective, network::ObjectiveAction::Update);

    network::PacketSerializer ser;
    packet.serialize(ser);
    sendToAllPlayers(network::PacketType::ScoreboardObjective, ser.buffer());

    markDirty();
}

void ServerScoreboard::onScoreChanged(scoreboard::Score& score) {
    // 广播给所有玩家
    auto packet = createScorePacket(score, network::ScoreAction::Change);

    network::PacketSerializer ser;
    packet.serialize(ser);
    sendToAllPlayers(network::PacketType::UpdateScore, ser.buffer());

    markDirty();
}

void ServerScoreboard::onScoreRemoved(scoreboard::Score& score) {
    // 广播给所有玩家
    auto packet = createScorePacket(score, network::ScoreAction::Remove);

    network::PacketSerializer ser;
    packet.serialize(ser);
    sendToAllPlayers(network::PacketType::UpdateScore, ser.buffer());

    markDirty();
}

void ServerScoreboard::onPlayerRemoved(const std::string& playerName) {
    // 广播移除玩家的所有分数
    auto objectives = getPlayerObjectives(playerName);
    for (const auto& objName : objectives) {
        network::UpdateScorePacket packet;
        packet.setPlayerName(playerName);
        packet.setObjectiveName(objName);
        packet.setAction(network::ScoreAction::Remove);

        network::PacketSerializer ser;
        packet.serialize(ser);
        sendToAllPlayers(network::PacketType::UpdateScore, ser.buffer());
    }

    markDirty();
}

void ServerScoreboard::onPlayerScoreRemoved(const std::string& playerName,
                                            scoreboard::ScoreObjective& objective) {
    // 广播移除特定分数
    network::UpdateScorePacket packet;
    packet.setPlayerName(playerName);
    packet.setObjectiveName(objective.getName());
    packet.setAction(network::ScoreAction::Remove);

    network::PacketSerializer ser;
    packet.serialize(ser);
    sendToAllPlayers(network::PacketType::UpdateScore, ser.buffer());

    markDirty();
}

void ServerScoreboard::onTeamAdded(scoreboard::ScorePlayerTeam& team) {
    // 广播给所有玩家
    auto packet = createTeamPacket(team, network::TeamAction::Create);

    network::PacketSerializer ser;
    packet.serialize(ser);
    sendToAllPlayers(network::PacketType::Teams, ser.buffer());

    markDirty();
}

void ServerScoreboard::onTeamChanged(scoreboard::ScorePlayerTeam& team) {
    // 广播给所有玩家
    auto packet = createTeamPacket(team, network::TeamAction::Update);

    network::PacketSerializer ser;
    packet.serialize(ser);
    sendToAllPlayers(network::PacketType::Teams, ser.buffer());

    markDirty();
}

void ServerScoreboard::onTeamRemoved(scoreboard::ScorePlayerTeam& team) {
    // 广播给所有玩家
    auto packet = createTeamPacket(team, network::TeamAction::Remove);

    network::PacketSerializer ser;
    packet.serialize(ser);
    sendToAllPlayers(network::PacketType::Teams, ser.buffer());

    markDirty();
}

void ServerScoreboard::onDisplaySlotChanged(scoreboard::DisplaySlot slot,
                                            scoreboard::ScoreObjective* objective) {
    // 广播给所有玩家
    network::DisplayObjectivePacket packet;
    packet.setPosition(static_cast<i32>(slot));
    packet.setObjectiveName(objective ? objective->getName() : "");

    network::PacketSerializer ser;
    packet.serialize(ser);
    sendToAllPlayers(network::PacketType::DisplayObjective, ser.buffer());

    markDirty();
}

network::ScoreboardObjectivePacket ServerScoreboard::createObjectivePacket(
    scoreboard::ScoreObjective& objective,
    network::ObjectiveAction action) {
    network::ScoreboardObjectivePacket packet;

    packet.setObjectiveName(objective.getName());
    packet.setAction(action);

    if (action == network::ObjectiveAction::Add || action == network::ObjectiveAction::Update) {
        // 获取显示名称的 JSON 表示
        if (auto* displayName = objective.getDisplayName()) {
            // TODO: 将 ITextComponent 转换为 JSON 字符串
            packet.setDisplayName(objective.getName());  // 临时使用名称
        } else {
            packet.setDisplayName(objective.getName());
        }

        packet.setRenderType(scoreboard::renderTypeToString(objective.getRenderType()));
    }

    return packet;
}

network::UpdateScorePacket ServerScoreboard::createScorePacket(
    scoreboard::Score& score,
    network::ScoreAction action) {
    network::UpdateScorePacket packet;

    packet.setPlayerName(score.getPlayerName());
    packet.setObjectiveName(score.getObjective().getName());
    packet.setAction(action);

    if (action == network::ScoreAction::Change) {
        packet.setScore(score.getScorePoints());
    }

    return packet;
}

network::DisplayObjectivePacket ServerScoreboard::createDisplayObjectivePacket(
    scoreboard::DisplaySlot slot,
    scoreboard::ScoreObjective* objective) {
    network::DisplayObjectivePacket packet;

    packet.setPosition(static_cast<i32>(slot));
    packet.setObjectiveName(objective ? objective->getName() : "");

    return packet;
}

network::TeamsPacket ServerScoreboard::createTeamPacket(
    scoreboard::ScorePlayerTeam& team,
    network::TeamAction action) {
    network::TeamsPacket packet;

    packet.setTeamName(team.getName());
    packet.setAction(action);

    if (action == network::TeamAction::Create || action == network::TeamAction::Update) {
        // 获取显示名称
        if (auto* displayName = team.getDisplayName()) {
            // TODO: 将 ITextComponent 转换为 JSON 字符串
            packet.setDisplayName(team.getName());  // 临时使用名称
        } else {
            packet.setDisplayName(team.getName());
        }

        // 获取前缀和后缀
        if (auto* prefix = team.getPrefix()) {
            // TODO: 将 ITextComponent 转换为 JSON 字符串
            // packet.setPrefix(prefixJson);
        }
        if (auto* suffix = team.getSuffix()) {
            // TODO: 将 ITextComponent 转换为 JSON 字符串
            // packet.setSuffix(suffixJson);
        }

        packet.setNameTagVisibility(scoreboard::teamVisibilityToString(team.getNameTagVisibility()));
        packet.setCollisionRule(scoreboard::teamCollisionRuleToString(team.getCollisionRule()));
        packet.setColor(text::toName(team.getColor()));
        packet.setFriendlyFlags(team.getFriendlyFlags());
    }

    if (action == network::TeamAction::Create ||
        action == network::TeamAction::AddMember ||
        action == network::TeamAction::RemoveMember) {
        packet.setPlayers(std::vector<std::string>(team.getMembers().begin(), team.getMembers().end()));
    }

    return packet;
}

} // namespace server
} // namespace mc
