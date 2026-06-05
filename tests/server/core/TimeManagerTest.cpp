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

#include "server/core/TimeManager.hpp"
#include <gtest/gtest.h>

using namespace mc::server::core;

/**
 * @brief TimeManager 单元测试
 */
class TimeManagerTest : public ::testing::Test {
protected:
    TimeManager manager;
};

TEST_F(TimeManagerTest, DefaultConstruction)
{
    EXPECT_EQ(manager.currentTick(), 0u);
    EXPECT_EQ(manager.gameTime(), 0);
    EXPECT_EQ(manager.dayTime(), 0);
    EXPECT_EQ(manager.dayTimeOfDay(), 0);
    EXPECT_TRUE(manager.daylightCycleEnabled());
}

TEST_F(TimeManagerTest, ConstructionWithInitialTime)
{
    TimeManager m(1000, 6000);
    EXPECT_EQ(m.gameTime(), 1000);
    EXPECT_EQ(m.currentTick(), 1000u);
    EXPECT_EQ(m.dayTime(), 6000);
    EXPECT_EQ(m.dayTimeOfDay(), 6000);
}

TEST_F(TimeManagerTest, TickIncreasesTime)
{
    manager.tick();
    EXPECT_EQ(manager.currentTick(), 1u);
    EXPECT_EQ(manager.gameTime(), 1);
    EXPECT_EQ(manager.dayTime(), 1);
    EXPECT_EQ(manager.dayTimeOfDay(), 1);

    manager.tick();
    manager.tick();
    EXPECT_EQ(manager.currentTick(), 3u);
    EXPECT_EQ(manager.gameTime(), 3);
}

TEST_F(TimeManagerTest, SetGameTime)
{
    manager.setGameTime(10000);
    EXPECT_EQ(manager.gameTime(), 10000);
    EXPECT_EQ(manager.currentTick(), 10000u);
}

TEST_F(TimeManagerTest, SetDayTime)
{
    manager.setDayTime(12000);
    EXPECT_EQ(manager.dayTime(), 12000);
    EXPECT_EQ(manager.dayTimeOfDay(), 12000);
}

TEST_F(TimeManagerTest, DayTimeUnbounded)
{
    // MC 1.16.5: dayTime is unbounded, does NOT wrap at 24000
    manager.setDayTime(23999);
    manager.tick();
    // dayTime should exceed 24000, not wrap to 0
    EXPECT_EQ(manager.dayTime(), 24000);
    // dayTimeOfDay returns the normalized value (0-23999)
    EXPECT_EQ(manager.dayTimeOfDay(), 0);

    // Test with larger values
    manager.setDayTime(100000);
    EXPECT_EQ(manager.dayTime(), 100000);
    EXPECT_EQ(manager.dayTimeOfDay(), 100000 % 24000);
}

TEST_F(TimeManagerTest, AddDayTime)
{
    manager.setDayTime(10000);
    manager.addDayTime(5000);
    EXPECT_EQ(manager.dayTime(), 15000);
    EXPECT_EQ(manager.dayTimeOfDay(), 15000);

    // Test addition that exceeds 24000
    manager.setDayTime(20000);
    manager.addDayTime(10000);
    EXPECT_EQ(manager.dayTime(), 30000);
    EXPECT_EQ(manager.dayTimeOfDay(), 6000);
}

TEST_F(TimeManagerTest, DaylightCycleDisable)
{
    manager.setDaylightCycleEnabled(false);
    EXPECT_FALSE(manager.daylightCycleEnabled());

    manager.setDayTime(10000);
    manager.tick();
    // dayTime should not change when daylight cycle is disabled
    EXPECT_EQ(manager.dayTime(), 10000);
    EXPECT_EQ(manager.dayTimeOfDay(), 10000);
}

TEST_F(TimeManagerTest, DayCount)
{
    manager.setGameTime(0);
    EXPECT_EQ(manager.dayCount(), 0);

    manager.setGameTime(24000);
    EXPECT_EQ(manager.dayCount(), 1);

    manager.setGameTime(48000);
    EXPECT_EQ(manager.dayCount(), 2);
}

TEST_F(TimeManagerTest, GameTimeObj)
{
    manager.setGameTime(50000);
    EXPECT_EQ(manager.gameTimeObj().gameTime(), 50000);
    // dayTime is independent of gameTime, starts at 0
    manager.setDayTime(2000);
    EXPECT_EQ(manager.gameTimeObj().dayTime(), 2000);
}

TEST_F(TimeManagerTest, DayTimeOfDayNegativeTime)
{
    // Test negative time handling
    manager.setDayTime(-100);
    // dayTime stores negative value
    EXPECT_EQ(manager.dayTime(), -100);
    // dayTimeOfDay normalizes to positive value in 0-23999 range
    EXPECT_EQ(manager.dayTimeOfDay(), 23900);
}
