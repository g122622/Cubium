#pragma once

#include "../../../core/Types.hpp"
#include <algorithm>

namespace mc::world::gen::structure {

/**
 * @brief 结构边界框
 *
 * 用于定义结构片段的世界坐标边界。
 */
class StructureBoundingBox {
public:
    StructureBoundingBox()
        : m_minX(0), m_minY(0), m_minZ(0)
        , m_maxX(0), m_maxY(0), m_maxZ(0)
        , m_valid(false) {}

    StructureBoundingBox(i32 x1, i32 y1, i32 z1, i32 x2, i32 y2, i32 z2)
        : m_minX(std::min(x1, x2))
        , m_minY(std::min(y1, y2))
        , m_minZ(std::min(z1, z2))
        , m_maxX(std::max(x1, x2))
        , m_maxY(std::max(y1, y2))
        , m_maxZ(std::max(z1, z2))
        , m_valid(true) {}

    static StructureBoundingBox fromChunk(i32 chunkX, i32 chunkZ) {
        return StructureBoundingBox(
            chunkX << 4, 0, chunkZ << 4,
            (chunkX << 4) + 15, 255, (chunkZ << 4) + 15
        );
    }

    [[nodiscard]] i32 minX() const { return m_minX; }
    [[nodiscard]] i32 minY() const { return m_minY; }
    [[nodiscard]] i32 minZ() const { return m_minZ; }
    [[nodiscard]] i32 maxX() const { return m_maxX; }
    [[nodiscard]] i32 maxY() const { return m_maxY; }
    [[nodiscard]] i32 maxZ() const { return m_maxZ; }

    [[nodiscard]] i32 xSpan() const { return m_maxX - m_minX + 1; }
    [[nodiscard]] i32 ySpan() const { return m_maxY - m_minY + 1; }
    [[nodiscard]] i32 zSpan() const { return m_maxZ - m_minZ + 1; }

    /**
     * @brief 获取中心 X 坐标
     */
    [[nodiscard]] i32 centerX() const { return (m_minX + m_maxX) / 2; }

    /**
     * @brief 获取中心 Y 坐标
     */
    [[nodiscard]] i32 centerY() const { return (m_minY + m_maxY) / 2; }

    /**
     * @brief 获取中心 Z 坐标
     */
    [[nodiscard]] i32 centerZ() const { return (m_minZ + m_maxZ) / 2; }

    [[nodiscard]] bool isValid() const { return m_valid; }

    [[nodiscard]] bool contains(i32 x, i32 y, i32 z) const {
        return x >= m_minX && x <= m_maxX &&
               y >= m_minY && y <= m_maxY &&
               z >= m_minZ && z <= m_maxZ;
    }

    /**
     * @brief 检查点是否在边界框内（BlockPos版本）
     *
     * 参考 MC 1.16.5 MutableBoundingBox.isVecInside
     */
    [[nodiscard]] bool isVecInside(i32 x, i32 y, i32 z) const {
        return contains(x, y, z);
    }

    /**
     * @brief 检查是否与另一个边界框相交
     *
     * 参考 MC 1.16.5 MutableBoundingBox.intersectsWith
     */
    [[nodiscard]] bool intersectsWith(const StructureBoundingBox& other) const {
        return m_maxX >= other.m_minX && m_minX <= other.m_maxX &&
               m_maxY >= other.m_minY && m_minY <= other.m_maxY &&
               m_maxZ >= other.m_minZ && m_minZ <= other.m_maxZ;
    }

    [[nodiscard]] bool intersectsChunk(i32 chunkX, i32 chunkZ) const {
        i32 chunkMinX = chunkX << 4;
        i32 chunkMinZ = chunkZ << 4;
        i32 chunkMaxX = chunkMinX + 15;
        i32 chunkMaxZ = chunkMinZ + 15;

        return m_maxX >= chunkMinX && m_minX <= chunkMaxX &&
               m_maxZ >= chunkMinZ && m_minZ <= chunkMaxZ;
    }

    void expandToInclude(i32 x, i32 y, i32 z) {
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
     *
     * 参考 MC 1.16.5 MutableBoundingBox.expandTo
     */
    void expandTo(const StructureBoundingBox& other) {
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
     *
     * 参考 MC 1.16.5 MutableBoundingBox.offset
     */
    void offset(i32 dx, i32 dy, i32 dz) {
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
    [[nodiscard]] StructureBoundingBox offseted(i32 dx, i32 dy, i32 dz) const {
        return StructureBoundingBox(
            m_minX + dx, m_minY + dy, m_minZ + dz,
            m_maxX + dx, m_maxY + dy, m_maxZ + dz
        );
    }

private:
    i32 m_minX, m_minY, m_minZ;
    i32 m_maxX, m_maxY, m_maxZ;
    bool m_valid;
};

} // namespace mc::world::gen::structure
