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
#include "common/world/block/base/types/BlockPos.hpp"
#include <gtest/gtest.h>

namespace mc {
namespace entity {
namespace {

// ========== canSleepAtTime 测试 ==========

class SleepManagerTimeTest : public ::testing::Test {
protected:
    // 睡眠判定跟随天空减暗因子（BedRule.WHEN_DARK -> isDarkOutside：减暗 >= 4）。
    // 下列时间点的期望值由 InternalLightUtils::calculateSkyDarkening 的曲线决定。
};

TEST_F(SleepManagerTimeTest, CanSleepWhenSkyIsDark)
{
    EXPECT_TRUE(SleepManager::canSleepAtTime(11000, false, false));
    EXPECT_TRUE(SleepManager::canSleepAtTime(12000, false, false));
    EXPECT_TRUE(SleepManager::canSleepAtTime(18000, false, false));
    EXPECT_TRUE(SleepManager::canSleepAtTime(23459, false, false));
}

TEST_F(SleepManagerTimeTest, CannotSleepAtBrightNoon)
{
    EXPECT_FALSE(SleepManager::canSleepAtTime(6000, false, false));
    EXPECT_FALSE(SleepManager::canSleepAtTime(8400, false, false));
    EXPECT_FALSE(SleepManager::canSleepAtTime(10000, false, false));
    EXPECT_FALSE(SleepManager::canSleepAtTime(10500, false, false));
}

TEST_F(SleepManagerTimeTest, ThunderAllowsSleepAtNoon)
{
    // 雷暴给天空减暗 +10，正午也能达到入睡阈值。
    EXPECT_TRUE(SleepManager::canSleepAtTime(6000, true, false));
    EXPECT_TRUE(SleepManager::canSleepAtTime(3000, true, false));
}

TEST_F(SleepManagerTimeTest, RainDarkensSkyButNotEnoughAtNoon)
{
    // 降雨给天空减暗 +3：正午仍不足 4，下午晚些时候才达到阈值。
    EXPECT_FALSE(SleepManager::canSleepAtTime(6000, false, true));
    EXPECT_FALSE(SleepManager::canSleepAtTime(8000, false, true));
    EXPECT_TRUE(SleepManager::canSleepAtTime(8400, false, true));
    EXPECT_TRUE(SleepManager::canSleepAtTime(10000, false, true));
}

// ========== isPlayerNearBed 测试 ==========

class SleepManagerNearBedTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        bedPos = BlockPos(100, 64, 100);
    }

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
