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

#include "common/network/sync/ChunkSync.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkData.hpp"

using namespace mc;
using namespace mc::network;

// ============================================================================
// ChunkView 测试
// ============================================================================

TEST(ChunkView, IsChunkInView)
{
    ChunkView view;
    view.centerX = 0;
    view.centerZ = 0;
    view.viewDistance = 5;

    // 应该在视距内
    EXPECT_TRUE(view.isChunkInView(0, 0));
    EXPECT_TRUE(view.isChunkInView(5, 0));
    EXPECT_TRUE(view.isChunkInView(-5, 0));
    EXPECT_TRUE(view.isChunkInView(0, 5));
    EXPECT_TRUE(view.isChunkInView(3, 3));

    // 应该在视距外
    EXPECT_FALSE(view.isChunkInView(6, 0));
    EXPECT_FALSE(view.isChunkInView(0, 6));
    EXPECT_FALSE(view.isChunkInView(10, 10));
}

TEST(ChunkView, GetChunksInView)
{
    ChunkView view;
    view.centerX = 0;
    view.centerZ = 0;
    view.viewDistance = 2;

    auto chunks = view.getChunksInView();

    // 视距2意味着 -2到2 共5x5 = 25个区块
    EXPECT_EQ(chunks.size(), 25u);

    // 验证中心区块存在
    bool hasCenter = false;
    for (const auto& pos : chunks) {
        if (pos.x == 0 && pos.z == 0) {
            hasCenter = true;
            break;
        }
    }
    EXPECT_TRUE(hasCenter);
}

TEST(ChunkView, CalculateChunkDiff)
{
    ChunkView view;
    view.centerX = 0;
    view.centerZ = 0;
    view.viewDistance = 1;

    // 当前只有 (0,0) 区块
    std::unordered_set<ChunkId> currentChunks;
    currentChunks.insert(ChunkId(0, 0, 0));

    std::vector<ChunkPos> toLoad;
    std::vector<ChunkPos> toUnload;

    view.calculateChunkDiff(currentChunks, toLoad, toUnload);

    // 视距1意味着 -1到1 共3x3 = 9个区块，已加载1个，还需加载8个
    EXPECT_EQ(toLoad.size(), 8u);
    EXPECT_EQ(toUnload.size(), 0u);

    // 玩家移动到 (5, 5)，原区块应卸载
    view.centerX = 5;
    view.centerZ = 5;
    currentChunks.clear();
    currentChunks.insert(ChunkId(0, 0, 0));

    toLoad.clear();
    toUnload.clear();
    view.calculateChunkDiff(currentChunks, toLoad, toUnload);

    // 应加载新区块，卸载旧区块
    EXPECT_EQ(toLoad.size(), 9u);
    EXPECT_EQ(toUnload.size(), 1u);
}

// ============================================================================
// PlayerChunkTracker 测试
// ============================================================================

TEST(PlayerChunkTracker, Construction)
{
    PlayerChunkTracker tracker(12345);

    EXPECT_EQ(tracker.playerId(), 12345u);
    EXPECT_EQ(tracker.viewDistance(), 10); // 默认视距
    EXPECT_EQ(tracker.loadedChunks().size(), 0u);
}

TEST(PlayerChunkTracker, AddRemoveChunk)
{
    PlayerChunkTracker tracker(1);

    tracker.addLoadedChunk(10, 20);
    EXPECT_TRUE(tracker.hasChunk(10, 20));
    EXPECT_FALSE(tracker.hasChunk(10, 21));
    EXPECT_EQ(tracker.loadedChunks().size(), 1u);

    tracker.addLoadedChunk(10, 21);
    EXPECT_EQ(tracker.loadedChunks().size(), 2u);

    tracker.removeLoadedChunk(10, 20);
    EXPECT_FALSE(tracker.hasChunk(10, 20));
    EXPECT_EQ(tracker.loadedChunks().size(), 1u);
}

TEST(PlayerChunkTracker, UpdateCenter)
{
    PlayerChunkTracker tracker(1);

    tracker.updateCenter(100, 200);

    EXPECT_EQ(tracker.view().centerX, 100);
    EXPECT_EQ(tracker.view().centerZ, 200);
}

TEST(PlayerChunkTracker, SetViewDistance)
{
    PlayerChunkTracker tracker(1);

    tracker.setViewDistance(5);
    EXPECT_EQ(tracker.viewDistance(), 5);

    // 边界测试
    tracker.setViewDistance(1);
    EXPECT_EQ(tracker.viewDistance(), 2); // 最小值2

    tracker.setViewDistance(100);
    EXPECT_EQ(tracker.viewDistance(), 32); // 最大值32
}

TEST(PlayerChunkTracker, CalculateChunkUpdates)
{
    PlayerChunkTracker tracker(1);
    tracker.setViewDistance(2); // 视距2: 5x5 = 25区块
    tracker.updateCenter(0, 0);

    // 初始状态，没有已加载区块
    std::vector<ChunkPos> toLoad;
    std::vector<ChunkPos> toUnload;

    tracker.calculateChunkUpdates(toLoad, toUnload);

    // 应加载25个区块 (5x5)
    EXPECT_EQ(toLoad.size(), 25u);
    EXPECT_EQ(toUnload.size(), 0u);

    // 标记中心区块为已加载
    tracker.addLoadedChunk(0, 0);

    toLoad.clear();
    toUnload.clear();
    tracker.calculateChunkUpdates(toLoad, toUnload);

    // 还需加载24个
    EXPECT_EQ(toLoad.size(), 24u);

    // 移动玩家到远处
    tracker.updateCenter(100, 100);

    toLoad.clear();
    toUnload.clear();
    tracker.calculateChunkUpdates(toLoad, toUnload);

    // 原区块应该卸载，新区块应该加载
    EXPECT_EQ(toUnload.size(), 1u);
    EXPECT_EQ(toLoad.size(), 25u);
}

