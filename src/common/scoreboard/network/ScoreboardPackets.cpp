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

#include "ScoreboardPackets.hpp"
#include <algorithm>

namespace mc::network {

// ========== ScoreboardObjectivePacket ==========

ScoreboardObjectivePacket::ScoreboardObjectivePacket(const std::string& objectiveName,
    ObjectiveAction action,
    const std::string& displayName,
    const std::string& renderType)
    : m_objectiveName(objectiveName)
    , m_action(action)
    , m_displayName(displayName)
    , m_renderType(renderType)
{
    setObjectiveName(objectiveName);
}

void ScoreboardObjectivePacket::setObjectiveName(const std::string& name)
{
    // 限制名称长度
    m_objectiveName = name.substr(0, std::min(name.length(), MAX_NAME_LENGTH));
}

void ScoreboardObjectivePacket::setDisplayName(const std::string& name)
{
    // 限制显示名称长度
    m_displayName = name.substr(0, std::min(name.length(), MAX_DISPLAY_NAME_LENGTH));
}

void ScoreboardObjectivePacket::setRenderType(const std::string& type)
{
    m_renderType = (type == "hearts") ? "hearts" : "integer";
}

void ScoreboardObjectivePacket::serialize(PacketSerializer& ser) const
{
    ser.writeString(m_objectiveName);
    ser.writeU8(static_cast<u8>(m_action));

    // Add/Update 需要额外数据
    if (m_action == ObjectiveAction::Add || m_action == ObjectiveAction::Update) {
        ser.writeString(m_displayName);
        ser.writeString(m_renderType);
    }
}

Result<UpdateScorePacket> UpdateScorePacket::deserialize(PacketDeserializer& deser)
{
    UpdateScorePacket packet;

    auto playerNameResult = deser.readString();
    if (playerNameResult.failed()) {
        return playerNameResult.error();
    }
    packet.m_playerName = playerNameResult.value();

    auto actionResult = deser.readU8();
    if (actionResult.failed()) {
        return actionResult.error();
    }
    packet.m_action = static_cast<ScoreAction>(actionResult.value());

    // Remove 时目标名称可选
    auto objectiveResult = deser.readString();
    if (objectiveResult.failed()) {
        return objectiveResult.error();
    }
    packet.m_objectiveName = objectiveResult.value();

    if (packet.m_action == ScoreAction::Change) {
        auto scoreResult = deser.readVarInt();
        if (scoreResult.failed()) {
            return scoreResult.error();
        }
        packet.m_score = scoreResult.value();
    }

    return packet;
}

// ========== UpdateScorePacket ==========

UpdateScorePacket::UpdateScorePacket(
    const std::string& playerName, const std::string& objectiveName, i32 score, ScoreAction action)
    : m_playerName(playerName)
    , m_objectiveName(objectiveName)
    , m_score(score)
    , m_action(action)
{
    setPlayerName(playerName);
    setObjectiveName(objectiveName);
}

void UpdateScorePacket::setPlayerName(const std::string& name)
{
    m_playerName = name.substr(0, std::min(name.length(), MAX_NAME_LENGTH));
}

void UpdateScorePacket::setObjectiveName(const std::string& name)
{
    m_objectiveName = name.substr(0, std::min(name.length(), ScoreboardObjectivePacket::MAX_NAME_LENGTH));
}

void UpdateScorePacket::serialize(PacketSerializer& ser) const
{
    ser.writeString(m_playerName);
    ser.writeU8(static_cast<u8>(m_action));

    // Change 需要目标名称
    if (m_action == ScoreAction::Change) {
        ser.writeString(m_objectiveName);
        ser.writeVarInt(m_score);
    } else {
        // Remove 时目标名称可选，可以指定移除特定目标的分数
        // 如果目标名称为空，则移除玩家的所有分数
        ser.writeString(m_objectiveName);
    }
}

Result<ScoreboardObjectivePacket> ScoreboardObjectivePacket::deserialize(PacketDeserializer& deser)
{
    ScoreboardObjectivePacket packet;

    auto nameResult = deser.readString();
    if (nameResult.failed()) {
        return nameResult.error();
    }
    packet.m_objectiveName = nameResult.value();

    auto actionResult = deser.readU8();
    if (actionResult.failed()) {
        return actionResult.error();
    }
    packet.m_action = static_cast<ObjectiveAction>(actionResult.value());

    if (packet.m_action == ObjectiveAction::Add || packet.m_action == ObjectiveAction::Update) {
        auto displayResult = deser.readString();
        if (displayResult.failed()) {
            return displayResult.error();
        }
        packet.m_displayName = displayResult.value();

        auto renderResult = deser.readString();
        if (renderResult.failed()) {
            return renderResult.error();
        }
        packet.m_renderType = renderResult.value();
    }

    return packet;
}

// ========== DisplayObjectivePacket ==========

DisplayObjectivePacket::DisplayObjectivePacket(i32 position, const std::string& objectiveName)
    : m_position(position)
    , m_objectiveName(objectiveName)
{
    setPosition(position);
}

void DisplayObjectivePacket::setPosition(i32 position)
{
    // 限制位置范围（0-18）
    m_position = std::clamp(position, 0, 18);
}

void DisplayObjectivePacket::setObjectiveName(const std::string& name)
{
    m_objectiveName = name.substr(0, std::min(name.length(), ScoreboardObjectivePacket::MAX_NAME_LENGTH));
}

void DisplayObjectivePacket::serialize(PacketSerializer& ser) const
{
    ser.writeU8(static_cast<u8>(m_position));
    ser.writeString(m_objectiveName);
}

Result<DisplayObjectivePacket> DisplayObjectivePacket::deserialize(PacketDeserializer& deser)
{
    DisplayObjectivePacket packet;

    auto positionResult = deser.readU8();
    if (positionResult.failed()) {
        return positionResult.error();
    }
    packet.m_position = positionResult.value();

    auto objectiveResult = deser.readString();
    if (objectiveResult.failed()) {
        return objectiveResult.error();
    }
    packet.m_objectiveName = objectiveResult.value();

    return packet;
}

// ========== TeamsPacket ==========

TeamsPacket::TeamsPacket(const std::string& teamName, TeamAction action)
    : m_teamName(teamName)
    , m_action(action)
{
    setTeamName(teamName);
}

void TeamsPacket::setTeamName(const std::string& name)
{
    m_teamName = name.substr(0, std::min(name.length(), MAX_NAME_LENGTH));
}

void TeamsPacket::setDisplayName(const std::string& name)
{
    m_displayName = name.substr(0, std::min(name.length(), MAX_DISPLAY_NAME_LENGTH));
}

void TeamsPacket::setPrefix(const std::string& prefix)
{
    m_prefix = prefix.substr(0, std::min(prefix.length(), MAX_PREFIX_SUFFIX_LENGTH));
}

void TeamsPacket::setSuffix(const std::string& suffix)
{
    m_suffix = suffix.substr(0, std::min(suffix.length(), MAX_PREFIX_SUFFIX_LENGTH));
}

void TeamsPacket::setNameTagVisibility(const std::string& visibility)
{
    // 验证可见性值
    if (visibility == "always" || visibility == "never" || visibility == "hideForOtherTeams" ||
        visibility == "hideForOwnTeam") {
        m_nameTagVisibility = visibility;
    } else {
        m_nameTagVisibility = "always";
    }
}

void TeamsPacket::setCollisionRule(const std::string& rule)
{
    // 验证碰撞规则值
    if (rule == "always" || rule == "never" || rule == "pushOtherTeams" || rule == "pushOwnTeam") {
        m_collisionRule = rule;
    } else {
        m_collisionRule = "always";
    }
}

void TeamsPacket::setColor(const std::string& color)
{
    m_color = color;
}

void TeamsPacket::setAllowFriendlyFire(bool allow) noexcept
{
    if (allow) {
        m_friendlyFlags |= 0x01;
    } else {
        m_friendlyFlags &= ~0x01;
    }
}

void TeamsPacket::setSeeFriendlyInvisibles(bool see) noexcept
{
    if (see) {
        m_friendlyFlags |= 0x02;
    } else {
        m_friendlyFlags &= ~0x02;
    }
}

void TeamsPacket::setPlayers(const std::vector<std::string>& players)
{
    m_players = players;
    // 限制每个玩家名称长度
    for (auto& player : m_players) {
        player = player.substr(0, std::min(player.length(), UpdateScorePacket::MAX_NAME_LENGTH));
    }
}

void TeamsPacket::serialize(PacketSerializer& ser) const
{
    ser.writeString(m_teamName);
    ser.writeU8(static_cast<u8>(m_action));

    if (m_action == TeamAction::Create || m_action == TeamAction::Update) {
        ser.writeString(m_displayName);
        ser.writeString(m_prefix);
        ser.writeString(m_suffix);
        ser.writeString(m_nameTagVisibility);
        ser.writeString(m_collisionRule);
        ser.writeU8(m_friendlyFlags);
        ser.writeString(m_color);
    }

    if (m_action == TeamAction::Create || m_action == TeamAction::AddMember || m_action == TeamAction::RemoveMember) {
        ser.writeVarInt(static_cast<i32>(m_players.size()));
        for (const auto& player : m_players) {
            ser.writeString(player);
        }
    }
}

Result<TeamsPacket> TeamsPacket::deserialize(PacketDeserializer& deser)
{
    TeamsPacket packet;

    auto nameResult = deser.readString();
    if (nameResult.failed()) {
        return nameResult.error();
    }
    packet.m_teamName = nameResult.value();

    auto actionResult = deser.readU8();
    if (actionResult.failed()) {
        return actionResult.error();
    }
    packet.m_action = static_cast<TeamAction>(actionResult.value());

    if (packet.m_action == TeamAction::Create || packet.m_action == TeamAction::Update) {
        auto displayResult = deser.readString();
        if (displayResult.failed()) {
            return displayResult.error();
        }
        packet.m_displayName = displayResult.value();

        auto prefixResult = deser.readString();
        if (prefixResult.failed()) {
            return prefixResult.error();
        }
        packet.m_prefix = prefixResult.value();

        auto suffixResult = deser.readString();
        if (suffixResult.failed()) {
            return suffixResult.error();
        }
        packet.m_suffix = suffixResult.value();

        auto visibilityResult = deser.readString();
        if (visibilityResult.failed()) {
            return visibilityResult.error();
        }
        packet.m_nameTagVisibility = visibilityResult.value();

        auto collisionResult = deser.readString();
        if (collisionResult.failed()) {
            return collisionResult.error();
        }
        packet.m_collisionRule = collisionResult.value();

        auto flagsResult = deser.readU8();
        if (flagsResult.failed()) {
            return flagsResult.error();
        }
        packet.m_friendlyFlags = flagsResult.value();

        auto colorResult = deser.readString();
        if (colorResult.failed()) {
            return colorResult.error();
        }
        packet.m_color = colorResult.value();
    }

    if (packet.m_action == TeamAction::Create || packet.m_action == TeamAction::AddMember ||
        packet.m_action == TeamAction::RemoveMember) {
        auto countResult = deser.readVarInt();
        if (countResult.failed()) {
            return countResult.error();
        }
        i32 count = countResult.value();

        packet.m_players.reserve(static_cast<size_t>(count));
        for (i32 i = 0; i < count; ++i) {
            auto playerResult = deser.readString();
            if (playerResult.failed()) {
                return playerResult.error();
            }
            packet.m_players.push_back(playerResult.value());
        }
    }

    return packet;
}

} // namespace mc::network
