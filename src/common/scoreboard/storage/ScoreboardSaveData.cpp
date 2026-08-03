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

#include "common/scoreboard/storage/ScoreboardSaveData.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/scoreboard/core/ScoreCriteria.hpp"
#include "common/scoreboard/core/ScoreCriteriaRenderType.hpp"
#include "common/scoreboard/core/TeamEnums.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/TextStyle.hpp"
#include <cstddef>
#include <ios>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc::scoreboard {

// ========== ObjectiveData ==========

nbt::tags::compound_tag ScoreboardSaveData::ObjectiveData::toNbt() const
{
    nbt::tags::compound_tag tag;
    tag.put("Name", name);
    tag.put("CriteriaName", criteriaName);
    tag.put("DisplayName", displayName);
    tag.put("RenderType", renderType);
    return tag;
}

Result<ScoreboardSaveData::ObjectiveData> ScoreboardSaveData::ObjectiveData::fromNbt(const nbt::tags::compound_tag& tag)
{
    ObjectiveData data;

    auto nameIt = tag.value.find("Name");
    if (nameIt == tag.value.end()) {
        return Error(ErrorCode::InvalidData, "Missing Name in ObjectiveData");
    }
    auto* nameTag = dynamic_cast<const nbt::tags::string_tag*>(nameIt->second.get());
    if (!nameTag) {
        return Error(ErrorCode::InvalidData, "Name is not a string in ObjectiveData");
    }
    data.name = nameTag->value;

    auto criteriaIt = tag.value.find("CriteriaName");
    if (criteriaIt == tag.value.end()) {
        return Error(ErrorCode::InvalidData, "Missing CriteriaName in ObjectiveData");
    }
    auto* criteriaTag = dynamic_cast<const nbt::tags::string_tag*>(criteriaIt->second.get());
    if (!criteriaTag) {
        return Error(ErrorCode::InvalidData, "CriteriaName is not a string in ObjectiveData");
    }
    data.criteriaName = criteriaTag->value;

    auto displayIt = tag.value.find("DisplayName");
    if (displayIt != tag.value.end()) {
        auto* displayTag = dynamic_cast<const nbt::tags::string_tag*>(displayIt->second.get());
        if (displayTag) {
            data.displayName = displayTag->value;
        } else {
            data.displayName = data.name;
        }
    } else {
        data.displayName = data.name;
    }

    auto renderIt = tag.value.find("RenderType");
    if (renderIt != tag.value.end()) {
        auto* renderTag = dynamic_cast<const nbt::tags::string_tag*>(renderIt->second.get());
        if (renderTag) {
            data.renderType = renderTag->value;
        } else {
            data.renderType = "integer";
        }
    } else {
        data.renderType = "integer";
    }

    return data;
}

// ========== ScoreData ==========

nbt::tags::compound_tag ScoreboardSaveData::ScoreData::toNbt() const
{
    nbt::tags::compound_tag tag;
    tag.put("Name", playerName);
    tag.put("Objective", objectiveName);
    tag.put("Score", score);
    // 使用 byte_tag 存储 bool
    auto lockedTag = std::make_unique<nbt::tags::byte_tag>();
    lockedTag->value = locked ? 1 : 0;
    tag.value.emplace("Locked", std::move(lockedTag));
    return tag;
}

Result<ScoreboardSaveData::ScoreData> ScoreboardSaveData::ScoreData::fromNbt(const nbt::tags::compound_tag& tag)
{
    ScoreData data;

    auto nameIt = tag.value.find("Name");
    if (nameIt == tag.value.end()) {
        return Error(ErrorCode::InvalidData, "Missing Name in ScoreData");
    }
    auto* nameTag = dynamic_cast<const nbt::tags::string_tag*>(nameIt->second.get());
    if (!nameTag) {
        return Error(ErrorCode::InvalidData, "Name is not a string in ScoreData");
    }
    data.playerName = nameTag->value;

    auto objectiveIt = tag.value.find("Objective");
    if (objectiveIt == tag.value.end()) {
        return Error(ErrorCode::InvalidData, "Missing Objective in ScoreData");
    }
    auto* objectiveTag = dynamic_cast<const nbt::tags::string_tag*>(objectiveIt->second.get());
    if (!objectiveTag) {
        return Error(ErrorCode::InvalidData, "Objective is not a string in ScoreData");
    }
    data.objectiveName = objectiveTag->value;

    auto scoreIt = tag.value.find("Score");
    if (scoreIt != tag.value.end()) {
        auto* scoreTag = dynamic_cast<const nbt::tags::int_tag*>(scoreIt->second.get());
        if (scoreTag) {
            data.score = scoreTag->value;
        }
    }

    auto lockedIt = tag.value.find("Locked");
    if (lockedIt != tag.value.end()) {
        auto* lockedTag = dynamic_cast<const nbt::tags::byte_tag*>(lockedIt->second.get());
        if (lockedTag) {
            data.locked = (lockedTag->value != 0);
        }
    }

    return data;
}

