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
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/world/chunk/ChunkPrimer.hpp"
#include <vector>

namespace mc::server {

/**
 * @brief 生成过程中的区块缓存
 *
 * 提供按世界坐标访问区块中间态的能力。
 * 在区块生成过程中，中心区块及其依赖环内的邻居区块
 * 的 ChunkPrimer 都通过此缓存访问。
 *
 * 缓存以中心区块为原点，存储 (2*radius+1)^2 个位置。
 * 每个位置可以指向一个 ChunkPrimer 或为空。
 */
class GenerationChunkCache {
public:
    /**
     * @brief 构造生成缓存
     * @param centerX 中心区块 X 坐标
     * @param centerZ 中心区块 Z 坐标
     * @param radius 缓存半径（0 = 仅中心，1 = 3x3，8 = 17x17）
     */
    GenerationChunkCache(ChunkCoord centerX, ChunkCoord centerZ, i32 radius);

    ~GenerationChunkCache() = default;

    GenerationChunkCache(const GenerationChunkCache&) = delete;
    GenerationChunkCache& operator=(const GenerationChunkCache&) = delete;
    GenerationChunkCache(GenerationChunkCache&&) noexcept = default;
    GenerationChunkCache& operator=(GenerationChunkCache&&) noexcept = default;

    /**
     * @brief 获取指定位置的 ChunkPrimer
     * @param x 区块 X 坐标（世界坐标）
     * @param z 区块 Z 坐标（世界坐标）
     * @return ChunkPrimer 指针；如果不在缓存范围内或未设置则返回 nullptr
     */
    [[nodiscard]] ChunkPrimer* get(ChunkCoord x, ChunkCoord z) const;

    /**
     * @brief 设置指定位置的 ChunkPrimer
     * @param x 区块 X 坐标（世界坐标）
     * @param z 区块 Z 坐标（世界坐标）
     * @param primer ChunkPrimer 指针（不获取所有权）
     */
    void set(ChunkCoord x, ChunkCoord z, ChunkPrimer* primer);

    /**
     * @brief 检查指定位置是否在缓存范围内
     */
    [[nodiscard]] bool contains(ChunkCoord x, ChunkCoord z) const;

    /**
     * @brief 获取指定位置的 IChunk（ChunkData 或 ChunkPrimer）
     *
     * 优先返回缓存中的 ChunkPrimer，其次返回外部提供的 ChunkData。
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param fallback 如果缓存中没有，使用此指针作为后备
     * @return IChunk 指针；如果都不可用则返回 nullptr
     */
    [[nodiscard]] IChunk* getOrFallback(ChunkCoord x, ChunkCoord z, IChunk* fallback) const;

    [[nodiscard]] ChunkCoord centerX() const { return m_centerX; }
    [[nodiscard]] ChunkCoord centerZ() const { return m_centerZ; }
    [[nodiscard]] i32 radius() const { return m_radius; }
    [[nodiscard]] i32 diameter() const { return m_diameter; }

private:
    [[nodiscard]] i32 _index(ChunkCoord x, ChunkCoord z) const;
    [[nodiscard]] bool _inBounds(ChunkCoord x, ChunkCoord z) const;

    ChunkCoord m_centerX;
    ChunkCoord m_centerZ;
    i32 m_radius;
    i32 m_diameter;
    std::vector<ChunkPrimer*> m_entries;
};

} // namespace mc::server
