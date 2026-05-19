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
