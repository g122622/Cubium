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

#include "Scoreboard.hpp"
#include "ScoreCriteria.hpp"
#include "common/scoreboard/core/ScoreCriteriaRenderType.hpp"
#include "common/scoreboard/core/ScoreObjective.hpp"
#include "common/scoreboard/core/ScorePlayerTeam.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <regex>
#include <string>
#include <utility>
#include <vector>

namespace mc::scoreboard {

// ========== 静态常量 ==========

namespace {

// 目标名称正则表达式：仅允许字母、数字、下划线和连字符
const std::regex s_validNameRegex("^[a-zA-Z0-9_\\-]+$");

// 玩家名称最大长度
constexpr size_t MAX_PLAYER_NAME_LENGTH = 40;

} // namespace

// ========== ScoreComparator ==========

bool ScoreComparator::operator()(const Score* a, const Score* b) const
{
    // 按分数降序排列
    if (a->getScorePoints() != b->getScorePoints()) {
        return a->getScorePoints() > b->getScorePoints();
    }
    // 分数相同时，按名称升序排列
    return a->getPlayerName() < b->getPlayerName();
}

// ========== 构造函数 ==========

Scoreboard::Scoreboard()
{
    // 初始化显示槽位为 nullptr
    m_displaySlots.fill(nullptr);
}

// ========== 目标管理 ==========

ScoreObjective* Scoreboard::addObjective(
    const std::string& name, ScoreCriteria& criteria, std::unique_ptr<text::ITextComponent> displayName)
{
    // 验证名称
    if (name.empty() || name.length() > ScoreObjective::MAX_NAME_LENGTH) {
        return nullptr;
    }

    if (!_isValidObjectiveName(name)) {
        return nullptr;
    }

    // 检查是否已存在
    if (hasObjective(name)) {
        return nullptr;
    }

    // 创建目标
    auto objective = std::make_unique<ScoreObjective>(*this,
        name,
        criteria,
        displayName ? std::move(displayName) : std::make_unique<text::StringTextComponent>(name),
        criteria.getDefaultRenderType());

    ScoreObjective* ptr = objective.get();

    // 添加到映射
    m_objectives[name] = std::move(objective);
    m_objectivesByCriteria[&criteria].push_back(ptr);

    // 触发回调
    onObjectiveAdded(*ptr);

    return ptr;
}

ScoreObjective* Scoreboard::getObjective(const std::string& name)
{
    auto it = m_objectives.find(name);
    return it != m_objectives.end() ? it->second.get() : nullptr;
}

const ScoreObjective* Scoreboard::getObjective(const std::string& name) const
{
    auto it = m_objectives.find(name);
    return it != m_objectives.end() ? it->second.get() : nullptr;
}

bool Scoreboard::hasObjective(const std::string& name) const
{
    return m_objectives.find(name) != m_objectives.end();
}

void Scoreboard::removeObjective(ScoreObjective& objective)
{
    const std::string& name = objective.getName();

    // 从判据映射中移除
    auto criteriaIt = m_objectivesByCriteria.find(&objective.getCriteria());
    if (criteriaIt != m_objectivesByCriteria.end()) {
        auto& objectives = criteriaIt->second;
        objectives.erase(std::remove(objectives.begin(), objectives.end(), &objective), objectives.end());
    }

    // 从显示槽位中移除
    for (size_t i = 0; i < m_displaySlots.size(); ++i) {
        if (m_displaySlots[i] == &objective) {
            m_displaySlots[i] = nullptr;
            onDisplaySlotChanged(static_cast<DisplaySlot>(i), nullptr);
        }
    }

    // 移除所有相关的分数
    for (auto& [playerName, scores] : m_playerScores) {
        auto scoreIt = scores.find(name);
        if (scoreIt != scores.end()) {
            onPlayerScoreRemoved(playerName, objective);
            scores.erase(scoreIt);
        }
    }

    // 触发回调
    onObjectiveRemoved(objective);

    // 从目标映射中移除
    m_objectives.erase(name);
}

std::vector<ScoreObjective*> Scoreboard::getObjectives()
{
    std::vector<ScoreObjective*> result;
    result.reserve(m_objectives.size());
    for (auto& [name, objective] : m_objectives) {
        result.push_back(objective.get());
    }
    return result;
}

std::vector<const ScoreObjective*> Scoreboard::getObjectives() const
{
    std::vector<const ScoreObjective*> result;
    result.reserve(m_objectives.size());
    for (const auto& [name, objective] : m_objectives) {
        result.push_back(objective.get());
    }
    return result;
}

std::vector<ScoreObjective*> Scoreboard::getObjectivesByCriteria(ScoreCriteria& criteria)
{
    auto it = m_objectivesByCriteria.find(&criteria);
    if (it != m_objectivesByCriteria.end()) {
        return it->second;
    }
    return {};
}

// ========== 分数管理 ==========

Score* Scoreboard::getOrCreateScore(const std::string& playerName, ScoreObjective& objective)
{
    // 验证玩家名称
    if (playerName.empty() || playerName.length() > MAX_PLAYER_NAME_LENGTH) {
        return nullptr;
    }

    // 获取或创建玩家分数映射
    auto& playerScores = m_playerScores[playerName];

    // 获取或创建分数
    const std::string& objName = objective.getName();
    auto it = playerScores.find(objName);
    if (it != playerScores.end()) {
        return it->second.get();
    }

    // 创建新分数
    auto score = std::make_unique<Score>(*this, objective, playerName);
    Score* ptr = score.get();
    playerScores[objName] = std::move(score);

    return ptr;
}

Score* Scoreboard::getScore(const std::string& playerName, ScoreObjective& objective)
{
    auto playerIt = m_playerScores.find(playerName);
    if (playerIt == m_playerScores.end()) {
        return nullptr;
    }

    auto scoreIt = playerIt->second.find(objective.getName());
    return scoreIt != playerIt->second.end() ? scoreIt->second.get() : nullptr;
}

const Score* Scoreboard::getScore(const std::string& playerName, ScoreObjective& objective) const
{
    auto playerIt = m_playerScores.find(playerName);
    if (playerIt == m_playerScores.end()) {
        return nullptr;
    }

    auto scoreIt = playerIt->second.find(objective.getName());
    return scoreIt != playerIt->second.end() ? scoreIt->second.get() : nullptr;
}

void Scoreboard::removeScore(const std::string& playerName, ScoreObjective* objective)
{
    auto playerIt = m_playerScores.find(playerName);
    if (playerIt == m_playerScores.end()) {
        return;
    }

    if (objective) {
        // 移除指定目标的分数
        auto scoreIt = playerIt->second.find(objective->getName());
        if (scoreIt != playerIt->second.end()) {
            onPlayerScoreRemoved(playerName, *objective);
            playerIt->second.erase(scoreIt);
        }
    } else {
        // 移除该玩家的所有分数
        onPlayerRemoved(playerName);
        m_playerScores.erase(playerIt);
    }
}

bool Scoreboard::entityHasObjective(const std::string& playerName, ScoreObjective& objective) const
{
    auto playerIt = m_playerScores.find(playerName);
    if (playerIt == m_playerScores.end()) {
        return false;
    }
    return playerIt->second.find(objective.getName()) != playerIt->second.end();
}

std::vector<Score*> Scoreboard::getSortedScores(ScoreObjective& objective)
{
    std::vector<Score*> scores;
    const std::string& objName = objective.getName();

    for (auto& [playerName, playerScores] : m_playerScores) {
        auto it = playerScores.find(objName);
        if (it != playerScores.end()) {
            scores.push_back(it->second.get());
        }
    }

    // 按分数排序
    std::sort(scores.begin(), scores.end(), ScoreComparator());

    return scores;
}

std::vector<std::string> Scoreboard::getPlayerObjectives(const std::string& playerName) const
{
    std::vector<std::string> result;

    auto playerIt = m_playerScores.find(playerName);
    if (playerIt != m_playerScores.end()) {
        result.reserve(playerIt->second.size());
        for (const auto& [objName, score] : playerIt->second) {
            result.push_back(objName);
        }
    }

    return result;
}

// ========== 显示槽位 ==========

void Scoreboard::setObjectiveInDisplaySlot(DisplaySlot slot, ScoreObjective* objective)
{
    const size_t index = static_cast<size_t>(slot);
    if (index >= m_displaySlots.size()) {
        return;
    }

    m_displaySlots[index] = objective;

    onDisplaySlotChanged(slot, objective);
}

ScoreObjective* Scoreboard::getObjectiveInDisplaySlot(DisplaySlot slot)
{
    const size_t index = static_cast<size_t>(slot);
    if (index >= m_displaySlots.size()) {
        return nullptr;
    }
    return m_displaySlots[index];
}

const ScoreObjective* Scoreboard::getObjectiveInDisplaySlot(DisplaySlot slot) const
{
    const size_t index = static_cast<size_t>(slot);
    if (index >= m_displaySlots.size()) {
        return nullptr;
    }
    return m_displaySlots[index];
}

std::vector<DisplaySlot> Scoreboard::getDisplaySlotsForObject(ScoreObjective& objective) const
{
    std::vector<DisplaySlot> slots;
    for (size_t i = 0; i < m_displaySlots.size(); ++i) {
        if (m_displaySlots[i] == &objective) {
            slots.push_back(static_cast<DisplaySlot>(i));
        }
    }
    return slots;
}

// ========== 队伍管理 ==========

ScorePlayerTeam* Scoreboard::createTeam(const std::string& name)
{
    // 验证名称
    if (name.empty() || name.length() > ScorePlayerTeam::MAX_NAME_LENGTH) {
        return nullptr;
    }

    if (!_isValidObjectiveName(name)) {
        return nullptr;
    }

    // 检查是否已存在
    if (hasTeam(name)) {
        return nullptr;
    }

    // 创建队伍
    auto team = std::make_unique<ScorePlayerTeam>(*this, name);
    ScorePlayerTeam* ptr = team.get();

    m_teams[name] = std::move(team);

    // 触发回调
    onTeamAdded(*ptr);

    return ptr;
}

void Scoreboard::removeTeam(ScorePlayerTeam& team)
{
    const std::string& name = team.getName();

    // 清空队伍成员（从 m_teamMemberships 中移除）
    const auto& members = team.getMembers();
    for (const auto& member : members) {
        m_teamMemberships.erase(member);
    }
    team.clearMembers();

    // 从队伍映射中移除
    m_teams.erase(name);

    // 触发回调
    onTeamRemoved(team);
}

ScorePlayerTeam* Scoreboard::getTeam(const std::string& name)
{
    auto it = m_teams.find(name);
    return it != m_teams.end() ? it->second.get() : nullptr;
}

const ScorePlayerTeam* Scoreboard::getTeam(const std::string& name) const
{
    auto it = m_teams.find(name);
    return it != m_teams.end() ? it->second.get() : nullptr;
}

bool Scoreboard::hasTeam(const std::string& name) const
{
    return m_teams.find(name) != m_teams.end();
}

bool Scoreboard::addPlayerToTeam(const std::string& playerName, ScorePlayerTeam& team)
{
    // 检查玩家是否已在其他队伍
    auto currentTeam = getPlayersTeam(playerName);
    if (currentTeam == &team) {
        return false; // 已经在该队伍
    }

    // 如果已在其他队伍，先移出
    if (currentTeam) {
        removePlayerFromTeam(playerName, *currentTeam);
    }

    // 添加到新队伍
    if (team.addMember(playerName)) {
        m_teamMemberships[playerName] = &team;
        return true;
    }

    return false;
}

bool Scoreboard::removePlayerFromTeam(const std::string& playerName, ScorePlayerTeam& team)
{
    if (team.removeMember(playerName)) {
        m_teamMemberships.erase(playerName);
        return true;
    }
    return false;
}

ScorePlayerTeam* Scoreboard::getPlayersTeam(const std::string& playerName) const
{
    auto it = m_teamMemberships.find(playerName);
    return it != m_teamMemberships.end() ? it->second : nullptr;
}

std::vector<ScorePlayerTeam*> Scoreboard::getTeams()
{
    std::vector<ScorePlayerTeam*> result;
    result.reserve(m_teams.size());
    for (auto& [name, team] : m_teams) {
        result.push_back(team.get());
    }
    return result;
}

std::vector<const ScorePlayerTeam*> Scoreboard::getTeams() const
{
    std::vector<const ScorePlayerTeam*> result;
    result.reserve(m_teams.size());
    for (const auto& [name, team] : m_teams) {
        result.push_back(team.get());
    }
    return result;
}

// ========== 回调默认实现 ==========

void Scoreboard::onObjectiveAdded(ScoreObjective& /*objective*/)
{
    // 默认空实现
}

void Scoreboard::onObjectiveRemoved(ScoreObjective& /*objective*/)
{
    // 默认空实现
}

void Scoreboard::onObjectiveChanged(ScoreObjective& /*objective*/)
{
    // 默认空实现
}

void Scoreboard::onScoreChanged(Score& /*score*/)
{
    // 默认空实现
}

void Scoreboard::onScoreRemoved(Score& /*score*/)
{
    // 默认空实现
}

void Scoreboard::onPlayerRemoved(const std::string& /*playerName*/)
{
    // 默认空实现
}

void Scoreboard::onPlayerScoreRemoved(const std::string& /*playerName*/, ScoreObjective& /*objective*/)
{
    // 默认空实现
}

void Scoreboard::onTeamAdded(ScorePlayerTeam& /*team*/)
{
    // 默认空实现
}

void Scoreboard::onTeamChanged(ScorePlayerTeam& /*team*/)
{
    // 默认空实现
}

void Scoreboard::onTeamRemoved(ScorePlayerTeam& /*team*/)
{
    // 默认空实现
}

void Scoreboard::onDisplaySlotChanged(DisplaySlot /*slot*/, ScoreObjective* /*objective*/)
{
    // 默认空实现
}

// ========== 批判据查询 ==========

void Scoreboard::forAllObjectives(
    ScoreCriteria& criteria, const std::string& playerName, std::function<void(Score&)> action)
{
    auto it = m_objectivesByCriteria.find(&criteria);
    if (it == m_objectivesByCriteria.end()) {
        return;
    }

    for (ScoreObjective* objective : it->second) {
        Score* score = getScore(playerName, *objective);
        if (score) {
            action(*score);
        }
    }
}

// ========== 静态辅助方法 ==========

bool Scoreboard::_isValidObjectiveName(const std::string& name)
{
    return std::regex_match(name, s_validNameRegex);
}

bool Scoreboard::_isValidPlayerName(const std::string& name)
{
    // 玩家名称可以包含更多字符
    return !name.empty() && name.length() <= MAX_PLAYER_NAME_LENGTH;
}

} // namespace mc::scoreboard
