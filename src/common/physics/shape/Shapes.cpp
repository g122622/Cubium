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

#include "Shapes.hpp"

#include "BitSetDiscreteVoxelShape.hpp"
#include "CubePointRange.hpp"
#include "IndexMergers.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/physics/shape/BooleanOp.hpp"
#include "common/physics/shape/DiscreteVoxelShape.hpp"
#include "common/physics/shape/VoxelShape.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"

#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {

// ============================================================================
// 静态成员初始化
// ============================================================================

VoxelShape Shapes::s_empty;
VoxelShape Shapes::s_block;
VoxelShape Shapes::s_infinity;
bool Shapes::s_initialized = false;

// ============================================================================
// 初始化
// ============================================================================

void Shapes::_ensureInitialized()
{
    if (s_initialized) return;

    // 创建空形状
    auto emptyShape = std::make_shared<DiscreteVoxelShape>(0, 0, 0);
    s_empty = VoxelShape(emptyShape, {0.0}, {0.0}, {0.0});

    // 创建完整方块
    auto blockShape = std::make_shared<DiscreteVoxelShape>(1, 1, 1);
    blockShape->fill(0, 0, 0);
    s_block = VoxelShape(blockShape, {0.0, 1.0}, {0.0, 1.0}, {0.0, 1.0});

    // 创建无限大形状
    s_infinity = VoxelShape(std::make_shared<DiscreteVoxelShape>(1, 1, 1),
        {-LARGE_COORDINATE, LARGE_COORDINATE},
        {-LARGE_COORDINATE, LARGE_COORDINATE},
        {-LARGE_COORDINATE, LARGE_COORDINATE});
    s_infinity.m_shape->fill(0, 0, 0);

    s_initialized = true;
}

// ============================================================================
// 基础形状
// ============================================================================

VoxelShape Shapes::empty()
{
    _ensureInitialized();
    return s_empty;
}

VoxelShape Shapes::block()
{
    _ensureInitialized();
    return s_block;
}

VoxelShape Shapes::infinity()
{
    _ensureInitialized();
    return s_infinity;
}

VoxelShape Shapes::box(f64 minX, f64 minY, f64 minZ, f64 maxX, f64 maxY, f64 maxZ)
{
    if (minX > maxX || minY > maxY || minZ > maxZ) {
        // 抛出异常或返回空
        return empty();
    }
    return create(minX, minY, minZ, maxX, maxY, maxZ);
}

VoxelShape Shapes::create(f64 minX, f64 minY, f64 minZ, f64 maxX, f64 maxY, f64 maxZ)
{
    // 检查尺寸是否太小
    if (maxX - minX < EPSILON || maxY - minY < EPSILON || maxZ - minZ < EPSILON) {
        return empty();
    }

    // 查找合适的分辨率
    const auto findBits = [](f64 minVal, f64 maxVal) -> i32 {
        if (minVal < -EPSILON || maxVal > 1.0 + EPSILON) {
            return -1;
        }
        for (i32 bits = 0; bits <= 3; ++bits) {
            const i32 scale = 1 << bits;
            const f64 scaledMin = minVal * scale;
            const f64 scaledMax = maxVal * scale;
            const bool minOk = std::abs(scaledMin - std::round(scaledMin)) < EPSILON * scale;
            const bool maxOk = std::abs(scaledMax - std::round(scaledMax)) < EPSILON * scale;
            if (minOk && maxOk) {
                return bits;
            }
        }
        return -1;
    };

    const i32 xBits = findBits(minX, maxX);
    const i32 yBits = findBits(minY, maxY);
    const i32 zBits = findBits(minZ, maxZ);

    if (xBits < 0 || yBits < 0 || zBits < 0) {
        // 使用数组形状
        auto shape = std::make_shared<DiscreteVoxelShape>(1, 1, 1);
        shape->fill(0, 0, 0);
        return VoxelShape(shape, {minX, maxX}, {minY, maxY}, {minZ, maxZ});
    }

    if (xBits == 0 && yBits == 0 && zBits == 0) {
        return block();
    }

    // 创建具有适当分辨率的形状
    const i32 xSize = 1 << xBits;
    const i32 ySize = 1 << yBits;
    const i32 zSize = 1 << zBits;

    auto shape = DiscreteVoxelShape::withFilledBounds(xSize,
        ySize,
        zSize,
        static_cast<i32>(std::round(minX * xSize)),
        static_cast<i32>(std::round(minY * ySize)),
        static_cast<i32>(std::round(minZ * zSize)),
        static_cast<i32>(std::round(maxX * xSize)),
        static_cast<i32>(std::round(maxY * ySize)),
        static_cast<i32>(std::round(maxZ * zSize)));

    // 创建立方体坐标点列表
    std::vector<f64> xPoints(xSize + 1);
    std::vector<f64> yPoints(ySize + 1);
    std::vector<f64> zPoints(zSize + 1);

    for (i32 i = 0; i <= xSize; ++i) {
        xPoints[i] = static_cast<f64>(i) / xSize;
    }
    for (i32 i = 0; i <= ySize; ++i) {
        yPoints[i] = static_cast<f64>(i) / ySize;
    }
    for (i32 i = 0; i <= zSize; ++i) {
        zPoints[i] = static_cast<f64>(i) / zSize;
    }

    return VoxelShape(std::make_shared<DiscreteVoxelShape>(std::move(shape)),
        std::move(xPoints),
        std::move(yPoints),
        std::move(zPoints));
}

