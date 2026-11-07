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

#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/world/WorldConstants.hpp"
#include <vector>

namespace mc {

/**
 * @brief 雕刻掩码
 *
 * 用于追踪哪些位置已被雕刻，防止重复雕刻。
 *
 * 几何参数（minY / height）必须来自**该区块所在维度**的生成设置，而不是世界常量：
 * 主世界是 (-64, 384)、下界是 (0, 128)、末地是 (0, 256)。传错会让 `y - minY` 的位移
 * 基准整体偏移，索引落到别的格上——洞穴在区块内被刻穿或漏刻，不会崩溃也不会报错。
 */
class CarvingMask {
public:
    /**
     * @brief 构造雕刻掩码
     * @param minY 该维度最低生成 Y 坐标（如主世界 -64、下界 0）
     * @param height 该维度生成高度（如主世界 384、下界 128）
     */
    CarvingMask(i32 minY, i32 height);

    /** @brief 检查指定位置是否已被雕刻 */
    [[nodiscard]] bool isCarved(BlockCoord x, i32 y, BlockCoord z) const;

    /** @brief 标记指定位置为已雕刻 */
    void setCarved(BlockCoord x, i32 y, BlockCoord z);

    /** @brief 获取最小Y坐标 */
    [[nodiscard]] i32 getMinY() const
    {
        return m_minY;
    }

    /** @brief 获取高度 */
    [[nodiscard]] i32 getHeight() const
    {
        return m_height;
    }

private:
    /**
     * @brief 把区块内局部坐标换算成掩码下标
     *
     * 公式是 `x & 15 | (z & 15) << 4 | (y - minY) << 8`：X、Z 按区块宽度掩码，
     * Y 用相对 minY 的偏移参与位移。调用方（WorldCarver）已把三个坐标夹进合法范围，
     * 这里不再重复校验。
     */
    [[nodiscard]] i32 _index(BlockCoord x, i32 y, BlockCoord z) const;

    i32 m_minY;
    i32 m_height;
    std::vector<bool> m_mask;
};

} // namespace mc
