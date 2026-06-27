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
 * @file PlayerSpectatorTest.cpp
 * @brief Player 旁观者模式跟踪状态单元测试
 *
 * 测试 Player 基类中旁观者摄像机跟踪的相关方法：
 * - m_cameraEntityId 字段的读写（getCameraEntityId/setCameraEntityId）
 * - isSpectating() 状态查询
 * - 旁观者模式下 noclip 设置
 * - 离开旁观者模式时 camera 清除
 * - 旁观者模式下 attack() 设置旁观目标
 */

#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include <gtest/gtest.h>

using namespace mc;

// ==================== Player 旁观者摄像机跟踪测试 ====================

class PlayerSpectatorTest : public ::testing::Test {
protected:
    void SetUp() override { player = std::make_unique<Player>(EntityId(1), "TestPlayer"); }

    void TearDown() override { player.reset(); }

    std::unique_ptr<Player> player;
};

// ---------- 基础状态测试 ----------

TEST_F(PlayerSpectatorTest, DefaultNotSpectating)
{
    // 默认情况下玩家没有旁观目标
    EXPECT_FALSE(player->isSpectating());
    EXPECT_FALSE(player->getCameraEntityId().has_value());
}

TEST_F(PlayerSpectatorTest, SetCameraEntityId)
{
    // 设置旁观目标
    player->setCameraEntityId(EntityId(42));
    EXPECT_TRUE(player->isSpectating());
    EXPECT_TRUE(player->getCameraEntityId().has_value());
    EXPECT_EQ(player->getCameraEntityId().value(), EntityId(42));
}

TEST_F(PlayerSpectatorTest, SetCameraEntityIdToNullopt)
{
    // 设置旁观目标后清除
    player->setCameraEntityId(EntityId(42));
    EXPECT_TRUE(player->isSpectating());

    player->setCameraEntityId(std::nullopt);
    EXPECT_FALSE(player->isSpectating());
    EXPECT_FALSE(player->getCameraEntityId().has_value());
}

TEST_F(PlayerSpectatorTest, SetCameraEntityIdOverwrite)
{
    // 多次设置旁观目标，后设置的覆盖前一个
    player->setCameraEntityId(EntityId(10));
    EXPECT_EQ(player->getCameraEntityId().value(), EntityId(10));

    player->setCameraEntityId(EntityId(20));
    EXPECT_EQ(player->getCameraEntityId().value(), EntityId(20));
    EXPECT_TRUE(player->isSpectating());
}

TEST_F(PlayerSpectatorTest, SetCameraEntityIdZeroIsValid)
{
    // 实体 ID 为 0 也应该合法（虽然实际中不太可能）
    player->setCameraEntityId(EntityId(0));
    EXPECT_TRUE(player->isSpectating());
    EXPECT_EQ(player->getCameraEntityId().value(), EntityId(0));
}

// ---------- 旁观者模式与 noclip 测试 ----------

TEST_F(PlayerSpectatorTest, SpectatorModeEnablesNoclip)
{
    // 切换到旁观者模式应该启用 noclip
    EXPECT_FALSE(player->noClip());

    player->setGameMode(GameMode::Spectator);
    EXPECT_TRUE(player->noClip());
    EXPECT_TRUE(player->isSpectator());
}

TEST_F(PlayerSpectatorTest, LeaveSpectatorModeDisablesNoclip)
{
    // 切换到旁观者模式后离开，应该关闭 noclip
    player->setGameMode(GameMode::Spectator);
    EXPECT_TRUE(player->noClip());

    player->setGameMode(GameMode::Survival);
    EXPECT_FALSE(player->noClip());
    EXPECT_FALSE(player->isSpectator());
}

TEST_F(PlayerSpectatorTest, LeaveSpectatorModeClearsCamera)
{
    // 切换到旁观者模式并设置旁观目标，离开旁观模式时应该清除
    player->setGameMode(GameMode::Spectator);
    player->setCameraEntityId(EntityId(100));
    EXPECT_TRUE(player->isSpectating());

    // 离开旁观者模式
    player->setGameMode(GameMode::Survival);
    EXPECT_FALSE(player->isSpectating());
    EXPECT_FALSE(player->getCameraEntityId().has_value());
}

TEST_F(PlayerSpectatorTest, SwitchBetweenSpectatorAndCreative)
{
    // 旁观者 → 创造 → 旁观者
    player->setGameMode(GameMode::Spectator);
    player->setCameraEntityId(EntityId(50));
    EXPECT_TRUE(player->isSpectating());

    player->setGameMode(GameMode::Creative);
    EXPECT_FALSE(player->isSpectating());
    EXPECT_FALSE(player->getCameraEntityId().has_value());

    player->setGameMode(GameMode::Spectator);
    EXPECT_TRUE(player->isSpectator());
    EXPECT_TRUE(player->noClip());
}

TEST_F(PlayerSpectatorTest, SwitchFromSurvivalToSpectatorNoCamera)
{
    // 生存模式切换到旁观者模式，不应该自动设置旁观目标
    player->setGameMode(GameMode::Survival);
    EXPECT_FALSE(player->isSpectating());

    player->setGameMode(GameMode::Spectator);
    EXPECT_TRUE(player->isSpectator());   // 是旁观者模式
    EXPECT_FALSE(player->isSpectating()); // 但没有旁观目标
    EXPECT_FALSE(player->getCameraEntityId().has_value());
}

// ---------- 旁观者模式 attack() 测试 ----------

TEST_F(PlayerSpectatorTest, SpectatorAttackSetsCameraTarget)
{
    // 旁观者模式下攻击实体应该设置旁观目标而非造成伤害
    player->setGameMode(GameMode::Spectator);

    // 创建一个目标实体用于 attack 测试
    Entity target(EntityId(99));

    // 旁观者模式下 attack 不应造成伤害，但会设置旁观目标
    player->attack(target);

    // 在 Player 基类中，旁观者 attack 会设置 cameraEntityId
    EXPECT_TRUE(player->isSpectating());
    EXPECT_TRUE(player->getCameraEntityId().has_value());
    EXPECT_EQ(player->getCameraEntityId().value(), EntityId(99));
}

TEST_F(PlayerSpectatorTest, NonSpectatorAttackDoesNotSetCamera)
{
    // 非旁观者模式下攻击不应该设置旁观目标
    player->setGameMode(GameMode::Survival);

    Entity target(EntityId(99));
    // 注意：非旁观者的 attack 会正常执行攻击逻辑，
    // 但对于没有世界/没有 LivingEntity 目标的情况，attack 会提前返回
    player->attack(target);

    // 不应该设置旁观目标
    EXPECT_FALSE(player->isSpectating());
    EXPECT_FALSE(player->getCameraEntityId().has_value());
}

// ---------- isInputSneaking 测试 ----------

TEST_F(PlayerSpectatorTest, IsInputSneakingDefaultFalse)
{
    // 默认不潜行
    EXPECT_FALSE(player->isInputSneaking());
}
