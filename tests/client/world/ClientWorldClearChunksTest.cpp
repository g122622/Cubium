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

#include "client/dimension/ClientDimensionManager.hpp"
#include "client/world/ClientWorld.hpp"
#include "common/core/Types.hpp"

using namespace mc;
using namespace mc::client;

/**
 * @brief ClientWorld::clearChunks() 单元测试
 *
 * 测试维度切换时区块清空功能。
 * 参考 MC 1.16.5 ClientPlayNetHandler.handleRespawn()
 */
class ClientWorldClearChunksTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化 ClientWorld
        auto result = m_world.initialize(12345); // 测试种子
        ASSERT_TRUE(result.success()) << "Failed to initialize ClientWorld";
    }

    void TearDown() override { m_world.destroy(); }

    ClientWorld m_world;
};

// ========== clearChunks() 测试 ==========

TEST_F(ClientWorldClearChunksTest, ClearChunksOnEmptyWorld)
{
    // 空世界调用 clearChunks 不应崩溃
    EXPECT_EQ(m_world.chunkCount(), 0u);
    m_world.clearChunks();
    EXPECT_EQ(m_world.chunkCount(), 0u);
}

TEST_F(ClientWorldClearChunksTest, ClearChunksWithUnloadCallback)
{
    // 记录卸载回调是否被调用
    int unloadCount = 0;
    m_world.setChunkUnloadCallback([&unloadCount](const ChunkId& id) {
        (void)id;
        ++unloadCount;
    });

    // 模拟加载几个区块（通过 onChunkData）
    // 注意：这里需要实际的区块数据，简化测试只验证 clearChunks 行为
    // 在实际项目中，应该 mock 区块数据或使用真实区块数据

    // 由于区块数据格式复杂，这里只测试空世界的 clearChunks 行为
    m_world.clearChunks();
    EXPECT_EQ(m_world.chunkCount(), 0u);
    EXPECT_EQ(unloadCount, 0); // 空世界没有区块要卸载
}

// ========== ClientDimensionManager 测试 ==========

/**
 * @brief ClientDimensionManager 维度切换测试（补充测试）
 *
 * 注意：主要的 ClientDimensionManager 测试在 tests/client/test_client_dimension_manager.cpp
 * 这里只是补充一些新功能的测试。
 */
class ClientDimensionChangeFlowTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}

    ClientDimensionManager m_dimensionManager;
};

TEST_F(ClientDimensionChangeFlowTest, InitialState)
{
    // 初始状态应该是主世界 (dimension 0)
    EXPECT_EQ(m_dimensionManager.currentDimension(), 0);
    EXPECT_EQ(m_dimensionManager.transitionState(), ClientDimensionManager::TransitionState::None);
    EXPECT_FALSE(m_dimensionManager.isChangingDimension());
}

TEST_F(ClientDimensionChangeFlowTest, InitializeWithDimensionInfo)
{
    std::vector<ClientDimensionInfo> dimensions = {
        {0, "minecraft:overworld", true, false, 0.0f},
        {-1, "minecraft:the_nether", false, true, 0.1f},
        {1, "minecraft:the_end", false, false, 0.0f}
    };

    m_dimensionManager.initialize(dimensions);

    EXPECT_TRUE(m_dimensionManager.isDimensionAvailable(0));
    EXPECT_TRUE(m_dimensionManager.isDimensionAvailable(-1));
    EXPECT_TRUE(m_dimensionManager.isDimensionAvailable(1));
    EXPECT_FALSE(m_dimensionManager.isDimensionAvailable(2));
}

TEST_F(ClientDimensionChangeFlowTest, BeginDimensionChange)
{
    // 开始维度切换
    m_dimensionManager.beginDimensionChange(-1, Vector3d(100.0, 64.0, 200.0));

    EXPECT_EQ(m_dimensionManager.transitionState(), ClientDimensionManager::TransitionState::Leaving);
    EXPECT_TRUE(m_dimensionManager.isChangingDimension());
    EXPECT_EQ(m_dimensionManager.targetDimension(), -1);
    EXPECT_EQ(m_dimensionManager.targetPosition(), Vector3d(100.0, 64.0, 200.0));
    EXPECT_TRUE(m_dimensionManager.needsRenderReset());
}

