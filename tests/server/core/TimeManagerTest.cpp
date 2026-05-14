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
    EXPECT_TRUE(manager.daylightCycleEnabled());
}

TEST_F(TimeManagerTest, ConstructionWithInitialTime)
{
    TimeManager m(1000, 6000);
    EXPECT_EQ(m.gameTime(), 1000);
    EXPECT_EQ(m.currentTick(), 1000u);
    EXPECT_EQ(m.dayTime(), 6000);
}

TEST_F(TimeManagerTest, TickIncreasesTime)
{
    manager.tick();
    EXPECT_EQ(manager.currentTick(), 1u);
    EXPECT_EQ(manager.gameTime(), 1);
    EXPECT_EQ(manager.dayTime(), 1);

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
}

TEST_F(TimeManagerTest, DayTimeWrapsAround)
{
    manager.setDayTime(23999);
    manager.tick();
    // DayTime wraps at 24000
    EXPECT_EQ(manager.dayTime(), 0);
}

TEST_F(TimeManagerTest, AddDayTime)
{
    manager.setDayTime(10000);
    manager.addDayTime(5000);
    EXPECT_EQ(manager.dayTime(), 15000);
}

TEST_F(TimeManagerTest, DaylightCycleDisable)
{
    manager.setDaylightCycleEnabled(false);
    EXPECT_FALSE(manager.daylightCycleEnabled());

    manager.setDayTime(10000);
    manager.tick();
    // dayTime should not change when daylight cycle is disabled
    EXPECT_EQ(manager.dayTime(), 10000);
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
