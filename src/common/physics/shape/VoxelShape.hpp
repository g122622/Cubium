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
#include "common/physics/shape/BooleanOp.hpp"
#include "common/physics/shape/DiscreteVoxelShape.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace mc {

// 前向声明
class Shapes;

/**
 * @brief 方块命中结果
 */
struct BlockHitResult {
    Vector3 location;    // 命中点（世界坐标）
    Direction direction; // 命中面方向
    BlockPos blockPos;   // 命中方块位置
    bool inside;         // 是否在方块内部

    BlockHitResult(Vector3 loc, Direction dir, BlockPos pos, bool in)
        : location(std::move(loc))
        , direction(dir)
        , blockPos(pos)
        , inside(in)
    {}
};

/**
 * @brief 体素形状
 *
 * 表示一个3D形状，由离散体素网格和坐标点列表组成。
 * 支持形状运算（并集、交集、差集）和碰撞检测。
 *
 * 关键概念：
 * - DiscreteVoxelShape: 离散的体素占用网格
 * - 坐标点列表: 每个轴上的实际坐标值
 * - 面形状缓存: 每个方向的面形状用于光照遮挡检测
 */
class VoxelShape {
public:
    /**
     * @brief 双精度线消费者
     */
    using DoubleLineConsumer = std::function<void(f64, f64, f64, f64, f64, f64)>;

    VoxelShape();
    explicit VoxelShape(std::shared_ptr<DiscreteVoxelShape> shape);
    VoxelShape(std::shared_ptr<DiscreteVoxelShape> shape,
        std::vector<f64> xPoints,
        std::vector<f64> yPoints,
        std::vector<f64> zPoints);

    virtual ~VoxelShape() = default;

    // 允许拷贝和移动
    VoxelShape(const VoxelShape& other);
    VoxelShape& operator=(const VoxelShape& other);
    VoxelShape(VoxelShape&& other) noexcept;
    VoxelShape& operator=(VoxelShape&& other) noexcept;

    // === 边界查询 ===

    /**
     * @brief 获取指定轴的最小值
     */
    [[nodiscard]] f64 min(Axis axis) const;

    /**
     * @brief 获取指定轴的最大值
     */
    [[nodiscard]] f64 max(Axis axis) const;

    /**
     * @brief 获取形状的AABB边界
     * @throws 如果形状为空
     */
    [[nodiscard]] AxisAlignedBB bounds() const;

    /**
     * @brief 获取包含所有体素的单个AABB
     */
    [[nodiscard]] VoxelShape singleEncompassing() const;

    /**
     * @brief 检查形状是否为空
     */
    [[nodiscard]] bool isEmpty() const;

    // === 坐标访问 ===

    /**
     * @brief 获取指定轴的坐标点列表
     */
    [[nodiscard]] virtual const std::vector<f64>& getCoords(Axis axis) const;

    /**
     * @brief 获取指定轴在指定索引处的坐标值
     */
    [[nodiscard]] f64 get(Axis axis, i32 index) const;

    // === 变换 ===

    /**
     * @brief 移动形状
     */
    [[nodiscard]] VoxelShape move(f64 dx, f64 dy, f64 dz) const;
    [[nodiscard]] VoxelShape move(const Vector3& delta) const;

    /**
     * @brief 优化形状（合并相邻体素）
     */
    [[nodiscard]] VoxelShape optimize() const;

    // === 遍历 ===

    /**
     * @brief 遍历所有边
     */
    void forAllEdges(const DoubleLineConsumer& consumer);

    /**
     * @brief 遍历所有盒子
     */
    void forAllBoxes(const DoubleLineConsumer& consumer) const;

    /**
     * @brief 转换为AABB列表
     */
    [[nodiscard]] std::vector<AxisAlignedBB> toAabbs() const;

    // === 面形状 ===

    /**
     * @brief 获取指定方向的面形状
     *
     * 用于光照遮挡检测。返回形状在指定方向上的投影。
     */
    [[nodiscard]] VoxelShape getFaceShape(Direction dir) const;

    /**
     * @brief 计算面形状
     */
private:
    [[nodiscard]] VoxelShape _calculateFace(Direction dir) const;

public:
    /**
     * @brief 检查是否类似立方体（沿指定轴）
     */
    [[nodiscard]] bool isCubeLikeAlong(Axis axis) const;

    /**
     * @brief 检查是否类似立方体
     */
    [[nodiscard]] bool isCubeLike() const;

    // === 碰撞检测 ===

    /**
     * @brief 计算碰撞偏移
     * @param axis 移动轴
     * @param entityBox 实体碰撞箱
     * @param movement 期望移动量
     * @return 实际可移动量
     */
    [[nodiscard]] f64 collide(Axis axis, const AxisAlignedBB& entityBox, f64 movement) const;

    // === 形状比较 ===

    /**
     * @brief 检查两个形状是否相等
     */
    [[nodiscard]] static bool equal(const VoxelShape& a, const VoxelShape& b);

    // === 底层访问 ===

    [[nodiscard]] const DiscreteVoxelShape& shape() const { return *m_shape; }
    [[nodiscard]] std::shared_ptr<DiscreteVoxelShape> shapePtr() const { return m_shape; }

    // === 索引查找 ===

    /**
     * @brief 查找坐标点索引
     * @return 坐标值所在区间的起始索引
     */
    [[nodiscard]] i32 findIndex(Axis axis, f64 coord) const;

    /**
     * @brief 获取指定轴的最小值（带约束）
     */
    [[nodiscard]] f64 min(Axis axis, f64 coord1, f64 coord2) const;

    /**
     * @brief 获取指定轴的最大值（带约束）
     */
    [[nodiscard]] f64 max(Axis axis, f64 coord1, f64 coord2) const;

    // === 光线投射 ===

    /**
     * @brief 射线检测
     * @param start 射线起点
     * @param end 射线终点
     * @param offset 方块偏移位置
     * @return 命中结果（如果没有命中返回nullopt）
     */
    [[nodiscard]] std::optional<BlockHitResult> clip(
        const Vector3& start, const Vector3& end, const BlockPos& offset) const;

    // === 最近点 ===

    /**
     * @brief 获取最近点
     */
    [[nodiscard]] std::optional<Vector3> closestPointTo(const Vector3& point) const;

    // === 点包含检测 ===

    /**
     * @brief 检查点是否在形状内部
     * @param x, y, z 点坐标（方块本地坐标，0-1范围）
     * @return 点是否在形状内部
     * @note 使用半开区间 [min, max)
     */
    [[nodiscard]] bool contains(f64 x, f64 y, f64 z) const;

    // === 形状运算（通过Shapes类） ===

    friend class Shapes;

protected:
    std::shared_ptr<DiscreteVoxelShape> m_shape;
    std::vector<f64> m_xPoints;
    std::vector<f64> m_yPoints;
    std::vector<f64> m_zPoints;

    // 面形状缓存
    mutable std::unique_ptr<VoxelShape[]> m_faces;

    // 碰撞检测内部方法
    [[nodiscard]] f64 _collideX(AxisCycle cycle, const AxisAlignedBB& entityBox, f64 movement) const;

    // 检查形状点列表是否匹配立方体
    [[nodiscard]] bool _isCubePointRange(Axis axis) const;

    // 初始化面形状缓存
    void _initFaceCache() const;
};

} // namespace mc
