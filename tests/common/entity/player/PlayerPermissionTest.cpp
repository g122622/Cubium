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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY KIND, EXPRESS OR IMPLIED,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "common/entity/entities/player/GameModeUtils.hpp"
#include "common/entity/entities/player/Player.hpp"
#include <gtest/gtest.h>

using namespace mc;

// ========== Player 权限等级测试 ==========

TEST(PlayerPermissionLevelTest, DefaultPermissionLevelIsZero)
{
    // 默认创建的玩家权限等级为 0（普通玩家）
    Player player(EntityInstanceId(1), "TestPlayer");
    EXPECT_EQ(player.permissionLevel(), 0);
    EXPECT_FALSE(player.hasPermission(1));
    EXPECT_FALSE(player.hasPermission(2));
}

TEST(PlayerPermissionLevelTest, SetPermissionLevel)
{
    Player player(EntityInstanceId(1), "TestPlayer");

    player.setPermissionLevel(2);
    EXPECT_EQ(player.permissionLevel(), 2);
    EXPECT_TRUE(player.hasPermission(0));
    EXPECT_TRUE(player.hasPermission(1));
    EXPECT_TRUE(player.hasPermission(2));
    EXPECT_FALSE(player.hasPermission(3));
    EXPECT_FALSE(player.hasPermission(4));
}

TEST(PlayerPermissionLevelTest, SetPermissionLevelOwner)
{
    Player player(EntityInstanceId(1), "TestPlayer");

    player.setPermissionLevel(4);
    EXPECT_EQ(player.permissionLevel(), 4);
    EXPECT_TRUE(player.hasPermission(0));
    EXPECT_TRUE(player.hasPermission(1));
    EXPECT_TRUE(player.hasPermission(2));
    EXPECT_TRUE(player.hasPermission(3));
    EXPECT_TRUE(player.hasPermission(4));
}

TEST(PlayerPermissionLevelTest, HasPermissionBoundary)
{
    Player player(EntityInstanceId(1), "TestPlayer");

    player.setPermissionLevel(1);
    EXPECT_TRUE(player.hasPermission(0));
    EXPECT_TRUE(player.hasPermission(1));
    EXPECT_FALSE(player.hasPermission(2));
}

// ========== canUseGameMasterBlocks 测试 ==========

TEST(PlayerCanUseGameMasterBlocksTest, SurvivalModeWithNoPermissionCannotUse)
{
    // 生存模式 + 无 OP 权限 = 不能使用管理员方块
    Player player(EntityInstanceId(1), "TestPlayer");
    EXPECT_EQ(player.gameMode(), GameMode::Survival);
    EXPECT_EQ(player.permissionLevel(), 0);
    EXPECT_FALSE(player.canUseGameMasterBlocks());
}

TEST(PlayerCanUseGameMasterBlocksTest, CreativeModeWithNoPermissionCannotUse)
{
    // 创造模式 + 无 OP 权限 = 不能使用管理员方块
    Player player(EntityInstanceId(1), "TestPlayer");
    player.setGameMode(GameMode::Creative);
    EXPECT_EQ(player.permissionLevel(), 0);
    EXPECT_FALSE(player.canUseGameMasterBlocks());
}

TEST(PlayerCanUseGameMasterBlocksTest, SurvivalModeWithOpPermissionCannotUse)
{
    // 生存模式 + OP 权限 = 不能使用管理员方块（需要创造模式）
    Player player(EntityInstanceId(1), "TestPlayer");
    player.setPermissionLevel(2);
    EXPECT_EQ(player.gameMode(), GameMode::Survival);
    EXPECT_FALSE(player.canUseGameMasterBlocks());
}

TEST(PlayerCanUseGameMasterBlocksTest, CreativeModeWithGameMasterPermissionCanUse)
{
    // 创造模式 + OP 等级 >= 2 = 可以使用管理员方块
    Player player(EntityInstanceId(1), "TestPlayer");
    player.setGameMode(GameMode::Creative);
    player.setPermissionLevel(2);
    EXPECT_TRUE(player.canUseGameMasterBlocks());
}

TEST(PlayerCanUseGameMasterBlocksTest, CreativeModeWithAdminPermissionCanUse)
{
    // 创造模式 + OP 等级 3 也可以
    Player player(EntityInstanceId(1), "TestPlayer");
    player.setGameMode(GameMode::Creative);
    player.setPermissionLevel(3);
    EXPECT_TRUE(player.canUseGameMasterBlocks());
}

