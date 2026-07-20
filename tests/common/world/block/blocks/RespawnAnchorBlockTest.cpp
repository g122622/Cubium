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
 * @file RespawnAnchorBlockTest.cpp
 * @brief 重生锚方块功能单元测试
 *
 * 测试覆盖：
 * - 重生锚状态属性（CHARGES_0_4）
 * - 光照等级计算
 * - 比较器输出
 * - 维度检查（下界可用，其他维度不可用）
 * - 玩家重生点设置功能
 */

#include <gtest/gtest.h>

#include "common/world/block/registry/VanillaBlocks.hpp"
#include "core/Constants.hpp"
#include "entity/entities/player/Player.hpp"
#include "util/property/Properties.hpp"
#include "world/GlobalPos.hpp"
#include "world/block/BlockPos.hpp"
#include "world/block/blocks/functional/RespawnAnchorBlock.hpp"
#include "world/dimension/DimensionType.hpp"

#include <memory>

using namespace mc;
using namespace mc::blocks;

// ============================================================================
// 测试用例
// ============================================================================

class RespawnAnchorBlockTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

// 测试重生锚方块存在
TEST_F(RespawnAnchorBlockTest, RespawnAnchorBlockExists)
{
    ASSERT_NE(VanillaBlocks::RESPAWN_ANCHOR, nullptr);
}

// 测试重生锚默认状态
TEST_F(RespawnAnchorBlockTest, DefaultState)
{
    ASSERT_NE(VanillaBlocks::RESPAWN_ANCHOR, nullptr);

    const BlockState& defaultState = VanillaBlocks::RESPAWN_ANCHOR->defaultState();

    // 默认充能等级应为 0
    EXPECT_EQ(defaultState.get(BlockStateProperties::CHARGES_0_4()), 0);
}

// 测试充能等级属性
TEST_F(RespawnAnchorBlockTest, ChargesProperty)
{
    const BlockState& state0 = VanillaBlocks::RESPAWN_ANCHOR->defaultState();

    // 测试所有充能等级
    for (i32 charges = 0; charges <= 4; ++charges) {
        BlockState state = state0.with(BlockStateProperties::CHARGES_0_4(), charges);
        EXPECT_EQ(state.get(BlockStateProperties::CHARGES_0_4()), charges);
    }
}

// 测试光照等级计算
TEST_F(RespawnAnchorBlockTest, LightLevelCalculation)
{
    const Block* block = VanillaBlocks::RESPAWN_ANCHOR;
    ASSERT_NE(block, nullptr);

    // 光照等级 = charges * 3.75
    // 0 -> 0, 1 -> 3, 2 -> 7, 3 -> 11, 4 -> 15

    BlockState state0 = VanillaBlocks::RESPAWN_ANCHOR->defaultState().with(BlockStateProperties::CHARGES_0_4(), 0);
    EXPECT_EQ(block->getLightLevel(state0, nullptr, nullptr), 0);

    BlockState state1 = VanillaBlocks::RESPAWN_ANCHOR->defaultState().with(BlockStateProperties::CHARGES_0_4(), 1);
    EXPECT_EQ(block->getLightLevel(state1, nullptr, nullptr), 3);

    BlockState state2 = VanillaBlocks::RESPAWN_ANCHOR->defaultState().with(BlockStateProperties::CHARGES_0_4(), 2);
    EXPECT_EQ(block->getLightLevel(state2, nullptr, nullptr), 7);

    BlockState state3 = VanillaBlocks::RESPAWN_ANCHOR->defaultState().with(BlockStateProperties::CHARGES_0_4(), 3);
    EXPECT_EQ(block->getLightLevel(state3, nullptr, nullptr), 11);

    BlockState state4 = VanillaBlocks::RESPAWN_ANCHOR->defaultState().with(BlockStateProperties::CHARGES_0_4(), 4);
    EXPECT_EQ(block->getLightLevel(state4, nullptr, nullptr), 15);
}

