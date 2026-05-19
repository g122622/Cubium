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

#include "world/redstone/RedstoneSystem.hpp"
#include "world/block/BlockPos.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::world::redstone;

/**
 * @brief RedstoneSystem 单元测试
 *
 * 测试红石系统管理和烧毁跟踪功能。
 */
class RedstoneSystemTest : public ::testing::Test {
protected:
    void SetUp() override { RedstoneSystem::instance().clear(); }

    void TearDown() override { RedstoneSystem::instance().clear(); }
};

// ========== 烧毁跟踪测试 ==========

TEST_F(RedstoneSystemTest, TorchFlipNotBurnedInitially)
{
    BlockPos pos(0, 0, 0);

    // 第一次翻转不应触发烧毁
    EXPECT_FALSE(RedstoneSystem::instance().checkAndRecordTorchFlip(pos, 0));
    EXPECT_FALSE(RedstoneSystem::instance().isTorchBurnedOut(pos, 0));
}

TEST_F(RedstoneSystemTest, TorchFlipNotBurnedAfterFewFlips)
{
    BlockPos pos(0, 0, 0);

    // 7次翻转不应触发烧毁（需要8次）
    for (int i = 0; i < 7; ++i) {
        EXPECT_FALSE(RedstoneSystem::instance().checkAndRecordTorchFlip(pos, i * 5));
    }
    EXPECT_FALSE(RedstoneSystem::instance().isTorchBurnedOut(pos, 35));
}

TEST_F(RedstoneSystemTest, TorchBurnoutAfter8Flips)
{
    BlockPos pos(0, 0, 0);

    // 8次翻转触发烧毁（在60 tick窗口内）
    for (int i = 0; i < 7; ++i) {
        EXPECT_FALSE(RedstoneSystem::instance().checkAndRecordTorchFlip(pos, i * 5))
            << "Flip " << i << " should not cause burnout";
    }

    // 第8次翻转触发烧毁
    EXPECT_TRUE(RedstoneSystem::instance().checkAndRecordTorchFlip(pos, 35));
    EXPECT_TRUE(RedstoneSystem::instance().isTorchBurnedOut(pos, 35));
}

TEST_F(RedstoneSystemTest, TorchBurnoutWithinWindow)
{
    BlockPos pos(0, 0, 0);

    // 在60 tick窗口内进行8次翻转
    for (int i = 0; i < 7; ++i) {
        EXPECT_FALSE(RedstoneSystem::instance().checkAndRecordTorchFlip(pos, i));
    }
    EXPECT_TRUE(RedstoneSystem::instance().checkAndRecordTorchFlip(pos, 7));
}

TEST_F(RedstoneSystemTest, TorchNoBurnoutOutsideWindow)
{
    BlockPos pos(0, 0, 0);

    // 第一次翻转在 tick 0
    EXPECT_FALSE(RedstoneSystem::instance().checkAndRecordTorchFlip(pos, 0));

    // 之后7次翻转在 tick 61 之后（第一次翻转已过期）
    // 此时窗口内只有7次翻转，不会烧毁
    for (int i = 0; i < 7; ++i) {
        EXPECT_FALSE(RedstoneSystem::instance().checkAndRecordTorchFlip(pos, 61 + i * 5))
            << "Flip at tick " << (61 + i * 5) << " should not cause burnout";
    }
}

TEST_F(RedstoneSystemTest, TorchBurnoutCooldown)
{
    BlockPos pos(0, 0, 0);

    // 触发烧毁
    for (int i = 0; i < 8; ++i) {
        RedstoneSystem::instance().checkAndRecordTorchFlip(pos, i);
    }

    // 烧毁后仍在冷却中 (tick 50 < 160)
    EXPECT_TRUE(RedstoneSystem::instance().isTorchBurnedOut(pos, 50));

    // 160 tick后冷却结束 (烧毁发生在tick 7，所以冷却到 tick 167)
    EXPECT_TRUE(RedstoneSystem::instance().isTorchBurnedOut(pos, 166));
    EXPECT_FALSE(RedstoneSystem::instance().isTorchBurnedOut(pos, 167));
}