TEST(PlayerChunkTracker, Clear)
{
    PlayerChunkTracker tracker(1);

    tracker.addLoadedChunk(0, 0);
    tracker.addLoadedChunk(1, 1);
    tracker.addLoadedChunk(2, 2);

    EXPECT_EQ(tracker.loadedChunks().size(), 3u);

    tracker.clear();

    EXPECT_EQ(tracker.loadedChunks().size(), 0u);
}

// ============================================================================
// ChunkSyncManager 测试
// ============================================================================

TEST(ChunkSyncManager, GetTracker)
{
    ChunkSyncManager manager;

    auto tracker1 = manager.getTracker(1);
    EXPECT_NE(tracker1, nullptr);
    EXPECT_EQ(tracker1->playerId(), 1u);

    // 再次获取应该返回相同的tracker
    auto tracker1Again = manager.getTracker(1);
    EXPECT_EQ(tracker1, tracker1Again);
}

TEST(ChunkSyncManager, RemoveTracker)
{
    ChunkSyncManager manager;

    auto tracker = manager.getTracker(1);
    EXPECT_NE(tracker, nullptr);

    manager.removeTracker(1);

    // 移除后再获取应该是新的tracker
    auto newTracker = manager.getTracker(1);
    EXPECT_NE(newTracker, tracker);
}

TEST(ChunkSyncManager, UpdatePlayerPosition)
{
    ChunkSyncManager manager;

    manager.updatePlayerPosition(1, 100.0, 200.0);

    auto tracker = manager.getTracker(1);
    EXPECT_EQ(tracker->view().centerX, 6);  // 100 / 16 = 6
    EXPECT_EQ(tracker->view().centerZ, 12); // 200 / 16 = 12
}

TEST(ChunkSyncManager, MarkChunkSent)
{
    ChunkSyncManager manager;

    manager.markChunkSent(1, 10, 20);

    auto tracker = manager.getTracker(1);
    EXPECT_TRUE(tracker->hasChunk(10, 20));

    // 订阅者检查
    auto subscribers = manager.getChunkSubscribers(10, 20);
    EXPECT_EQ(subscribers.size(), 1u);
    EXPECT_EQ(subscribers[0], 1u);
}

TEST(ChunkSyncManager, MarkChunkUnloaded)
{
    ChunkSyncManager manager;

    manager.markChunkSent(1, 10, 20);
    EXPECT_TRUE(manager.getTracker(1)->hasChunk(10, 20));

    manager.markChunkUnloaded(1, 10, 20);
    EXPECT_FALSE(manager.getTracker(1)->hasChunk(10, 20));

    // 订阅者应该为空
    auto subscribers = manager.getChunkSubscribers(10, 20);
    EXPECT_EQ(subscribers.size(), 0u);
}

TEST(ChunkSyncManager, MultiplePlayers)
{
    ChunkSyncManager manager;

    // 两个玩家加载同一区块
    manager.markChunkSent(1, 0, 0);
    manager.markChunkSent(2, 0, 0);

    auto subscribers = manager.getChunkSubscribers(0, 0);
    EXPECT_EQ(subscribers.size(), 2u);

    // 一个玩家卸载
    manager.markChunkUnloaded(1, 0, 0);
    subscribers = manager.getChunkSubscribers(0, 0);
    EXPECT_EQ(subscribers.size(), 1u);
    EXPECT_EQ(subscribers[0], 2u);
}

TEST(ChunkSyncManager, CalculateUpdates)
{
    ChunkSyncManager manager;
    manager.setDefaultViewDistance(2); // 视距2: 5x5 = 25区块

    // 初始化玩家位置
    manager.updatePlayerPosition(1, 0.0, 0.0);

    std::vector<ChunkPos> toLoad;
    std::vector<ChunkPos> toUnload;

    manager.calculateUpdates(1, toLoad, toUnload);

    // 应该有25个区块需要加载
    EXPECT_EQ(toLoad.size(), 25u);
    EXPECT_EQ(toUnload.size(), 0u);
}

TEST(ChunkSyncManager, BlockToChunk)
{
    EXPECT_EQ(ChunkSyncManager::blockToChunk(0.0), 0);
    EXPECT_EQ(ChunkSyncManager::blockToChunk(15.9), 0);
    EXPECT_EQ(ChunkSyncManager::blockToChunk(16.0), 1);
    EXPECT_EQ(ChunkSyncManager::blockToChunk(-1.0), -1);
    EXPECT_EQ(ChunkSyncManager::blockToChunk(-16.0), -1);
    EXPECT_EQ(ChunkSyncManager::blockToChunk(-17.0), -2);
}

// ============================================================================
// ChunkSerializer 测试
// ============================================================================

class ChunkSerializerTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(ChunkSerializerTest, SerializeEmptyChunk)
{
    ChunkData chunk(10, -5);

    auto result = ChunkSerializer::serializeChunk(chunk);
    EXPECT_TRUE(result.success());

    const auto& data = result.value();
    EXPECT_FALSE(data.empty());
}

TEST_F(ChunkSerializerTest, SerializeChunkWithBlocks)
{
    ChunkData chunk(0, 0);

    // 填充一些方块
    auto section = chunk.createSection(0);
    ASSERT_NE(section, nullptr);

    u32 stoneStateId = VanillaBlocks::STONE->defaultState().stateId();
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            section->setBlockStateId(x, 0, z, stoneStateId);
        }
    }

    auto result = ChunkSerializer::serializeChunk(chunk);
    EXPECT_TRUE(result.success());

    const auto& data = result.value();
    EXPECT_FALSE(data.empty());

    // 验证大小
    size_t expectedSize = ChunkSerializer::calculateChunkSize(chunk);
    EXPECT_EQ(data.size(), expectedSize);
}