VoxelShape Shapes::create(const AxisAlignedBB& aabb)
{
    return create(aabb.minX, aabb.minY, aabb.minZ, aabb.maxX, aabb.maxY, aabb.maxZ);
}

// ============================================================================
// 布尔运算
// ============================================================================

VoxelShape Shapes::or_(const VoxelShape& a, const VoxelShape& b)
{
    return join(a, b, BooleanOps::Or());
}

VoxelShape Shapes::or_(const VoxelShape& first, const std::vector<VoxelShape>& others)
{
    VoxelShape result = first;
    for (const auto& shape : others) {
        result = or_(result, shape);
    }
    return result;
}

VoxelShape Shapes::join(const VoxelShape& a, const VoxelShape& b, const BooleanOp& op)
{
    return joinUnoptimized(a, b, op).optimize();
}

VoxelShape Shapes::joinUnoptimized(const VoxelShape& a, const VoxelShape& b, const BooleanOp& op)
{
    // 布尔运算的 false,false 情况会导致无限体积
    if (!op.apply(false, false)) {
        // 标准布尔运算都满足此条件（AND, OR, ONLY_FIRST, ONLY_SECOND 等）
        // 这是正确的路径
    } else {
        // FALSE 和 TRUE 操作不在 join 中处理
        return empty();
    }

    // 相同形状快捷路径
    if (&a == &b) {
        return op.apply(true, true) ? a : empty();
    }

    // 空形状快捷路径
    if (a.isEmpty()) {
        return op.apply(false, true) ? b : empty();
    }

    if (b.isEmpty()) {
        return op.apply(true, false) ? a : empty();
    }

    // 计算布尔运算的包含标志
    const bool includeA = op.apply(true, false); // A 独占区域是否包含
    const bool includeB = op.apply(false, true); // B 独占区域是否包含
    const bool includeAB = op.apply(true, true); // A 和 B 重叠区域是否包含

    // 创建三个轴的索引合并器
    auto xMerger = createIndexMerger(1, a.getCoords(Axis::X), b.getCoords(Axis::X), includeA, includeB);
    auto yMerger =
        createIndexMerger(xMerger->size() - 1, a.getCoords(Axis::Y), b.getCoords(Axis::Y), includeA, includeB);
    auto zMerger = createIndexMerger(
        (xMerger->size() - 1) * (yMerger->size() - 1), a.getCoords(Axis::Z), b.getCoords(Axis::Z), includeA, includeB);

    // 使用 BitSetDiscreteVoxelShape::join 执行体素级布尔运算
    DiscreteVoxelShape resultShape =
        BitSetDiscreteVoxelShape::join(a.shape(), b.shape(), *xMerger, *yMerger, *zMerger, op);

    // 从合并器获取合并后的坐标列表
    std::vector<f64> xPoints = xMerger->getList();
    std::vector<f64> yPoints = yMerger->getList();
    std::vector<f64> zPoints = zMerger->getList();

    // 如果结果为空，返回空形状
    if (resultShape.isEmpty()) {
        return empty();
    }

    return VoxelShape(std::make_shared<DiscreteVoxelShape>(std::move(resultShape)),
        std::move(xPoints),
        std::move(yPoints),
        std::move(zPoints));
}

