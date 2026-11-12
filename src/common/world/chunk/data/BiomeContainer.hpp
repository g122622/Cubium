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
#include <span>
#include <utility>
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
    BiomeContainer(const BiomeContainer&) = default;
    BiomeContainer& operator=(const BiomeContainer&) = default;

    /** @brief 转移紧凑存储，源容器恢复为全零均匀态。 */
    BiomeContainer(BiomeContainer&& other) noexcept
        : m_uniform(std::exchange(other.m_uniform, 0))
        , m_bits(std::exchange(other.m_bits, 0))
        , m_palette(std::move(other.m_palette))
        , m_storage(std::move(other.m_storage))
    {}

    /** @brief 转移紧凑存储并释放目标的旧数组。 */
    BiomeContainer& operator=(BiomeContainer&& other) noexcept
    {
        if (this != &other) {
            m_uniform = std::exchange(other.m_uniform, 0);
            m_bits = std::exchange(other.m_bits, 0);
            m_palette = std::move(other.m_palette);
            m_storage = std::move(other.m_storage);
        }
        return *this;
    }

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

    /** @brief 展开指定段到调用方持有的短期数组，不物化常驻副本。 */
    [[nodiscard]] std::array<BiomeId, SECTION_BIOME_SIZE> sectionData(i32 sectionIndex) const;

    /** @brief 从连续采样数据恢复一段，保留其余段。 */
    void setSectionData(i32 sectionIndex, std::span<const BiomeId, SECTION_BIOME_SIZE> biomes);

    /** @brief 删除已经不再使用的调色板条目，并收缩为最小表示。 */
    void compact();

    /** @brief 当前对象及其独立分配的字节数，不含分配器开销。 */
    [[nodiscard]] size_t estimatedMemoryUsage() const noexcept;

    /**
     * @brief 序列化
     */
    [[nodiscard]] std::vector<u8> serialize() const;
    static Result<BiomeContainer> deserialize(const u8* data, size_t size);

private:
    // 均匀态不分配；低基数使用小调色板；超过 512 种时直接保存 u16，限制最坏开销。
    // 条目不跨 u64 边界，与 Heightmap 的位布局一致。
    BiomeId m_uniform = 0;
    u8 m_bits = 0;
    std::vector<BiomeId> m_palette;
    std::vector<u64> m_storage;

    /** @brief 读取有效线性采样索引。 */
    [[nodiscard]] BiomeId _get(size_t index) const;
    /** @brief 写入线性索引，按值种类自动选择表示。 */
    void _set(size_t index, BiomeId biome);
    /** @brief 读取非均匀表示中的索引或直接 ID。 */
    [[nodiscard]] u16 _getPacked(size_t index) const;
    /** @brief 写入已分配位数组中的条目。 */
    void _setPacked(size_t index, u16 value);
    /** @brief 改变位宽，切到直接存储时展开调色板索引。 */
    void _repack(u8 bits);
};

} // namespace mc::world::chunk

namespace mc {
using BiomeContainer = world::chunk::BiomeContainer;
} // namespace mc
