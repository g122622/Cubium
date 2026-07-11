/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
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

#include "common/core/Constants.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/lighting/IChunkLightProvider.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"
#include "common/world/lighting/storage/SWMRNibbleArray.hpp"
#include "server/world/ServerLightQueue.hpp"

namespace mc::server {

namespace {

/// 单区块光照提供者：仅暴露一个 ChunkData 供 WorldLightManager 访问
class _SingleChunkProvider : public StarLightLightingProvider {
public:
    _SingleChunkProvider(i32 minBuildHeight, i32 maxBuildHeight)
        : m_minBuildHeight(minBuildHeight)
        , m_maxBuildHeight(maxBuildHeight)
    {}

    void setChunk(ChunkData* chunk) { m_chunk = chunk; }

    IChunk* getChunkForLight(ChunkCoord x, ChunkCoord z) override { return lookup(x, z); }
    const IChunk* getChunkForLight(ChunkCoord x, ChunkCoord z) const override { return lookup(x, z); }

    const BlockState* getBlockStateForLight(const BlockPos& pos) const override
    {
        if (m_chunk == nullptr || pos.chunkX() != m_chunk->x() || pos.chunkZ() != m_chunk->z()) {
            return nullptr;
        }
        return m_chunk->getBlockState(pos.x & 0xF, pos.y, pos.z & 0xF);
    }

    IWorld* getWorld() override { return nullptr; }
    const IWorld* getWorld() const override { return nullptr; }
    void markLightChanged(LightType, const SectionPos&) override {}
    bool hasSkyLight() const override { return false; }
    i32 getMinBuildHeight() const override { return m_minBuildHeight; }
    i32 getMaxBuildHeight() const override { return m_maxBuildHeight; }
    i32 getSectionCount() const override { return (m_maxBuildHeight - m_minBuildHeight) >> 4; }

private:
    [[nodiscard]] ChunkData* lookup(ChunkCoord x, ChunkCoord z) const
    {
        if (m_chunk == nullptr || m_chunk->x() != x || m_chunk->z() != z) {
            return nullptr;
        }
        return m_chunk;
    }

    ChunkData* m_chunk = nullptr;
    i32 m_minBuildHeight;
    i32 m_maxBuildHeight;
};

void ensureVanillaBlocksInitialized()
{
    static bool initialized = false;
    if (!initialized) {
        VanillaBlocks::initialize();
        initialized = true;
    }
}

/// 主世界方块 Y=70 → sectionY=4，nibble 索引 = 4 - m_minLightSection(-5) = 9
constexpr i32 NIBBLE_INDEX_Y70 = 9;

} // namespace

// 入队后队列非空，drain 后清空
TEST(ServerLightQueueTest, EnqueueThenDrainClearsQueue)
{
    ServerLightQueue queue;
    EXPECT_TRUE(queue.empty());

    queue.queueBlockChange(1, 70, 2);
    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.pendingChunkCount(), 1u);

    _SingleChunkProvider provider(world::MIN_BUILD_HEIGHT, world::MAX_BUILD_HEIGHT);
    ChunkData chunk(0, 0);
    chunk.setStatus(ChunkLoadStatus::Generated);
    provider.setChunk(&chunk);
    WorldLightManager manager(&provider, true, false);

    queue.drainAndProcess(manager);
    EXPECT_TRUE(queue.empty());
}

// 同一坐标多次入队自动去重，drain 仅传播一次
TEST(ServerLightQueueTest, DuplicatePositionDeduplicated)
{
    ensureVanillaBlocksInitialized();

    _SingleChunkProvider provider(world::MIN_BUILD_HEIGHT, world::MAX_BUILD_HEIGHT);
    ChunkData chunk(0, 0);
    chunk.setStatus(ChunkLoadStatus::Generated);
    provider.setChunk(&chunk);
    WorldLightManager manager(&provider, true, false);

    const BlockState* glowstone = &VanillaBlocks::GLOWSTONE->defaultState();
    const BlockState* air = &VanillaBlocks::AIR->defaultState();
    chunk.setBlockState(8, 70, 8, glowstone);
    manager.updateSectionStatus(SectionPos(0, 4, 0), false);
    manager.lightChunk(&chunk, false);

    // 模拟 ServerWorld::setBlockState 流程：先写新方块状态，再入队光照变更
    chunk.setBlockState(8, 70, 8, air);

    ServerLightQueue queue;
    queue.queueBlockChange(8, 70, 8);
    queue.queueBlockChange(8, 70, 8); // 重复
    queue.queueBlockChange(8, 70, 8); // 重复
    EXPECT_EQ(queue.pendingChunkCount(), 1u);

    queue.drainAndProcess(manager);

    auto* nibbles = chunk.getBlockNibbles();
    ASSERT_NE(nibbles, nullptr);
    SWMRNibbleArray* nibble = nibbles[NIBBLE_INDEX_Y70];
    ASSERT_NE(nibble, nullptr);

    // 光源已移除并经队列批量传播，光源处应归零（验证去重后传播确实执行了）
    EXPECT_EQ(nibble->getUpdating(8, 70 - 64, 8), static_cast<u8>(0));
}

