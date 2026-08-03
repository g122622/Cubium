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

#include "BooleanOp.hpp"
#include "VoxelShape.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace mc {

/**
 * @brief 体素形状工厂类
 *
 * 提供创建和操作 VoxelShape 的静态方法。
 * 包括：
 * - 基础形状创建（空、完整方块、盒子）
 * - 布尔运算（并集、交集、差集）
 * - 面遮挡检测（用于光照系统）
 */
class Shapes {
public:
    // ========================================================================
    // 常量
    // ========================================================================

    /// 浮点数比较精度
    static constexpr f64 EPSILON = 1.0E-7;
    static constexpr f64 BIG_EPSILON = 1.0E-6;
    static constexpr f64 LARGE_COORDINATE = 1.0E30;

    // ========================================================================
    // 基础形状
    // ========================================================================

    /**
     * @brief 获取空形状
     */
    [[nodiscard]] static VoxelShape empty();

    /**
     * @brief 获取完整方块形状 (0,0,0)->(1,1,1)
     */
    [[nodiscard]] static VoxelShape block();

    /**
     * @brief 获取无限大形状
     */
    [[nodiscard]] static VoxelShape infinity();

    /**
     * @brief 创建盒子形状
     * @param minX, minY, minZ 最小坐标（方块本地坐标）
     * @param maxX, maxY, maxZ 最大坐标（方块本地坐标）
     * @throws 如果 min > max
     */
    [[nodiscard]] static VoxelShape box(f64 minX, f64 minY, f64 minZ, f64 maxX, f64 maxY, f64 maxZ);

    /**
     * @brief 创建盒子形状（不检查参数）
     */
    [[nodiscard]] static VoxelShape create(f64 minX, f64 minY, f64 minZ, f64 maxX, f64 maxY, f64 maxZ);

    /**
     * @brief 从AABB创建形状
     */
    [[nodiscard]] static VoxelShape create(const AxisAlignedBB& aabb);

    // ========================================================================
    // 布尔运算
    // ========================================================================

    /**
     * @brief 并集操作
     */
    [[nodiscard]] static VoxelShape or_(const VoxelShape& a, const VoxelShape& b);

    /**
     * @brief 多形状并集
     */
    [[nodiscard]] static VoxelShape or_(const VoxelShape& first, const std::vector<VoxelShape>& others);

    /**
     * @brief 连接两个形状
     * @param a 第一个形状
     * @param b 第二个形状
     * @param op 布尔操作
     * @return 操作后的形状（已优化）
     */
    [[nodiscard]] static VoxelShape join(const VoxelShape& a, const VoxelShape& b, const BooleanOp& op);

    /**
     * @brief 连接两个形状（未优化）
     * @return 未优化的形状，用于内部操作
     */
    [[nodiscard]] static VoxelShape joinUnoptimized(const VoxelShape& a, const VoxelShape& b, const BooleanOp& op);

    /**
     * @brief 检查连接结果是否非空
     */
    [[nodiscard]] static bool joinIsNotEmpty(const VoxelShape& a, const VoxelShape& b, const BooleanOp& op);

    // ========================================================================
    // 面遮挡检测（用于光照系统）
    // ========================================================================

    /**
     * @brief 检查一个方块是否遮挡另一个方块的面
     *
     * 这是光照系统的核心函数，用于判断光线是否可以穿过两个相邻方块之间的边界。
     *
     * @param sourceShape 源方块的遮挡形状（光线发出的方块）
     * @param targetShape 目标方块的遮挡形状（光线进入的方块）
     * @param direction 光线传播方向（从源指向目标）
     * @return true 如果面被完全遮挡（光线无法通过）
     *
     * 算法：
     * 1. 如果两个形状都是完整方块，返回true（完全遮挡）
     * 2. 如果目标形状为空，返回false（完全不遮挡）
     * 3. 获取两个形状在边界面的投影形状
     * 4. 使用 ONLY_FIRST 操作检查投影是否完全覆盖
     */
    [[nodiscard]] static bool blockOccludes(
        const VoxelShape& sourceShape, const VoxelShape& targetShape, Direction direction);

