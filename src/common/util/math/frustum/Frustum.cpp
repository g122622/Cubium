#include "Frustum.hpp"
#include "common/core/Constants.hpp"
#include <cmath>

namespace mc::math::frustum {

// 引用世界常量
using mc::world::CHUNK_WIDTH;
using mc::world::CHUNK_SECTION_HEIGHT;

void Frustum::extractFromMatrix(const glm::mat4& m) {
    // Gribb-Hartmann 方法：从组合的 view-projection 矩阵提取视锥平面
    // 矩阵 m 是列主序的，所以 m[i][j] 表示第 j 列第 i 行
    //
    // 平面提取公式：
    // Left   = m[3] + m[0]  (第 3 列 + 第 0 列)
    // Right  = m[3] - m[0]  (第 3 列 - 第 0 列)
    // Bottom = m[3] + m[1]  (第 3 列 + 第 1 列)
    // Top    = m[3] - m[1]  (第 3 列 - 第 1 列)
    // Near   = m[3] + m[2]  (第 3 列 + 第 2 列)
    // Far    = m[3] - m[2]  (第 3 列 - 第 2 列)

    // 左平面
    m_planes[Left].normal.x = m[0][3] + m[0][0];
    m_planes[Left].normal.y = m[1][3] + m[1][0];
    m_planes[Left].normal.z = m[2][3] + m[2][0];
    m_planes[Left].distance = m[3][3] + m[3][0];

    // 右平面
    m_planes[Right].normal.x = m[0][3] - m[0][0];
    m_planes[Right].normal.y = m[1][3] - m[1][0];
    m_planes[Right].normal.z = m[2][3] - m[2][0];
    m_planes[Right].distance = m[3][3] - m[3][0];

    // 底平面
    m_planes[Bottom].normal.x = m[0][3] + m[0][1];
    m_planes[Bottom].normal.y = m[1][3] + m[1][1];
    m_planes[Bottom].normal.z = m[2][3] + m[2][1];
    m_planes[Bottom].distance = m[3][3] + m[3][1];

    // 顶平面
    m_planes[Top].normal.x = m[0][3] - m[0][1];
    m_planes[Top].normal.y = m[1][3] - m[1][1];
    m_planes[Top].normal.z = m[2][3] - m[2][1];
    m_planes[Top].distance = m[3][3] - m[3][1];

    // 近平面
    m_planes[Near].normal.x = m[0][3] + m[0][2];
    m_planes[Near].normal.y = m[1][3] + m[1][2];
    m_planes[Near].normal.z = m[2][3] + m[2][2];
    m_planes[Near].distance = m[3][3] + m[3][2];

    // 远平面
    m_planes[Far].normal.x = m[0][3] - m[0][2];
    m_planes[Far].normal.y = m[1][3] - m[1][2];
    m_planes[Far].normal.z = m[2][3] - m[2][2];
    m_planes[Far].distance = m[3][3] - m[3][2];

    // 归一化所有平面，使距离ToPoint() 返回真正的距离
    for (auto& plane : m_planes) {
        plane.normalize();
    }

    m_valid = true;
}

void Frustum::extractFromMatrices(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix) {
    const glm::mat4 viewProjection = projectionMatrix * viewMatrix;
    extractFromMatrix(viewProjection);
}

bool Frustum::isPointVisible(const Vector3& point) const noexcept {
    // 点在视锥内当且仅当它在所有 6 个平面的内侧
    for (const auto& plane : m_planes) {
        if (plane.distanceToPoint(point) < 0.0f) {
            return false;
        }
    }
    return true;
}

bool Frustum::isPointVisible(const glm::vec3& point) const noexcept {
    for (const auto& plane : m_planes) {
        if (plane.distanceToPoint(point) < 0.0f) {
            return false;
        }
    }
    return true;
}

bool Frustum::isSphereVisible(const Vector3& center, f32 radius) const noexcept {
    // 球与视锥相交当且仅当它不完全在任何平面的外侧
    for (const auto& plane : m_planes) {
        const f32 distance = plane.distanceToPoint(center);
        // 如果球心到平面的距离小于负半径，球完全在平面外侧
        if (distance < -radius) {
            return false;
        }
    }
    return true;
}

bool Frustum::isSphereVisible(const glm::vec3& center, f32 radius) const noexcept {
    for (const auto& plane : m_planes) {
        const f32 distance = plane.distanceToPoint(center);
        if (distance < -radius) {
            return false;
        }
    }
    return true;
}

bool Frustum::isAABBVisible(const AxisAlignedBB& aabb) const noexcept {
    // p-vertex 优化：对每个平面，找到 AABB 上离平面法向量方向最远的顶点
    // 如果 p-vertex 在平面外侧，则整个 AABB 在平面外侧
    //
    // p-vertex 的选择规则：
    // 如果法向量的某个分量为正，选择 AABB 的 max 分量
    // 如果法向量的某个分量为负，选择 AABB 的 min 分量

    for (const auto& plane : m_planes) {
        // 计算 p-vertex
        const Vector3 pVertex(
            plane.normal.x > 0.0f ? aabb.maxX : aabb.minX,
            plane.normal.y > 0.0f ? aabb.maxY : aabb.minY,
            plane.normal.z > 0.0f ? aabb.maxZ : aabb.minZ
        );

        // 如果 p-vertex 在平面外侧，整个 AABB 在平面外侧
        if (plane.distanceToPoint(pVertex) < 0.0f) {
            return false;
        }
    }

    return true;
}

bool Frustum::isAABBVisibleWorld(const AxisAlignedBB& aabb) const noexcept {
    // 将世界坐标 AABB 转换为相机相对坐标 AABB
    // 这样可以提高大坐标下的浮点精度
    const AxisAlignedBB relativeAABB(
        aabb.minX - m_cameraPosition.x,
        aabb.minY - m_cameraPosition.y,
        aabb.minZ - m_cameraPosition.z,
        aabb.maxX - m_cameraPosition.x,
        aabb.maxY - m_cameraPosition.y,
        aabb.maxZ - m_cameraPosition.z
    );

    return isAABBVisible(relativeAABB);
}

bool Frustum::isChunkVisible(i32 chunkX, i32 chunkZ, i32 minY, i32 maxY) const noexcept {
    // 创建区块 AABB（世界坐标）
    const f32 worldX = static_cast<f32>(chunkX * CHUNK_WIDTH);
    const f32 worldZ = static_cast<f32>(chunkZ * CHUNK_WIDTH);

    const AxisAlignedBB aabb(
        worldX,
        static_cast<f32>(minY),
        worldZ,
        worldX + static_cast<f32>(CHUNK_WIDTH),
        static_cast<f32>(maxY),
        worldZ + static_cast<f32>(CHUNK_WIDTH)
    );

    return isAABBVisibleWorld(aabb);
}

bool Frustum::isChunkSectionVisible(i32 chunkX, i32 sectionY, i32 chunkZ) const noexcept {
    // 创建区块段 AABB（世界坐标）
    const f32 worldX = static_cast<f32>(chunkX * CHUNK_WIDTH);
    const f32 worldY = static_cast<f32>(sectionY * CHUNK_SECTION_HEIGHT);
    const f32 worldZ = static_cast<f32>(chunkZ * CHUNK_WIDTH);

    const AxisAlignedBB aabb(
        worldX,
        worldY,
        worldZ,
        worldX + static_cast<f32>(CHUNK_WIDTH),
        worldY + static_cast<f32>(CHUNK_SECTION_HEIGHT),
        worldZ + static_cast<f32>(CHUNK_WIDTH)
    );

    return isAABBVisibleWorld(aabb);
}

// ========== 工具函数实现 ==========

namespace FrustumUtils {

AxisAlignedBB createChunkAABB(i32 chunkX, i32 chunkZ, i32 minY, i32 maxY) noexcept {
    const f32 worldX = static_cast<f32>(chunkX * CHUNK_WIDTH);
    const f32 worldZ = static_cast<f32>(chunkZ * CHUNK_WIDTH);

    return AxisAlignedBB(
        worldX,
        static_cast<f32>(minY),
        worldZ,
        worldX + static_cast<f32>(CHUNK_WIDTH),
        static_cast<f32>(maxY),
        worldZ + static_cast<f32>(CHUNK_WIDTH)
    );
}

AxisAlignedBB createSectionAABB(i32 chunkX, i32 sectionY, i32 chunkZ, i32 sectionHeight) noexcept {
    const f32 worldX = static_cast<f32>(chunkX * CHUNK_WIDTH);
    const f32 worldY = static_cast<f32>(sectionY * sectionHeight);
    const f32 worldZ = static_cast<f32>(chunkZ * CHUNK_WIDTH);

    return AxisAlignedBB(
        worldX,
        worldY,
        worldZ,
        worldX + static_cast<f32>(CHUNK_WIDTH),
        worldY + static_cast<f32>(sectionHeight),
        worldZ + static_cast<f32>(CHUNK_WIDTH)
    );
}

AxisAlignedBB createEntityAABB(const Vector3& position, f32 width, f32 height) noexcept {
    const f32 halfWidth = width * 0.5f;
    return AxisAlignedBB(
        position.x - halfWidth,
        position.y,
        position.z - halfWidth,
        position.x + halfWidth,
        position.y + height,
        position.z + halfWidth
    );
}

AxisAlignedBB createBlockAABB(i32 x, i32 y, i32 z) noexcept {
    return AxisAlignedBB(
        static_cast<f32>(x),
        static_cast<f32>(y),
        static_cast<f32>(z),
        static_cast<f32>(x + 1),
        static_cast<f32>(y + 1),
        static_cast<f32>(z + 1)
    );
}

AxisAlignedBB expandAABB(const AxisAlignedBB& aabb, f32 margin) noexcept {
    return AxisAlignedBB(
        aabb.minX - margin,
        aabb.minY - margin,
        aabb.minZ - margin,
        aabb.maxX + margin,
        aabb.maxY + margin,
        aabb.maxZ + margin
    );
}

}  // namespace FrustumUtils

}  // namespace mc::math::frustum
