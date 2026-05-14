#include "GameTime.hpp"
#include <algorithm>

namespace mc::time {

void GameTime::tick()
{
    m_gameTime++;

    if (m_daylightCycleEnabled) {
        m_dayTime = (m_dayTime + 1) % TimeConstants::TICKS_PER_DAY;
    }
}

void GameTime::setDayTime(i64 time)
{
    // 处理负数和时间循环
    m_dayTime = ((time % TimeConstants::TICKS_PER_DAY) + TimeConstants::TICKS_PER_DAY) % TimeConstants::TICKS_PER_DAY;
}

void GameTime::addDayTime(i64 ticks)
{
    setDayTime(m_dayTime + ticks);
}

void GameTime::setGameTime(i64 time)
{
    m_gameTime = time;
}

void GameTime::setDaylightCycleEnabled(bool enabled)
{
    m_daylightCycleEnabled = enabled;
}

bool GameTime::isDay() const
{
    return m_dayTime >= TimeConstants::SUNRISE && m_dayTime < TimeConstants::SUNSET;
}

bool GameTime::isNight() const
{
    return !isDay();
}

i64 GameTime::dayTimeForNetwork() const
{
    // MC 协议: 负数表示日光周期禁用
    return m_daylightCycleEnabled ? m_dayTime : -m_dayTime;
}

} // namespace mc::time