TEST_F(RedstoneSystemTest, TorchMultiplePositions)
{
    BlockPos pos1(0, 0, 0);
    BlockPos pos2(10, 0, 0);
    BlockPos pos3(20, 0, 0);

    // 不同位置的火把独立计算
    for (int i = 0; i < 8; ++i) {
        RedstoneSystem::instance().checkAndRecordTorchFlip(pos1, i);
    }

    EXPECT_TRUE(RedstoneSystem::instance().isTorchBurnedOut(pos1, 10));
    EXPECT_FALSE(RedstoneSystem::instance().isTorchBurnedOut(pos2, 10));
    EXPECT_FALSE(RedstoneSystem::instance().isTorchBurnedOut(pos3, 10));

    // pos2 只有7次翻转
    for (int i = 0; i < 7; ++i) {
        RedstoneSystem::instance().checkAndRecordTorchFlip(pos2, i);
    }
    EXPECT_FALSE(RedstoneSystem::instance().isTorchBurnedOut(pos2, 10));
}

TEST_F(RedstoneSystemTest, TorchClearRecord)
{
    BlockPos pos(0, 0, 0);

    // 触发烧毁
    for (int i = 0; i < 8; ++i) {
        RedstoneSystem::instance().checkAndRecordTorchFlip(pos, i);
    }
    EXPECT_TRUE(RedstoneSystem::instance().isTorchBurnedOut(pos, 10));

    // 清除记录
    RedstoneSystem::instance().clearTorchRecord(pos);
    EXPECT_FALSE(RedstoneSystem::instance().isTorchBurnedOut(pos, 10));
}

TEST_F(RedstoneSystemTest, TorchCleanupExpiredRecords)
{
    BlockPos pos(0, 0, 0);

    // 触发烧毁
    for (int i = 0; i < 8; ++i) {
        RedstoneSystem::instance().checkAndRecordTorchFlip(pos, i);
    }

    // 冷却结束后清理
    RedstoneSystem::instance().cleanupBurnoutRecords(200);
    EXPECT_FALSE(RedstoneSystem::instance().isTorchBurnedOut(pos, 200));
}

// ========== 递归保护测试 ==========

TEST_F(RedstoneSystemTest, IsUpdatingInitiallyFalse)
{
    BlockPos pos(0, 0, 0);
    EXPECT_FALSE(RedstoneSystem::instance().isUpdating(pos));
}

TEST_F(RedstoneSystemTest, BeginEndUpdate)
{
    BlockPos pos(10, 20, 30);

    EXPECT_FALSE(RedstoneSystem::instance().isUpdating(pos));

    RedstoneSystem::instance().beginUpdate(pos);
    EXPECT_TRUE(RedstoneSystem::instance().isUpdating(pos));

    RedstoneSystem::instance().endUpdate(pos);
    EXPECT_FALSE(RedstoneSystem::instance().isUpdating(pos));
}

TEST_F(RedstoneSystemTest, DepthManagement)
{
    EXPECT_EQ(RedstoneSystem::instance().canPushDepth(), true);

    RedstoneSystem::instance().pushDepth();
    EXPECT_EQ(RedstoneSystem::instance().canPushDepth(), true);

    // 增加到最大深度
    for (int i = 1; i < RedstoneContext::MAX_DEPTH; ++i) {
        RedstoneSystem::instance().pushDepth();
    }
    EXPECT_FALSE(RedstoneSystem::instance().canPushDepth());

    RedstoneSystem::instance().popDepth();
    EXPECT_TRUE(RedstoneSystem::instance().canPushDepth());
}

// ========== 常量测试 ==========

TEST_F(RedstoneSystemTest, BurnoutConstants)
{
    EXPECT_EQ(RedstoneSystem::BURNOUT_WINDOW, 60);
    EXPECT_EQ(RedstoneSystem::BURNOUT_FLIPS, 8);
    EXPECT_EQ(RedstoneSystem::BURNOUT_COOLDOWN, 160);
}
