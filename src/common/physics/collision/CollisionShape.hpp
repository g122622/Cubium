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
    /** @brief 共享空形状，供返回引用的方块接口使用。 */
    [[nodiscard]] static const CollisionShape& emptyShape() noexcept
    {
        static const CollisionShape shape = empty();
        return shape;
    }

    /** @brief 共享完整方块形状，供返回引用的方块接口使用。 */
    [[nodiscard]] static const CollisionShape& fullBlockShape() noexcept
    {
        static const CollisionShape shape = fullBlock();
        return shape;
    }
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
    [[nodiscard]] static CollisionShape empty() noexcept
    {
        return CollisionShape();
    }

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
        // **只要加进至少一个盒子，形状就不再是空**。
        // 此前写成 `m_boxes.size() > 1`，于是「恰好一个盒子」的形状仍被标成 Type::Empty——
        // 任何先默认构造、再 addBox 一次的形状都会静默丢失（isEmpty() 返回 true，
        // 旋转/合并等操作的输入也据此短路）。由差分能力 block_shape_all 在
        // big_dripleaf 的 tilt=full 上查出：该状态只剩一根单盒的茎，旋转后整格变成空形状。
        //
        // 单盒且恰为 0..1 满格时提升为 FullBlock，保持 `isFullBlock()` 等快速路径可用。
        const bool singleUnitBox = m_boxes.size() == 1 && minX <= 0.0f && minY <= 0.0f && minZ <= 0.0f &&
            maxX >= 1.0f && maxY >= 1.0f && maxZ >= 1.0f;
        m_type = singleUnitBox ? Type::FullBlock : Type::SimpleBox;
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
     * @param op 合并操作
     * @return 合并后的形状
     *
     * 三种操作都基于 AABB 列表实现：
     *
     * - `OR`：直接拼接两个列表（重复区域不影响判定）。
     * - `AND`：两两求交，交集非空才保留。对应原版 `Shapes.join(a, b, BooleanOp.AND)`。
     * - `NOT`：a 减去 b。逐个把 a 的盒子按 b 的盒子切分，切完后剩下的碎块保留。
     *   对应原版 `Shapes.join(a, b, BooleanOp.ONLY_FIRST)`。
     *
     * 注意 `NOT` 的结果盒数可能显著增加：一个盒子被 n 个盒子切分后最多产生 6n 个碎块。
     * 这对形状判定是正确的（判定按点是否落在任一盒内），但会放大后续遍历开销。
     */
    /**
     * @brief 整体平移形状。
     *
     * 用于 `offsetType` 方块：同一方块在不同坐标处有微小位置偏移（花、作物等），
     * 形状需要跟着平移。空形状平移后仍为空。
     *
     * @param dx X 方向平移量
     * @param dy Y 方向平移量
     * @param dz Z 方向平移量
     * @return 平移后的新形状
     */
    [[nodiscard]] CollisionShape offset(f32 dx, f32 dy, f32 dz) const
    {
        if (m_boxes.empty()) {
            return *this;
        }
        std::vector<AxisAlignedBB> shifted;
        shifted.reserve(m_boxes.size());
        for (const auto& box : m_boxes) {
            shifted.emplace_back(
                box.minX + dx, box.minY + dy, box.minZ + dz, box.maxX + dx, box.maxY + dy, box.maxZ + dz);
        }
        return _fromBoxes(std::move(shifted));
    }

    [[nodiscard]] static CollisionShape combine(
        const CollisionShape& a, const CollisionShape& b, CombineOp op = CombineOp::OR)
    {
        switch (op) {
            case CombineOp::OR:
                return _combineOr(a, b);
            case CombineOp::AND:
                return _combineAnd(a, b);
            case CombineOp::NOT:
                return _combineNot(a, b);
        }
        return CollisionShape();
    }

    // 查询
    [[nodiscard]] bool isEmpty() const noexcept
    {
        return m_type == Type::Empty;
    }

    [[nodiscard]] bool isFullBlock() const noexcept
    {
        return m_type == Type::FullBlock;
    }

    [[nodiscard]] Type type() const noexcept
    {
        return m_type;
    }

    [[nodiscard]] const std::vector<AxisAlignedBB>& boxes() const noexcept
    {
        return m_boxes;
    }

    [[nodiscard]] size_t boxCount() const noexcept
    {
        return m_boxes.size();
    }

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
        if (isEmpty()) {
            return false;
        }

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
     *
     * @return 如果形状覆盖整个单位方块返回 true
     */
    [[nodiscard]] bool coversFullBlock() const;