TEST_F(ChunkSerializerTest, DeserializeChunk)
{
    // 创建并序列化一个区块
    ChunkData original(100, -200);

    auto section = original.createSection(4);
    ASSERT_NE(section, nullptr);

    // 设置一些方块
    u32 stoneStateId = VanillaBlocks::STONE->defaultState().stateId();
    u32 dirtStateId = VanillaBlocks::DIRT->defaultState().stateId();
    section->setBlockStateId(5, 5, 5, stoneStateId);
    section->setBlockStateId(10, 10, 10, dirtStateId);

    auto serializeResult = ChunkSerializer::serializeChunk(original);
    ASSERT_TRUE(serializeResult.success());

    // 反序列化
    auto deserializeResult = ChunkSerializer::deserializeChunk(100, -200, serializeResult.value());
    ASSERT_TRUE(deserializeResult.success());

    auto restored = deserializeResult.value();
    EXPECT_EQ(restored->x(), 100);
    EXPECT_EQ(restored->z(), -200);
    EXPECT_TRUE(restored->isFullyGenerated());
}

TEST_F(ChunkSerializerTest, DeserializeChunkPreservesBiomeData)
{
    ChunkData original(3, 7);

    BiomeContainer biomes;
    biomes.setBiome(0, 0, 0, Biomes::Forest);
    biomes.setBiome(3, 3, 3, Biomes::Badlands);
    original.setBiomes(std::move(biomes));

    auto serializeResult = ChunkSerializer::serializeChunk(original);
    ASSERT_TRUE(serializeResult.success());

    auto deserializeResult = ChunkSerializer::deserializeChunk(3, 7, serializeResult.value());
    ASSERT_TRUE(deserializeResult.success());

    const auto& restored = deserializeResult.value();
    EXPECT_EQ(restored->getBiomeAtBlock(0, 0, 0), Biomes::Forest);
    EXPECT_EQ(restored->getBiomeAtBlock(15, 63, 15), Biomes::Badlands);
}

TEST_F(ChunkSerializerTest, SectionMask)
{
    ChunkData chunk(0, 0);

    // 空区块，位掩码应为0
    u16 mask = ChunkSerializer::calculateSectionMask(chunk);
    EXPECT_EQ(mask, 0);

    // 创建一个非空区块段
    auto section = chunk.createSection(5);
    u32 stoneStateId = VanillaBlocks::STONE->defaultState().stateId();
    section->setBlockStateId(0, 0, 0, stoneStateId);

    mask = ChunkSerializer::calculateSectionMask(chunk);
    EXPECT_EQ(mask, (1 << 5)); // 第5位应该被设置
}

TEST_F(ChunkSerializerTest, SectionSize)
{
    ChunkSection section;

    size_t size = ChunkSerializer::calculateSectionSize(section);
    // 新格式: 方块数据 (4096 * 4) + 天空光照 (2048) + 方块光照 (2048) + 计数 (2)
    EXPECT_EQ(size, 2 + ChunkSection::VOLUME * 4 + 2048 + 2048);
}

// ============================================================================
// ChunkView 扩展测试
// ============================================================================

class ChunkViewExtendedTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(ChunkViewExtendedTest, NegativeCoordinates)
{
    ChunkView view;
    view.centerX = -10;
    view.centerZ = -20;
    view.viewDistance = 5;

    // 中心区块应该在视距内
    EXPECT_TRUE(view.isChunkInView(-10, -20));

    // 边缘区块
    EXPECT_TRUE(view.isChunkInView(-15, -20));
    EXPECT_TRUE(view.isChunkInView(-10, -25));
    EXPECT_TRUE(view.isChunkInView(-5, -20));
    EXPECT_TRUE(view.isChunkInView(-10, -15));

    // 超出视距
    EXPECT_FALSE(view.isChunkInView(-16, -20));
    EXPECT_FALSE(view.isChunkInView(-10, -26));
}

TEST_F(ChunkViewExtendedTest, LargeViewDistance)
{
    ChunkView view;
    view.centerX = 0;
    view.centerZ = 0;
    view.viewDistance = 32; // 最大视距

    // 边缘区块
    EXPECT_TRUE(view.isChunkInView(32, 0));
    EXPECT_TRUE(view.isChunkInView(0, 32));
    EXPECT_TRUE(view.isChunkInView(-32, 0));
    EXPECT_TRUE(view.isChunkInView(0, -32));

    // 超出视距
    EXPECT_FALSE(view.isChunkInView(33, 0));
    EXPECT_FALSE(view.isChunkInView(0, 33));
}

TEST_F(ChunkViewExtendedTest, ZeroViewDistance)
{
    ChunkView view;
    view.centerX = 0;
    view.centerZ = 0;
    view.viewDistance = 0;

    // 只有中心区块应该在视距内
    EXPECT_TRUE(view.isChunkInView(0, 0));
    EXPECT_FALSE(view.isChunkInView(1, 0));
    EXPECT_FALSE(view.isChunkInView(0, 1));
    EXPECT_FALSE(view.isChunkInView(-1, 0));
}

TEST_F(ChunkViewExtendedTest, GetChunksInViewCount)
{
    ChunkView view;
    view.centerX = 0;
    view.centerZ = 0;

    // 视距 n = (2n+1)^2 个区块
    view.viewDistance = 0;
    EXPECT_EQ(view.getChunksInView().size(), 1u); // 1x1

    view.viewDistance = 1;
    EXPECT_EQ(view.getChunksInView().size(), 9u); // 3x3

    view.viewDistance = 2;
    EXPECT_EQ(view.getChunksInView().size(), 25u); // 5x5

    view.viewDistance = 5;
    EXPECT_EQ(view.getChunksInView().size(), 121u); // 11x11

    view.viewDistance = 10;
    EXPECT_EQ(view.getChunksInView().size(), 441u); // 21x21
}

TEST_F(ChunkViewExtendedTest, GetChunksInViewOffsetCenter)
{
    ChunkView view;
    view.centerX = 100;
    view.centerZ = -50;
    view.viewDistance = 2;

    auto chunks = view.getChunksInView();
    EXPECT_EQ(chunks.size(), 25u);

    // 验证中心区块存在
    bool hasCenter = false;
    for (const auto& pos : chunks) {
        if (pos.x == 100 && pos.z == -50) {
            hasCenter = true;
            break;
        }
    }
    EXPECT_TRUE(hasCenter);

    // 验证边界区块
    bool hasCorner = false;
    for (const auto& pos : chunks) {
        if (pos.x == 102 && pos.z == -48) {
            hasCorner = true;
            break;
        }
    }
    EXPECT_TRUE(hasCorner);
}

