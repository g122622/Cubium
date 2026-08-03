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

#include "ScoreCriteria.hpp"
#include "common/core/Result.hpp"
#include "common/scoreboard/criteria/DeathCountCriteria.hpp"
#include "common/scoreboard/criteria/DummyCriteria.hpp"
#include "common/scoreboard/criteria/KillCountCriteria.hpp"
#include "common/scoreboard/criteria/ReadOnlyCriteria.hpp"
#include "common/scoreboard/criteria/TeamKillCriteria.hpp"
#include "common/scoreboard/criteria/TriggerCriteria.hpp"
#include <cassert>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc::scoreboard {

// ========== ScoreCriteriaRegistry ==========

ScoreCriteriaRegistry& ScoreCriteriaRegistry::instance()
{
    static ScoreCriteriaRegistry s_instance;
    return s_instance;
}

Result<void> ScoreCriteriaRegistry::registerCriteria(std::unique_ptr<ScoreCriteria> criteria)
{
    if (!criteria) {
        return Error(ErrorCode::InvalidArgument, "Criteria cannot be null");
    }

    const std::string& name = criteria->getName();
    if (name.empty()) {
        return Error(ErrorCode::InvalidArgument, "Criteria name cannot be empty");
    }

    if (hasCriteria(name)) {
        return Error(ErrorCode::AlreadyExists, "Criteria already exists: " + name);
    }

    m_criteria[name] = std::move(criteria);
    return {};
}

ScoreCriteria* ScoreCriteriaRegistry::getCriteria(const std::string& name)
{
    auto it = m_criteria.find(name);
    return it != m_criteria.end() ? it->second.get() : nullptr;
}

const ScoreCriteria* ScoreCriteriaRegistry::getCriteria(const std::string& name) const
{
    auto it = m_criteria.find(name);
    return it != m_criteria.end() ? it->second.get() : nullptr;
}

bool ScoreCriteriaRegistry::hasCriteria(const std::string& name) const
{
    return m_criteria.find(name) != m_criteria.end();
}

std::vector<std::string> ScoreCriteriaRegistry::getAllCriteriaNames() const
{
    std::vector<std::string> names;
    names.reserve(m_criteria.size());
    for (const auto& [name, criteria] : m_criteria) {
        names.push_back(name);
    }
    return names;
}

void ScoreCriteriaRegistry::registerBuiltinCriteria()
{
    // 基础判据
    registerCriteria(std::make_unique<DummyCriteria>());
    registerCriteria(std::make_unique<TriggerCriteria>());
    registerCriteria(std::make_unique<DeathCountCriteria>());
    registerCriteria(std::make_unique<PlayerKillCountCriteria>());
    registerCriteria(std::make_unique<TotalKillCountCriteria>());

    // 只读判据
    registerCriteria(std::make_unique<HealthCriteria>());
    registerCriteria(std::make_unique<FoodCriteria>());
    registerCriteria(std::make_unique<AirCriteria>());
    registerCriteria(std::make_unique<ArmorCriteria>());
    registerCriteria(std::make_unique<XpCriteria>());
    registerCriteria(std::make_unique<LevelCriteria>());

    // 队伍击杀判据（16种颜色）
    const TextFormatting colors[] = {TextFormatting::Black,
        TextFormatting::DarkBlue,
        TextFormatting::DarkGreen,
        TextFormatting::DarkAqua,
        TextFormatting::DarkRed,
        TextFormatting::DarkPurple,
        TextFormatting::Gold,
        TextFormatting::Gray,
        TextFormatting::DarkGray,
        TextFormatting::Blue,
        TextFormatting::Green,
        TextFormatting::Aqua,
        TextFormatting::Red,
        TextFormatting::LightPurple,
        TextFormatting::Yellow,
        TextFormatting::White};

    for (TextFormatting color : colors) {
        if (TeamKillCriteria::isSupportedColor(color)) {
            registerCriteria(std::make_unique<TeamKillCriteria>(color, TeamKillCriteria::Type::TeamKill));
            registerCriteria(std::make_unique<TeamKillCriteria>(color, TeamKillCriteria::Type::KilledByTeam));
        }
    }
}

void ScoreCriteriaRegistry::clear()
{
    m_criteria.clear();
}

} // namespace mc::scoreboard
