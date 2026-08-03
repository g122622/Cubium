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

#include "../../../core/Types.hpp"
#include "../../block/BlockState.hpp"
#include <cstddef>
#include <vector>

namespace mc {

/**
 * @brief 噪声列 — MC 1.21 NoiseColumn
 *
 * 表示一条垂直列的方块状态，从 minY 到 minY + height - 1。
 * 用于 getBaseColumn() 方法返回指定 X/Z 位置的完整地形列。
 *
 * MC 1.21: NoiseColumn(int minY, BlockState[] column)
 *   - minY: 列的最低 Y 坐标
 *   - column: 方块状态数组，索引 0 对应 minY
 */
class NoiseColumn {
public:
    NoiseColumn() = default;

    /**
     * @brief 构造指定大小的空列（所有位置为 nullptr/空气）
     * @param minY 最低 Y 坐标
     * @param height 列高度
     */
    NoiseColumn(i32 minY, i32 height)
        : m_minY(minY)
        , m_column(static_cast<size_t>(height), nullptr)
    {}

    /** 获取最低 Y 坐标 */
    [[nodiscard]] i32 minY() const { return m_minY; }

    /** 获取列高度 */
    [[nodiscard]] i32 height() const { return static_cast<i32>(m_column.size()); }

    /** 获取指定 Y 坐标的方块状态，越界返回 nullptr（空气） */
    [[nodiscard]] const BlockState* getBlock(i32 y) const
    {
        const i32 idx = y - m_minY;
        if (idx < 0 || idx >= static_cast<i32>(m_column.size())) {
            return nullptr;
        }
        return m_column[static_cast<size_t>(idx)];
    }

    /** 设置指定 Y 坐标的方块状态 */
    void setBlock(i32 y, const BlockState* state)
    {
        const i32 idx = y - m_minY;
        if (idx >= 0 && idx < static_cast<i32>(m_column.size())) {
            m_column[static_cast<size_t>(idx)] = state;
        }
    }

private:
    i32 m_minY = 0;
    std::vector<const BlockState*> m_column;
};

} // namespace mc
