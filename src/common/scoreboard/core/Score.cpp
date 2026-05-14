#include "Score.hpp"
#include "ScoreCriteria.hpp"
#include "ScoreObjective.hpp"
#include "Scoreboard.hpp"
#include <algorithm>
#include <climits>

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