    /**
     * @brief 检查合并后的面形状是否遮挡
     *
     * 用于光照传播，检查两个相邻方块的合并面形状是否完全遮挡光线。
     *
     * @param sourceShape 源形状
     * @param targetShape 目标形状
     * @param direction 从源到目标的方向
     * @return true 如果面被完全遮挡
     */
    [[nodiscard]] static bool mergedFaceOccludes(
        const VoxelShape& sourceShape, const VoxelShape& targetShape, Direction direction);

    /**
     * @brief 检查两个面形状是否互相遮挡
     *
     * 这是最常用的遮挡检测函数，直接检查两个投影形状。
     *
     * @param faceShape1 第一个面形状
     * @param faceShape2 第二个面形状
     * @return true 如果合并后的形状完全遮挡单位正方形
     */
    [[nodiscard]] static bool faceShapeOccludes(const VoxelShape& faceShape1, const VoxelShape& faceShape2);

    // ========================================================================
    // 切片操作
    // ========================================================================

    /**
     * @brief 获取形状在指定轴上的切片
     * @param shape 源形状
     * @param axis 切片轴
     * @param index 切片索引
     */
    [[nodiscard]] static VoxelShape slice(const VoxelShape& shape, Axis axis, i32 index);

    /**
     * @brief 创建切片形状（内部使用）
     */
    [[nodiscard]] static VoxelShape createSlice(const VoxelShape& shape, Axis axis, i32 index);

    // ========================================================================
    // 碰撞检测
    // ========================================================================

    /**
     * @brief 计算碰撞偏移
     * @param axis 移动轴
     * @param entityBox 实体碰撞箱
     * @param shapes 碰撞形状集合
     * @param movement 期望移动量
     * @return 实际可移动量
     */
    [[nodiscard]] static f64 collide(
        Axis axis, const AxisAlignedBB& entityBox, const std::vector<VoxelShape>& shapes, f64 movement);

    // ========================================================================
    // 辅助函数
    // ========================================================================

    /**
     * @brief 检查形状是否为完整方块
     */
    [[nodiscard]] static bool isBlock(const VoxelShape& shape);

    /**
     * @brief 检查形状是否为空
     */
    [[nodiscard]] static bool isEmpty(const VoxelShape& shape);

    /**
     * @brief 将 CollisionShape 转换为 VoxelShape
     *
     * 对于完整方块和空形状有优化路径。
     * 对于多碰撞盒形状，使用 Shapes::or_() 合并所有盒。
     *
     * @param shape 碰撞形状
     * @return 对应的体素形状
     */
    [[nodiscard]] static VoxelShape fromCollisionShape(const CollisionShape& shape);

    // 索引合并器（用于布尔运算）
    class IndexMerger;
    [[nodiscard]] static std::unique_ptr<IndexMerger> createIndexMerger(
        i32 size, const std::vector<f64>& a, const std::vector<f64>& b, bool aIncluded, bool bIncluded);

private:
    // 缓存的形状
    static VoxelShape s_empty;
    static VoxelShape s_block;
    static VoxelShape s_infinity;
    static bool s_initialized;

    // 初始化静态缓存
    static void _ensureInitialized();
};

/**
 * @brief 索引合并器接口
 *
 * 用于合并两个形状的坐标点列表。
 */
class Shapes::IndexMerger {
public:
    virtual ~IndexMerger() = default;

    /**
     * @brief 获取合并后的坐标列表
     */
    [[nodiscard]] virtual const std::vector<f64>& getList() const noexcept = 0;

    /**
     * @brief 遍历合并的索引
     * @param consumer 消费者函数 (indexA, indexB, mergedIndex)
     * @return 是否完成遍历
     */
    [[nodiscard]] virtual bool forMergedIndexes(const std::function<bool(i32, i32, i32)>& consumer) const = 0;

    /**
     * @brief 获取合并后的大小
     */
    [[nodiscard]] virtual i32 size() const noexcept = 0;
};

} // namespace mc
