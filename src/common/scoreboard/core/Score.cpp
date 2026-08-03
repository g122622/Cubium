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

#include "Score.hpp"
#include "ScoreCriteria.hpp"
#include "ScoreObjective.hpp"
#include "Scoreboard.hpp"
#include "common/core/Types.hpp"
#include <algorithm>
#include <string>

namespace mc::scoreboard {

Score::Score(Scoreboard& scoreboard, ScoreObjective& objective, const std::string& playerName)
    : m_scoreboard(scoreboard)
    , m_objective(&objective)
    , m_playerName(playerName)
    , m_score(0)
    , m_locked(false)
    , m_forceUpdate(true) // 新创建的分数需要强制更新
{}

void Score::setScorePoints(i32 points)
{
    // 限制分数范围
    points = std::clamp(points, MIN_SCORE, MAX_SCORE);

    if (m_score != points) {
        i32 oldScore = m_score;
        m_score = points;
        m_forceUpdate = false;

        // 通知判据
        m_objective->getCriteria().onScoreChanged(*this, oldScore);

        // 通知记分板
        m_scoreboard.onScoreChanged(*this);
    }
}

void Score::addScore(i32 amount)
{
    // 检查溢出
    if (amount > 0 && m_score > MAX_SCORE - amount) {
        setScorePoints(MAX_SCORE);
    } else if (amount < 0 && m_score < MIN_SCORE - amount) {
        setScorePoints(MIN_SCORE);
    } else {
        setScorePoints(m_score + amount);
    }
}

void Score::subtractScore(i32 amount)
{
    addScore(-amount);
}

void Score::reset()
{
    setScorePoints(0);
    m_locked = false;
}

} // namespace mc::scoreboard
