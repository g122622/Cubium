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

#pragma once

#include "common/core/Types.hpp"
#include "common/world/chunk/base/ChunkId.hpp"
#include "server/network/sync/chunk/ChunkView.hpp"

#include <unordered_set>
#include <vector>

namespace mc::server::sync {

// ============================================================================
// 玩家区块跟踪器 - 跟踪每个玩家已加载的区块
// ============================================================================

/**
 * @brief 单个玩家已下发区块的记账
 *
 * 仅服务端使用：记录"哪个玩家已经收到哪个区块"，用于计算区块推送差集。
 * 非线程安全，调用方需保证同一玩家的访问在主线程串行。
 */
class PlayerChunkTracker {
public:
    explicit PlayerChunkTracker(PlayerId playerId);

    [[nodiscard]] PlayerId playerId() const { return m_playerId; }
    [[nodiscard]] const ChunkView& view() const { return m_view; }
    ChunkView& view() { return m_view; }

    // 已加载区块管理
    void addLoadedChunk(ChunkCoord x, ChunkCoord z);
    void removeLoadedChunk(ChunkCoord x, ChunkCoord z);
    [[nodiscard]] bool hasChunk(ChunkCoord x, ChunkCoord z) const;
    [[nodiscard]] const std::unordered_set<ChunkId>& loadedChunks() const { return m_loadedChunks; }

    // 更新视距中心
    void updateCenter(ChunkCoord x, ChunkCoord z);

    // 计算需要的区块更新
    void calculateChunkUpdates(std::vector<ChunkPos>& chunksToLoad, std::vector<ChunkPos>& chunksToUnload);

    // 设置视距
    void setViewDistance(i32 distance);
    [[nodiscard]] i32 viewDistance() const { return m_view.viewDistance; }

    // 清空所有已加载区块
    void clear();

private:
    PlayerId m_playerId;
    ChunkView m_view;
    std::unordered_set<ChunkId> m_loadedChunks;
};

} // namespace mc::server::sync