TEST_F(ChunkViewExtendedTest, CalculateChunkDiffPartialOverlap)
{
    ChunkView view;
    view.centerX = 0;
    view.centerZ = 0;
    view.viewDistance = 2;

    // 当前有部分区块已加载
    std::unordered_set<ChunkId> currentChunks;
    // 加载左半边
    for (int z = -2; z <= 2; ++z) {
        for (int x = -2; x <= 0; ++x) {
            currentChunks.insert(ChunkId(x, z, 0));
        }
    }
    // 15 个区块已加载

    std::vector<ChunkPos> toLoad;
    std::vector<ChunkPos> toUnload;

    view.calculateChunkDiff(currentChunks, toLoad, toUnload);

    // 应加载右半边（10 个区块）
    EXPECT_EQ(toLoad.size(), 10u);
    EXPECT_EQ(toUnload.size(), 0u);
}

TEST_F(ChunkViewExtendedTest, CalculateChunkDiffMoveAway)
{
    ChunkView view;
    view.centerX = 100;
    view.centerZ = 100;
    view.viewDistance = 1;

    // 当前有远处的区块已加载
    std::unordered_set<ChunkId> currentChunks;
    currentChunks.insert(ChunkId(0, 0, 0));
    currentChunks.insert(ChunkId(1, 0, 0));
    currentChunks.insert(ChunkId(0, 1, 0));

    std::vector<ChunkPos> toLoad;
    std::vector<ChunkPos> toUnload;

    view.calculateChunkDiff(currentChunks, toLoad, toUnload);

    // 应卸载所有旧区块，加载所有新区块
    EXPECT_EQ(toLoad.size(), 9u);   // 3x3
    EXPECT_EQ(toUnload.size(), 3u); // 原有的 3 个
}

TEST_F(ChunkViewExtendedTest, CalculateChunkDiffSamePosition)
{
    ChunkView view;
    view.centerX = 0;
    view.centerZ = 0;
    view.viewDistance = 2;

    // 所有区块都已加载
    std::unordered_set<ChunkId> currentChunks;
    for (int z = -2; z <= 2; ++z) {
        for (int x = -2; x <= 2; ++x) {
            currentChunks.insert(ChunkId(x, z, 0));
        }
    }

    std::vector<ChunkPos> toLoad;
    std::vector<ChunkPos> toUnload;

    view.calculateChunkDiff(currentChunks, toLoad, toUnload);

    // 不需要任何更新
    EXPECT_EQ(toLoad.size(), 0u);
    EXPECT_EQ(toUnload.size(), 0u);
}

TEST_F(ChunkViewExtendedTest, ViewDistanceChangeEffect)
{
    ChunkView view;
    view.centerX = 0;
    view.centerZ = 0;
    view.viewDistance = 5;

    // 当前加载视距 5 的区块
    std::unordered_set<ChunkId> currentChunks;
    for (int z = -5; z <= 5; ++z) {
        for (int x = -5; x <= 5; ++x) {
            currentChunks.insert(ChunkId(x, z, 0));
        }
    }

    // 视距减小
    view.viewDistance = 3;

    std::vector<ChunkPos> toLoad;
    std::vector<ChunkPos> toUnload;

    view.calculateChunkDiff(currentChunks, toLoad, toUnload);

    // 不需要加载新区块
    EXPECT_EQ(toLoad.size(), 0u);
    // 应该卸载外围区块（视距 5 减到 3，卸载 2 层）
    // 卸载数量 = 11^2 - 7^2 = 121 - 49 = 72
    EXPECT_EQ(toUnload.size(), 72u);
}

// ============================================================================
// PlayerChunkTracker (network) 扩展测试
// ============================================================================

class PlayerChunkTrackerNetworkTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(PlayerChunkTrackerNetworkTest, MultipleChunkOperations)
{
    PlayerChunkTracker tracker(1);

    // 添加多个区块
    for (int x = 0; x < 10; ++x) {
        for (int z = 0; z < 10; ++z) {
            tracker.addLoadedChunk(x, z);
        }
    }

    EXPECT_EQ(tracker.loadedChunks().size(), 100u);

    // 移除部分区块
    for (int x = 0; x < 5; ++x) {
        for (int z = 0; z < 5; ++z) {
            tracker.removeLoadedChunk(x, z);
        }
    }

    EXPECT_EQ(tracker.loadedChunks().size(), 75u);

    // 验证剩余区块
    EXPECT_TRUE(tracker.hasChunk(5, 5));
    EXPECT_TRUE(tracker.hasChunk(9, 9));
    EXPECT_FALSE(tracker.hasChunk(0, 0));
    EXPECT_FALSE(tracker.hasChunk(4, 4));
}

TEST_F(PlayerChunkTrackerNetworkTest, DuplicateOperations)
{
    PlayerChunkTracker tracker(1);

    // 重复添加同一区块
    tracker.addLoadedChunk(0, 0);
    tracker.addLoadedChunk(0, 0);
    tracker.addLoadedChunk(0, 0);

    EXPECT_EQ(tracker.loadedChunks().size(), 1u);

    // 重复移除
    tracker.removeLoadedChunk(0, 0);
    EXPECT_EQ(tracker.loadedChunks().size(), 0u);

    // 移除不存在的区块（不应该崩溃）
    tracker.removeLoadedChunk(0, 0);
    EXPECT_EQ(tracker.loadedChunks().size(), 0u);
}

TEST_F(PlayerChunkTrackerNetworkTest, NegativeChunkCoordinates)
{
    PlayerChunkTracker tracker(1);

    tracker.addLoadedChunk(-100, -200);
    tracker.addLoadedChunk(-1, -1);
    tracker.addLoadedChunk(-1, 0);
    tracker.addLoadedChunk(0, -1);

    EXPECT_EQ(tracker.loadedChunks().size(), 4u);
    EXPECT_TRUE(tracker.hasChunk(-100, -200));
    EXPECT_TRUE(tracker.hasChunk(-1, -1));
    EXPECT_TRUE(tracker.hasChunk(-1, 0));
    EXPECT_TRUE(tracker.hasChunk(0, -1));

    tracker.removeLoadedChunk(-100, -200);
    EXPECT_FALSE(tracker.hasChunk(-100, -200));
}