TEST(PlayerCanUseGameMasterBlocksTest, CreativeModeWithOwnerPermissionCanUse)
{
    // 创造模式 + OP 等级 4 也可以
    Player player(EntityInstanceId(1), "TestPlayer");
    player.setGameMode(GameMode::Creative);
    player.setPermissionLevel(4);
    EXPECT_TRUE(player.canUseGameMasterBlocks());
}

TEST(PlayerCanUseGameMasterBlocksTest, CreativeModeWithModeratorPermissionCannotUse)
{
    // 创造模式 + OP 等级 1（版主）= 不能使用管理员方块（需要 >= 2）
    Player player(EntityInstanceId(1), "TestPlayer");
    player.setGameMode(GameMode::Creative);
    player.setPermissionLevel(1);
    EXPECT_FALSE(player.canUseGameMasterBlocks());
}

TEST(PlayerCanUseGameMasterBlocksTest, SpectatorModeWithOpPermissionCannotUse)
{
    // 旁观者模式即使有 OP 权限也不能使用管理员方块
    Player player(EntityInstanceId(1), "TestPlayer");
    player.setGameMode(GameMode::Spectator);
    player.setPermissionLevel(2);
    EXPECT_FALSE(player.canUseGameMasterBlocks());
}

TEST(PlayerCanUseGameMasterBlocksTest, AdventureModeWithOpPermissionCannotUse)
{
    // 冒险模式即使有 OP 权限也不能使用管理员方块
    Player player(EntityInstanceId(1), "TestPlayer");
    player.setGameMode(GameMode::Adventure);
    player.setPermissionLevel(2);
    EXPECT_FALSE(player.canUseGameMasterBlocks());
}

TEST(PlayerCanUseGameMasterBlocksTest, SetGameModeResetsAbilitiesButNotPermissionLevel)
{
    // 切换游戏模式会重置能力，但不会重置权限等级
    Player player(EntityInstanceId(1), "TestPlayer");
    player.setPermissionLevel(2);
    player.setGameMode(GameMode::Creative);
    EXPECT_TRUE(player.canUseGameMasterBlocks());

    // 切换到生存模式，creativeMode 变为 false
    player.setGameMode(GameMode::Survival);
    EXPECT_EQ(player.permissionLevel(), 2);        // 权限等级不变
    EXPECT_FALSE(player.canUseGameMasterBlocks()); // 但不能使用管理员方块

    // 再切换回创造模式
    player.setGameMode(GameMode::Creative);
    EXPECT_TRUE(player.canUseGameMasterBlocks());
}

// ========== GameMode 与权限等级组合的完整覆盖 ==========

class CanUseGameMasterBlocksComboTest : public ::testing::TestWithParam<std::tuple<GameMode, i32, bool>> {};

TEST_P(CanUseGameMasterBlocksComboTest, ParameterizedCheck)
{
    auto [gameMode, permLevel, expected] = GetParam();
    Player player(EntityInstanceId(1), "TestPlayer");
    player.setGameMode(gameMode);
    player.setPermissionLevel(permLevel);
    EXPECT_EQ(player.canUseGameMasterBlocks(), expected)
        << "gameMode=" << static_cast<int>(gameMode) << " permLevel=" << permLevel;
}

INSTANTIATE_TEST_SUITE_P(CanUseGameMasterBlocksCombos,
    CanUseGameMasterBlocksComboTest,
    ::testing::Values(
        // Survival + 任何权限等级 = false
        std::make_tuple(GameMode::Survival, 0, false),
        std::make_tuple(GameMode::Survival, 1, false),
        std::make_tuple(GameMode::Survival, 2, false),
        std::make_tuple(GameMode::Survival, 3, false),
        std::make_tuple(GameMode::Survival, 4, false),
        // Creative + < 2 = false
        std::make_tuple(GameMode::Creative, 0, false),
        std::make_tuple(GameMode::Creative, 1, false),
        // Creative + >= 2 = true
        std::make_tuple(GameMode::Creative, 2, true),
        std::make_tuple(GameMode::Creative, 3, true),
        std::make_tuple(GameMode::Creative, 4, true),
        // Adventure = always false
        std::make_tuple(GameMode::Adventure, 0, false),
        std::make_tuple(GameMode::Adventure, 2, false),
        std::make_tuple(GameMode::Adventure, 4, false),
        // Spectator = always false
        std::make_tuple(GameMode::Spectator, 0, false),
        std::make_tuple(GameMode::Spectator, 2, false),
        std::make_tuple(GameMode::Spectator, 4, false)));