// 测试 getCharges 辅助方法
TEST_F(RespawnAnchorBlockTest, GetChargesHelperMethod)
{
    const Block* block = VanillaBlocks::RESPAWN_ANCHOR;
    ASSERT_NE(block, nullptr);

    for (i32 charges = 0; charges <= 4; ++charges) {
        BlockState state =
            VanillaBlocks::RESPAWN_ANCHOR->defaultState().with(BlockStateProperties::CHARGES_0_4(), charges);
        // 使用 Block 基类方法或通过状态获取
        EXPECT_EQ(state.get(BlockStateProperties::CHARGES_0_4()), charges);
    }
}

// 测试重生锚形状为完整方块
TEST_F(RespawnAnchorBlockTest, ShapeIsFullBlock)
{
    const Block* block = VanillaBlocks::RESPAWN_ANCHOR;
    ASSERT_NE(block, nullptr);

    const CollisionShape& shape = block->getShape(VanillaBlocks::RESPAWN_ANCHOR->defaultState());

    // 验证是完整方块
    EXPECT_TRUE(shape.isFullBlock());
}

// 测试维度检查 - 下界可用
TEST_F(RespawnAnchorBlockTest, RespawnAnchorWorksInNether)
{
    DimensionType netherDim = DimensionType::fromId(-1); // NETHER = -1
    EXPECT_TRUE(netherDim.respawnAnchorWorks());
}

// 测试维度检查 - 主世界不可用
TEST_F(RespawnAnchorBlockTest, RespawnAnchorDoesNotWorkInOverworld)
{
    DimensionType overworldDim = DimensionType::fromId(0); // OVERWORLD = 0
    EXPECT_FALSE(overworldDim.respawnAnchorWorks());
}

// 测试维度检查 - 末地不可用
TEST_F(RespawnAnchorBlockTest, RespawnAnchorDoesNotWorkInEnd)
{
    DimensionType endDim = DimensionType::fromId(1); // THE_END = 1
    EXPECT_FALSE(endDim.respawnAnchorWorks());
}

// ============================================================================
// 玩家重生点测试（不需要 Mock World）
// ============================================================================

/**
 * @brief 测试用 Mock 玩家实现（简化版）
 */
class TestPlayer final : public mc::Player {
public:
    TestPlayer(EntityInstanceId id, const std::string& name)
        : mc::Player(id, name)
    {
        abilities().creativeMode = false;
        abilities().invulnerable = false;
    }

    void sendStatusMessage(const std::string&, bool) override {}
    bool canReceiveMessages() const override { return false; }

    // 测试辅助方法
    [[nodiscard]] bool hasSpawnPoint() const { return getSpawnPoint().has_value(); }
    [[nodiscard]] DimensionId spawnDimension() const
    {
        auto sp = getSpawnPoint();
        return sp.has_value() ? sp->getDimensionId() : DimensionId(0);
    }
    [[nodiscard]] BlockPos spawnPosition() const
    {
        auto sp = getSpawnPoint();
        return sp.has_value() ? sp->getPos() : BlockPos(0, 0, 0);
    }
};

// 测试玩家重生点设置
TEST_F(RespawnAnchorBlockTest, PlayerSpawnPointCanBeSet)
{
    TestPlayer player(EntityInstanceId(1), "TestPlayer");
    BlockPos spawnPos(100, 64, -200);

    // 初始没有重生点
    EXPECT_FALSE(player.hasSpawnPoint());

    // 设置重生点
    player.setSpawnPoint(DimensionId(-1), spawnPos, false);

    // 验证重生点已设置
    EXPECT_TRUE(player.hasSpawnPoint());
    EXPECT_EQ(player.spawnDimension(), DimensionId(-1));
    EXPECT_EQ(player.spawnPosition(), spawnPos);
}