// ========== TeamData ==========

nbt::tags::compound_tag ScoreboardSaveData::TeamData::toNbt() const
{
    nbt::tags::compound_tag tag;
    tag.put("TeamName", name);
    tag.put("DisplayName", displayName);
    tag.put("Prefix", prefix);
    tag.put("Suffix", suffix);
    tag.put("TeamColor", color);
    tag.put("NameTagVisibility", nameTagVisibility);
    tag.put("DeathMessageVisibility", deathMessageVisibility);
    tag.put("CollisionRule", collisionRule);

    // 使用 byte_tag 存储 bool
    auto friendlyFireTag = std::make_unique<nbt::tags::byte_tag>();
    friendlyFireTag->value = allowFriendlyFire ? 1 : 0;
    tag.value.emplace("AllowFriendlyFire", std::move(friendlyFireTag));

    auto seeInvisTag = std::make_unique<nbt::tags::byte_tag>();
    seeInvisTag->value = seeFriendlyInvisibles ? 1 : 0;
    tag.value.emplace("SeeFriendlyInvisibles", std::move(seeInvisTag));

    // 使用 string_list_tag 存储成员列表
    auto membersList = std::make_unique<nbt::tags::string_list_tag>();
    membersList->value.reserve(members.size());
    for (const auto& member : members) {
        membersList->value.push_back(member);
    }
    tag.value.emplace("Members", std::move(membersList));

    return tag;
}