bool Shapes::joinIsNotEmpty(const VoxelShape& a, const VoxelShape& b, const BooleanOp& op)
{
    // MC Java: 如果 op(false, false) 为 true，则该操作没有意义
    // （结果永远是满的，无法判断"是否非空"）
    if (op.apply(false, false)) {
        return true;
    }

    const bool aEmpty = a.isEmpty();
    const bool bEmpty = b.isEmpty();

    // 处理空形状情况
    if (aEmpty && bEmpty) {
        return op.apply(false, false);
    }
    if (aEmpty) {
        return op.apply(false, true);
    }
    if (bEmpty) {
        return op.apply(true, false);
    }

    // 两个形状都不为空
    if (&a == &b) {
        return op.apply(true, true);
    }

    const bool aOnly = op.apply(true, false);
    const bool bOnly = op.apply(false, true);

    // 快速边界检查：如果两个形状在任意轴上不重叠，则没有重叠区域
    for (Axis axis : {Axis::X, Axis::Y, Axis::Z}) {
        if (a.max(axis) < b.min(axis) - EPSILON) {
            // a 和 b 不重叠，a 在 b 前面
            return aOnly || bOnly;
        }
        if (b.max(axis) < a.min(axis) - EPSILON) {
            // a 和 b 不重叠，b 在 a 前面
            return aOnly || bOnly;
        }
    }

    // MC Java: 使用 IndexMerger 进行逐体素检查
    auto xMerger = createIndexMerger(1, a.getCoords(Axis::X), b.getCoords(Axis::X), aOnly, bOnly);
    auto yMerger = createIndexMerger(xMerger->size() - 1, a.getCoords(Axis::Y), b.getCoords(Axis::Y), aOnly, bOnly);
    auto zMerger = createIndexMerger(
        (xMerger->size() - 1) * (yMerger->size() - 1), a.getCoords(Axis::Z), b.getCoords(Axis::Z), aOnly, bOnly);

    // 三重循环：遍历所有合并后的体素单元，使用 isFullWide 逐体素检查
    // forMergedIndexes 返回 true 表示遍历完成（没有提前退出）
    // 内层回调返回 !op.apply(inA, inB) -- 找到满足条件的体素时返回 false（停止遍历）
    // 外层取反：如果遍历提前停止，说明找到了满足条件的体素，返回 true（非空）
    return !xMerger->forMergedIndexes([&](i32 xIdxA, i32 xIdxB, i32 /*xIdxMerged*/) -> bool {
        return yMerger->forMergedIndexes([&](i32 yIdxA, i32 yIdxB, i32 /*yIdxMerged*/) -> bool {
            return zMerger->forMergedIndexes([&](i32 zIdxA, i32 zIdxB, i32 /*zIdxMerged*/) -> bool {
                const bool inA = a.shape().isFullWide(xIdxA, yIdxA, zIdxA);
                const bool inB = b.shape().isFullWide(xIdxB, yIdxB, zIdxB);
                return !op.apply(inA, inB); // 找到满足条件的体素时返回 false（停止遍历）
            });
        });
    });
}

// ============================================================================
// 面遮挡检测
// ============================================================================

bool Shapes::blockOccludes(const VoxelShape& sourceShape, const VoxelShape& targetShape, Direction direction)
{
    // 如果两个都是完整方块，完全遮挡
    if (isBlock(sourceShape) && isBlock(targetShape)) {
        return true;
    }

    // 如果目标为空，不遮挡
    if (targetShape.isEmpty()) {
        return false;
    }

    const Axis axis = Directions::getAxis(direction);
    const AxisDirection axisDir = Directions::getAxisDirection(direction);

    // 确定哪个是"靠近"哪个是"远离"的形状
    const VoxelShape& nearShape = (axisDir == AxisDirection::Positive) ? sourceShape : targetShape;
    const VoxelShape& farShape = (axisDir == AxisDirection::Positive) ? targetShape : sourceShape;

    // 检查边界条件
    // 近形状应该在轴的正方向有边界（max接近1.0）
    // 远形状应该在轴的负方向有边界（min接近0.0）
    if (std::abs(nearShape.max(axis) - 1.0) >= EPSILON) {
        // 近形状没有接触边界
        return false;
    }
    if (std::abs(farShape.min(axis)) >= EPSILON) {
        // 远形状没有接触边界
        return false;
    }

    // 获取面形状并进行遮挡检测
    const VoxelShape nearFace = nearShape.getFaceShape(direction);
    const VoxelShape farFace = farShape.getFaceShape(Directions::opposite(direction));

    // 使用 ONLY_FIRST 或 ONLY_SECOND 检查
    if (axisDir == AxisDirection::Positive) {
        return !joinIsNotEmpty(nearFace, farFace, BooleanOps::OnlyFirst());
    } else {
        return !joinIsNotEmpty(nearFace, farFace, BooleanOps::OnlySecond());
    }
}

