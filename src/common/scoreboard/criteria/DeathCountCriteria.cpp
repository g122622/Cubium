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

#include "DeathCountCriteria.hpp"
#include "../core/Score.hpp"
#include "../core/Scoreboard.hpp"
#include <string>

namespace mc::scoreboard {

DeathCountCriteria::DeathCountCriteria()
    : m_name(NAME)
{}

void DeathCountCriteria::onPlayerDeath(const std::string& playerName, Scoreboard& scoreboard)
{
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
