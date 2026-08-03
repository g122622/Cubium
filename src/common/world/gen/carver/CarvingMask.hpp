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
 * MC 1.21.11: 掩码大小基于 minY 和 height（生成范围），不硬编码世界常量。
 */
class CarvingMask {
public:
    /**
     * @brief 构造雕刻掩码
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @param minY 最小生成Y坐标（如 -64）
     * @param height 生成高度（如 384）
     */
    CarvingMask(
        ChunkCoord chunkX, ChunkCoord chunkZ, i32 minY = world::MIN_BUILD_HEIGHT, i32 height = world::CHUNK_HEIGHT);

    /** @brief 检查指定位置是否已被雕刻 */
    [[nodiscard]] bool isCarved(BlockCoord x, i32 y, BlockCoord z) const;

    /** @brief 标记指定位置为已雕刻 */
    void setCarved(BlockCoord x, i32 y, BlockCoord z);

    /** @brief 获取最小Y坐标 */
    [[nodiscard]] i32 getMinY() const { return m_minY; }

    /** @brief 获取高度 */
    [[nodiscard]] i32 getHeight() const { return m_height; }

private:
    ChunkCoord m_chunkX;
    ChunkCoord m_chunkZ;
    i32 m_minY;
    i32 m_height;
    std::vector<bool> m_mask;
};

} // namespace mc
