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
#include "../../../util/AxisAlignedBB.hpp"
#include "../../../util/math/Vector3.hpp"
#include <array>
#include <cstddef>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace mc::math::frustum {

/**
 * @brief 视锥平面
 *
 * 存储平面方程 Ax + By + Cz + D = 0 的系数。
 * 平面法向量 (A, B, C) 指向视锥内部。
 *
 * 平面方程为：normal.x * x + normal.y * y + normal.z * z + distance = 0
 * 法向量归一化后，distanceToPoint() 返回点到平面的真实距离。
 */
struct FrustumPlane {
    Vector3f normal{0.0f, 0.0f, 0.0f}; ///< 平面法向量（归一化，指向视锥内部）
    f32 distance = 0.0f;               ///< 平面到原点的距离（D 系数）

    /**
     * @brief 计算点到平面的有符号距离
     *
     * @param point 点坐标
     * @return 正值表示点在平面内侧（视锥内），负值表示在外侧
     */
    [[nodiscard]] f32 distanceToPoint(const Vector3f& point) const noexcept { return normal.dot(point) + distance; }

    /**
     * @brief 计算点到平面的有符号距离（glm 版本）
     *
     * @param point 点坐标
     * @return 正值表示点在平面内侧（视锥内），负值表示在外侧
     */
    [[nodiscard]] f32 distanceToPoint(const glm::vec3& point) const noexcept
    {
        return normal.x * point.x + normal.y * point.y + normal.z * point.z + distance;
    }

    /**
     * @brief 归一化平面方程
     *
     * 确保法向量长度为 1，这样 distanceToPoint() 返回真正的距离。
     * 归一化后平面方程性质不变，但距离值具有物理意义。
     */
    void normalize() noexcept
    {
        const f32 length = normal.length();
        if (length > 0.0001f) {
            const f32 invLength = 1.0f / length;
            normal.x *= invLength;
            normal.y *= invLength;
            normal.z *= invLength;
            distance *= invLength;
        }
    }
};

/**
 * @brief 视锥体类
 *
 * 从视图-投影矩阵提取 6 个视锥平面，提供点、球、AABB 的可见性测试。
 * 采用 Gribb-Hartmann 方法从 VP 矩阵提取平面，使用 p-vertex 优化进行 AABB 测试。
 *
 * 使用方法：
 * @code
 * Frustum frustum;
 * frustum.extractFromMatrix(viewProjectionMatrix);
 * frustum.setCameraPosition(cameraPos);
 *
 * if (frustum.isAABBVisibleWorld(aabb)) {
 *     // 渲染物体
 * }
 * @endcode
 *
 * 注意：
 * - 必须在每帧更新视锥（相机移动后）
 * - isAABBVisible() 使用相机相对坐标以提高精度
 * - isAABBVisibleWorld() 自动转换为相机相对坐标
 * - 测试是保守的：可能包含一些实际不可见的物体，但不会遗漏可见物体
 *
 * 性能提示：
 * - extractFromMatrix() 每帧调用一次
 * - isSphereVisible() 比 isAABBVisible() 更快，适合小物体
 * - 区块级别测试使用 isChunkVisible() 或 isChunkSectionVisible()
 */
class Frustum {
public:
    /// 视锥平面索引
    enum PlaneIndex : u8 {
        Left = 0,   ///< 左平面
        Right = 1,  ///< 右平面
        Bottom = 2, ///< 底平面
        Top = 3,    ///< 顶平面
        Near = 4,   ///< 近裁剪面
        Far = 5     ///< 远裁剪面
    };

    /// 视锥平面数量
    static constexpr size_t PLANE_COUNT = 6;

    // ========== 平面提取 ==========

    /**
     * @brief 从视图-投影矩阵提取视锥平面
     *
     * 使用 Gribb-Hartmann 方法从组合的 view-projection 矩阵提取 6 个视锥平面。
     * 提取后平面法向量指向视锥内部，便于可见性测试。
     *
     * @param viewProjectionMatrix 组合的视图-投影矩阵（projection * view）
     *
     * 注意：
     * - 矩阵应为 projection * view 的结果
     * - 提取后平面会自动归一化
     * - 应在每帧相机移动后调用
     */
    void extractFromMatrix(const glm::mat4& viewProjectionMatrix);

