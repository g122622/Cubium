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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/world/WorldConstants.hpp"
#include <array>
#include <cstddef>
#include <vector>

namespace mc::world::chunk {

// ============================================================================
// 生物群系容器
// ============================================================================

/**
 * @brief 生物群系容器
 *
 * 存储区块内的生物群系信息。每个区块有 4x4x4 的生物群系采样点。
 */
class BiomeContainer {
public:
    // 生物群系采样尺寸（每个区块段的生物群系采样点数量）
    static constexpr i32 HORIZ_SIZE = 4;                                           // 水平方向采样点
    static constexpr i32 VERT_SIZE = 4;                                            // 每段垂直采样点
    static constexpr i32 SECTION_BIOME_SIZE = HORIZ_SIZE * VERT_SIZE * HORIZ_SIZE; // 64
    static constexpr i32 SECTION_COUNT = mc::world::CHUNK_SECTIONS;
    static constexpr i32 TOTAL_SIZE = SECTION_BIOME_SIZE * SECTION_COUNT; // 1536

    BiomeContainer() = default;

    /**
     * @brief 设置指定区块段的生物群系
     * @param sectionIndex 区块段索引 (0-23)
     * @param x X 采样位置 (0-3)
     * @param y Y 采样位置 (0-3)
     * @param z Z 采样位置 (0-3)
     * @param biome 生物群系 ID
     */
    void setBiome(i32 sectionIndex, i32 x, i32 y, i32 z, BiomeId biome);

    /**
     * @brief 获取指定区块段的生物群系
     * @param sectionIndex 区块段索引 (0-23)
     * @param x X 采样位置 (0-3)
     * @param y Y 采样位置 (0-3)
     * @param z Z 采样位置 (0-3)
     */
    [[nodiscard]] BiomeId getBiome(i32 sectionIndex, i32 x, i32 y, i32 z) const;

    /**
     * @brief 获取方块位置的生物群系（3D 插值）
     * @param x 方块 X 坐标（世界坐标，相对区块 0-15）
     * @param y 方块 Y 坐标（世界坐标）
     * @param z 方块 Z 坐标（世界坐标，相对区块 0-15）
     *
     * 自动将世界 Y 坐标映射到正确的 section 和 biome Y 索引。
     */
    [[nodiscard]] BiomeId getBiomeAtBlock(i32 x, i32 y, i32 z) const;

    /**
     * @brief 序列化
     */
    [[nodiscard]] std::vector<u8> serialize() const;
    static Result<BiomeContainer> deserialize(const u8* data, size_t size);

private:
    // 存储所有 section 的生物群系 ID，初始化为 0
    std::array<BiomeId, TOTAL_SIZE> m_biomes{};
};

} // namespace mc::world::chunk

namespace mc {
using BiomeContainer = world::chunk::BiomeContainer;
} // namespace mc
