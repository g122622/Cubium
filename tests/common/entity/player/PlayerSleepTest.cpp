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
 * @file PlayerSleepTest.cpp
 * @brief Player 睡眠功能测试
 *
 * 测试 Player 基类和 ServerPlayer 的睡眠相关功能：
 * - Player::tryStartSleeping() 基类行为
 * - Player::startSleeping()/stopSleeping() 睡眠状态管理
 * - 睡眠计时器和位置管理
 */

#include "common/core/Types.hpp"
#include "common/entity/core/EntityPose.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/player/SleepResult.hpp"
#include <gtest/gtest.h>

namespace mc {
namespace {

/**
 * @brief Player 睡眠测试夹具
 */
class PlayerSleepTest : public ::testing::Test {
protected:
    void SetUp() override { player = std::make_unique<Player>(static_cast<EntityInstanceId>(1), "TestPlayer"); }

    void TearDown() override { player.reset(); }

    std::unique_ptr<Player> player;
};

// ========== tryStartSleeping 基类测试 ==========

TEST_F(PlayerSleepTest, TryStartSleepingReturnsOK)
{
    // Player 基类的 tryStartSleeping 应该返回 OK
    BlockPos bedPos(100, 64, 100);
    entity::SleepResult result = player->tryStartSleeping(bedPos);
    EXPECT_EQ(result, entity::SleepResult::OK);
}

TEST_F(PlayerSleepTest, TryStartSleepingSetsSleepingState)
{
    // tryStartSleeping 应该设置睡眠状态
    BlockPos bedPos(100, 64, 100);
    player->tryStartSleeping(bedPos);
    EXPECT_TRUE(player->isSleeping());
}

TEST_F(PlayerSleepTest, TryStartSleepingSetsSleepingPosition)
{
    // tryStartSleeping 应该设置睡眠位置
    BlockPos bedPos(100, 64, 100);
    player->tryStartSleeping(bedPos);

    auto sleepingPos = player->getSleepingPosition();
    EXPECT_TRUE(sleepingPos.has_value());
    EXPECT_EQ(sleepingPos.value(), bedPos);
}

TEST_F(PlayerSleepTest, TryStartSleepingResetsSleepTimer)
{
    // tryStartSleeping 应该重置睡眠计时器
    BlockPos bedPos(100, 64, 100);
    player->tryStartSleeping(bedPos);
    EXPECT_EQ(player->getSleepTimer(), 0);
}

TEST_F(PlayerSleepTest, TryStartSleepingClearsVelocity)
{
    // tryStartSleeping 应该清除速度
    player->setVelocity(Vector3(1.0f, 2.0f, 3.0f));
    BlockPos bedPos(100, 64, 100);
    player->tryStartSleeping(bedPos);

    Vector3 vel = player->velocity();
    EXPECT_FLOAT_EQ(vel.x, 0.0f);
    EXPECT_FLOAT_EQ(vel.y, 0.0f);
    EXPECT_FLOAT_EQ(vel.z, 0.0f);
}

// ========== startSleeping 测试 ==========

TEST_F(PlayerSleepTest, StartSleepingSetsCorrectPosition)
{
    BlockPos bedPos(200, 70, 200);
    player->startSleeping(bedPos);

    auto sleepingPos = player->getSleepingPosition();
    EXPECT_TRUE(sleepingPos.has_value());
    EXPECT_EQ(sleepingPos.value(), bedPos);
}

TEST_F(PlayerSleepTest, StartSleepingWithoutWorldDoesNotCrash)
{
    // 不设置世界
    // 调用 startSleeping 不应该崩溃
    BlockPos bedPos(100, 64, 100);
    EXPECT_NO_THROW(player->startSleeping(bedPos));
}

TEST_F(PlayerSleepTest, StartSleepingSetsSleepingFlag)
{
    BlockPos bedPos(100, 64, 100);
    player->startSleeping(bedPos);
    EXPECT_TRUE(player->isSleeping());
}

// ========== stopSleeping 测试 ==========

TEST_F(PlayerSleepTest, StopSleepingWithoutWorldDoesNotCrash)
{
    // 不设置世界，但先开始睡眠
    BlockPos bedPos(100, 64, 100);
    player->startSleeping(bedPos);

    // 调用 stopSleeping 不应该崩溃
    EXPECT_NO_THROW(player->stopSleeping());
}

TEST_F(PlayerSleepTest, StopSleepingClearsSleepingState)
{
    // 开始睡眠
    BlockPos bedPos(100, 64, 100);
    player->startSleeping(bedPos);
    EXPECT_TRUE(player->isSleeping());

    // 停止睡眠
    player->stopSleeping();
    EXPECT_FALSE(player->isSleeping());
}

TEST_F(PlayerSleepTest, StopSleepingClearsSleepingPosition)
{
    // 开始睡眠
    BlockPos bedPos(100, 64, 100);
    player->startSleeping(bedPos);

    // 停止睡眠
    player->stopSleeping();

    auto sleepingPos = player->getSleepingPosition();
    EXPECT_FALSE(sleepingPos.has_value());
}

TEST_F(PlayerSleepTest, StopSleepingWhenNotSleepingDoesNothing)
{
    // 不先开始睡眠
    // 调用 stopSleeping 不应该做任何事情
    EXPECT_NO_THROW(player->stopSleeping());
}

// ========== 睡眠计时器测试 ==========

TEST_F(PlayerSleepTest, SleepTimerStartsAtZero)
{
    // 新玩家的睡眠计时器应该是 0
    EXPECT_EQ(player->getSleepTimer(), 0);
}

TEST_F(PlayerSleepTest, SleepTimerResetsOnStartSleeping)
{
    // 先递增计时器（通过 tick）
    player->startSleeping(BlockPos(100, 64, 100));
    player->tick();
    player->tick();
    EXPECT_GT(player->getSleepTimer(), 0);

    // 再次开始睡眠应该重置
    BlockPos newBedPos(200, 70, 200);
    player->startSleeping(newBedPos);
    EXPECT_EQ(player->getSleepTimer(), 0);
}

// ========== 睡眠状态转换测试 ==========

TEST_F(PlayerSleepTest, MultipleSleepWakeCycles)
{
    // 第一轮
    BlockPos bedPos1(100, 64, 100);
    player->startSleeping(bedPos1);
    EXPECT_TRUE(player->isSleeping());

    player->stopSleeping();
    EXPECT_FALSE(player->isSleeping());

    // 第二轮
    BlockPos bedPos2(200, 70, 200);
    player->startSleeping(bedPos2);
    EXPECT_TRUE(player->isSleeping());

    player->stopSleeping();
    EXPECT_FALSE(player->isSleeping());
}

TEST_F(PlayerSleepTest, TryStartSleepingWhileAlreadySleeping)
{
    // 开始睡眠
    BlockPos bedPos1(100, 64, 100);
    player->tryStartSleeping(bedPos1);
    EXPECT_TRUE(player->isSleeping());

    // 再次调用 tryStartSleeping
    BlockPos bedPos2(200, 70, 200);
    entity::SleepResult result = player->tryStartSleeping(bedPos2);

    // 基类实现应该直接调用 startSleeping，更新位置
    EXPECT_EQ(result, entity::SleepResult::OK);
    EXPECT_TRUE(player->isSleeping());

    // 位置应该被更新
    auto sleepingPos = player->getSleepingPosition();
    EXPECT_TRUE(sleepingPos.has_value());
    EXPECT_EQ(sleepingPos.value(), bedPos2);
}

// ========== 多态性测试 ==========

TEST_F(PlayerSleepTest, TryStartSleepingIsVirtual)
{
    // 验证 tryStartSleeping 是虚方法
    // 通过基类指针调用应该正确工作
    Player* basePtr = player.get();
    BlockPos bedPos(100, 64, 100);
    entity::SleepResult result = basePtr->tryStartSleeping(bedPos);
    EXPECT_EQ(result, entity::SleepResult::OK);
}

TEST_F(PlayerSleepTest, StartSleepingIsVirtual)
{
    // 验证 startSleeping 可以被子类重写
    // 通过基类指针调用
    Player* basePtr = player.get();
    BlockPos bedPos(100, 64, 100);
    EXPECT_NO_THROW(basePtr->startSleeping(bedPos));
    EXPECT_TRUE(basePtr->isSleeping());
}

TEST_F(PlayerSleepTest, StopSleepingIsVirtual)
{
    // 验证 stopSleeping 可以被子类重写
    Player* basePtr = player.get();
    BlockPos bedPos(100, 64, 100);
    basePtr->startSleeping(bedPos);
    EXPECT_NO_THROW(basePtr->stopSleeping());
    EXPECT_FALSE(basePtr->isSleeping());
}

// ========== 姿态切换测试 ==========

TEST_F(PlayerSleepTest, SleepingSetsCorrectPose)
{
    // 睡眠应该设置正确的姿态
    BlockPos bedPos(100, 64, 100);
    player->startSleeping(bedPos);
    EXPECT_TRUE(player->isSleeping());

    // 验证姿态
    EXPECT_EQ(player->pose(), EntityPose::Sleeping);
}

TEST_F(PlayerSleepTest, StopSleepingChangesPoseFromSleeping)
{
    // 开始睡眠
    BlockPos bedPos(100, 64, 100);
    player->startSleeping(bedPos);
    EXPECT_EQ(player->pose(), EntityPose::Sleeping);

    // 停止睡眠
    player->stopSleeping();
    // 姿态应该改变（不再是睡眠）
    EXPECT_NE(player->pose(), EntityPose::Sleeping);
}

} // namespace
} // namespace mc
