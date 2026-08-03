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

#include "../../../core/Constants.hpp"
#include "../../../core/Types.hpp"
#include "../../../util/Direction.hpp"
#include "common/world/WorldConstants.hpp"
#include <algorithm>

namespace mc::world::gen::structure {

/**
 * @brief 结构边界框
 *
 * 用于定义结构片段的世界坐标边界。
 */
class StructureBoundingBox {
public:
    StructureBoundingBox() noexcept
        : m_minX(0)
        , m_minY(0)
        , m_minZ(0)
        , m_maxX(0)
        , m_maxY(0)
        , m_maxZ(0)
        , m_valid(false)
    {}

    StructureBoundingBox(i32 x1, i32 y1, i32 z1, i32 x2, i32 y2, i32 z2) noexcept
        : m_minX(std::min(x1, x2))
        , m_minY(std::min(y1, y2))
        , m_minZ(std::min(z1, z2))
        , m_maxX(std::max(x1, x2))
        , m_maxY(std::max(y1, y2))
        , m_maxZ(std::max(z1, z2))
        , m_valid(true)
    {}

    static StructureBoundingBox fromChunk(i32 chunkX, i32 chunkZ) noexcept;

    /**
     * @brief 创建带方向偏移的边界框
     *
     * @param x 基准 X 坐标
     * @param y 基准 Y 坐标
     * @param z 基准 Z 坐标
     * @param offsetX X 偏移（相对于方向）
     * @param offsetY Y 偏移
     * @param offsetZ Z 偏移（相对于方向）
     * @param sizeX X 方向大小
     * @param sizeY Y 方向大小
     * @param sizeZ Z 方向大小
     * @param direction 方向（Direction::North/South/East/West，或使用整数 0-3）
     */
    static StructureBoundingBox createBox(i32 x,
        i32 y,
        i32 z,
        i32 offsetX,
        i32 offsetY,
        i32 offsetZ,
        i32 sizeX,
        i32 sizeY,
        i32 sizeZ,
        Direction direction) noexcept;

    [[nodiscard]] i32 minX() const noexcept { return m_minX; }
    [[nodiscard]] i32 minY() const noexcept { return m_minY; }
    [[nodiscard]] i32 minZ() const noexcept { return m_minZ; }
    [[nodiscard]] i32 maxX() const noexcept { return m_maxX; }
    [[nodiscard]] i32 maxY() const noexcept { return m_maxY; }
    [[nodiscard]] i32 maxZ() const noexcept { return m_maxZ; }

    [[nodiscard]] i32 xSpan() const noexcept { return m_maxX - m_minX + 1; }
    [[nodiscard]] i32 ySpan() const noexcept { return m_maxY - m_minY + 1; }
    [[nodiscard]] i32 zSpan() const noexcept { return m_maxZ - m_minZ + 1; }

    /**
     * @brief 获取中心 X 坐标
     */
    [[nodiscard]] i32 centerX() const noexcept { return (m_minX + m_maxX) / 2; }

    /**
     * @brief 获取中心 Y 坐标
     */
    [[nodiscard]] i32 centerY() const noexcept { return (m_minY + m_maxY) / 2; }

    /**
     * @brief 获取中心 Z 坐标
     */
    [[nodiscard]] i32 centerZ() const noexcept { return (m_minZ + m_maxZ) / 2; }

    [[nodiscard]] bool isValid() const noexcept { return m_valid; }

    [[nodiscard]] bool contains(i32 x, i32 y, i32 z) const noexcept
    {
        return x >= m_minX && x <= m_maxX && y >= m_minY && y <= m_maxY && z >= m_minZ && z <= m_maxZ;
    }

    /**
     * @brief 检查点是否在边界框内（BlockPos版本）
     */
    [[nodiscard]] bool isVecInside(i32 x, i32 y, i32 z) const noexcept { return contains(x, y, z); }

    /**
     * @brief 检查是否与另一个边界框相交
     */
    [[nodiscard]] bool intersectsWith(const StructureBoundingBox& other) const noexcept
    {
        return m_maxX >= other.m_minX && m_minX <= other.m_maxX && m_maxY >= other.m_minY && m_minY <= other.m_maxY &&
            m_maxZ >= other.m_minZ && m_minZ <= other.m_maxZ;
    }

    /**
     * @brief 检查是否与另一个边界框相交（别名）
     */
    [[nodiscard]] bool intersects(const StructureBoundingBox& other) const noexcept { return intersectsWith(other); }

    /**
     * @brief 检查点是否在边界框内（别名）
     */
    [[nodiscard]] bool isInside(i32 x, i32 y, i32 z) const noexcept { return contains(x, y, z); }

    [[nodiscard]] bool intersectsChunk(i32 chunkX, i32 chunkZ) const noexcept
    {
        const i32 chunkMinX = chunkX << world::CHUNK_SHIFT;
        const i32 chunkMinZ = chunkZ << world::CHUNK_SHIFT;
        const i32 chunkMaxX = chunkMinX + world::CHUNK_WIDTH - 1;
        const i32 chunkMaxZ = chunkMinZ + world::CHUNK_WIDTH - 1;

        return m_maxX >= chunkMinX && m_minX <= chunkMaxX && m_maxZ >= chunkMinZ && m_minZ <= chunkMaxZ;
    }

    void expandToInclude(i32 x, i32 y, i32 z) noexcept
    {
        if (!m_valid) {
            m_minX = m_maxX = x;
            m_minY = m_maxY = y;
            m_minZ = m_maxZ = z;
            m_valid = true;
            return;
        }

        m_minX = std::min(m_minX, x);
        m_minY = std::min(m_minY, y);
        m_minZ = std::min(m_minZ, z);
        m_maxX = std::max(m_maxX, x);
        m_maxY = std::max(m_maxY, y);
        m_maxZ = std::max(m_maxZ, z);
    }

    /**
     * @brief 扩展边界框以包含另一个边界框
     */
    void expandTo(const StructureBoundingBox& other) noexcept
    {
        if (!other.m_valid) {
            return;
        }

        if (!m_valid) {
            *this = other;
            return;
        }

        m_minX = std::min(m_minX, other.m_minX);
        m_minY = std::min(m_minY, other.m_minY);
        m_minZ = std::min(m_minZ, other.m_minZ);
        m_maxX = std::max(m_maxX, other.m_maxX);
        m_maxY = std::max(m_maxY, other.m_maxY);
        m_maxZ = std::max(m_maxZ, other.m_maxZ);
    }

    /**
     * @brief 偏移边界框
     */
    void offset(i32 dx, i32 dy, i32 dz) noexcept
    {
        m_minX += dx;
        m_minY += dy;
        m_minZ += dz;
        m_maxX += dx;
        m_maxY += dy;
        m_maxZ += dz;
    }

    /**
     * @brief 创建偏移后的边界框副本
     */
    [[nodiscard]] StructureBoundingBox offseted(i32 dx, i32 dy, i32 dz) const noexcept
    {
        return StructureBoundingBox(m_minX + dx, m_minY + dy, m_minZ + dz, m_maxX + dx, m_maxY + dy, m_maxZ + dz);
    }

private:
    i32 m_minX, m_minY, m_minZ;
    i32 m_maxX, m_maxY, m_maxZ;
    bool m_valid;
};

} // namespace mc::world::gen::structure
