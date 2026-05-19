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

#include "../core/Types.hpp"
#include "block/BlockPos.hpp"
#include <functional>

namespace mc {

/**
 * @brief 全局位置 - 方块位置 + 维度ID
 *
 * 用于标识跨维度的位置信息
 * 参考 MC 1.16.5 GlobalPos
 */
class GlobalPos {
public:
    GlobalPos()
        : m_dimensionId(0)
        , m_pos()
    {}

    GlobalPos(DimensionId dimensionId, const BlockPos& pos)
        : m_dimensionId(dimensionId)
        , m_pos(pos)
    {}

    GlobalPos(DimensionId dimensionId, BlockCoord x, BlockCoord y, BlockCoord z)
        : m_dimensionId(dimensionId)
        , m_pos(x, y, z)
    {}

    [[nodiscard]] DimensionId getDimensionId() const { return m_dimensionId; }
    [[nodiscard]] const BlockPos& getPos() const { return m_pos; }
    [[nodiscard]] BlockCoord x() const { return m_pos.x; }
    [[nodiscard]] BlockCoord y() const { return m_pos.y; }
    [[nodiscard]] BlockCoord z() const { return m_pos.z; }

    bool operator==(const GlobalPos& other) const
    {
        return m_dimensionId == other.m_dimensionId && m_pos == other.m_pos;
    }

    bool operator!=(const GlobalPos& other) const { return !(*this == other); }

    /**
     * @brief 检查是否在同一维度
     */
    [[nodiscard]] bool sameDimension(const GlobalPos& other) const { return m_dimensionId == other.m_dimensionId; }

private:
    DimensionId m_dimensionId;
    BlockPos m_pos;
};

} // namespace mc

// 哈希函数特化
namespace std {
template <>
struct hash<mc::GlobalPos> {
    size_t operator()(const mc::GlobalPos& pos) const
    {
        size_t h1 = std::hash<int>{}(static_cast<int>(pos.getDimensionId()));
        size_t h2 = std::hash<int>{}(static_cast<int>(pos.x())) ^ (std::hash<int>{}(static_cast<int>(pos.y())) << 1) ^
            (std::hash<int>{}(static_cast<int>(pos.z())) << 2);
        return h1 ^ (h2 << 1);
    }
};
} // namespace std