Result<ScoreboardSaveData::TeamData> ScoreboardSaveData::TeamData::fromNbt(const nbt::tags::compound_tag& tag)
{
    TeamData data;

    auto nameIt = tag.value.find("TeamName");
    if (nameIt == tag.value.end()) {
        return Error(ErrorCode::InvalidData, "Missing TeamName in TeamData");
    }
    auto* nameTag = dynamic_cast<const nbt::tags::string_tag*>(nameIt->second.get());
    if (!nameTag) {
        return Error(ErrorCode::InvalidData, "TeamName is not a string in TeamData");
    }
    data.name = nameTag->value;

    auto displayIt = tag.value.find("DisplayName");
    if (displayIt != tag.value.end()) {
        auto* displayTag = dynamic_cast<const nbt::tags::string_tag*>(displayIt->second.get());
        if (displayTag) {
            data.displayName = displayTag->value;
        } else {
            data.displayName = data.name;
        }
    } else {
        data.displayName = data.name;
    }

    auto prefixIt = tag.value.find("Prefix");
    if (prefixIt != tag.value.end()) {
        auto* prefixTag = dynamic_cast<const nbt::tags::string_tag*>(prefixIt->second.get());
        if (prefixTag) {
            data.prefix = prefixTag->value;
        }
    }

    auto suffixIt = tag.value.find("Suffix");
    if (suffixIt != tag.value.end()) {
        auto* suffixTag = dynamic_cast<const nbt::tags::string_tag*>(suffixIt->second.get());
        if (suffixTag) {
            data.suffix = suffixTag->value;
        }
    }

    auto colorIt = tag.value.find("TeamColor");
    if (colorIt != tag.value.end()) {
        auto* colorTag = dynamic_cast<const nbt::tags::string_tag*>(colorIt->second.get());
        if (colorTag) {
            data.color = colorTag->value;
        } else {
            data.color = "white";
        }
    } else {
        data.color = "white";
    }

    auto nameTagVisIt = tag.value.find("NameTagVisibility");
    if (nameTagVisIt != tag.value.end()) {
        auto* nameTagVisTag = dynamic_cast<const nbt::tags::string_tag*>(nameTagVisIt->second.get());
        if (nameTagVisTag) {
            data.nameTagVisibility = nameTagVisTag->value;
        } else {
            data.nameTagVisibility = "always";
        }
    } else {
        data.nameTagVisibility = "always";
    }

    auto deathMsgVisIt = tag.value.find("DeathMessageVisibility");
    if (deathMsgVisIt != tag.value.end()) {
        auto* deathMsgVisTag = dynamic_cast<const nbt::tags::string_tag*>(deathMsgVisIt->second.get());
        if (deathMsgVisTag) {
            data.deathMessageVisibility = deathMsgVisTag->value;
        } else {
            data.deathMessageVisibility = "always";
        }
    } else {
        data.deathMessageVisibility = "always";
    }

    auto collisionIt = tag.value.find("CollisionRule");
    if (collisionIt != tag.value.end()) {
        auto* collisionTag = dynamic_cast<const nbt::tags::string_tag*>(collisionIt->second.get());
        if (collisionTag) {
            data.collisionRule = collisionTag->value;
        } else {
            data.collisionRule = "always";
        }
    } else {
        data.collisionRule = "always";
    }

    auto friendlyFireIt = tag.value.find("AllowFriendlyFire");
    if (friendlyFireIt != tag.value.end()) {
        auto* friendlyFireTag = dynamic_cast<const nbt::tags::byte_tag*>(friendlyFireIt->second.get());
        if (friendlyFireTag) {
            data.allowFriendlyFire = (friendlyFireTag->value != 0);
        } else {
            data.allowFriendlyFire = true;
        }
    } else {
        data.allowFriendlyFire = true;
    }

    auto seeInvisIt = tag.value.find("SeeFriendlyInvisibles");
    if (seeInvisIt != tag.value.end()) {
        // 尝试解析为字节
        auto* seeInvisByteTag = dynamic_cast<const nbt::tags::byte_tag*>(seeInvisIt->second.get());
        if (seeInvisByteTag) {
            data.seeFriendlyInvisibles = (seeInvisByteTag->value != 0);
        } else {
            // 尝试解析为字符串（旧格式）
            auto* seeInvisTag = dynamic_cast<const nbt::tags::string_tag*>(seeInvisIt->second.get());
            if (seeInvisTag) {
                data.seeFriendlyInvisibles = (seeInvisTag->value != "false");
            } else {
                data.seeFriendlyInvisibles = true;
            }
        }
    } else {
        data.seeFriendlyInvisibles = true;
    }

    auto membersIt = tag.value.find("Members");
    if (membersIt != tag.value.end()) {
        auto* membersTag = dynamic_cast<const nbt::tags::string_list_tag*>(membersIt->second.get());
        if (membersTag) {
            for (const auto& member : membersTag->value) {
                data.members.push_back(member);
            }
        }
    }

    return data;
}

// ========== DisplaySlotData ==========

nbt::tags::compound_tag ScoreboardSaveData::DisplaySlotData::toNbt() const
{
    nbt::tags::compound_tag tag;
    tag.put("Slot", slot);
    tag.put("Objective", objectiveName);
    return tag;
}

Result<ScoreboardSaveData::DisplaySlotData> ScoreboardSaveData::DisplaySlotData::fromNbt(
    const nbt::tags::compound_tag& tag)
{
    DisplaySlotData data;

    auto slotIt = tag.value.find("Slot");
    if (slotIt == tag.value.end()) {
        return Error(ErrorCode::InvalidData, "Missing Slot in DisplaySlotData");
    }
    auto* slotTag = dynamic_cast<const nbt::tags::int_tag*>(slotIt->second.get());
    if (!slotTag) {
        return Error(ErrorCode::InvalidData, "Slot is not an int in DisplaySlotData");
    }
    data.slot = slotTag->value;

    auto objectiveIt = tag.value.find("Objective");
    if (objectiveIt == tag.value.end()) {
        return Error(ErrorCode::InvalidData, "Missing Objective in DisplaySlotData");
    }
    auto* objectiveTag = dynamic_cast<const nbt::tags::string_tag*>(objectiveIt->second.get());
    if (!objectiveTag) {
        return Error(ErrorCode::InvalidData, "Objective is not a string in DisplaySlotData");
    }
    data.objectiveName = objectiveTag->value;

    return data;
}

// ========== ScoreboardSaveData ==========

