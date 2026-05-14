/**
 * @file SleepManagerTest.cpp
 * @brief 睡眠系统测试
 *
 * 测试睡眠管理器的各个功能：
 * - canSleepAtTime: 睡眠时间检测
 * - isPlayerNearBed: 玩家距离床检测
 * - getSleepResultMessage: 睡眠结果消息
 */

#include "common/entity/player/SleepManager.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/weather/WeatherConstants.hpp"
#include <gtest/gtest.h>

namespace mc {
namespace entity {
namespace {

// ========== canSleepAtTime 测试 ==========

class SleepManagerTimeTest : public ::testing::Test {
protected:
    // 时间常量参考 WeatherConstants.hpp
    static constexpr i64 CLEAR_START = 12542; // 晴天睡眠开始时间
    static constexpr i64 CLEAR_END = 23459;   // 晴天睡眠结束时间
    static constexpr i64 RAIN_START = 12010;  // 降雨睡眠开始时间
    static constexpr i64 RAIN_END = 23991;    // 降雨睡眠结束时间
    static constexpr i64 DAY_LENGTH = 24000;  // 一天的长度
};

TEST_F(SleepManagerTimeTest, CanSleepDuringClearNight)
{
    // 晴天夜间应该可以睡眠
    EXPECT_TRUE(SleepManager::canSleepAtTime(12542, false, false)); // 刚好进入夜间
    EXPECT_TRUE(SleepManager::canSleepAtTime(13000, false, false)); // 深夜
    EXPECT_TRUE(SleepManager::canSleepAtTime(18000, false, false)); // 午夜
    EXPECT_TRUE(SleepManager::canSleepAtTime(23459, false, false)); // 夜间结束
}

TEST_F(SleepManagerTimeTest, CannotSleepDuringDay)
{
    // 白天不应该能睡眠
    EXPECT_FALSE(SleepManager::canSleepAtTime(0, false, false));     // 日出
    EXPECT_FALSE(SleepManager::canSleepAtTime(5000, false, false));  // 中午
    EXPECT_FALSE(SleepManager::canSleepAtTime(10000, false, false)); // 下午
    EXPECT_FALSE(SleepManager::canSleepAtTime(12541, false, false)); // 夜间前一 tick
}

TEST_F(SleepManagerTimeTest, CanSleepAnytimeDuringThunder)
{
    // 雷暴时任何时间都可以睡眠
    EXPECT_TRUE(SleepManager::canSleepAtTime(0, true, false));     // 白天雷暴
    EXPECT_TRUE(SleepManager::canSleepAtTime(6000, true, false));  // 中午雷暴
    EXPECT_TRUE(SleepManager::canSleepAtTime(12000, true, false)); // 傍晚雷暴
}

TEST_F(SleepManagerTimeTest, CanSleepEarlierDuringRain)
{
    // 降雨时睡眠时间范围更宽
    EXPECT_TRUE(SleepManager::canSleepAtTime(12010, false, true)); // 降雨开始时间
    EXPECT_TRUE(SleepManager::canSleepAtTime(23991, false, true)); // 降雨结束时间

    // 降雨时间范围外不能睡眠
    EXPECT_FALSE(SleepManager::canSleepAtTime(12000, false, true)); // 降雨开始前
    EXPECT_FALSE(SleepManager::canSleepAtTime(12009, false, true)); // 降雨开始前一 tick
}

TEST_F(SleepManagerTimeTest, RainSleepTimeDoesNotCrossMidnight)
{
    // 根据 WeatherConstants，降雨时间范围是 12010-23991，不跨午夜
    // 午夜后（0-12009）不能睡眠
    EXPECT_FALSE(SleepManager::canSleepAtTime(0, false, true));
    EXPECT_FALSE(SleepManager::canSleepAtTime(100, false, true));
    EXPECT_FALSE(SleepManager::canSleepAtTime(10000, false, true));
    EXPECT_FALSE(SleepManager::canSleepAtTime(12009, false, true));
}

// ========== isPlayerNearBed 测试 ==========

class SleepManagerNearBedTest : public ::testing::Test {
protected:
    void SetUp() override { bedPos = BlockPos(100, 64, 100); }
    BlockPos bedPos;
};

TEST_F(SleepManagerNearBedTest, PlayerOnBedIsNear)
{
    // 玩家在床上应该算近
    Vector3 playerPos(100.5f, 64.5f, 100.5f); // 床的中心
    EXPECT_TRUE(SleepManager::isPlayerNearBed(playerPos, bedPos));
}

TEST_F(SleepManagerNearBedTest, PlayerWithinHorizontalRange)
{
    // 水平范围 3 格内应该算近
    EXPECT_TRUE(SleepManager::isPlayerNearBed(Vector3(100.0f, 64.5f, 97.5f), bedPos));  // Z 方向 -3
    EXPECT_TRUE(SleepManager::isPlayerNearBed(Vector3(100.0f, 64.5f, 103.5f), bedPos)); // Z 方向 +3
    EXPECT_TRUE(SleepManager::isPlayerNearBed(Vector3(97.5f, 64.5f, 100.0f), bedPos));  // X 方向 -3
    EXPECT_TRUE(SleepManager::isPlayerNearBed(Vector3(103.5f, 64.5f, 100.0f), bedPos)); // X 方向 +3
}

TEST_F(SleepManagerNearBedTest, PlayerOutsideHorizontalRange)
{
    // 水平范围超过 3 格应该不算近
    // 床位置是 (100, 64, 100)，检查超过 3 格距离的位置
    EXPECT_FALSE(SleepManager::isPlayerNearBed(Vector3(100.0f, 64.5f, 96.4f), bedPos));  // Z 方向 -3.6
    EXPECT_FALSE(SleepManager::isPlayerNearBed(Vector3(100.0f, 64.5f, 103.6f), bedPos)); // Z 方向 +3.6
    EXPECT_FALSE(SleepManager::isPlayerNearBed(Vector3(96.4f, 64.5f, 100.0f), bedPos));  // X 方向 -3.6
    EXPECT_FALSE(SleepManager::isPlayerNearBed(Vector3(103.6f, 64.5f, 100.0f), bedPos)); // X 方向 +3.6
}

TEST_F(SleepManagerNearBedTest, PlayerWithinVerticalRange)
{
    // 垂直范围 2 格内应该算近
    EXPECT_TRUE(SleepManager::isPlayerNearBed(Vector3(100.5f, 62.5f, 100.5f), bedPos)); // Y 方向 -2
    EXPECT_TRUE(SleepManager::isPlayerNearBed(Vector3(100.5f, 66.5f, 100.5f), bedPos)); // Y 方向 +2
}

TEST_F(SleepManagerNearBedTest, PlayerOutsideVerticalRange)
{
    // 垂直范围超过 2 格应该不算近
    EXPECT_FALSE(SleepManager::isPlayerNearBed(Vector3(100.5f, 61.4f, 100.5f), bedPos)); // Y 方向 -3
    EXPECT_FALSE(SleepManager::isPlayerNearBed(Vector3(100.5f, 66.6f, 100.5f), bedPos)); // Y 方向 +3
}

// ========== getSleepResultMessage 测试 ==========

class SleepResultMessageTest : public ::testing::Test {};

TEST_F(SleepResultMessageTest, ReturnsNullptrForOK)
{
    // OK 不需要显示消息
    const char* msg = getSleepResultMessage(SleepResult::OK);
    EXPECT_EQ(msg, nullptr);
}

TEST_F(SleepResultMessageTest, ReturnsCorrectMessageForNotPossibleHere)
{
    // NOT_POSSIBLE_HERE 不显示消息（用于下界爆炸场景）
    const char* msg = getSleepResultMessage(SleepResult::NOT_POSSIBLE_HERE);
    EXPECT_EQ(msg, nullptr);
}

TEST_F(SleepResultMessageTest, ReturnsCorrectMessageForNotPossibleNow)
{
    const char* msg = getSleepResultMessage(SleepResult::NOT_POSSIBLE_NOW);
    EXPECT_NE(msg, nullptr);
    EXPECT_STREQ(msg, "block.minecraft.bed.no_sleep");
}

TEST_F(SleepResultMessageTest, ReturnsCorrectMessageForTooFarAway)
{
    const char* msg = getSleepResultMessage(SleepResult::TOO_FAR_AWAY);
    EXPECT_NE(msg, nullptr);
    EXPECT_STREQ(msg, "block.minecraft.bed.too_far_away");
}

TEST_F(SleepResultMessageTest, ReturnsCorrectMessageForObstructed)
{
    const char* msg = getSleepResultMessage(SleepResult::OBSTRUCTED);
    EXPECT_NE(msg, nullptr);
    EXPECT_STREQ(msg, "block.minecraft.bed.obstructed");
}

TEST_F(SleepResultMessageTest, ReturnsCorrectMessageForNotSafe)
{
    const char* msg = getSleepResultMessage(SleepResult::NOT_SAFE);
    EXPECT_NE(msg, nullptr);
    EXPECT_STREQ(msg, "block.minecraft.bed.not_safe");
}

TEST_F(SleepResultMessageTest, ReturnsNullptrForOtherProblem)
{
    // OTHER_PROBLEM 不显示消息
    const char* msg = getSleepResultMessage(SleepResult::OTHER_PROBLEM);
    EXPECT_EQ(msg, nullptr);
}

// ========== isSleepSuccess 测试 ==========

class SleepSuccessTest : public ::testing::Test {};

TEST_F(SleepSuccessTest, OKReturnsTrue)
{
    EXPECT_TRUE(isSleepSuccess(SleepResult::OK));
}

TEST_F(SleepSuccessTest, FailureCodesReturnFalse)
{
    EXPECT_FALSE(isSleepSuccess(SleepResult::NOT_POSSIBLE_HERE));
    EXPECT_FALSE(isSleepSuccess(SleepResult::NOT_POSSIBLE_NOW));
    EXPECT_FALSE(isSleepSuccess(SleepResult::TOO_FAR_AWAY));
    EXPECT_FALSE(isSleepSuccess(SleepResult::OBSTRUCTED));
    EXPECT_FALSE(isSleepSuccess(SleepResult::NOT_SAFE));
    EXPECT_FALSE(isSleepSuccess(SleepResult::OTHER_PROBLEM));
}

} // namespace
} // namespace entity
} // namespace mc
