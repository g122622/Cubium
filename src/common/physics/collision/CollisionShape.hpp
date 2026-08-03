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
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <utility>
#include <vector>

namespace mc {

// Axis 枚举已移至 Direction.hpp

/**
 * @brief 碰撞形状
 *
 * 简化版的VoxelShape，支持空、完整方块、简单盒三种类型。
 * 碰撞箱使用方块本地坐标（0-1范围）。
 *
 * 注意：
 * - 空形状表示没有碰撞（如空气、水、岩浆）
 * - 完整方块是最常见的形状，有优化路径
 * - 简单盒支持自定义碰撞箱（如台阶、楼梯等）
 *
 * 面形状投影：
 * - getFaceShape(Direction) 返回形状在指定方向上的投影
 * - 用于光照遮挡检测，判断光线是否能穿过相邻方块之间的边界
 */
class CollisionShape {
public:
    /**
     * @brief 形状类型
     */
    enum class Type : u8 {
        Empty,     // 无碰撞
        FullBlock, // 完整方块 (0,0,0) -> (1,1,1)
        SimpleBox  // 简单盒（可能有多个碰撞箱）
    };

    // 默认构造：空形状
    CollisionShape() noexcept
        : m_type(Type::Empty)
    {}

    /**
     * @brief 创建空形状（无碰撞）
     */
    [[nodiscard]] static CollisionShape empty() noexcept { return CollisionShape(); }