ScoreboardSaveData ScoreboardSaveData::fromScoreboard(const Scoreboard& scoreboard)
{
    ScoreboardSaveData data;

    // 注意：由于 getSortedScores 需要非 const 引用，我们需要 const_cast
    // 这是安全的，因为 getSortedScores 实际上不会修改目标
    Scoreboard& mutableScoreboard = const_cast<Scoreboard&>(scoreboard);

    // 提取所有目标
    auto objectives = mutableScoreboard.getObjectives();
    for (auto* objective : objectives) {
        ObjectiveData objData;
        objData.name = objective->getName();
        objData.criteriaName = objective->getCriteria().getName();

        // 获取显示名称，将 ITextComponent 序列化为 JSON 字符串
        if (auto* displayName = objective->getDisplayName()) {
            objData.displayName = displayName->toJson().dump();
        } else {
            objData.displayName = objective->getName();
        }

        objData.renderType = renderTypeToString(objective->getRenderType());
        data.addObjective(std::move(objData));
    }

    // 提取所有分数
    for (auto* objective : objectives) {
        auto scores = mutableScoreboard.getSortedScores(*objective);
        for (auto* score : scores) {
            ScoreData scoreData;
            scoreData.playerName = score->getPlayerName();
            scoreData.objectiveName = objective->getName();
            scoreData.score = score->getScorePoints();
            scoreData.locked = score->isLocked();
            data.addScore(std::move(scoreData));
        }
    }

    // 提取所有队伍
    auto teams = scoreboard.getTeams();
    for (auto* team : teams) {
        TeamData teamData;
        teamData.name = team->getName();

        // 将 ITextComponent 序列化为 JSON 字符串
        if (auto* displayName = team->getDisplayName()) {
            teamData.displayName = displayName->toJson().dump();
        } else {
            teamData.displayName = team->getName();
        }

        if (auto* prefix = team->getPrefix()) {
            teamData.prefix = prefix->toJson().dump();
        }

        if (auto* suffix = team->getSuffix()) {
            teamData.suffix = suffix->toJson().dump();
        }

        teamData.color = text::toName(team->getColor());
        teamData.nameTagVisibility = teamVisibilityToString(team->getNameTagVisibility());
        teamData.deathMessageVisibility = teamVisibilityToString(team->getDeathMessageVisibility());
        teamData.collisionRule = teamCollisionRuleToString(team->getCollisionRule());
        teamData.allowFriendlyFire = team->getAllowFriendlyFire();
        teamData.seeFriendlyInvisibles = team->canSeeFriendlyInvisibles();

        const auto& members = team->getMembers();
        teamData.members.assign(members.begin(), members.end());

        data.addTeam(std::move(teamData));
    }

    // 提取显示槽位
    for (size_t i = 0; i < DISPLAY_SLOT_COUNT; ++i) {
        auto slot = static_cast<DisplaySlot>(i);
        if (auto* objective = scoreboard.getObjectiveInDisplaySlot(slot)) {
            DisplaySlotData slotData;
            slotData.slot = static_cast<i32>(i);
            slotData.objectiveName = objective->getName();
            data.addDisplaySlot(std::move(slotData));
        }
    }

    return data;
}

