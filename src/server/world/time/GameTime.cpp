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

#include "GameTime.hpp"
#include "common/core/Types.hpp"

namespace mc::time {

void GameTime::tick()
{
    m_gameTime++;

    if (m_daylightCycleEnabled) {
        m_dayTime++;
    }
}

void GameTime::setDayTime(i64 time)
{
    m_dayTime = time;
}

void GameTime::addDayTime(i64 ticks)
{
    m_dayTime += ticks;
}

void GameTime::setGameTime(i64 time)
{
    m_gameTime = time;
}

void GameTime::setDaylightCycleEnabled(bool enabled)
{
    m_daylightCycleEnabled = enabled;
}

i64 GameTime::dayTimeOfDay() const
{
    // 确保负数也能正确取模
    return ((m_dayTime % TimeConstants::TICKS_PER_DAY) + TimeConstants::TICKS_PER_DAY) % TimeConstants::TICKS_PER_DAY;
}

bool GameTime::isDay() const
{
    i64 tod = dayTimeOfDay();
    return tod >= TimeConstants::SUNRISE && tod < TimeConstants::SUNSET;
}

bool GameTime::isNight() const
{
    return !isDay();
}

i64 GameTime::dayTimeForNetwork() const
{
    i64 tod = dayTimeOfDay();
    return m_daylightCycleEnabled ? tod : -tod;
}

i64 GameTime::dayCount() const
{
    // 天数 = gameTime / 24000
    return m_gameTime / TimeConstants::TICKS_PER_DAY;
}

} // namespace mc::time
