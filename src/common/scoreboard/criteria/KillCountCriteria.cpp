#include "KillCountCriteria.hpp"
#include "../core/Score.hpp"
#include "../core/Scoreboard.hpp"

namespace mc::scoreboard {

KillCountCriteria::KillCountCriteria(const std::string& name, bool playersOnly)
    : m_name(name)
    , m_playersOnly(playersOnly)
{}

void KillCountCriteria::onPlayerKill(
    const std::string& playerName, const std::string& /*victimType*/, bool isPlayer, Scoreboard& scoreboard)
{
    // 如果只统计击杀玩家，且受害者不是玩家，则跳过
    if (m_playersOnly && !isPlayer) {
        return;
    }

    // 获取所有使用此判据的目标
    auto objectives = scoreboard.getObjectivesByCriteria(*this);
    for (auto* objective : objectives) {
        auto* score = scoreboard.getOrCreateScore(playerName, *objective);
        if (score) {
            score->incrementScore();
        }
    }
}

PlayerKillCountCriteria::PlayerKillCountCriteria()
    : KillCountCriteria(NAME, true)
{}

TotalKillCountCriteria::TotalKillCountCriteria()
    : KillCountCriteria(NAME, false)
{}

} // namespace mc::scoreboard
