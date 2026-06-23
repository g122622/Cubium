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
 * LIABILITY, IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"

#include <cstddef>
#include <functional>
#include <vector>

namespace mc::server {

/**
 * @brief 预分配的二维区块缓存，构造时一次性填充所有条目
 *
 * 对齐 Moonrise 的 `StaticCache2D<GenerationChunkHolder>`。
 *
 * 与 `GenerationChunkCache` 的关键区别：
 * - **一次性填充**：构造时调用 `loader` 填充 `(2*radius+1)²` 个条目，之后只读
 * - **不允许 nullptr/空洞**：loader 必须为每个位置返回有效条目，保证窗口内无空洞
 * - **不持有所有权**：存储 `T` 值（通常为 `shared_ptr` 或轻量句柄），不管理 `T` 的生命周期
 * - **越界断言**：`get` 对越界坐标触发 `MC_ASSERT_RELEASE`（Release 模式也启用）
 *
 * 这保证了 WorldGenRegion 访问窗口的"无空洞"不变量：调用方在构造缓存前必须确认
 * 所有邻居已就绪（由 `ChunkTaskScheduler::checkNeighbour` 保证），构造后 `get` 绝不会
 * 返回无效条目。
 *
 * @tparam T 缓存条目类型，必须可移动构造（用于 loader 返回值填充）
 */
template <typename T>
class StaticChunkCache2D {
public:
    /**
     * @brief 加载器回调，为指定区块坐标返回缓存条目
     *
     * 调用方必须保证返回有效条目（不得返回默认/空值），否则破坏无空洞不变量。
     */
    using Loader = std::function<T(ChunkCoord x, ChunkCoord z)>;

    /**
     * @brief 构造缓存并一次性填充所有条目
     *
     * @param centerX 中心区块 X
     * @param centerZ 中心区块 Z
     * @param radius 半径（区块数），覆盖 `[centerX±radius, centerZ±radius]`，边长 `2*radius+1`
     * @param loader 为每个位置 `(x, z)` 返回条目的回调
     */
    StaticChunkCache2D(ChunkCoord centerX, ChunkCoord centerZ, i32 radius, Loader loader)
        : m_centerX(centerX)
        , m_centerZ(centerZ)
        , m_radius(radius)
        , m_diameter(2 * radius + 1)
        , m_entries(static_cast<size_t>(m_diameter) * static_cast<size_t>(m_diameter))
    {
        MC_ASSERT_RELEASE_MSG(radius >= 0, "StaticChunkCache2D: radius must be non-negative");
        MC_ASSERT_RELEASE_MSG(loader, "StaticChunkCache2D: loader must not be null");

        for (i32 dz = -radius; dz <= radius; ++dz) {
            for (i32 dx = -radius; dx <= radius; ++dx) {
                const ChunkCoord x = centerX + dx;
                const ChunkCoord z = centerZ + dz;
                m_entries[static_cast<size_t>(index(dx, dz))] = loader(x, z);
            }
        }
    }

    StaticChunkCache2D(const StaticChunkCache2D&) = delete;
    StaticChunkCache2D& operator=(const StaticChunkCache2D&) = delete;
    StaticChunkCache2D(StaticChunkCache2D&&) noexcept = default;
    StaticChunkCache2D& operator=(StaticChunkCache2D&&) noexcept = default;
    ~StaticChunkCache2D() = default;

    /**
     * @brief 获取指定位置的缓存条目
     *
     * 越界坐标触发 `MC_ASSERT_RELEASE`。窗口内条目在构造时已填充，必为有效值。
     *
     * @return 条目的 const 引用
     */
    [[nodiscard]] const T& get(ChunkCoord x, ChunkCoord z) const
    {
        MC_ASSERT_RELEASE_MSG(inBounds(x, z), "StaticChunkCache2D: coordinate out of bounds");
        return m_entries[static_cast<size_t>(index(x - m_centerX, z - m_centerZ))];
    }

    /**
     * @brief 检查坐标是否在缓存窗口内
     */
    [[nodiscard]] bool inBounds(ChunkCoord x, ChunkCoord z) const
    {
        const i32 dx = x - m_centerX;
        const i32 dz = z - m_centerZ;
        // Chebyshev 距离检查
        return dx >= -m_radius && dx <= m_radius && dz >= -m_radius && dz <= m_radius;
    }

    /**
     * @brief 中心区块 X
     */
    [[nodiscard]] ChunkCoord centerX() const { return m_centerX; }

    /**
     * @brief 中心区块 Z
     */
    [[nodiscard]] ChunkCoord centerZ() const { return m_centerZ; }

    /**
     * @brief 缓存半径
     */
    [[nodiscard]] i32 radius() const { return m_radius; }

    /**
     * @brief 缓存边长（= 2*radius + 1）
     */
    [[nodiscard]] i32 diameter() const { return m_diameter; }

    /**
     * @brief 条目总数（= diameter²）
     */
    [[nodiscard]] std::size_t size() const { return m_entries.size(); }

private:
    /**
     * @brief 计算条目在 m_entries 中的线性索引
     *
     * @param dx 相对中心的 X 偏移（[-radius, radius]）
     * @param dz 相对中心的 Z 偏移（[-radius, radius]）
     * @return 线性索引（调用方需保证 dx/dz 在范围内）
     */
    [[nodiscard]] i32 index(i32 dx, i32 dz) const
    {
        // 行优先：dz 行内 dx 列
        return (dz + m_radius) * m_diameter + (dx + m_radius);
    }

    ChunkCoord m_centerX;
    ChunkCoord m_centerZ;
    i32 m_radius;
    i32 m_diameter;
    std::vector<T> m_entries;
};

} // namespace mc::server
