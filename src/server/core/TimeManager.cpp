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

#include "TimeManager.hpp"
#include "common/core/Types.hpp"

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

i64 TimeManager::dayTimeOfDay() const
{
    return m_gameTime.dayTimeOfDay();
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