    /**
     * @brief 从视图和投影矩阵分别提取视锥平面
     *
     * @param projectionMatrix 投影矩阵
     * @param viewMatrix 视图矩阵
     */
    void extractFromMatrices(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix);

    // ========== 相机位置设置 ==========

    /**
     * @brief 设置相机位置（用于相机相对坐标测试）
     *
     * 设置相机位置后，isAABBVisibleWorld() 会自动将世界坐标转换为相机相对坐标，
     * 这可以提高大坐标下的浮点精度。
     *
     * @param position 相机世界位置
     */
    void setCameraPosition(const Vector3f& position) noexcept { m_cameraPosition = position; }

    /**
     * @brief 设置相机位置（glm 版本）
     * @param position 相机世界位置
     */
    void setCameraPosition(const glm::vec3& position) noexcept
    {
        m_cameraPosition = Vector3f(position.x, position.y, position.z);
    }

    // ========== 可见性测试 ==========

    /**
     * @brief 测试点是否在视锥内
     *
     * @param point 点坐标（世界坐标）
     * @return true 如果点在视锥内或边界上
     */
    [[nodiscard]] bool isPointVisible(const Vector3f& point) const noexcept;

    /**
     * @brief 测试点是否在视锥内（glm 版本）
     * @param point 点坐标（世界坐标）
     * @return true 如果点在视锥内或边界上
     */
    [[nodiscard]] bool isPointVisible(const glm::vec3& point) const noexcept;

    /**
     * @brief 测试球是否与视锥相交
     *
     * 使用球心到平面的距离与半径比较进行快速测试。
     * 适用于粒子、小物体等球形包围体的可见性检测。
     *
     * @param center 球心坐标（世界坐标）
     * @param radius 球半径
     * @return true 如果球与视锥相交或在视锥内
     */
    [[nodiscard]] bool isSphereVisible(const Vector3f& center, f32 radius) const noexcept;

    /**
     * @brief 测试球是否与视锥相交（glm 版本）
     * @param center 球心坐标（世界坐标）
     * @param radius 球半径
     * @return true 如果球与视锥相交或在视锥内
     */
    [[nodiscard]] bool isSphereVisible(const glm::vec3& center, f32 radius) const noexcept;

    /**
     * @brief 测试 AABB 是否与视锥相交（相机相对坐标）
     *
     * 使用 p-vertex 优化算法：对每个平面，找到 AABB 上离平面法向量方向最远的顶点，
     * 如果该顶点在平面外侧，则整个 AABB 在平面外侧。
     *
     * 这是一个保守测试：可能返回 false positive（报告可见但实际不可见），
     * 但不会返回 false negative（报告不可见但实际可见）。
     *
     * @param aabb AABB（相机相对坐标）
     * @return true 如果 AABB 与视锥相交或在视锥内
     *
     * 注意：此方法期望 AABB 使用相机相对坐标，调用者需要先转换：
     * @code
     * AxisAlignedBB relativeAABB(
     *     worldAABB.minX - cameraPos.x, ...
     * );
     * if (frustum.isAABBVisible(relativeAABB)) { ... }
     * @endcode
     */
    [[nodiscard]] bool isAABBVisible(const AxisAlignedBB& aabb) const noexcept;

    /**
     * @brief 测试 AABB 是否与视锥相交（世界坐标）
     *
     * 自动将世界坐标 AABB 转换为相机相对坐标后进行测试。
     * 需要先调用 setCameraPosition() 设置相机位置。
     *
     * @param aabb AABB（世界坐标）
     * @return true 如果 AABB 与视锥相交或在视锥内
     */
    [[nodiscard]] bool isAABBVisibleWorld(const AxisAlignedBB& aabb) const noexcept;

    /**
     * @brief 测试区块是否可见
     *
     * 创建覆盖整个区块的 AABB 并测试可见性。
     * 区块 AABB 从 (x * 16, minY) 到 (x * 16 + 16, maxY)。
     *
     * @param chunkX 区块 X 坐标（世界区块坐标）
     * @param chunkZ 区块 Z 坐标（世界区块坐标）
     * @param minY 区块最小 Y 高度
     * @param maxY 区块最大 Y 高度
     * @return true 如果区块与视锥相交或在视锥内
     */
    [[nodiscard]] bool isChunkVisible(i32 chunkX, i32 chunkZ, i32 minY, i32 maxY) const noexcept;

