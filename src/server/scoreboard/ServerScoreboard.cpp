/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "ServerScoreboard.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/scoreboard/core/ScoreCriteriaRenderType.hpp"
#include "common/scoreboard/core/TeamEnums.hpp"
#include "common/scoreboard/storage/ScoreboardDataManager.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/TextStyle.hpp"
#include "server/application/IServer.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/player/ServerPlayer.hpp"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace mc {
namespace server {

// 使用 mc::scoreboard 命名空间中的类型
using ::mc::scoreboard::DISPLAY_SLOT_COUNT;
using ::mc::scoreboard::DisplaySlot;
using ::mc::scoreboard::Score;
using ::mc::scoreboard::Scoreboard;
using ::mc::scoreboard::ScoreObjective;
using ::mc::scoreboard::ScorePlayerTeam;

namespace {

/// 把 ITextComponent（或字符串名）序列化为 1.21.11 组件 opaque 字节（JSON 文本）。
/// TODO(Phase6): 当前仅以 JSON 字符串字节承载，未对齐 1.21.11 ComponentType 前缀树，
///   真互通需补完整 Component codec。我方互通客户端按相同约定解析即可。
std::vector<u8> componentToBytes(const ::mc::text::ITextComponent* component, const std::string& fallback)
{
    std::string json = component ? component->toJson().dump() : fallback;
    return std::vector<u8>(json.begin(), json.end());
}

/// 渲染类型字符串→1.21.11 整数序号（0=integer,1=hearts）
i32 renderTypeToOrdinal(::mc::scoreboard::RenderType type)
{
    switch (type) {
    case ::mc::scoreboard::RenderType::Hearts: return 1;
    case ::mc::scoreboard::RenderType::Integer: return 0;
    default: return 0;
    }
}

/// 构造一个 Play 阶段的 IrPacket
mc::network::ir::IrPacket makePlayPacket(mc::network::ir::PlayPacket pkt)
{
    return mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play, std::move(pkt)};
}

} // namespace

ServerScoreboard::ServerScoreboard(IServer& server)
    : m_server(server)
{}

ServerScoreboard::~ServerScoreboard() noexcept {}