TEST_F(PlayerChunkTrackerNetworkTest, ViewDistanceBoundaryValues)
{
    PlayerChunkTracker tracker(1);

    // 最小视距
    tracker.setViewDistance(1);
    EXPECT_EQ(tracker.viewDistance(), 2); // 最小值为 2

    // 最大视距
    tracker.setViewDistance(100);
    EXPECT_EQ(tracker.viewDistance(), 32); // 最大值为 32

    // 正常值
    tracker.setViewDistance(10);
    EXPECT_EQ(tracker.viewDistance(), 10);
}

TEST_F(PlayerChunkTrackerNetworkTest, CalculateChunkUpdatesWithExistingChunks)
{
    PlayerChunkTracker tracker(1);
    tracker.setViewDistance(3);
    tracker.updateCenter(0, 0);

    // 预加载部分区块
    for (int x = -1; x <= 1; ++x) {
        for (int z = -1; z <= 1; ++z) {
            tracker.addLoadedChunk(x, z);
        }
    }
    // 9 个区块已加载

    std::vector<ChunkPos> toLoad;
    std::vector<ChunkPos> toUnload;

    tracker.calculateChunkUpdates(toLoad, toUnload);

    // 视距 3 = 7x7 = 49 区块，已加载 9 个，还需加载 40 个
    EXPECT_EQ(toLoad.size(), 40u);
    EXPECT_EQ(toUnload.size(), 0u);
}

TEST_F(PlayerChunkTrackerNetworkTest, CalculateChunkUpdatesPlayerMove)
{
    PlayerChunkTracker tracker(1);
    tracker.setViewDistance(2);
    tracker.updateCenter(0, 0);

    // 初始加载所有区块
    for (int x = -2; x <= 2; ++x) {
        for (int z = -2; z <= 2; ++z) {
            tracker.addLoadedChunk(x, z);
        }
    }

    // 玩家移动
    tracker.updateCenter(10, 10);

    std::vector<ChunkPos> toLoad;
    std::vector<ChunkPos> toUnload;

    tracker.calculateChunkUpdates(toLoad, toUnload);

    // 应加载新区块，卸载旧区块
    EXPECT_EQ(toLoad.size(), 25u);   // 5x5 新区块
    EXPECT_EQ(toUnload.size(), 25u); // 5x5 旧区块
}

TEST_F(PlayerChunkTrackerNetworkTest, ClearTracker)
{
    PlayerChunkTracker tracker(1);

    // 添加大量区块
    for (int x = 0; x < 100; ++x) {
        for (int z = 0; z < 100; ++z) {
            tracker.addLoadedChunk(x, z);
        }
    }

    EXPECT_EQ(tracker.loadedChunks().size(), 10000u);

    tracker.clear();

    EXPECT_EQ(tracker.loadedChunks().size(), 0u);
    EXPECT_FALSE(tracker.hasChunk(0, 0));
    EXPECT_FALSE(tracker.hasChunk(50, 50));
}

TEST_F(PlayerChunkTrackerNetworkTest, LargeCoordinateValues)
{
    PlayerChunkTracker tracker(1);

    // 测试大坐标值
    ChunkCoord largeX = 1000000;
    ChunkCoord largeZ = -1000000;

    tracker.addLoadedChunk(largeX, largeZ);
    EXPECT_TRUE(tracker.hasChunk(largeX, largeZ));

    tracker.updateCenter(largeX, largeZ);
    EXPECT_EQ(tracker.view().centerX, largeX);
    EXPECT_EQ(tracker.view().centerZ, largeZ);
}

// ============================================================================
// ChunkSyncManager 扩展测试
// ============================================================================

class ChunkSyncManagerExtendedTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(ChunkSyncManagerExtendedTest, MultiplePlayersSameChunk)
{
    ChunkSyncManager manager;

    // 多个玩家加载同一区块
    manager.markChunkSent(1, 0, 0);
    manager.markChunkSent(2, 0, 0);
    manager.markChunkSent(3, 0, 0);
    manager.markChunkSent(4, 0, 0);

    auto subscribers = manager.getChunkSubscribers(0, 0);
    EXPECT_EQ(subscribers.size(), 4u);

    // 验证所有玩家都在订阅列表中
    EXPECT_NE(std::find(subscribers.begin(), subscribers.end(), 1), subscribers.end());
    EXPECT_NE(std::find(subscribers.begin(), subscribers.end(), 2), subscribers.end());
    EXPECT_NE(std::find(subscribers.begin(), subscribers.end(), 3), subscribers.end());
    EXPECT_NE(std::find(subscribers.begin(), subscribers.end(), 4), subscribers.end());
}

TEST_F(ChunkSyncManagerExtendedTest, PlayerDisconnectionCleanup)
{
    ChunkSyncManager manager;

    // 玩家加载多个区块
    manager.markChunkSent(1, 0, 0);
    manager.markChunkSent(1, 1, 0);
    manager.markChunkSent(1, 0, 1);
    manager.markChunkSent(1, 1, 1);

    // 验证区块有订阅者
    EXPECT_EQ(manager.getChunkSubscribers(0, 0).size(), 1u);
    EXPECT_EQ(manager.getChunkSubscribers(1, 0).size(), 1u);

    // 移除玩家
    manager.removeTracker(1);

    // 验证所有区块的订阅者都已清除
    EXPECT_EQ(manager.getChunkSubscribers(0, 0).size(), 0u);
    EXPECT_EQ(manager.getChunkSubscribers(1, 0).size(), 0u);
    EXPECT_EQ(manager.getChunkSubscribers(0, 1).size(), 0u);
    EXPECT_EQ(manager.getChunkSubscribers(1, 1).size(), 0u);
}

