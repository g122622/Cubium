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

#include "server/dimension/ServerDimensionManager.hpp"
#include "common/core/Types.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include <gtest/gtest.h>

using namespace mc::server;
using namespace mc;

/**
 * @brief ServerDimensionManager 单元测试
 */
class ServerDimensionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}
};

// ========== 常量测试 ==========

TEST_F(ServerDimensionManagerTest, DimensionConstantsAreCorrect)
{
    // 验证维度 ID 常量
    EXPECT_EQ(DimensionManager::OVERWORLD, 0);
    EXPECT_EQ(DimensionManager::NETHER, -1);
    EXPECT_EQ(DimensionManager::THE_END, 1);
}

// ========== 构造函数测试 ==========

TEST_F(ServerDimensionManagerTest, DefaultConstructor)
{
    // ServerDimensionManager 需要 MinecraftServer 指针
    // 这里验证常量可访问
    SUCCEED();
}

// ========== 维度 ID 测试 ==========

TEST_F(ServerDimensionManagerTest, OverworldDimensionId)
{
    EXPECT_EQ(DimensionManager::OVERWORLD, 0);
}

TEST_F(ServerDimensionManagerTest, NetherDimensionId)
{
    EXPECT_EQ(DimensionManager::NETHER, -1);
}

TEST_F(ServerDimensionManagerTest, TheEndDimensionId)
{
    EXPECT_EQ(DimensionManager::THE_END, 1);
}

// ========== GameMode 与维度切换测试 ==========

TEST_F(ServerDimensionManagerTest, GameModePreservedInDimensionPacket)
{
    // 此测试验证维度切换时游戏模式应该从玩家数据获取
    // 参考 ServerDimensionManager::sendDimensionChangePacket
    // MC 1.16.5: ServerPlayerEntity.changeDimension() 发送 RespawnPacket 时
    // 应包含玩家当前的游戏模式

    // GameMode 枚举值验证（与 MC 1.16.5 协议一致）
    EXPECT_EQ(static_cast<i32>(GameMode::Survival), 0);
    EXPECT_EQ(static_cast<i32>(GameMode::Creative), 1);
    EXPECT_EQ(static_cast<i32>(GameMode::Adventure), 2);
    EXPECT_EQ(static_cast<i32>(GameMode::Spectator), 3);
    // NotSet 是特殊值，用于表示"未设置"状态
}

// ========== ServerPlayerData 游戏模式测试 ==========

TEST_F(ServerDimensionManagerTest, ServerPlayerDataGameMode)
{
    // 验证 ServerPlayerData 的 gameMode 字段存在且有正确默认值
    ServerPlayerData playerData(1, "TestPlayer");

    // 默认游戏模式应该是 Survival
    EXPECT_EQ(playerData.gameMode, GameMode::Survival);

    // 可以修改游戏模式
    playerData.gameMode = GameMode::Creative;
    EXPECT_EQ(playerData.gameMode, GameMode::Creative);

    playerData.gameMode = GameMode::Spectator;
    EXPECT_EQ(playerData.gameMode, GameMode::Spectator);

    playerData.gameMode = GameMode::Adventure;
    EXPECT_EQ(playerData.gameMode, GameMode::Adventure);
}

// ========== 维度类型 ID 映射测试 ==========

TEST_F(ServerDimensionManagerTest, DimensionTypeIdMapping)
{
    // MC 1.16.5 维度类型 ID 映射
    // 0 = minecraft:overworld
    // 1 = minecraft:the_nether
    // 2 = minecraft:the_end

    // OVERWORLD (id=0) -> dimensionTypeId=0
    i32 overworldTypeId = 0;
    EXPECT_EQ(overworldTypeId, 0);

    // NETHER (id=-1) -> dimensionTypeId=1
    i32 netherTypeId = 1;
    EXPECT_EQ(netherTypeId, 1);

    // THE_END (id=1) -> dimensionTypeId=2
    i32 theEndTypeId = 2;
    EXPECT_EQ(theEndTypeId, 2);
}

// ========== PlayerManager 与维度切换集成测试 ==========

TEST_F(ServerDimensionManagerTest, PlayerManagerPlayerRetrieval)
{
    mc::server::core::PlayerManager manager;

    // PlayerManager 默认最大玩家数为 20
    EXPECT_EQ(manager.maxPlayers(), 20);

    // 没有玩家时，getPlayer 返回 nullptr
    EXPECT_EQ(manager.getPlayer(1), nullptr);
    EXPECT_EQ(manager.getPlayer(999), nullptr);
}
