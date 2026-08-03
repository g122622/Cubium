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

#include "BiomeColorBlender.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include <array>

namespace mc::client {

/**
 * @brief 区块生物群系访问器
 *
 * 实现 IBiomeAccessor 接口，支持从当前区块和邻居区块获取生物群系。
 * 用于 ChunkMesher 在网格构建时访问生物群系数据。
 *
 * 使用示例：
 * @code
 * ChunkBiomeAccessor accessor(chunk, neighbors, chunkX, chunkZ);
 * const Biome* biome = accessor.getBiome(x, y, z);
 * @endcode
 */
class ChunkBiomeAccessor : public BiomeColorBlender::IBiomeAccessor {
public:
    /**
     * @brief 构造函数
     *
     * @param chunk 当前区块数据
     * @param neighbors 周围区块数据数组，顺序: -X, +X, -Z, +Z
     *                  索引: 0=西, 1=东, 2=北, 3=南
     * @param chunkX 当前区块X坐标
     * @param chunkZ 当前区块Z坐标
     * @param minBuildHeight 世界最小建筑高度
     * @param maxBuildHeight 世界最大建筑高度
     */
    ChunkBiomeAccessor(const ChunkData& chunk,
        const std::array<const ChunkData*, 4>& neighbors,
        ChunkCoord chunkX,
        ChunkCoord chunkZ,
        i32 minBuildHeight = mc::world::MIN_BUILD_HEIGHT,
        i32 maxBuildHeight = mc::world::MAX_BUILD_HEIGHT);

    /**
     * @brief 从单个区块构造简化访问器
     *
     * 不支持邻居区块访问，仅用于区块内部查询。
     */
    ChunkBiomeAccessor(const ChunkData& chunk,
        ChunkCoord chunkX,
        ChunkCoord chunkZ,
        i32 minBuildHeight = mc::world::MIN_BUILD_HEIGHT,
        i32 maxBuildHeight = mc::world::MAX_BUILD_HEIGHT);

    // ========================================================================
    // IBiomeAccessor 接口实现
    // ========================================================================

    [[nodiscard]] const Biome* getBiome(i32 x, i32 y, i32 z) const override;
    [[nodiscard]] bool isChunkLoaded(ChunkCoord x, ChunkCoord z) const override;

    // ========================================================================
    // 额外方法
    // ========================================================================

    /**
     * @brief 获取区块局部坐标的生物群系
     *
     * @param localX 区块内X坐标 (0-15)
     * @param y 世界Y坐标
     * @param localZ 区块内Z坐标 (0-15)
     * @return 生物群系指针
     */
    [[nodiscard]] const Biome* getBiomeLocal(i32 localX, i32 y, i32 localZ) const;

    /**
     * @brief 获取当前区块坐标
     */
    [[nodiscard]] ChunkCoord chunkX() const { return m_chunkX; }
    [[nodiscard]] ChunkCoord chunkZ() const { return m_chunkZ; }

private:
    /**
     * @brief 确定应该从哪个区块获取生物群系
     *
     * @param worldX 世界X坐标
     * @param worldZ 世界Z坐标
     * @param outLocalX 输出区块内X坐标
     * @param outLocalZ 输出区块内Z坐标
     * @return 指向对应区块的指针，如果超出范围返回 nullptr
     */
    [[nodiscard]] const ChunkData* _resolveChunk(i32 worldX, i32 worldZ, i32& outLocalX, i32& outLocalZ) const;

    const ChunkData& m_chunk;
    std::array<const ChunkData*, 4> m_neighbors; // -X, +X, -Z, +Z
    ChunkCoord m_chunkX;
    ChunkCoord m_chunkZ;
    i32 m_minBuildHeight;
    i32 m_maxBuildHeight;
};

} // namespace mc::client