Result<void> ScoreboardSaveData::applyToScoreboard(Scoreboard& scoreboard) const
{
    auto& registry = ScoreCriteriaRegistry::instance();

    // 添加目标
    for (const auto& objData : m_objectives) {
        auto* criteria = registry.getCriteria(objData.criteriaName);
        if (!criteria) {
            // 使用 dummy 作为默认判据
            criteria = registry.getCriteria("dummy");
            if (!criteria) {
                continue; // 跳过无法创建的目标
            }
        }

        // 从 JSON 字符串创建显示名称，或使用名称作为纯文本
        std::unique_ptr<text::ITextComponent> displayName;
        if (!objData.displayName.empty()) {
            try {
                nlohmann::json json = nlohmann::json::parse(objData.displayName);
                displayName = text::ITextComponent::fromJson(json);
            }
            catch (const nlohmann::json::exception&) {
                // JSON 解析失败，使用纯文本
                displayName = std::make_unique<text::StringTextComponent>(objData.displayName);
            }
        }
        if (!displayName) {
            displayName = std::make_unique<text::StringTextComponent>(objData.name);
        }

        auto* objective = scoreboard.addObjective(objData.name, *criteria, std::move(displayName));
        if (objective) {
            auto renderType = renderTypeFromString(objData.renderType);
            objective->setRenderType(renderType);
        }
    }

    // 添加分数
    for (const auto& scoreData : m_scores) {
        auto* objective = scoreboard.getObjective(scoreData.objectiveName);
        if (!objective) {
            continue;
        }

        auto* score = scoreboard.getOrCreateScore(scoreData.playerName, *objective);
        if (score) {
            score->setScorePoints(scoreData.score);
            score->setLocked(scoreData.locked);
        }
    }

    // 添加队伍
    for (const auto& teamData : m_teams) {
        auto* team = scoreboard.createTeam(teamData.name);
        if (!team) {
            continue;
        }

        // 从 JSON 字符串创建显示名称
        if (!teamData.displayName.empty()) {
            try {
                nlohmann::json json = nlohmann::json::parse(teamData.displayName);
                auto displayName = text::ITextComponent::fromJson(json);
                if (displayName) {
                    team->setDisplayName(std::move(displayName));
                }
            }
            catch (const nlohmann::json::exception&) {
                // JSON 解析失败，使用纯文本
                team->setDisplayName(std::make_unique<text::StringTextComponent>(teamData.displayName));
            }
        }

        // 从 JSON 字符串创建前缀
        if (!teamData.prefix.empty()) {
            try {
                nlohmann::json json = nlohmann::json::parse(teamData.prefix);
                auto prefix = text::ITextComponent::fromJson(json);
                if (prefix) {
                    team->setPrefix(std::move(prefix));
                }
            }
            catch (const nlohmann::json::exception&) {
                // JSON 解析失败，使用纯文本
                team->setPrefix(std::make_unique<text::StringTextComponent>(teamData.prefix));
            }
        }

        // 从 JSON 字符串创建后缀
        if (!teamData.suffix.empty()) {
            try {
                nlohmann::json json = nlohmann::json::parse(teamData.suffix);
                auto suffix = text::ITextComponent::fromJson(json);
                if (suffix) {
                    team->setSuffix(std::move(suffix));
                }
            }
            catch (const nlohmann::json::exception&) {
                // JSON 解析失败，使用纯文本
                team->setSuffix(std::make_unique<text::StringTextComponent>(teamData.suffix));
            }
        }

        auto color = text::fromName(teamData.color);
        team->setColor(color);

        team->setNameTagVisibility(teamVisibilityFromString(teamData.nameTagVisibility));
        team->setDeathMessageVisibility(teamVisibilityFromString(teamData.deathMessageVisibility));
        team->setCollisionRule(teamCollisionRuleFromString(teamData.collisionRule));
        team->setAllowFriendlyFire(teamData.allowFriendlyFire);
        team->setSeeFriendlyInvisibles(teamData.seeFriendlyInvisibles);

        for (const auto& member : teamData.members) {
            scoreboard.addPlayerToTeam(member, *team);
        }
    }

    // 设置显示槽位
    for (const auto& slotData : m_displaySlots) {
        if (slotData.slot < 0 || slotData.slot >= static_cast<i32>(DISPLAY_SLOT_COUNT)) {
            continue;
        }

        auto* objective = scoreboard.getObjective(slotData.objectiveName);
        if (objective) {
            auto slot = static_cast<DisplaySlot>(slotData.slot);
            scoreboard.setObjectiveInDisplaySlot(slot, objective);
        }
    }

    return {};
}

nbt::tags::compound_tag ScoreboardSaveData::toNbt() const
{
    nbt::tags::compound_tag root;

    // 目标列表
    auto objectivesList = std::make_unique<nbt::tags::compound_list_tag>();
    objectivesList->value.reserve(m_objectives.size());
    for (const auto& obj : m_objectives) {
        objectivesList->value.push_back(obj.toNbt());
    }
    root.value.emplace("Objectives", std::move(objectivesList));

    // 分数列表
    auto scoresList = std::make_unique<nbt::tags::compound_list_tag>();
    scoresList->value.reserve(m_scores.size());
    for (const auto& score : m_scores) {
        scoresList->value.push_back(score.toNbt());
    }
    root.value.emplace("PlayerScores", std::move(scoresList));

    // 队伍列表
    auto teamsList = std::make_unique<nbt::tags::compound_list_tag>();
    teamsList->value.reserve(m_teams.size());
    for (const auto& team : m_teams) {
        teamsList->value.push_back(team.toNbt());
    }
    root.value.emplace("Teams", std::move(teamsList));

    // 显示槽位
    auto displaySlotsList = std::make_unique<nbt::tags::compound_list_tag>();
    displaySlotsList->value.reserve(m_displaySlots.size());
    for (const auto& slot : m_displaySlots) {
        displaySlotsList->value.push_back(slot.toNbt());
    }
    root.value.emplace("DisplaySlots", std::move(displaySlotsList));

    return root;
}

