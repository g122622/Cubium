#include "TimeManager.hpp"

namespace mc::server::core {

TimeManager::TimeManager(i64 initialGameTime, i64 initialDayTime)
    : m_gameTime()
{
    m_gameTime.setGameTime(initialGameTime);
    m_gameTime.setDayTime(initialDayTime);
}

void TimeManager::tick()
{
    m_gameTime.tick();
}

i64 TimeManager::gameTime() const
{
    return m_gameTime.gameTime();
}

void TimeManager::setGameTime(i64 time)
{
    m_gameTime.setGameTime(time);
}

i64 TimeManager::dayTime() const
{
    return m_gameTime.dayTime();
}

void TimeManager::setDayTime(i64 time)
{
    m_gameTime.setDayTime(time);
}

void TimeManager::addDayTime(i64 ticks)
{
    m_gameTime.addDayTime(ticks);
}

i64 TimeManager::dayCount() const
{
    return m_gameTime.dayCount();
}

void TimeManager::setDaylightCycleEnabled(bool enabled)
{
    m_gameTime.setDaylightCycleEnabled(enabled);
}

} // namespace mc::server::core