TEST_F(ChunkSyncManagerExtendedTest, CalculateUpdatesForMultiplePlayers)
{
    ChunkSyncManager manager;
    manager.setDefaultViewDistance(2);

    // 玩家 1 在原点
    manager.updatePlayerPosition(1, 0.0, 0.0);

    // 玩家 2 在远处
    manager.updatePlayerPosition(2, 100.0, 100.0);

    std::vector<ChunkPos> toLoad1, toUnload1;
    manager.calculateUpdates(1, toLoad1, toUnload1);
    EXPECT_EQ(toLoad1.size(), 25u); // 5x5

    std::vector<ChunkPos> toLoad2, toUnload2;
    manager.calculateUpdates(2, toLoad2, toUnload2);
    EXPECT_EQ(toLoad2.size(), 25u); // 5x5

    // 验证两个玩家的加载区域不重叠
    auto isInRange = [](const std::vector<ChunkPos>& chunks, ChunkCoord x, ChunkCoord z) {
        return std::find_if(chunks.begin(), chunks.end(), [&](const ChunkPos& p) { return p.x == x && p.z == z; }) !=
            chunks.end();
    };

    EXPECT_TRUE(isInRange(toLoad1, 0, 0));
    EXPECT_FALSE(isInRange(toLoad1, 6, 6)); // 玩家 1 的范围外

    EXPECT_TRUE(isInRange(toLoad2, 6, 6)); // 玩家 2 的范围（100/16 = 6）
    EXPECT_FALSE(isInRange(toLoad2, 0, 0));
}

TEST_F(ChunkSyncManagerExtendedTest, ViewDistanceChange)
{
    ChunkSyncManager manager;
    manager.setDefaultViewDistance(5);

    manager.updatePlayerPosition(1, 0.0, 0.0);

    auto tracker = manager.getTracker(1);
    EXPECT_EQ(tracker->viewDistance(), 5);

    // 改变默认视距不影响已有玩家
    manager.setDefaultViewDistance(10);
    EXPECT_EQ(tracker->viewDistance(), 5); // 保持原来的视距

    // 新玩家使用新视距
    manager.updatePlayerPosition(2, 0.0, 0.0);
    auto tracker2 = manager.getTracker(2);
    EXPECT_EQ(tracker2->viewDistance(), 10);
}

TEST_F(ChunkSyncManagerExtendedTest, BlockToChunkEdgeCases)
{
    // 边界情况测试
    EXPECT_EQ(ChunkSyncManager::blockToChunk(0.0), 0);
    EXPECT_EQ(ChunkSyncManager::blockToChunk(15.9), 0);
    EXPECT_EQ(ChunkSyncManager::blockToChunk(16.0), 1);
    EXPECT_EQ(ChunkSyncManager::blockToChunk(-0.1), -1);
    EXPECT_EQ(ChunkSyncManager::blockToChunk(-1.0), -1);
    EXPECT_EQ(ChunkSyncManager::blockToChunk(-16.0), -1);
    EXPECT_EQ(ChunkSyncManager::blockToChunk(-16.1), -2);
    EXPECT_EQ(ChunkSyncManager::blockToChunk(-32.0), -2);
    EXPECT_EQ(ChunkSyncManager::blockToChunk(1000000.0), 62500);
    EXPECT_EQ(ChunkSyncManager::blockToChunk(-1000000.0), -62500);
}

TEST_F(ChunkSyncManagerExtendedTest, UpdatePlayerPositionTriggersCenterChange)
{
    ChunkSyncManager manager;
    manager.setDefaultViewDistance(5);

    // 初始位置
    manager.updatePlayerPosition(1, 100.0, 200.0);
    auto tracker = manager.getTracker(1);
    EXPECT_EQ(tracker->view().centerX, 6);  // 100 / 16 = 6.25 → floor = 6
    EXPECT_EQ(tracker->view().centerZ, 12); // 200 / 16 = 12.5 → floor = 12

    // 移动到同一区块内的不同位置（不触发更新）
    // 105/16 = 6.5625 → floor = 6（同一区块）
    // 210/16 = 13.125 → floor = 13（不同区块！）
    manager.updatePlayerPosition(1, 105.0, 210.0);
    EXPECT_EQ(tracker->view().centerX, 6);
    EXPECT_EQ(tracker->view().centerZ, 13); // 210 / 16 = 13.125 → floor = 13

    // 移动到新区块
    manager.updatePlayerPosition(1, 120.0, 230.0);
    EXPECT_EQ(tracker->view().centerX, 7);  // 120 / 16 = 7.5 → floor = 7
    EXPECT_EQ(tracker->view().centerZ, 14); // 230 / 16 = 14.375 → floor = 14
}

TEST_F(ChunkSyncManagerExtendedTest, ChunkSentAndUnloadSequence)
{
    ChunkSyncManager manager;

    // 玩家连接
    manager.updatePlayerPosition(1, 0.0, 0.0);

    // 加载区块
    manager.markChunkSent(1, 0, 0);
    manager.markChunkSent(1, 1, 0);
    manager.markChunkSent(1, 0, 1);

    EXPECT_TRUE(manager.getTracker(1)->hasChunk(0, 0));
    EXPECT_TRUE(manager.getTracker(1)->hasChunk(1, 0));
    EXPECT_TRUE(manager.getTracker(1)->hasChunk(0, 1));

    // 卸载部分区块
    manager.markChunkUnloaded(1, 1, 0);

    EXPECT_TRUE(manager.getTracker(1)->hasChunk(0, 0));
    EXPECT_FALSE(manager.getTracker(1)->hasChunk(1, 0));
    EXPECT_TRUE(manager.getTracker(1)->hasChunk(0, 1));

    // 验证订阅者更新
    EXPECT_EQ(manager.getChunkSubscribers(0, 0).size(), 1u);
    EXPECT_EQ(manager.getChunkSubscribers(1, 0).size(), 0u);
    EXPECT_EQ(manager.getChunkSubscribers(0, 1).size(), 1u);
}