// 同区块多个不同坐标一次性入队，drain 批量传播全部生效
TEST(ServerLightQueueTest, SameChunkMultiplePositionsBatched)
{
    ensureVanillaBlocksInitialized();

    _SingleChunkProvider provider(world::MIN_BUILD_HEIGHT, world::MAX_BUILD_HEIGHT);
    ChunkData chunk(0, 0);
    chunk.setStatus(ChunkLoadStatus::Generated);
    provider.setChunk(&chunk);
    WorldLightManager manager(&provider, true, false);

    const BlockState* glowstone = &VanillaBlocks::GLOWSTONE->defaultState();
    chunk.setBlockState(8, 70, 8, glowstone);
    chunk.setBlockState(12, 70, 8, glowstone);
    manager.updateSectionStatus(SectionPos(0, 4, 0), false);
    manager.lightChunk(&chunk, false);

    auto* nibbles = chunk.getBlockNibbles();
    ASSERT_NE(nibbles, nullptr);
    SWMRNibbleArray* nibble = nibbles[NIBBLE_INDEX_Y70];
    ASSERT_NE(nibble, nullptr);

    // 两个光源初始都有光
    EXPECT_GT(nibble->getUpdating(8, 70 - 64, 8), static_cast<u8>(0));
    EXPECT_GT(nibble->getUpdating(12, 70 - 64, 8), static_cast<u8>(0));

    // 一次性入队两个移除变更，drain 批量传播
    const BlockState* air = &VanillaBlocks::AIR->defaultState();
    chunk.setBlockState(8, 70, 8, air);
    chunk.setBlockState(12, 70, 8, air);

    ServerLightQueue queue;
    queue.queueBlockChange(8, 70, 8);
    queue.queueBlockChange(12, 70, 8);
    EXPECT_EQ(queue.pendingChunkCount(), 1u); // 同区块合并
    queue.drainAndProcess(manager);

    // 两处都应归零，证明批量传播覆盖了全部坐标
    EXPECT_EQ(nibble->getUpdating(8, 70 - 64, 8), static_cast<u8>(0));
    EXPECT_EQ(nibble->getUpdating(12, 70 - 64, 8), static_cast<u8>(0));
}

// 跨区块坐标分别入队，pendingChunkCount 反映分组数，drain 后全部传播
TEST(ServerLightQueueTest, CrossChunkPositionsGroupedSeparately)
{
    ensureVanillaBlocksInitialized();

    _SingleChunkProvider provider(world::MIN_BUILD_HEIGHT, world::MAX_BUILD_HEIGHT);
    // 区块 (0,0)：provider 仅认识这一个区块
    ChunkData chunk(0, 0);
    chunk.setStatus(ChunkLoadStatus::Generated);
    provider.setChunk(&chunk);
    WorldLightManager manager(&provider, true, false);

    const BlockState* glowstone = &VanillaBlocks::GLOWSTONE->defaultState();
    const BlockState* air = &VanillaBlocks::AIR->defaultState();
    chunk.setBlockState(8, 70, 8, glowstone); // 区块 (0,0)
    manager.updateSectionStatus(SectionPos(0, 4, 0), false);
    manager.lightChunk(&chunk, false);

    // 模拟 ServerWorld::setBlockState：先写新方块状态，再入队光照变更
    chunk.setBlockState(8, 70, 8, air);

    ServerLightQueue queue;
    queue.queueBlockChange(8, 70, 8); // 区块 (0,0)
    queue.queueBlockChange(20,
        70,
        8); // 区块 (1,0) —— provider 不持有，drain 时 blocksChangedInChunk 内部 getChunkInCache 返回 nullptr 会安全跳过
    EXPECT_EQ(queue.pendingChunkCount(), 2u); // 两个不同区块

    queue.drainAndProcess(manager);
    EXPECT_TRUE(queue.empty());

    // 区块 (0,0) 的光源已移除并经队列传播生效
    auto* nibbles = chunk.getBlockNibbles();
    ASSERT_NE(nibbles, nullptr);
    SWMRNibbleArray* nibble = nibbles[NIBBLE_INDEX_Y70];
    ASSERT_NE(nibble, nullptr);
    EXPECT_EQ(nibble->getUpdating(8, 70 - 64, 8), static_cast<u8>(0));
}

} // namespace mc::server
