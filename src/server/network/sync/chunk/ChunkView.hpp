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
#include "common/world/chunk/base/ChunkId.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"

#include <cstdlib>
#include <unordered_set>
#include <vector>

namespace mc::server::sync {

// ============================================================================
// 区块视图距离管理
// ============================================================================

/**
 * @brief 以某坐标为圆心的正方形区块视距范围
 *
 * 注意 getChunksInView 返回的是**正方形区域**而非圆形：视距 n 表示半径 n 的正方形，
 * 区块数量为 (2n + 1)^2。
 */
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
        std::vector<ChunkPos>& chunksToUnload) const
    {
        chunksToLoad.clear();
        chunksToUnload.clear();

        // 获取当前视距内的区块（使用输出参数避免分配）
        std::vector<ChunkPos> viewChunks;
        getChunksInView(viewChunks);

        // 构建视距内区块ID集合
        std::unordered_set<ChunkId> viewChunkIds;
        viewChunkIds.reserve(viewChunks.size());
        for (const auto& pos : viewChunks) {
            viewChunkIds.insert(ChunkId(pos.x, pos.z, 0));
        }

        // 找出需要加载的区块（在视距内但不在当前集合中）
        for (const auto& id : viewChunkIds) {
            if (currentChunks.find(id) == currentChunks.end()) {
                chunksToLoad.emplace_back(id.x, id.z);
            }
        }

        // 找出需要卸载的区块（在当前集合中但不在视距内）
        for (const auto& id : currentChunks) {
            if (viewChunkIds.find(id) == viewChunkIds.end()) {
                chunksToUnload.emplace_back(id.x, id.z);
            }
        }
    }
};

} // namespace mc::server::sync