private:
    /**
     * @brief 由盒列表构造形状，自动判定 Empty / FullBlock / SimpleBox。
     */
    [[nodiscard]] static CollisionShape _fromBoxes(std::vector<AxisAlignedBB> boxes)
    {
        CollisionShape shape;
        if (boxes.empty()) {
            return shape; // Type::Empty
        }
        if (boxes.size() == 1) {
            const AxisAlignedBB& only = boxes.front();
            if (only.minX <= 0.0f && only.minY <= 0.0f && only.minZ <= 0.0f && only.maxX >= 1.0f && only.maxY >= 1.0f &&
                only.maxZ >= 1.0f) {
                shape.m_type = Type::FullBlock;
                shape.m_boxes = std::move(boxes);
                return shape;
            }
        }
        shape.m_type = Type::SimpleBox;
        shape.m_boxes = std::move(boxes);
        return shape;
    }

    [[nodiscard]] static CollisionShape _combineOr(const CollisionShape& a, const CollisionShape& b)
    {
        if (a.m_boxes.empty()) {
            return b;
        }
        if (b.m_boxes.empty()) {
            return a;
        }
        std::vector<AxisAlignedBB> boxes = a.m_boxes;
        boxes.insert(boxes.end(), b.m_boxes.begin(), b.m_boxes.end());
        // 并集里出现完整方块时保持 FullBlock 标记，与原实现一致。
        if (a.m_type == Type::FullBlock || b.m_type == Type::FullBlock) {
            CollisionShape result;
            result.m_type = Type::FullBlock;
            result.m_boxes = std::move(boxes);
            return result;
        }
        return _fromBoxes(std::move(boxes));
    }

    [[nodiscard]] static CollisionShape _combineAnd(const CollisionShape& a, const CollisionShape& b)
    {
        std::vector<AxisAlignedBB> boxes;
        for (const AxisAlignedBB& lhs : a.m_boxes) {
            for (const AxisAlignedBB& rhs : b.m_boxes) {
                const f32 minX = std::max(lhs.minX, rhs.minX);
                const f32 minY = std::max(lhs.minY, rhs.minY);
                const f32 minZ = std::max(lhs.minZ, rhs.minZ);
                const f32 maxX = std::min(lhs.maxX, rhs.maxX);
                const f32 maxY = std::min(lhs.maxY, rhs.maxY);
                const f32 maxZ = std::min(lhs.maxZ, rhs.maxZ);
                // 退化的交集（某轴 max <= min）不含体积，丢弃。
                if (maxX <= minX || maxY <= minY || maxZ <= minZ) {
                    continue;
                }
                boxes.emplace_back(minX, minY, minZ, maxX, maxY, maxZ);
            }
        }
        return _fromBoxes(std::move(boxes));
    }

    [[nodiscard]] static CollisionShape _combineNot(const CollisionShape& a, const CollisionShape& b)
    {
        std::vector<AxisAlignedBB> boxes = a.m_boxes;
        for (const AxisAlignedBB& cutter : b.m_boxes) {
            std::vector<AxisAlignedBB> next;
            next.reserve(boxes.size());
            for (const AxisAlignedBB& piece : boxes) {
                _subtractBox(piece, cutter, next);
            }
            boxes = std::move(next);
            if (boxes.empty()) {
                break;
            }
        }
        return _fromBoxes(std::move(boxes));
    }

    /**
     * @brief 把 piece 减去 cutter，把剩余的碎块追加到 out。
     *
     * 按 X / Y / Z 三个方向依次切分，最多产生 6 个碎块（每个方向两侧各一块）。
     * 与 cutter 无重叠时原样保留 piece。
     */
    static void _subtractBox(const AxisAlignedBB& piece, const AxisAlignedBB& cutter, std::vector<AxisAlignedBB>& out)
    {
        if (piece.maxX <= cutter.minX || piece.minX >= cutter.maxX || piece.maxY <= cutter.minY ||
            piece.minY >= cutter.maxY || piece.maxZ <= cutter.minZ || piece.minZ >= cutter.maxZ) {
            out.push_back(piece);
            return;
        }

        // X 方向：左段、右段。
        if (piece.minX < cutter.minX) {
            out.emplace_back(piece.minX, piece.minY, piece.minZ, cutter.minX, piece.maxY, piece.maxZ);
        }
        if (piece.maxX > cutter.maxX) {
            out.emplace_back(cutter.maxX, piece.minY, piece.minZ, piece.maxX, piece.maxY, piece.maxZ);
        }
        // Y 方向：只在 X 与 cutter 重叠的区间内切。
        const f32 midMinX = std::max(piece.minX, cutter.minX);
        const f32 midMaxX = std::min(piece.maxX, cutter.maxX);
        if (piece.minY < cutter.minY) {
            out.emplace_back(midMinX, piece.minY, piece.minZ, midMaxX, cutter.minY, piece.maxZ);
        }
        if (piece.maxY > cutter.maxY) {
            out.emplace_back(midMinX, cutter.maxY, piece.minZ, midMaxX, piece.maxY, piece.maxZ);
        }
        // Z 方向：只在 X、Y 都与 cutter 重叠的区间内切。
        const f32 midMinY = std::max(piece.minY, cutter.minY);
        const f32 midMaxY = std::min(piece.maxY, cutter.maxY);
        if (piece.minZ < cutter.minZ) {
            out.emplace_back(midMinX, midMinY, piece.minZ, midMaxX, midMaxY, cutter.minZ);
        }
        if (piece.maxZ > cutter.maxZ) {
            out.emplace_back(midMinX, midMinY, cutter.maxZ, midMaxX, midMaxY, piece.maxZ);
        }
    }

    Type m_type = Type::Empty;
    std::vector<AxisAlignedBB> m_boxes; // 方块本地坐标 (0-1范围)
};

} // namespace mc