bool Shapes::mergedFaceOccludes(const VoxelShape& sourceShape, const VoxelShape& targetShape, Direction direction)
{
    // 如果任意一个是完整方块，完全遮挡
    if (isBlock(sourceShape) || isBlock(targetShape)) {
        return true;
    }

    const Axis axis = Directions::getAxis(direction);
    const AxisDirection axisDir = Directions::getAxisDirection(direction);

    // 确定哪个是"靠近"哪个是"远离"的形状
    VoxelShape nearShape = (axisDir == AxisDirection::Positive) ? sourceShape : targetShape;
    VoxelShape farShape = (axisDir == AxisDirection::Positive) ? targetShape : sourceShape;

    // 检查边界条件，修正面形状
    if (std::abs(nearShape.max(axis) - 1.0) >= EPSILON) {
        nearShape = empty();
    }
    if (std::abs(farShape.min(axis)) >= EPSILON) {
        farShape = empty();
    }

    // 获取面形状
    const VoxelShape nearFace = nearShape.getFaceShape(direction);
    const VoxelShape farFace = farShape.getFaceShape(Directions::opposite(direction));

    // 合并面形状并检查是否完全遮挡
    // MC Java 使用 joinUnoptimized 而非 or_（避免触发 optimize）
    const VoxelShape merged = joinUnoptimized(nearFace, farFace, BooleanOps::Or());
    return !joinIsNotEmpty(block(), merged, BooleanOps::OnlyFirst());
}

bool Shapes::faceShapeOccludes(const VoxelShape& faceShape1, const VoxelShape& faceShape2)
{
    // 如果任意一个是完整方块，完全遮挡
    if (isBlock(faceShape1) || isBlock(faceShape2)) {
        return true;
    }

    // 如果两个都为空，不遮挡
    if (faceShape1.isEmpty() && faceShape2.isEmpty()) {
        return false;
    }

    // 合并两个面形状，检查是否完全遮挡单位正方形
    // MC Java 使用 joinUnoptimized 而非 or_（避免触发 optimize）
    const VoxelShape merged = joinUnoptimized(faceShape1, faceShape2, BooleanOps::Or());
    return !joinIsNotEmpty(block(), merged, BooleanOps::OnlyFirst());
}

// ============================================================================
// 切片操作
// ============================================================================

VoxelShape Shapes::slice(const VoxelShape& shape, Axis axis, i32 index)
{
    if (shape.isEmpty()) {
        return empty();
    }

    const i32 size = shape.shape().getSize(axis);
    if (index < 0 || index >= size) {
        return empty();
    }

    // 获取原始形状
    const DiscreteVoxelShape& sliceShape = shape.shape();

    // 根据轴设置子形状边界
    i32 startX = 0, startY = 0, startZ = 0;
    i32 endX = sliceShape.getXSize(), endY = sliceShape.getYSize(), endZ = sliceShape.getZSize();

    switch (axis) {
        case Axis::X:
            startX = index;
            endX = index + 1;
            break;
        case Axis::Y:
            startY = index;
            endY = index + 1;
            break;
        case Axis::Z:
            startZ = index;
            endZ = index + 1;
            break;
    }

    // 创建新的离散形状
    auto newShape = std::make_shared<DiscreteVoxelShape>(endX - startX, endY - startY, endZ - startZ);

    for (i32 x = startX; x < endX; ++x) {
        for (i32 y = startY; y < endY; ++y) {
            for (i32 z = startZ; z < endZ; ++z) {
                if (sliceShape.isFull(x, y, z)) {
                    newShape->fill(x - startX, y - startY, z - startZ);
                }
            }
        }
    }

    // 创建坐标点列表
    // 切片轴使用 {0.0, 1.0}，非切片轴使用原始坐标数组
    // 坐标数组大小为 size + 1，因此需要取 endX + 1 (endY + 1, endZ + 1) 个元素
    const std::vector<f64>& xCoords = shape.getCoords(Axis::X);
    const std::vector<f64>& yCoords = shape.getCoords(Axis::Y);
    const std::vector<f64>& zCoords = shape.getCoords(Axis::Z);

    std::vector<f64> newXCoords, newYCoords, newZCoords;
    static const std::vector<f64> sliceCoords = {0.0, 1.0};

    switch (axis) {
        case Axis::X:
            newXCoords = sliceCoords;
            newYCoords.assign(yCoords.begin() + startY, yCoords.begin() + endY + 1);
            newZCoords.assign(zCoords.begin() + startZ, zCoords.begin() + endZ + 1);
            break;
        case Axis::Y:
            newXCoords.assign(xCoords.begin() + startX, xCoords.begin() + endX + 1);
            newYCoords = sliceCoords;
            newZCoords.assign(zCoords.begin() + startZ, zCoords.begin() + endZ + 1);
            break;
        case Axis::Z:
            newXCoords.assign(xCoords.begin() + startX, xCoords.begin() + endX + 1);
            newYCoords.assign(yCoords.begin() + startY, yCoords.begin() + endY + 1);
            newZCoords = sliceCoords;
            break;
    }

    return VoxelShape(newShape, newXCoords, newYCoords, newZCoords);
}

