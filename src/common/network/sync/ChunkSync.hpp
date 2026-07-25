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

#include "../../core/Result.hpp"
#include "../codec/PacketSerializer.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace mc::network {

// ============================================================================
// 区块序列化器 - 将区块数据序列化为网络传输格式
// ============================================================================

class ChunkSerializer {
public:
    // 序列化区块数据到包格式
    static Result<std::vector<u8>> serializeChunk(const ChunkData& chunk);
    static std::vector<u8> serializeSection(const ChunkSection& section);

    // 反序列化区块数据
    static Result<std::unique_ptr<ChunkData>> deserializeChunk(ChunkCoord x, ChunkCoord z, const std::vector<u8>& data);
    /** 直接对原始字节缓冲反序列化（零拷贝视图，不拥有 data 生命周期，调用方负责保活）。 */
    static Result<std::unique_ptr<ChunkData>> deserializeChunk(ChunkCoord x, ChunkCoord z, const u8* data, size_t size);
    static Result<std::unique_ptr<ChunkSection>> deserializeChunkSection(const u8* data, size_t size);

    // 计算序列化后的大小
    static size_t calculateChunkSize(const ChunkData& chunk);
    static size_t calculateSectionSize(const ChunkSection& section);

    // 区块段位掩码计算
    static u32 calculateSectionMask(const ChunkData& chunk);
};

// ============================================================================
// 区块视图距离管理
// ============================================================================

struct ChunkView {
    ChunkCoord centerX = 0;
    ChunkCoord centerZ = 0;
    i32 viewDistance = world::CHUNK_LOAD_RADIUS; // 默认视距

    // 检查区块是否在视距内
    [[nodiscard]] bool isChunkInView(ChunkCoord x, ChunkCoord z) const
    {
        i32 dx = std::abs(x - centerX);
        i32 dz = std::abs(z - centerZ);
        return dx <= viewDistance && dz <= viewDistance;
    }

    // 获取所有在视距内的区块坐标
    [[nodiscard]] std::vector<ChunkPos> getChunksInView() const
    {
        std::vector<ChunkPos> chunks;
        getChunksInView(chunks);
        return chunks;
    }

    /**
     * @brief 获取所有在视距内的区块坐标（输出参数版本，避免分配）
     * @param out 输出向量（会被清空并填充）
     */
    void getChunksInView(std::vector<ChunkPos>& out) const
    {
        out.clear();
        const size_t diameter = static_cast<size_t>(viewDistance) * 2 + 1;
        out.reserve(diameter * diameter);

        for (ChunkCoord x = centerX - viewDistance; x <= centerX + viewDistance; ++x) {
            for (ChunkCoord z = centerZ - viewDistance; z <= centerZ + viewDistance; ++z) {
                out.emplace_back(x, z);
            }
        }
    }

    // 计算需要加载的新区块和需要卸载的旧区块
    void calculateChunkDiff(const std::unordered_set<ChunkId>& currentChunks,
        std::vector<ChunkPos>& chunksToLoad,
        std::vector<ChunkPos>& chunksToUnload) const;
};

// ============================================================================
// 玩家区块跟踪器 - 跟踪每个玩家已加载的区块
// ============================================================================

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

// ============================================================================
// 区块同步管理器 - 管理所有玩家的区块同步
// ============================================================================

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

} // namespace mc::network
