#include "ScoreCriteria.hpp"
#include "Scoreboard.hpp"
#include "Score.hpp"
#include "../criteria/DummyCriteria.hpp"
#include "../criteria/TriggerCriteria.hpp"
#include "../criteria/DeathCountCriteria.hpp"
#include "../criteria/KillCountCriteria.hpp"
#include "../criteria/ReadOnlyCriteria.hpp"
#include "../criteria/TeamKillCriteria.hpp"
#include "../../util/text/TextStyle.hpp"
#include <cassert>

namespace mc::scoreboard {

// ========== ScoreCriteriaRegistry ==========

ScoreCriteriaRegistry& ScoreCriteriaRegistry::instance() {
    static ScoreCriteriaRegistry s_instance;
    return s_instance;
}

Result<void> ScoreCriteriaRegistry::registerCriteria(std::unique_ptr<ScoreCriteria> criteria) {
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

ScoreCriteria* ScoreCriteriaRegistry::getCriteria(const std::string& name) {
    auto it = m_criteria.find(name);
    return it != m_criteria.end() ? it->second.get() : nullptr;
}

const ScoreCriteria* ScoreCriteriaRegistry::getCriteria(const std::string& name) const {
    auto it = m_criteria.find(name);
    return it != m_criteria.end() ? it->second.get() : nullptr;
}

bool ScoreCriteriaRegistry::hasCriteria(const std::string& name) const {
    return m_criteria.find(name) != m_criteria.end();
}

std::vector<std::string> ScoreCriteriaRegistry::getAllCriteriaNames() const {
    std::vector<std::string> names;
    names.reserve(m_criteria.size());
    for (const auto& [name, criteria] : m_criteria) {
        names.push_back(name);
    }
    return names;
}

void ScoreCriteriaRegistry::registerBuiltinCriteria() {
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
    const TextFormatting colors[] = {
        TextFormatting::Black,
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
        TextFormatting::White
    };

    for (TextFormatting color : colors) {
        if (TeamKillCriteria::isSupportedColor(color)) {
            registerCriteria(std::make_unique<TeamKillCriteria>(color, TeamKillCriteria::Type::TeamKill));
            registerCriteria(std::make_unique<TeamKillCriteria>(color, TeamKillCriteria::Type::KilledByTeam));
        }
    }
}

void ScoreCriteriaRegistry::clear() {
    m_criteria.clear();
}

} // namespace mc::scoreboard