    /**
     * @brief 测试区块段是否可见
     *
     * 创建覆盖整个区块段的 AABB 并测试可见性。
     * 区块段是 16x16x16 的立方体。
     *
     * @param chunkX 区块 X 坐标
     * @param sectionY 区块段索引（0..CHUNK_SECTIONS-1）
     * @param chunkZ 区块 Z 坐标
     * @return true 如果区块段与视锥相交或在视锥内
     */
    [[nodiscard]] bool isChunkSectionVisible(i32 chunkX, i32 sectionY, i32 chunkZ) const noexcept;

    // ========== 访问器 ==========

    /**
     * @brief 获取指定平面
     * @param index 平面索引
     * @return 平面引用
     */
    [[nodiscard]] const FrustumPlane& getPlane(PlaneIndex index) const noexcept
    {
        return m_planes[static_cast<size_t>(index)];
    }

    /**
     * @brief 获取所有平面
     * @return 平面数组
     */
    [[nodiscard]] const std::array<FrustumPlane, PLANE_COUNT>& getPlanes() const noexcept { return m_planes; }

    /**
     * @brief 获取相机位置
     * @return 相机世界位置
     */
    [[nodiscard]] const Vector3f& getCameraPosition() const noexcept { return m_cameraPosition; }

    /**
     * @brief 检查视锥是否已初始化
     * @return true 如果已从矩阵提取平面
     */
    [[nodiscard]] bool isValid() const noexcept { return m_valid; }

private:
    std::array<FrustumPlane, PLANE_COUNT> m_planes;
    Vector3f m_cameraPosition{0.0f, 0.0f, 0.0f};
    bool m_valid = false;
};

// ========== 工具函数 ==========

/**
 * @brief AABB-视锥相交测试工具函数
 */
namespace FrustumUtils {

/**
 * @brief 为区块创建 AABB
 *
 * @param chunkX 区块 X 坐标
 * @param chunkZ 区块 Z 坐标
 * @param minY 最小 Y 高度
 * @param maxY 最大 Y 高度
 * @return 覆盖整个区块的 AABB
 */
[[nodiscard]] AxisAlignedBB createChunkAABB(i32 chunkX, i32 chunkZ, i32 minY, i32 maxY) noexcept;

/**
 * @brief 为区块段创建 AABB
 *
 * @param chunkX 区块 X 坐标
 * @param sectionY 区块段 Y 索引（每个段高 16 格）
 * @param chunkZ 区块 Z 坐标
 * @param sectionHeight 区块段高度（默认 16）
 * @return 覆盖整个区块段的 AABB
 */
[[nodiscard]] AxisAlignedBB createSectionAABB(i32 chunkX, i32 sectionY, i32 chunkZ, i32 sectionHeight = 16) noexcept;

/**
 * @brief 为实体创建 AABB
 *
 * @param position 实体位置（脚底）
 * @param width 实体宽度
 * @param height 实体高度
 * @return 实体 AABB
 */
[[nodiscard]] AxisAlignedBB createEntityAABB(const Vector3f& position, f32 width, f32 height) noexcept;

/**
 * @brief 为方块创建 AABB
 *
 * @param x 方块 X 坐标
 * @param y 方块 Y 坐标
 * @param z 方块 Z 坐标
 * @return 覆盖整个方块的 AABB
 */
[[nodiscard]] AxisAlignedBB createBlockAABB(i32 x, i32 y, i32 z) noexcept;

/**
 * @brief 扩展 AABB 以包含扩展距离
 *
 * 某些情况下需要稍微扩展 AABB 以确保边界物体不被误剔除。
 *
 * @param aabb 原始 AABB
 * @param margin 扩展距离
 * @return 扩展后的 AABB
 */
[[nodiscard]] AxisAlignedBB expandAABB(const AxisAlignedBB& aabb, f32 margin) noexcept;

} // namespace FrustumUtils

} // namespace mc::math::frustum
