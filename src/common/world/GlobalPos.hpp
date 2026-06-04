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
#include "world/block/BlockPos.hpp"
#include <functional>

namespace mc {

/**
 * @brief 全局位置 - 方块位置 + 维度ID
 *
 * 用于标识跨维度的位置信息。
 */
class GlobalPos {
public:
    /**
     * @brief 默认构造函数
     *
     * 构造一个维度ID为0、位置为原点的全局位置。
     */
    GlobalPos() noexcept
        : m_dimensionId(0)
        , m_pos()
    {}

    /**
     * @brief 从维度ID和方块位置构造
     *
     * @param dimensionId 维度ID
     * @param pos 方块位置
     */
    GlobalPos(DimensionId dimensionId, const BlockPos& pos) noexcept
        : m_dimensionId(dimensionId)
        , m_pos(pos)
    {}

    /**
     * @brief 从维度ID和坐标构造
     *
     * @param dimensionId 维度ID
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     */
    GlobalPos(DimensionId dimensionId, BlockCoord x, BlockCoord y, BlockCoord z) noexcept
        : m_dimensionId(dimensionId)
        , m_pos(x, y, z)
    {}

    /** @brief 获取维度ID */
    [[nodiscard]] DimensionId getDimensionId() const noexcept { return m_dimensionId; }

    /** @brief 获取方块位置 */
    [[nodiscard]] const BlockPos& getPos() const noexcept { return m_pos; }

    /** @brief 获取X坐标 */
    [[nodiscard]] BlockCoord x() const noexcept { return m_pos.x; }

    /** @brief 获取Y坐标 */
    [[nodiscard]] BlockCoord y() const noexcept { return m_pos.y; }

    /** @brief 获取Z坐标 */
    [[nodiscard]] BlockCoord z() const noexcept { return m_pos.z; }

    /**
     * @brief 相等比较
     *
     * @param other 另一个全局位置
     * @return 是否相等（维度ID和位置都相同）
     */
    [[nodiscard]] bool operator==(const GlobalPos& other) const noexcept
    {
        return m_dimensionId == other.m_dimensionId && m_pos == other.m_pos;
    }

    /**
     * @brief 不相等比较
     *
     * @param other 另一个全局位置
     * @return 是否不相等
     */
    [[nodiscard]] bool operator!=(const GlobalPos& other) const noexcept { return !(*this == other); }

    /**
     * @brief 检查是否在同一维度
     *
     * @param other 另一个全局位置
     * @return 是否在同一维度
     */
    [[nodiscard]] bool sameDimension(const GlobalPos& other) const noexcept
    {
        return m_dimensionId == other.m_dimensionId;
    }

private:
    DimensionId m_dimensionId; ///< 维度ID
    BlockPos m_pos;            ///< 方块位置
};

} // namespace mc

// ============================================================================
// 哈希函数特化（std::hash）
// ============================================================================
namespace std {
template <>
struct hash<mc::GlobalPos> {
    /**
     * @brief 计算GlobalPos的哈希值
     *
     * 将维度ID和坐标组合成一个唯一的哈希值。
     */
    mc::Size operator()(const mc::GlobalPos& pos) const noexcept
    {
        const mc::Size h1 = std::hash<mc::i32>{}(pos.getDimensionId());
        const mc::Size h2 =
            static_cast<mc::Size>(pos.x()) ^ (static_cast<mc::Size>(pos.y()) << 1) ^ (static_cast<mc::Size>(pos.z()) << 2);
        return h1 ^ (h2 << 1);
    }
};
} // namespace std