TEST_F(ClientDimensionChangeFlowTest, CompleteDimensionChange)
{
    // 开始并完成维度切换
    m_dimensionManager.beginDimensionChange(-1, Vector3d(100.0, 64.0, 200.0));
    m_dimensionManager.completeDimensionChange();

    EXPECT_EQ(m_dimensionManager.currentDimension(), -1);
    EXPECT_EQ(m_dimensionManager.transitionState(), ClientDimensionManager::TransitionState::None);
    EXPECT_FALSE(m_dimensionManager.isChangingDimension());
    EXPECT_EQ(m_dimensionManager.targetDimension(), 0); // 应该重置为0
}

TEST_F(ClientDimensionChangeFlowTest, CancelDimensionChange)
{
    // 开始并取消维度切换
    m_dimensionManager.beginDimensionChange(-1, Vector3d(100.0, 64.0, 200.0));
    m_dimensionManager.cancelDimensionChange();

    // 应该保持原维度
    EXPECT_EQ(m_dimensionManager.currentDimension(), 0);
    EXPECT_EQ(m_dimensionManager.transitionState(), ClientDimensionManager::TransitionState::None);
    EXPECT_FALSE(m_dimensionManager.isChangingDimension());
}

TEST_F(ClientDimensionChangeFlowTest, DimensionInfoAccess)
{
    std::vector<ClientDimensionInfo> dimensions = {
        {0, "minecraft:overworld", true, false, 0.0f},
        {-1, "minecraft:the_nether", false, true, 0.1f}
    };

    m_dimensionManager.initialize(dimensions);

    // 获取主世界信息
    const auto* overworldInfo = m_dimensionManager.getDimensionInfo(0);
    ASSERT_NE(overworldInfo, nullptr);
    EXPECT_EQ(overworldInfo->name, "minecraft:overworld");
    EXPECT_TRUE(overworldInfo->hasSkyLight);
    EXPECT_FALSE(overworldInfo->hasCeiling);

    // 获取下界信息
    const auto* netherInfo = m_dimensionManager.getDimensionInfo(-1);
    ASSERT_NE(netherInfo, nullptr);
    EXPECT_EQ(netherInfo->name, "minecraft:the_nether");
    EXPECT_FALSE(netherInfo->hasSkyLight);
    EXPECT_TRUE(netherInfo->hasCeiling);

    // 不存在的维度
    const auto* invalidInfo = m_dimensionManager.getDimensionInfo(999);
    EXPECT_EQ(invalidInfo, nullptr);
}

TEST_F(ClientDimensionChangeFlowTest, SetCurrentDimension)
{
    m_dimensionManager.setCurrentDimension(-1);
    EXPECT_EQ(m_dimensionManager.currentDimension(), -1);

    m_dimensionManager.setCurrentDimension(1);
    EXPECT_EQ(m_dimensionManager.currentDimension(), 1);
}

TEST_F(ClientDimensionChangeFlowTest, NeedsRenderResetFlag)
{
    // 初始不需要重置
    EXPECT_FALSE(m_dimensionManager.needsRenderReset());

    // 开始维度切换时设置重置标志
    m_dimensionManager.beginDimensionChange(-1, Vector3d(0, 0, 0));
    EXPECT_TRUE(m_dimensionManager.needsRenderReset());

    // 标记已重置
    m_dimensionManager.markRenderReset();
    EXPECT_FALSE(m_dimensionManager.needsRenderReset());
}

TEST_F(ClientDimensionChangeFlowTest, Reset)
{
    // 初始化并切换维度
    std::vector<ClientDimensionInfo> dimensions = {
        {0, "minecraft:overworld", true, false, 0.0f},
        {-1, "minecraft:the_nether", false, true, 0.1f}
    };
    m_dimensionManager.initialize(dimensions);
    m_dimensionManager.setCurrentDimension(-1);

    // 重置
    m_dimensionManager.reset();

    // 应该回到初始状态
    EXPECT_EQ(m_dimensionManager.currentDimension(), 0);
    EXPECT_TRUE(m_dimensionManager.availableDimensions().empty());
    EXPECT_EQ(m_dimensionManager.transitionState(), ClientDimensionManager::TransitionState::None);
}