VoxelShape Shapes::createSlice(const VoxelShape& shape, Axis axis, i32 index)
{
    return slice(shape, axis, index);
}

// ============================================================================
// 碰撞检测
// ============================================================================

f64 Shapes::collide(Axis axis, const AxisAlignedBB& entityBox, const std::vector<VoxelShape>& shapes, f64 movement)
{
    for (const VoxelShape& shape : shapes) {
        if (std::abs(movement) < EPSILON) {
            return 0.0;
        }
        movement = shape.collide(axis, entityBox, movement);
    }
    return movement;
}

// ============================================================================
// 辅助函数
// ============================================================================

bool Shapes::isBlock(const VoxelShape& shape)
{
    _ensureInitialized();
    return shape.isCubeLike();
}

bool Shapes::isEmpty(const VoxelShape& shape)
{
    return shape.isEmpty();
}

VoxelShape Shapes::fromCollisionShape(const CollisionShape& shape)
{
    if (shape.isEmpty()) {
        return empty();
    }
    if (shape.isFullBlock()) {
        return block();
    }
    const auto& boxes = shape.boxes();
    if (boxes.empty()) {
        return empty();
    }
    if (boxes.size() == 1) {
        const auto& box = boxes[0];
        return Shapes::box(box.minX, box.minY, box.minZ, box.maxX, box.maxY, box.maxZ);
    }
    // 多碰撞盒：合并所有盒为 VoxelShape
    VoxelShape result =
        Shapes::box(boxes[0].minX, boxes[0].minY, boxes[0].minZ, boxes[0].maxX, boxes[0].maxY, boxes[0].maxZ);
    for (size_t i = 1; i < boxes.size(); ++i) {
        const auto& box = boxes[i];
        VoxelShape part = Shapes::box(box.minX, box.minY, box.minZ, box.maxX, box.maxY, box.maxZ);
        result = Shapes::or_(result, part);
    }
    return result;
}

// ============================================================================
// IndexMerger 创建
// ============================================================================

std::unique_ptr<Shapes::IndexMerger> Shapes::createIndexMerger(
    i32 size, const std::vector<f64>& a, const std::vector<f64>& b, bool aIncluded, bool bIncluded)
{
    const i32 aSegs = static_cast<i32>(a.size()) - 1;
    const i32 bSegs = static_cast<i32>(b.size()) - 1;

    // 检测 CubePointRange
    const i32 aCubeBits = detectCubePointRange(a);
    const i32 bCubeBits = detectCubePointRange(b);

    // 情况 1：两个都是 CubePointRange，且复杂度在预算内
    if (aCubeBits > 0 && bCubeBits > 0) {
        const i64 lcmVal = lcm(static_cast<i64>(aCubeBits), static_cast<i64>(bCubeBits));
        if (static_cast<i64>(size) * lcmVal <= 256LL) {
            return std::make_unique<DiscreteCubeMerger>(aCubeBits, bCubeBits);
        }
    }

    // 情况 2：不重叠（a 完全在 b 之前）
    if (a.back() < b.front() - 1.0E-7) {
        return std::make_unique<NonOverlappingMerger>(std::vector<f64>(a), std::vector<f64>(b), false);
    }

    // 情况 3：不重叠（b 完全在 a 之前）
    if (b.back() < a.front() - 1.0E-7) {
        return std::make_unique<NonOverlappingMerger>(std::vector<f64>(b), std::vector<f64>(a), true);
    }

    // 情况 4：相同的坐标列表
    if (a.size() == b.size()) {
        bool identical = true;
        for (size_t i = 0; i < a.size(); ++i) {
            if (std::abs(a[i] - b[i]) > 1.0E-7) {
                identical = false;
                break;
            }
        }
        if (identical) {
            return std::make_unique<IdenticalMerger>(std::vector<f64>(a));
        }
    }

    // 情况 5：通用归并排序
    return std::make_unique<IndirectMerger>(a, b, aIncluded, bIncluded);
}

} // namespace mc
