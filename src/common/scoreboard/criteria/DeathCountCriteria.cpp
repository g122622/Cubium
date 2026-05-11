#include "DeathCountCriteria.hpp"
#include "../core/Scoreboard.hpp"
#include "../core/Score.hpp"

namespace mc::scoreboard {

DeathCountCriteria::DeathCountCriteria()
    : m_name(NAME)
{
}

void DeathCountCriteria::onPlayerDeath(const std::string& playerName, Scoreboard& scoreboard) {
    // 获取所有使用此判据的目标
    auto objectives = scoreboard.getObjectivesByCriteria(*this);
    for (auto* objective : objectives) {
        // 获取或创建分数并增加
        auto* score = scoreboard.getOrCreateScore(playerName, *objective);
        if (score) {
            score->incrementScore();
        }
    }
}

} // namespace mc::scoreboard