    /**
     * @brief 创建完整方块形状
     */
    [[nodiscard]] static CollisionShape fullBlock() noexcept
    {
        CollisionShape shape;
        shape.m_type = Type::FullBlock;
        shape.m_boxes.emplace_back(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        return shape;
    }

    /**
     * @brief 创建简单盒形状
     * @param minX, minY, minZ 最小坐标（方块本地坐标，0-1范围）
     * @param maxX, maxY, maxZ 最大坐标（方块本地坐标，0-1范围）
     */
    [[nodiscard]] static CollisionShape box(f32 minX, f32 minY, f32 minZ, f32 maxX, f32 maxY, f32 maxZ) noexcept
    {
        CollisionShape shape;
        shape.m_type = Type::SimpleBox;
        shape.m_boxes.emplace_back(minX, minY, minZ, maxX, maxY, maxZ);
        return shape;
    }

    /**
     * @brief 从像素坐标创建碰撞形状（16像素 = 1方块）
     * @param minX, minY, minZ 最小坐标（像素坐标，0-16范围）
     * @param maxX, maxY, maxZ 最大坐标（像素坐标，0-16范围）
     */
    [[nodiscard]] static CollisionShape fromPixelBox(
        f32 minX, f32 minY, f32 minZ, f32 maxX, f32 maxY, f32 maxZ) noexcept
    {
        constexpr f32 PIXEL_TO_BLOCK = 1.0f / 16.0f;
        return box(minX * PIXEL_TO_BLOCK,
            minY * PIXEL_TO_BLOCK,
            minZ * PIXEL_TO_BLOCK,
            maxX * PIXEL_TO_BLOCK,
            maxY * PIXEL_TO_BLOCK,
            maxZ * PIXEL_TO_BLOCK);
    }

    /**
     * @brief 添加额外的碰撞盒
     * 用于复杂形状（如楼梯）
     */
    CollisionShape& addBox(f32 minX, f32 minY, f32 minZ, f32 maxX, f32 maxY, f32 maxZ)
    {
        m_boxes.emplace_back(minX, minY, minZ, maxX, maxY, maxZ);
        if (m_boxes.size() > 1) {
            m_type = Type::SimpleBox;
        }
        return *this;
    }

    /**
     * @brief 合并操作类型
     */
    enum class CombineOp : u8 {
        OR,  // 并集
        AND, // 交集
        NOT  // 差集
    };

    /**
     * @brief 合并两个碰撞形状
     * @param a 第一个形状
     * @param b 第二个形状
     * @param op 合并操作（目前只支持 OR）
     * @return 合并后的形状
     */
    [[nodiscard]] static CollisionShape combine(
        const CollisionShape& a, const CollisionShape& b, CombineOp op = CombineOp::OR)
    {
        CollisionShape result;

        if (op == CombineOp::OR) {
            // 并集：简单地复制所有碰撞盒
            result.m_type =
                (a.m_type == Type::FullBlock || b.m_type == Type::FullBlock) ? Type::FullBlock : Type::SimpleBox;

            result.m_boxes = a.m_boxes;
            for (const auto& box : b.m_boxes) {
                result.m_boxes.push_back(box);
            }
        }
        // AND 和 NOT 操作暂不实现（需要更复杂的几何运算）

        return result;
    }

    // 查询
    [[nodiscard]] bool isEmpty() const noexcept { return m_type == Type::Empty; }
    [[nodiscard]] bool isFullBlock() const noexcept { return m_type == Type::FullBlock; }
    [[nodiscard]] Type type() const noexcept { return m_type; }
    [[nodiscard]] const std::vector<AxisAlignedBB>& boxes() const noexcept { return m_boxes; }
    [[nodiscard]] size_t boxCount() const noexcept { return m_boxes.size(); }

    /**
     * @brief 获取世界坐标碰撞箱
     * @param blockX, blockY, blockZ 方块在世界中的位置
     * @return 世界坐标下的碰撞箱列表
     */
    [[nodiscard]] std::vector<AxisAlignedBB> getWorldBoxes(i32 blockX, i32 blockY, i32 blockZ) const
    {
        std::vector<AxisAlignedBB> worldBoxes;
        worldBoxes.reserve(m_boxes.size());

        f32 bx = static_cast<f32>(blockX);
        f32 by = static_cast<f32>(blockY);
        f32 bz = static_cast<f32>(blockZ);

        for (const auto& localBox : m_boxes) {
            worldBoxes.emplace_back(bx + localBox.minX,
                by + localBox.minY,
                bz + localBox.minZ,
                bx + localBox.maxX,
                by + localBox.maxY,
                bz + localBox.maxZ);
        }
        return worldBoxes;
    }

    /**
     * @brief 检测与实体碰撞箱是否相交
     * @param entityBox 实体的世界坐标碰撞箱
     * @param blockX, blockY, blockZ 方块位置
     * @return 是否相交
     */
    [[nodiscard]] bool intersects(const AxisAlignedBB& entityBox, i32 blockX, i32 blockY, i32 blockZ) const
    {
        if (isEmpty()) return false;

        f32 bx = static_cast<f32>(blockX);
        f32 by = static_cast<f32>(blockY);
        f32 bz = static_cast<f32>(blockZ);

        for (const auto& localBox : m_boxes) {
            AxisAlignedBB worldBox(bx + localBox.minX,
                by + localBox.minY,
                bz + localBox.minZ,
                bx + localBox.maxX,
                by + localBox.maxY,
                bz + localBox.maxZ);
            if (entityBox.intersects(worldBox)) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief 获取形状在指定方向上的面形状
     *
     * 这是光照系统的核心函数，用于判断光线是否能穿过相邻方块之间的边界。
     * 返回一个表示该面上投影区域的新碰撞形状。
     *
     * @param direction 方向（Down=0, Up=1, North=2, South=3, West=4, East=5）
     * @return 该方向上的面投影形状
     */
    [[nodiscard]] CollisionShape getFaceShape(Direction direction) const noexcept
    {
        // 空形状和完整方块快速路径
        if (isEmpty()) {
            return empty();
        }
        if (isFullBlock()) {
            return fullBlock();
        }

        const Axis axis = Directions::getAxis(direction);
        const AxisDirection axisDir = Directions::getAxisDirection(direction);

        // 收集所有接触指定面的碰撞箱
        CollisionShape result;
        result.m_type = Type::Empty;
        result.m_boxes.reserve(m_boxes.size());

        // 精度常量，用于浮点数比较
        constexpr f32 EPSILON = 1.0e-7f;

        for (const auto& box : m_boxes) {
            // 检查碰撞箱是否延伸到指定面
            bool touchesFace = false;
            if (axisDir == AxisDirection::Positive) {
                // 正方向：检查 max[axis] ≈ 1.0
                f32 maxCoord = 0.0f;
                switch (axis) {
                    case Axis::X:
                        maxCoord = box.maxX;
                        break;
                    case Axis::Y:
                        maxCoord = box.maxY;
                        break;
                    case Axis::Z:
                        maxCoord = box.maxZ;
                        break;
                }
                touchesFace = std::abs(maxCoord - 1.0f) < EPSILON;
            } else {
                // 负方向：检查 min[axis] ≈ 0.0
                f32 minCoord = 0.0f;
                switch (axis) {
                    case Axis::X:
                        minCoord = box.minX;
                        break;
                    case Axis::Y:
                        minCoord = box.minY;
                        break;
                    case Axis::Z:
                        minCoord = box.minZ;
                        break;
                }
                touchesFace = std::abs(minCoord) < EPSILON;
            }

            if (touchesFace) {
                // 创建投影箱：将切片轴的范围扩展到 [0, 1]
                // 这表示该碰撞箱在该面上的投影覆盖了整个深度
                AxisAlignedBB projBox = box;
                switch (axis) {
                    case Axis::X:
                        projBox.minX = 0.0f;
                        projBox.maxX = 1.0f;
                        break;
                    case Axis::Y:
                        projBox.minY = 0.0f;
                        projBox.maxY = 1.0f;
                        break;
                    case Axis::Z:
                        projBox.minZ = 0.0f;
                        projBox.maxZ = 1.0f;
                        break;
                }
                result.m_boxes.push_back(projBox);
            }
        }

        // 更新结果类型
        if (result.m_boxes.empty()) {
            return empty();
        }
        if (result.m_boxes.size() == 1) {
            const auto& singleBox = result.m_boxes[0];
            if (singleBox.minX == 0.0f && singleBox.minY == 0.0f && singleBox.minZ == 0.0f && singleBox.maxX == 1.0f &&
                singleBox.maxY == 1.0f && singleBox.maxZ == 1.0f) {
                return fullBlock();
            }
            result.m_type = Type::SimpleBox;
        } else {
            result.m_type = Type::SimpleBox;
        }

        return result;
    }

    /**
     * @brief 检查形状是否覆盖整个单位方块
     *
     * 使用 VoxelShape 布尔运算精确判断，与 MC Java 的
     * Block.isShapeFullBlock 实现一致：
     * !Shapes.joinIsNotEmpty(Shapes.block(), shape, BooleanOp.NOT_SAME)
     * 即检查是否存在任何体素使得完整方块和本形状不一致，
     * 如果不存在差异体素，则本形状覆盖整个方块。
     *
     * 参考: net.minecraft.block.Block#isShapeFullBlock
     *
     * @return 如果形状覆盖整个单位方块返回 true
     */
    [[nodiscard]] bool coversFullBlock() const;

private:
    Type m_type = Type::Empty;
    std::vector<AxisAlignedBB> m_boxes; // 方块本地坐标 (0-1范围)
};

} // namespace mc