TEST_F(ChunkSyncManagerExtendedTest, ReconnectPlayer)
{
    ChunkSyncManager manager;

    // 玩家连接并加载区块
    manager.updatePlayerPosition(1, 0.0, 0.0);
    manager.markChunkSent(1, 0, 0);
    manager.markChunkSent(1, 1, 0);

    // 玩家断开
    manager.removeTracker(1);

    // 玩家重新连接
    manager.updatePlayerPosition(1, 0.0, 0.0);

    // 新的 tracker 应该没有已加载区块
    auto tracker = manager.getTracker(1);
    EXPECT_EQ(tracker->loadedChunks().size(), 0u);
}

TEST_F(ChunkSyncManagerExtendedTest, GetNonExistentTracker)
{
    ChunkSyncManager manager;

    // 获取不存在的玩家 tracker 会自动创建
    auto tracker = manager.getTracker(999);
    EXPECT_NE(tracker, nullptr);
    EXPECT_EQ(tracker->playerId(), 999u);
}

TEST_F(ChunkSyncManagerExtendedTest, NonExistentChunkSubscribers)
{
    ChunkSyncManager manager;

    // 不存在的区块应该返回空列表
    auto subscribers = manager.getChunkSubscribers(12345, 67890);
    EXPECT_EQ(subscribers.size(), 0u);
}

// ============================================================================
// ChunkSerializer 扩展测试
// ============================================================================

class ChunkSerializerExtendedTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(ChunkSerializerExtendedTest, SerializeDeserializeConsistency)
{
    // 创建一个复杂的区块
    ChunkData original(42, -100);

    // 填充多个区块段
    for (int sectionY = 0; sectionY < 5; ++sectionY) {
        auto section = original.createSection(sectionY);
        ASSERT_NE(section, nullptr);

        u32 stoneStateId = VanillaBlocks::STONE->defaultState().stateId();
        u32 dirtStateId = VanillaBlocks::DIRT->defaultState().stateId();
        u32 grassStateId = VanillaBlocks::GRASS_BLOCK->defaultState().stateId();

        for (int x = 0; x < 16; ++x) {
            for (int z = 0; z < 16; ++z) {
                // 底层石头
                section->setBlockStateId(x, 0, z, stoneStateId);
                // 中层泥土
                section->setBlockStateId(x, 1, z, dirtStateId);
                // 顶层草地
                section->setBlockStateId(x, 2, z, grassStateId);
            }
        }
    }

    // 设置生物群系
    BiomeContainer biomes;
    biomes.setBiome(0, 0, 0, Biomes::Forest);
    biomes.setBiome(7, 7, 7, Biomes::Desert);
    biomes.setBiome(15, 15, 15, Biomes::Ocean);
    original.setBiomes(std::move(biomes));

    // 序列化
    auto serializeResult = ChunkSerializer::serializeChunk(original);
    ASSERT_TRUE(serializeResult.success());

    // 反序列化
    auto deserializeResult = ChunkSerializer::deserializeChunk(42, -100, serializeResult.value());
    ASSERT_TRUE(deserializeResult.success());

    auto restored = deserializeResult.value();
    EXPECT_EQ(restored->x(), 42);
    EXPECT_EQ(restored->z(), -100);

    // 验证区块段
    for (int sectionY = 0; sectionY < 5; ++sectionY) {
        EXPECT_TRUE(restored->hasSection(sectionY));
    }
}

TEST_F(ChunkSerializerExtendedTest, SerializeChunkWithAir)
{
    ChunkData chunk(0, 0);
    auto section = chunk.createSection(0);
    ASSERT_NE(section, nullptr);

    // 填充空气
    u32 airStateId = VanillaBlocks::AIR->defaultState().stateId();
    for (int x = 0; x < 16; ++x) {
        for (int y = 0; y < 16; ++y) {
            for (int z = 0; z < 16; ++z) {
                section->setBlockStateId(x, y, z, airStateId);
            }
        }
    }

    auto result = ChunkSerializer::serializeChunk(chunk);
    EXPECT_TRUE(result.success());
}

TEST_F(ChunkSerializerExtendedTest, EmptySectionMask)
{
    ChunkData chunk(0, 0);

    // 不创建任何区块段
    u16 mask = ChunkSerializer::calculateSectionMask(chunk);
    EXPECT_EQ(mask, 0);
}

TEST_F(ChunkSerializerExtendedTest, MultipleSectionsMask)
{
    ChunkData chunk(0, 0);

    // 创建多个区块段
    chunk.createSection(0);
    chunk.createSection(5);
    chunk.createSection(10);

    // 设置一些方块使其非空
    u32 stoneStateId = VanillaBlocks::STONE->defaultState().stateId();
    chunk.getSection(0)->setBlockStateId(0, 0, 0, stoneStateId);
    chunk.getSection(5)->setBlockStateId(0, 0, 0, stoneStateId);
    chunk.getSection(10)->setBlockStateId(0, 0, 0, stoneStateId);

    u16 mask = ChunkSerializer::calculateSectionMask(chunk);
    EXPECT_EQ(mask, (1 << 0) | (1 << 5) | (1 << 10));
}

TEST_F(ChunkSerializerExtendedTest, DeserializeInvalidData)
{
    // 太小的数据
    std::vector<u8> smallData = {0x01, 0x02, 0x03};
    auto result1 = ChunkSerializer::deserializeChunk(0, 0, smallData);
    EXPECT_FALSE(result1.success());

    // 坐标不匹配
    ChunkData chunk(10, 20);
    auto serializeResult = ChunkSerializer::serializeChunk(chunk);
    ASSERT_TRUE(serializeResult.success());
    auto result2 = ChunkSerializer::deserializeChunk(30, 40, serializeResult.value());
    EXPECT_FALSE(result2.success());
}

TEST_F(ChunkSerializerExtendedTest, ChunkSizeCalculation)
{
    ChunkData chunk(0, 0);

    // 空区块
    size_t emptySize = ChunkSerializer::calculateChunkSize(chunk);

    // 添加一个区块段
    auto section = chunk.createSection(0);
    u32 stoneStateId = VanillaBlocks::STONE->defaultState().stateId();
    section->setBlockStateId(0, 0, 0, stoneStateId);

    size_t oneSectionSize = ChunkSerializer::calculateChunkSize(chunk);
    EXPECT_GT(oneSectionSize, emptySize);

    // 验证实际序列化大小
    auto result = ChunkSerializer::serializeChunk(chunk);
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().size(), oneSectionSize);
}

