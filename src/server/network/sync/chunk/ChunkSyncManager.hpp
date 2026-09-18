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
#include "common/world/WorldConstants.hpp"
#include "server/network/sync/chunk/PlayerChunkTracker.hpp"

#include <cmath>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mc::server::sync {

// ============================================================================
// 区块同步管理器 - 管理所有玩家的区块同步
// ============================================================================

/**
 * @brief 全体玩家的区块推送记账：玩家 → 已下发区块，区块 → 订阅玩家
 *
 * 仅服务端使用。双向索引缺一不可：前者用于计算某个玩家的推送差集，
 * 后者用于区块被卸载时反查需要通知的玩家。
 *
 * 本类不是线程安全的，多线程环境下需外部同步。
 */
class ChunkSyncManager {
public:
    // 获取或创建玩家区块跟踪器
    [[nodiscard]] std::shared_ptr<PlayerChunkTracker> getTracker(PlayerId playerId);
    void removeTracker(PlayerId playerId);

    // 更新玩家位置
    void updatePlayerPosition(PlayerId playerId, f64 x, f64 z);

    // 计算区块更新
    void calculateUpdates(
        PlayerId playerId, std::vector<ChunkPos>& chunksToLoad, std::vector<ChunkPos>& chunksToUnload);

    // 标记区块为已发送
    void markChunkSent(PlayerId playerId, ChunkCoord x, ChunkCoord z);

    // 标记区块为已卸载
    void markChunkUnloaded(PlayerId playerId, ChunkCoord x, ChunkCoord z);

    // 获取区块订阅者（哪些玩家需要这个区块）
    [[nodiscard]] std::vector<PlayerId> getChunkSubscribers(ChunkCoord x, ChunkCoord z) const
    {
        std::vector<PlayerId> subscribers;
        getChunkSubscribers(x, z, subscribers);
        return subscribers;
    }

    /**
     * @brief 获取区块订阅者（输出参数版本，避免分配）
     * @param x 区块X坐标
     * @param z 区块Z坐标
     * @param out 输出向量（会被清空并填充）
     */
    void getChunkSubscribers(ChunkCoord x, ChunkCoord z, std::vector<PlayerId>& out) const
    {
        out.clear();

        ChunkId chunkId(x, z, 0);
        auto it = m_chunkSubscribers.find(chunkId);
        if (it != m_chunkSubscribers.end()) {
            out.reserve(it->second.size());
            for (PlayerId playerId : it->second) {
                out.push_back(playerId);
            }
        }
    }

    // 全局设置
    void setDefaultViewDistance(i32 distance) { m_defaultViewDistance = distance; }
    [[nodiscard]] i32 defaultViewDistance() const { return m_defaultViewDistance; }

    // 区块坐标转换工具
    static ChunkCoord blockToChunk(f64 blockCoord)
    {
        return static_cast<ChunkCoord>(std::floor(blockCoord / static_cast<f64>(world::CHUNK_WIDTH)));
    }

private:
    std::unordered_map<PlayerId, std::shared_ptr<PlayerChunkTracker>> m_trackers;

    // 区块 -> 订阅玩家映射
    std::unordered_map<ChunkId, std::unordered_set<PlayerId>> m_chunkSubscribers;

    i32 m_defaultViewDistance = world::CHUNK_LOAD_RADIUS;
};

} // namespace mc::server::sync