Result<ScoreboardSaveData> ScoreboardSaveData::fromNbt(const nbt::tags::compound_tag& tag)
{
    ScoreboardSaveData data;

    // 读取目标
    auto objectivesIt = tag.value.find("Objectives");
    if (objectivesIt != tag.value.end()) {
        auto* objectivesTag = dynamic_cast<const nbt::tags::compound_list_tag*>(objectivesIt->second.get());
        if (objectivesTag) {
            for (const auto& objTag : objectivesTag->value) {
                auto result = ObjectiveData::fromNbt(objTag);
                if (result.success()) {
                    data.addObjective(std::move(result.value()));
                }
            }
        }
    }

    // 读取分数
    auto scoresIt = tag.value.find("PlayerScores");
    if (scoresIt != tag.value.end()) {
        auto* scoresTag = dynamic_cast<const nbt::tags::compound_list_tag*>(scoresIt->second.get());
        if (scoresTag) {
            for (const auto& scoreTag : scoresTag->value) {
                auto result = ScoreData::fromNbt(scoreTag);
                if (result.success()) {
                    data.addScore(std::move(result.value()));
                }
            }
        }
    }

    // 读取队伍
    auto teamsIt = tag.value.find("Teams");
    if (teamsIt != tag.value.end()) {
        auto* teamsTag = dynamic_cast<const nbt::tags::compound_list_tag*>(teamsIt->second.get());
        if (teamsTag) {
            for (const auto& teamTag : teamsTag->value) {
                auto result = TeamData::fromNbt(teamTag);
                if (result.success()) {
                    data.addTeam(std::move(result.value()));
                }
            }
        }
    }

    // 读取显示槽位
    auto displaySlotsIt = tag.value.find("DisplaySlots");
    if (displaySlotsIt != tag.value.end()) {
        auto* displaySlotsTag = dynamic_cast<const nbt::tags::compound_list_tag*>(displaySlotsIt->second.get());
        if (displaySlotsTag) {
            for (const auto& slotTag : displaySlotsTag->value) {
                auto result = DisplaySlotData::fromNbt(slotTag);
                if (result.success()) {
                    data.addDisplaySlot(std::move(result.value()));
                }
            }
        }
    }

    return data;
}

// ========== 辅助函数：NBT序列化 ==========

static Result<std::vector<u8>> serializeNbtToBytes(const nbt::tags::compound_tag& tag)
{
    std::ostringstream oss(std::ios::binary);
    oss << nbt::Contexts::java;
    nbt::operator<<(oss, tag);
    if (!oss) {
        return Error(ErrorCode::FileWriteFailed, "Failed to serialize NBT");
    }
    std::string str = oss.str();
    return std::vector<u8>(str.begin(), str.end());
}

static Result<nbt::tags::compound_tag> deserializeNbtFromBytes(const std::vector<u8>& data)
{
    std::istringstream iss(std::string(data.begin(), data.end()), std::ios::binary);
    iss >> nbt::Contexts::java;
    auto root = nbt::tags::compound_tag::read(iss);
    if (!root) {
        return Error(ErrorCode::InvalidData, "Failed to deserialize NBT");
    }
    return std::move(*root);
}

Result<std::vector<u8>> ScoreboardSaveData::serialize() const
{
    auto nbt = toNbt();
    return serializeNbtToBytes(nbt);
}

Result<ScoreboardSaveData> ScoreboardSaveData::deserialize(const std::vector<u8>& data)
{
    auto nbtResult = deserializeNbtFromBytes(data);
    if (nbtResult.failed()) {
        return nbtResult.error();
    }

    return fromNbt(nbtResult.value());
}

} // namespace mc::scoreboard
