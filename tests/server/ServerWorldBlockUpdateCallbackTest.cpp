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

#include <gtest/gtest.h>

#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "server/world/ServerWorld.hpp"

#include <utility>
#include <vector>

using namespace mc;
using namespace mc::server;

namespace {

class ServerWorldBlockUpdateCallbackTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();

        ServerWorldConfig config;
        config.viewDistance = 8;
        config.dimension = 0;
        config.seed = 12345;
        // 注意：isDebugWorld 字段已移除，改用 isDebugWorld() 方法通过检测区块生成器类型判断

        m_world = std::make_unique<ServerWorld>(config);
        auto result = m_world->initialize();
        ASSERT_TRUE(result.success());
    }

    void TearDown() override
    {
        if (m_world) {
            m_world->shutdown();
            m_world.reset();
        }
    }

    std::unique_ptr<ServerWorld> m_world;
};

} // namespace

TEST_F(ServerWorldBlockUpdateCallbackTest, SetBlockInvokesBlockChangedCallback)
{
    std::vector<std::pair<BlockPos, u32>> blockUpdates;

    m_world->setOnBlockChanged(
        [&blockUpdates](const BlockPos& pos, u32 blockStateId) { blockUpdates.emplace_back(pos, blockStateId); });

    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    m_world->setBlockState(0, 64, 0, stoneState);
    m_world->setBlockState(0, 64, 0, nullptr);

    ASSERT_EQ(blockUpdates.size(), 2u);
    EXPECT_EQ(blockUpdates[0].first, BlockPos(0, 64, 0));
    EXPECT_EQ(blockUpdates[0].second, stoneState->stateId());
    EXPECT_EQ(blockUpdates[1].first, BlockPos(0, 64, 0));
    EXPECT_EQ(blockUpdates[1].second, 0u);
}

TEST_F(ServerWorldBlockUpdateCallbackTest, BreakingIceAboveSolidTurnsIntoWater)
{
    std::vector<std::pair<BlockPos, u32>> blockUpdates;

    m_world->setBlockState(0, 63, 0, &VanillaBlocks::STONE->defaultState());
    m_world->setBlockState(0, 64, 0, &VanillaBlocks::ICE->defaultState());

    m_world->setOnBlockChanged(
        [&blockUpdates](const BlockPos& pos, u32 blockStateId) { blockUpdates.emplace_back(pos, blockStateId); });

    ASSERT_TRUE(m_world->setBlockState(0, 64, 0, nullptr));

    const BlockState* finalState = m_world->getBlockState(0, 64, 0);
    ASSERT_NE(finalState, nullptr);
    EXPECT_EQ(finalState->stateId(), VanillaBlocks::WATER->defaultState().stateId());

    ASSERT_EQ(blockUpdates.size(), 1u);
    EXPECT_EQ(blockUpdates[0].first, BlockPos(0, 64, 0));
    EXPECT_EQ(blockUpdates[0].second, VanillaBlocks::WATER->defaultState().stateId());
}

TEST_F(ServerWorldBlockUpdateCallbackTest, BreakingIceWithoutSupportTurnsIntoAir)
{
    std::vector<std::pair<BlockPos, u32>> blockUpdates;

    m_world->setBlockState(0, 63, 0, nullptr);
    m_world->setBlockState(0, 64, 0, &VanillaBlocks::ICE->defaultState());

    m_world->setOnBlockChanged(
        [&blockUpdates](const BlockPos& pos, u32 blockStateId) { blockUpdates.emplace_back(pos, blockStateId); });

    ASSERT_TRUE(m_world->setBlockState(0, 64, 0, nullptr));

    const BlockState* finalState = m_world->getBlockState(0, 64, 0);
    ASSERT_NE(finalState, nullptr);
    EXPECT_TRUE(finalState->isAir());

    ASSERT_EQ(blockUpdates.size(), 1u);
    EXPECT_EQ(blockUpdates[0].first, BlockPos(0, 64, 0));
    EXPECT_EQ(blockUpdates[0].second, 0u);
}

// ========== notifyBlockUpdate 测试 ==========

TEST_F(ServerWorldBlockUpdateCallbackTest, NotifyBlockUpdate_InvokesCallback)
{
    // 设置方块，确保区块已加载
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    m_world->setBlockState(5, 64, 5, stoneState);

    std::vector<std::pair<BlockPos, u32>> blockUpdates;
    m_world->setOnBlockChanged(
        [&blockUpdates](const BlockPos& pos, u32 blockStateId) { blockUpdates.emplace_back(pos, blockStateId); });

    // notifyBlockUpdate 应该即使方块状态未改变也触发回调
    m_world->notifyBlockUpdate(BlockPos(5, 64, 5));

    ASSERT_EQ(blockUpdates.size(), 1u);
    EXPECT_EQ(blockUpdates[0].first, BlockPos(5, 64, 5));
    EXPECT_EQ(blockUpdates[0].second, stoneState->stateId());
}

TEST_F(ServerWorldBlockUpdateCallbackTest, NotifyBlockUpdate_DoesNotChangeBlockState)
{
    // 设置方块
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    m_world->setBlockState(5, 64, 5, stoneState);

    // notifyBlockUpdate 不应该改变方块状态
    m_world->notifyBlockUpdate(BlockPos(5, 64, 5));

    const BlockState* stateAfter = m_world->getBlockState(5, 64, 5);
    ASSERT_NE(stateAfter, nullptr);
    EXPECT_EQ(stateAfter->stateId(), stoneState->stateId());
}

TEST_F(ServerWorldBlockUpdateCallbackTest, NotifyBlockUpdate_WithoutCallback_DoesNotCrash)
{
    // 不设置回调，notifyBlockUpdate 不应该崩溃
    m_world->setBlockState(5, 64, 5, &VanillaBlocks::STONE->defaultState());

    // m_onBlockChanged 为空，应安全跳过
    m_world->notifyBlockUpdate(BlockPos(5, 64, 5));

    // 如果到达这里，说明没有崩溃
    SUCCEED();
}

TEST_F(ServerWorldBlockUpdateCallbackTest, NotifyBlockUpdate_UnloadedChunk_DoesNotCrash)
{
    std::vector<std::pair<BlockPos, u32>> blockUpdates;
    m_world->setOnBlockChanged(
        [&blockUpdates](const BlockPos& pos, u32 blockStateId) { blockUpdates.emplace_back(pos, blockStateId); });

    // 远处未加载区块的坐标，getBlockState 应返回 nullptr
    m_world->notifyBlockUpdate(BlockPos(10000, 64, 10000));

    // 回调不应被触发（因为 getBlockState 返回 nullptr）
    EXPECT_TRUE(blockUpdates.empty());
}
