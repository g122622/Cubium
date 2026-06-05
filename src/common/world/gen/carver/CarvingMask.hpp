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
#include <vector>

namespace mc {

/**
 * @brief 雕刻掩码
 *
 * 用于追踪哪些位置已被雕刻，防止重复雕刻。
 */
class CarvingMask {
public:
    /**
     * @brief 构造雕刻掩码
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     */
    CarvingMask(ChunkCoord chunkX, ChunkCoord chunkZ);

    /**
     * @brief 检查指定位置是否已被雕刻
     * @param x 区块内X坐标 (0-15)
     * @param y Y坐标
     * @param z 区块内Z坐标 (0-15)
     * @return 是否已被雕刻
     */
    [[nodiscard]] bool isCarved(BlockCoord x, i32 y, BlockCoord z) const;

    /**
     * @brief 标记指定位置为已雕刻
     * @param x 区块内X坐标 (0-15)
     * @param y Y坐标
     * @param z 区块内Z坐标 (0-15)
     */
    void setCarved(BlockCoord x, i32 y, BlockCoord z);

    /**
     * @brief 获取掩码索引
     * @param x 区块内X坐标 (0-15)
     * @param y Y坐标
     * @param z 区块内Z坐标 (0-15)
     * @return 位索引
     */
    [[nodiscard]] static constexpr i32 getIndex(BlockCoord x, i32 y, BlockCoord z)
    {
        // y 需要转换为相对于 MIN_BUILD_HEIGHT 的偏移量
        const i32 relativeY = y - world::MIN_BUILD_HEIGHT;
        return static_cast<i32>(x) | (static_cast<i32>(z) << world::CHUNK_SHIFT) |
            (relativeY << (world::CHUNK_SHIFT + world::SECTION_SHIFT));
    }

private:
    ChunkCoord m_chunkX;
    ChunkCoord m_chunkZ;
    std::vector<bool> m_mask; // 使用 vector<bool> 作为 BitSet
};

} // namespace mc