void ServerScoreboard::onPlayerJoin(mc::ServerPlayer& player)
{
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
    for (size_t i = 0; i < DISPLAY_SLOT_COUNT; ++i) {
        auto slot = static_cast<DisplaySlot>(i);
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

void ServerScoreboard::onPlayerLeave(PlayerId playerId, const std::string& playerName)
{
    // 移除玩家的所有分数
    removeScore(playerName);
    markDirty();
}

void ServerScoreboard::sendToAllPlayers(const mc::network::ir::IrPacket& packet)
{
    auto& playerManager = m_server.playerManager();
    auto& connectionManager = m_server.connectionManager();

    playerManager.forEachPlayer([&](ServerPlayerData& data) {
        if (data.hasConnection()) {
            connectionManager.sendToPlayer(data.playerId, packet);
        }
    });
}

void ServerScoreboard::sendToPlayer(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    m_server.connectionManager().sendToPlayer(playerId, packet);
}

void ServerScoreboard::sendObjectiveToPlayer(ScoreObjective& objective, PlayerId playerId)
{
    sendToPlayer(playerId, makePlayPacket(mc::network::ir::PlayPacket{_createObjectivePacket(objective, 0)}));
}

void ServerScoreboard::sendRemoveObjectiveToPlayer(ScoreObjective& objective, PlayerId playerId)
{
    sendToPlayer(playerId, makePlayPacket(mc::network::ir::PlayPacket{_createObjectivePacket(objective, 1)}));
}

void ServerScoreboard::sendScoreToPlayer(Score& score, PlayerId playerId)
{
    sendToPlayer(playerId, makePlayPacket(mc::network::ir::PlayPacket{_createSetScorePacket(score, true)}));
}

void ServerScoreboard::sendRemoveScoreToPlayer(
    const std::string& playerName, const std::string& objectiveName, PlayerId playerId)
{
    mc::network::ir::play::ResetScore pkt;
    pkt.owner = playerName;
    pkt.objectiveName = objectiveName;
    sendToPlayer(playerId, makePlayPacket(mc::network::ir::PlayPacket{std::move(pkt)}));
}

void ServerScoreboard::sendDisplayObjectiveToPlayer(DisplaySlot slot, ScoreObjective* objective, PlayerId playerId)
{
    sendToPlayer(playerId, makePlayPacket(mc::network::ir::PlayPacket{_createDisplayObjectivePacket(slot, objective)}));
}

void ServerScoreboard::sendTeamToPlayer(ScorePlayerTeam& team, PlayerId playerId)
{
    sendToPlayer(playerId, makePlayPacket(mc::network::ir::PlayPacket{_createTeamPacket(team, 0)}));
}

void ServerScoreboard::sendRemoveTeamToPlayer(ScorePlayerTeam& team, PlayerId playerId)
{
    sendToPlayer(playerId, makePlayPacket(mc::network::ir::PlayPacket{_createTeamPacket(team, 1)}));
}

void ServerScoreboard::save()
{
    if (m_dirty && m_dataManager) {
        auto result = m_dataManager->saveScoreboard(*this);
        if (result.success()) {
            m_dirty = false;
        } else {
            // 记录错误但不抛异常
            spdlog::error("ServerScoreboard: Failed to save scoreboard: {}", result.error().message());
        }
    }
}

void ServerScoreboard::load()
{
    if (m_dataManager) {
        auto result = m_dataManager->loadScoreboard(*this);
        if (result.success()) {
            // 加载完成后，将所有目标标记为已同步
            for (auto* objective : getObjectives()) {
                m_addedObjectives.insert(objective);
            }
        } else {
            // 记录错误但不抛异常
            spdlog::error("ServerScoreboard: Failed to load scoreboard: {}", result.error().message());
        }
    }
}

void ServerScoreboard::onObjectiveAdded(ScoreObjective& objective)
{
    m_addedObjectives.insert(&objective);

    // 广播给所有玩家
    sendToAllPlayers(makePlayPacket(mc::network::ir::PlayPacket{_createObjectivePacket(objective, 0)}));

    markDirty();
}

void ServerScoreboard::onObjectiveRemoved(ScoreObjective& objective)
{
    m_addedObjectives.erase(&objective);

    // 广播给所有玩家
    sendToAllPlayers(makePlayPacket(mc::network::ir::PlayPacket{_createObjectivePacket(objective, 1)}));

    markDirty();
}

void ServerScoreboard::onObjectiveChanged(ScoreObjective& objective)
{
    // 广播给所有玩家
    sendToAllPlayers(makePlayPacket(mc::network::ir::PlayPacket{_createObjectivePacket(objective, 2)}));

    markDirty();
}

void ServerScoreboard::onScoreChanged(Score& score)
{
    // 广播给所有玩家
    sendToAllPlayers(makePlayPacket(mc::network::ir::PlayPacket{_createSetScorePacket(score, true)}));

    markDirty();
}

void ServerScoreboard::onScoreRemoved(Score& score)
{
    // 广播给所有玩家（移除分数 → ResetScore）
    mc::network::ir::play::ResetScore pkt;
    pkt.owner = score.getPlayerName();
    pkt.objectiveName = score.getObjective().getName();
    sendToAllPlayers(makePlayPacket(mc::network::ir::PlayPacket{std::move(pkt)}));

    markDirty();
}

void ServerScoreboard::onPlayerRemoved(const std::string& playerName)
{
    // 广播移除玩家的所有分数
    auto objectives = getPlayerObjectives(playerName);
    for (const auto& objName : objectives) {
        mc::network::ir::play::ResetScore pkt;
        pkt.owner = playerName;
        pkt.objectiveName = objName;
        sendToAllPlayers(makePlayPacket(mc::network::ir::PlayPacket{std::move(pkt)}));
    }

    markDirty();
}

void ServerScoreboard::onPlayerScoreRemoved(const std::string& playerName, ScoreObjective& objective)
{
    // 广播移除特定分数
    mc::network::ir::play::ResetScore pkt;
    pkt.owner = playerName;
    pkt.objectiveName = objective.getName();
    sendToAllPlayers(makePlayPacket(mc::network::ir::PlayPacket{std::move(pkt)}));

    markDirty();
}

void ServerScoreboard::onTeamAdded(ScorePlayerTeam& team)
{
    // 广播给所有玩家
    sendToAllPlayers(makePlayPacket(mc::network::ir::PlayPacket{_createTeamPacket(team, 0)}));

    markDirty();
}

void ServerScoreboard::onTeamChanged(ScorePlayerTeam& team)
{
    // 广播给所有玩家
    sendToAllPlayers(makePlayPacket(mc::network::ir::PlayPacket{_createTeamPacket(team, 2)}));

    markDirty();
}

void ServerScoreboard::onTeamRemoved(ScorePlayerTeam& team)
{
    // 广播给所有玩家
    sendToAllPlayers(makePlayPacket(mc::network::ir::PlayPacket{_createTeamPacket(team, 1)}));

    markDirty();
}

void ServerScoreboard::onDisplaySlotChanged(DisplaySlot slot, ScoreObjective* objective)
{
    // 广播给所有玩家
    sendToAllPlayers(makePlayPacket(mc::network::ir::PlayPacket{_createDisplayObjectivePacket(slot, objective)}));

    markDirty();
}

mc::network::ir::play::SetObjective ServerScoreboard::_createObjectivePacket(
    ScoreObjective& objective, u8 method)
{
    // method: 0=Add 1=Remove 2=Change
    mc::network::ir::play::SetObjective pkt;
    pkt.objectiveName = objective.getName();
    pkt.method = method;

    if (method == 0 || method == 2) {
        pkt.displayName = componentToBytes(objective.getDisplayName(), objective.getName());
        pkt.renderType = renderTypeToOrdinal(objective.getRenderType());
        // numberFormat 留空（TODO(Phase6): 1.21.11 NumberFormat 编码）
    }

    return pkt;
}

mc::network::ir::play::SetScore ServerScoreboard::_createSetScorePacket(Score& score, bool change)
{
    mc::network::ir::play::SetScore pkt;
    pkt.owner = score.getPlayerName();
    pkt.objectiveName = score.getObjective().getName();
    pkt.score = change ? score.getScorePoints() : 0;
    // display/numberFormat 留空（TODO(Phase6): 1.21.11 组件/NumberFormat 编码）
    return pkt;
}

mc::network::ir::play::SetDisplayObjective ServerScoreboard::_createDisplayObjectivePacket(
    DisplaySlot slot, ScoreObjective* objective)
{
    mc::network::ir::play::SetDisplayObjective pkt;
    pkt.slot = static_cast<i32>(slot);
    pkt.objectiveName = objective ? objective->getName() : "";
    return pkt;
}

mc::network::ir::play::SetPlayerTeam ServerScoreboard::_createTeamPacket(ScorePlayerTeam& team, u8 method)
{
    // method: 0=Create 1=Remove 2=Change 3=Join 4=Leave
    mc::network::ir::play::SetPlayerTeam pkt;
    pkt.name = team.getName();
    pkt.method = method;

    if (method == 0 || method == 2) {
        // 1.21.11 将 displayName/prefix/suffix/visibility/collision/color/friendlyFlags
        // 打包进一个 opaque parameters blob。当前以 JSON 字符串承载，未完整对齐
        // 1.21.11 序列化格式。TODO(Phase6): 补完整 TeamParameters codec。
        nlohmann::json params;
        params["displayName"] = componentToBytes(team.getDisplayName(), team.getName());
        if (auto* prefix = team.getPrefix()) {
            params["prefix"] = componentToBytes(prefix, "");
        }
        if (auto* suffix = team.getSuffix()) {
            params["suffix"] = componentToBytes(suffix, "");
        }
        params["nameTagVisibility"] = scoreboard::teamVisibilityToString(team.getNameTagVisibility());
        params["collisionRule"] = scoreboard::teamCollisionRuleToString(team.getCollisionRule());
        params["color"] = text::toName(team.getColor());
        params["friendlyFlags"] = team.getFriendlyFlags();
        std::string dumped = params.dump();
        pkt.parameters = std::vector<u8>(dumped.begin(), dumped.end());
    }

    if (method == 0 || method == 3 || method == 4) {
        pkt.players = std::vector<std::string>(team.getMembers().begin(), team.getMembers().end());
    }

    return pkt;
}

} // namespace server
} // namespace mc