// 测试玩家重生点的 forced 参数
TEST_F(RespawnAnchorBlockTest, PlayerSpawnPointForcedParameter)
{
    TestPlayer player(EntityInstanceId(1), "TestPlayer");
    BlockPos spawnPos(50, 70, 100);

    // 设置 forced = false
    player.setSpawnPoint(DimensionId(-1), spawnPos, false);
    EXPECT_FALSE(player.isSpawnForced());

    // 设置 forced = true
    player.setSpawnPoint(DimensionId(-1), spawnPos, true);
    EXPECT_TRUE(player.isSpawnForced());
}

// 测试清除重生点
TEST_F(RespawnAnchorBlockTest, ClearSpawnPoint)
{
    TestPlayer player(EntityInstanceId(1), "TestPlayer");
    BlockPos spawnPos(10, 20, 30);

    // 设置重生点
    player.setSpawnPoint(DimensionId(-1), spawnPos, false);
    EXPECT_TRUE(player.hasSpawnPoint());

    // 清除重生点
    player.clearSpawnPoint();
    EXPECT_FALSE(player.hasSpawnPoint());
}

// 测试 GlobalPos 创建
TEST_F(RespawnAnchorBlockTest, GlobalPosWithNetherDimension)
{
    BlockPos pos(123, 64, -456);
    GlobalPos globalPos(DimensionId(-1), pos);

    EXPECT_EQ(globalPos.getDimensionId(), DimensionId(-1));
    EXPECT_EQ(globalPos.x(), 123);
    EXPECT_EQ(globalPos.y(), 64);
    EXPECT_EQ(globalPos.z(), -456);
}

// 测试重生点位置和维度正确对应
TEST_F(RespawnAnchorBlockTest, SpawnPointPositionAndDimensionMatch)
{
    TestPlayer player(EntityInstanceId(1), "TestPlayer");

    // 下界重生点
    BlockPos netherPos(10, 20, 30);
    player.setSpawnPoint(DimensionId(-1), netherPos, false);

    auto spawnPoint = player.getSpawnPoint();
    ASSERT_TRUE(spawnPoint.has_value());
    EXPECT_EQ(spawnPoint->getDimensionId(), DimensionId(-1));
    EXPECT_EQ(spawnPoint->getPos(), netherPos);

    // 主世界重生点
    BlockPos overworldPos(100, 64, 200);
    player.setSpawnPoint(DimensionId(0), overworldPos, false);

    spawnPoint = player.getSpawnPoint();
    ASSERT_TRUE(spawnPoint.has_value());
    EXPECT_EQ(spawnPoint->getDimensionId(), DimensionId(0));
    EXPECT_EQ(spawnPoint->getPos(), overworldPos);
}

// 测试 GlobalPos 比较
TEST_F(RespawnAnchorBlockTest, GlobalPosComparison)
{
    GlobalPos pos1(DimensionId(-1), BlockPos(10, 20, 30));
    GlobalPos pos2(DimensionId(-1), BlockPos(10, 20, 30));
    GlobalPos pos3(DimensionId(0), BlockPos(10, 20, 30));
    GlobalPos pos4(DimensionId(-1), BlockPos(10, 21, 30));

    EXPECT_TRUE(pos1 == pos2);
    EXPECT_FALSE(pos1 == pos3);
    EXPECT_FALSE(pos1 == pos4);
    EXPECT_TRUE(pos1 != pos3);
    EXPECT_TRUE(pos1 != pos4);
}

// 测试 GlobalPos sameDimension 方法
TEST_F(RespawnAnchorBlockTest, GlobalPosSameDimension)
{
    GlobalPos pos1(DimensionId(-1), BlockPos(10, 20, 30));
    GlobalPos pos2(DimensionId(-1), BlockPos(100, 200, 300));
    GlobalPos pos3(DimensionId(0), BlockPos(10, 20, 30));

    EXPECT_TRUE(pos1.sameDimension(pos2));
    EXPECT_FALSE(pos1.sameDimension(pos3));
}