// ============================================================================
// 光照数据序列化测试
// ============================================================================

class ChunkSerializerLightTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(ChunkSerializerLightTest, SerializeDeserializeLightData)
{
    // 创建一个区块并设置光照数据
    ChunkData original(0, 0);
    auto section = original.createSection(4); // Y=64-79
    ASSERT_NE(section, nullptr);

    // 设置一些方块
    u32 stoneStateId = VanillaBlocks::STONE->defaultState().stateId();
    section->setBlockStateId(5, 5, 5, stoneStateId);
    section->setBlockStateId(10, 10, 10, stoneStateId);

    // 设置天空光照
    section->setSkyLight(0, 0, 0, 15);
    section->setSkyLight(5, 5, 5, 10);
    section->setSkyLight(10, 10, 10, 5);

    // 设置方块光照
    section->setBlockLight(0, 0, 0, 0);
    section->setBlockLight(5, 5, 5, 8);
    section->setBlockLight(10, 10, 10, 12);

    // 序列化
    auto serializeResult = ChunkSerializer::serializeChunk(original);
    ASSERT_TRUE(serializeResult.success());

    // 反序列化
    auto deserializeResult = ChunkSerializer::deserializeChunk(0, 0, serializeResult.value());
    ASSERT_TRUE(deserializeResult.success());

    auto restored = deserializeResult.value();
    ASSERT_TRUE(restored->hasSection(4));

    const ChunkSection* restoredSection = restored->getSection(4);
    ASSERT_NE(restoredSection, nullptr);

    // 验证天空光照
    EXPECT_EQ(restoredSection->getSkyLight(0, 0, 0), 15);
    EXPECT_EQ(restoredSection->getSkyLight(5, 5, 5), 10);
    EXPECT_EQ(restoredSection->getSkyLight(10, 10, 10), 5);

    // 验证方块光照
    EXPECT_EQ(restoredSection->getBlockLight(0, 0, 0), 0);
    EXPECT_EQ(restoredSection->getBlockLight(5, 5, 5), 8);
    EXPECT_EQ(restoredSection->getBlockLight(10, 10, 10), 12);
}

TEST_F(ChunkSerializerLightTest, LightDataNibbleArrayFormat)
{
    // 测试 NibbleArray 的打包和解包
    ChunkData original(0, 0);
    auto section = original.createSection(0);
    ASSERT_NE(section, nullptr);

    // 设置一些方块使区块段非空（否则不会被序列化）
    u32 stoneStateId = VanillaBlocks::STONE->defaultState().stateId();
    section->setBlockStateId(0, 0, 0, stoneStateId);

    // 设置多种光照值
    for (int i = 0; i < 16; ++i) {
        section->setSkyLight(i, 0, 0, static_cast<u8>(i));
        section->setBlockLight(0, i, 0, static_cast<u8>(15 - i));
    }

    // 序列化和反序列化
    auto serializeResult = ChunkSerializer::serializeChunk(original);
    ASSERT_TRUE(serializeResult.success());

    auto deserializeResult = ChunkSerializer::deserializeChunk(0, 0, serializeResult.value());
    ASSERT_TRUE(deserializeResult.success());

    auto restored = deserializeResult.value();
    const ChunkSection* restoredSection = restored->getSection(0);
    ASSERT_NE(restoredSection, nullptr);

    // 验证所有光照值
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(restoredSection->getSkyLight(i, 0, 0), static_cast<u8>(i)) << "Sky light mismatch at i=" << i;
        EXPECT_EQ(restoredSection->getBlockLight(0, i, 0), static_cast<u8>(15 - i))
            << "Block light mismatch at i=" << i;
    }
}

TEST_F(ChunkSerializerLightTest, MultipleSectionsLightData)
{
    // 测试多个区块段的光照数据
    ChunkData original(0, 0);

    for (int sectionY = 0; sectionY < 16; ++sectionY) {
        auto section = original.createSection(sectionY);
        ASSERT_NE(section, nullptr);

        // 设置不同段的光照
        section->setSkyLight(0, 0, 0, static_cast<u8>(sectionY));
        section->setBlockLight(0, 0, 0, static_cast<u8>(15 - sectionY));

        // 设置一些方块使其非空
        u32 stoneStateId = VanillaBlocks::STONE->defaultState().stateId();
        section->setBlockStateId(0, 0, 0, stoneStateId);
    }

    // 序列化和反序列化
    auto serializeResult = ChunkSerializer::serializeChunk(original);
    ASSERT_TRUE(serializeResult.success());

    auto deserializeResult = ChunkSerializer::deserializeChunk(0, 0, serializeResult.value());
    ASSERT_TRUE(deserializeResult.success());

    auto restored = deserializeResult.value();

    // 验证每个段的光照数据
    for (int sectionY = 0; sectionY < 16; ++sectionY) {
        EXPECT_TRUE(restored->hasSection(sectionY));
        const ChunkSection* section = restored->getSection(sectionY);
        ASSERT_NE(section, nullptr);

        EXPECT_EQ(section->getSkyLight(0, 0, 0), static_cast<u8>(sectionY))
            << "Sky light mismatch in section " << sectionY;
        EXPECT_EQ(section->getBlockLight(0, 0, 0), static_cast<u8>(15 - sectionY))
            << "Block light mismatch in section " << sectionY;
    }
}

TEST_F(ChunkSerializerLightTest, LightDataSectionSizeCalculation)
{
    // 测试区块段大小计算是否包含光照数据
    ChunkSection section;

    // 设置一些方块
    u32 stoneStateId = VanillaBlocks::STONE->defaultState().stateId();
    section.setBlockStateId(0, 0, 0, stoneStateId);

    size_t size = ChunkSerializer::calculateSectionSize(section);

    // 新格式: 2(计数) + 4096*4(方块) + 2048(天空光照) + 2048(方块光照) = 18434 字节
    EXPECT_EQ(size, 2 + ChunkSection::VOLUME * 4 + 2048 + 2048);
}
