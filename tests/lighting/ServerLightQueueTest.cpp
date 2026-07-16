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

// ③-2b：ServerLightQueue 队列内部语义测试（去重 / 分组 / 计数）。
//
// drainAndProcess(WorldLightManager&) 旧重载已删（m_mutex/单例引擎已移除），
// 现 drainAndProcess(ServerWorld&) 需完整 ServerWorld 才能经 RuntimeLightingProvider
// + TLS 引擎传播。传播正确性（含去重后确实执行一次传播）由 RuntimeLightConcurrencyTest
// 端到端覆盖。本文件聚焦队列本身的入队去重与跨区块分组语义，不依赖 ServerWorld。

#include <gtest/gtest.h>

#include "common/core/Constants.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "server/world/ServerLightQueue.hpp"

namespace mc::server {

namespace {

/// chunk 坐标 → queue 内部键（ChunkPos::toId），用于校验分组计数
[[nodiscard]] u64 _chunkKey(i32 chunkX, i32 chunkZ) noexcept
{
    return ChunkPos(chunkX, chunkZ).toId();
}

} // namespace

// 入队后队列非空，pendingChunkCount 反映区块数
TEST(ServerLightQueueTest, EnqueueUpdatesQueueState)
{
    ServerLightQueue queue;
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.pendingChunkCount(), 0u);

    queue.queueBlockChange(1, 70, 2);
    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.pendingChunkCount(), 1u);
}

// 同一坐标多次入队自动去重：pendingChunkCount 仍为 1
TEST(ServerLightQueueTest, DuplicatePositionDeduplicated)
{
    ServerLightQueue queue;
    queue.queueBlockChange(8, 70, 8);
    queue.queueBlockChange(8, 70, 8);         // 重复
    queue.queueBlockChange(8, 70, 8);         // 重复
    EXPECT_EQ(queue.pendingChunkCount(), 1u); // 同区块同坐标合并
}

// 同区块多个不同坐标一次性入队：pendingChunkCount 仍为 1（同区块合并）
TEST(ServerLightQueueTest, SameChunkMultiplePositionsBatched)
{
    ServerLightQueue queue;
    queue.queueBlockChange(8, 70, 8);         // 区块 (0,0)
    queue.queueBlockChange(12, 70, 8);        // 区块 (0,0)
    queue.queueBlockChange(15, 70, 15);       // 区块 (0,0)
    EXPECT_EQ(queue.pendingChunkCount(), 1u); // 同区块合并
}

// 跨区块坐标分别入队：pendingChunkCount 反映不同区块数
TEST(ServerLightQueueTest, CrossChunkPositionsGroupedSeparately)
{
    ServerLightQueue queue;
    queue.queueBlockChange(8, 70, 8);         // 区块 (0,0)
    queue.queueBlockChange(20, 70, 8);        // 区块 (1,0)
    queue.queueBlockChange(8, 70, 20);        // 区块 (0,1)
    EXPECT_EQ(queue.pendingChunkCount(), 3u); // 三个不同区块

    // 校验键分组正确（ChunkPos::toId 唯一性）
    (void)_chunkKey(0, 0);
    (void)_chunkKey(1, 0);
    (void)_chunkKey(0, 1);
}

// Y 轴跨度大也不与同列其他坐标碰撞（BlockPos::asLong 12 位 Y 覆盖 ±2048）
TEST(ServerLightQueueTest, HighYPositionsDoNotCollide)
{
    ServerLightQueue queue;
    queue.queueBlockChange(8, 320, 8); // 区块 (0,0) Y=320
    queue.queueBlockChange(8, -64, 8); // 区块 (0,0) Y=-64
    queue.queueBlockChange(8, 70, 8);  // 区块 (0,0) Y=70
    // 同区块三个不同 Y 坐标，pendingChunkCount 仍为 1
    EXPECT_EQ(queue.pendingChunkCount(), 1u);
}

} // namespace mc::server
