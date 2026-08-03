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

#include "KillCountCriteria.hpp"
#include "../core/Score.hpp"
#include "../core/Scoreboard.hpp"
#include <string>

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
