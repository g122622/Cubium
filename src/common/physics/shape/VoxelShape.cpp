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

#include "VoxelShape.hpp"

#include "Shapes.hpp"
#include "common/core/Types.hpp"
#include "common/physics/shape/BooleanOp.hpp"
#include "common/physics/shape/DiscreteVoxelShape.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace mc {

// ============================================================================
// 常量
// ============================================================================

static constexpr f64 EPSILON = 1.0E-7;

// ============================================================================
// 构造函数
// ============================================================================

VoxelShape::VoxelShape()
    : m_shape(std::make_shared<DiscreteVoxelShape>(0, 0, 0))
{
    m_xPoints = {0.0};
    m_yPoints = {0.0};
    m_zPoints = {0.0};
}

VoxelShape::VoxelShape(std::shared_ptr<DiscreteVoxelShape> shape)
    : m_shape(std::move(shape))
{
    if (m_shape) {
        // 创建立方体坐标点（0, 1, 2, ... n）
        const i32 xSize = m_shape->getXSize();
        const i32 ySize = m_shape->getYSize();
        const i32 zSize = m_shape->getZSize();

        m_xPoints.resize(static_cast<size_t>(xSize) + 1);
        m_yPoints.resize(static_cast<size_t>(ySize) + 1);
        m_zPoints.resize(static_cast<size_t>(zSize) + 1);

        for (i32 i = 0; i <= xSize; ++i) {
            m_xPoints[static_cast<size_t>(i)] = static_cast<f64>(i) / static_cast<f64>(xSize);
        }
        for (i32 i = 0; i <= ySize; ++i) {
            m_yPoints[static_cast<size_t>(i)] = static_cast<f64>(i) / static_cast<f64>(ySize);
        }
        for (i32 i = 0; i <= zSize; ++i) {
            m_zPoints[static_cast<size_t>(i)] = static_cast<f64>(i) / static_cast<f64>(zSize);
        }
    }
}

VoxelShape::VoxelShape(std::shared_ptr<DiscreteVoxelShape> shape,
    std::vector<f64> xPoints,
    std::vector<f64> yPoints,
    std::vector<f64> zPoints)
    : m_shape(std::move(shape))
    , m_xPoints(std::move(xPoints))
    , m_yPoints(std::move(yPoints))
    , m_zPoints(std::move(zPoints))
{}

VoxelShape::VoxelShape(const VoxelShape& other)
    : m_shape(other.m_shape)
    , m_xPoints(other.m_xPoints)
    , m_yPoints(other.m_yPoints)
    , m_zPoints(other.m_zPoints)
{
    // 不拷贝面形状缓存
}

VoxelShape& VoxelShape::operator=(const VoxelShape& other)
{
    if (this != &other) {
        m_shape = other.m_shape;
        m_xPoints = other.m_xPoints;
        m_yPoints = other.m_yPoints;
        m_zPoints = other.m_zPoints;
        m_faces.reset();
    }
    return *this;
}

VoxelShape::VoxelShape(VoxelShape&& other) noexcept
    : m_shape(std::move(other.m_shape))
    , m_xPoints(std::move(other.m_xPoints))
    , m_yPoints(std::move(other.m_yPoints))
    , m_zPoints(std::move(other.m_zPoints))
    , m_faces(std::move(other.m_faces))
{}

VoxelShape& VoxelShape::operator=(VoxelShape&& other) noexcept
{
    if (this != &other) {
        m_shape = std::move(other.m_shape);
        m_xPoints = std::move(other.m_xPoints);
        m_yPoints = std::move(other.m_yPoints);
        m_zPoints = std::move(other.m_zPoints);
        m_faces = std::move(other.m_faces);
    }
    return *this;
}

// ============================================================================
// 边界查询
// ============================================================================

f64 VoxelShape::min(Axis axis) const
{
    const i32 first = m_shape->firstFull(axis);
    const i32 size = m_shape->getSize(axis);
    if (first >= size) {
        return Shapes::LARGE_COORDINATE;
    }
    return get(axis, first);
}

f64 VoxelShape::max(Axis axis) const
{
    const i32 last = m_shape->lastFull(axis);
    if (last <= 0) {
        return -Shapes::LARGE_COORDINATE;
    }
    return get(axis, last);
}

AxisAlignedBB VoxelShape::bounds() const
{
    if (isEmpty()) {
        // 抛出异常或返回无效AABB
        return AxisAlignedBB(0, 0, 0, 0, 0, 0);
    }
    return AxisAlignedBB(min(Axis::X), min(Axis::Y), min(Axis::Z), max(Axis::X), max(Axis::Y), max(Axis::Z));
}

VoxelShape VoxelShape::singleEncompassing() const
{
    if (isEmpty()) {
        return Shapes::empty();
    }
    return Shapes::box(min(Axis::X), min(Axis::Y), min(Axis::Z), max(Axis::X), max(Axis::Y), max(Axis::Z));
}

bool VoxelShape::isEmpty() const
{
    return m_shape->isEmpty();
}

// ============================================================================
// 坐标访问
// ============================================================================

const std::vector<f64>& VoxelShape::getCoords(Axis axis) const
{
    switch (axis) {
        case Axis::X:
            return m_xPoints;
        case Axis::Y:
            return m_yPoints;
        case Axis::Z:
            return m_zPoints;
    }
    static const std::vector<f64> empty;
    return empty;
}

f64 VoxelShape::get(Axis axis, i32 index) const
{
    return getCoords(axis)[static_cast<size_t>(index)];
}

// ============================================================================
// 变换
// ============================================================================

VoxelShape VoxelShape::move(f64 dx, f64 dy, f64 dz) const
{
    if (isEmpty()) {
        return Shapes::empty();
    }

    std::vector<f64> newXPoints(m_xPoints.size());
    std::vector<f64> newYPoints(m_yPoints.size());
    std::vector<f64> newZPoints(m_zPoints.size());

    for (size_t i = 0; i < m_xPoints.size(); ++i) {
        newXPoints[i] = m_xPoints[i] + dx;
    }
    for (size_t i = 0; i < m_yPoints.size(); ++i) {
        newYPoints[i] = m_yPoints[i] + dy;
    }
    for (size_t i = 0; i < m_zPoints.size(); ++i) {
        newZPoints[i] = m_zPoints[i] + dz;
    }

    return VoxelShape(m_shape, std::move(newXPoints), std::move(newYPoints), std::move(newZPoints));
}

VoxelShape VoxelShape::move(const Vector3& delta) const
{
    return move(static_cast<f64>(delta.x), static_cast<f64>(delta.y), static_cast<f64>(delta.z));
}

VoxelShape VoxelShape::optimize() const
{
    if (isEmpty()) {
        return Shapes::empty();
    }

    // 收集所有AABB，然后直接创建VoxelShape合并
    // 注意：不能使用 Shapes::or_()，因为那会调用 Shapes::join()，
    // 而 join() 会再次调用 optimize()，导致无限递归。
    // MC Java 版的优化策略：将形状分解为独立的盒子，然后合并。
    // 由于每个盒子已经是简单的形状，合并后的结果是优化过的。
    std::vector<AxisAlignedBB> boxes = toAabbs();
    if (boxes.empty()) {
        return Shapes::empty();
    }

    // 单个盒子的情况，直接返回优化过的形状
    if (boxes.size() == 1) {
        const auto& b = boxes[0];
        return Shapes::create(b.minX, b.minY, b.minZ, b.maxX, b.maxY, b.maxZ);
    }

    // 多个盒子：使用 joinUnoptimized 直接合并（跳过 optimize 步骤避免递归）
    // 然后对结果再次 optimize（此时递归会终止，因为合并后的盒子数不会增加）
    VoxelShape result = Shapes::create(boxes[0]);
    for (size_t i = 1; i < boxes.size(); ++i) {
        VoxelShape boxShape = Shapes::create(boxes[i]);
        // 直接调用 joinUnoptimized 避免递归，然后手动 optimize
        result = Shapes::joinUnoptimized(result, boxShape, BooleanOps::Or());
    }

    // After merging, check if simplification occurred (fewer boxes)
    // If so, recursively optimize. If not, we have reached a fixed point.
    std::vector<AxisAlignedBB> newBoxes = result.toAabbs();
    if (newBoxes.size() < boxes.size()) {
        return result.optimize();
    }
    return result;
}

// ============================================================================
// 遍历
// ============================================================================

void VoxelShape::forAllEdges(const DoubleLineConsumer& consumer)
{
    m_shape->forAllEdges(
        [this, &consumer](i32 x1, i32 y1, i32 z1, i32 x2, i32 y2, i32 z2) {
            consumer(get(Axis::X, x1),
                get(Axis::Y, y1),
                get(Axis::Z, z1),
                get(Axis::X, x2),
                get(Axis::Y, y2),
                get(Axis::Z, z2));
        },
        true);
}

void VoxelShape::forAllBoxes(const DoubleLineConsumer& consumer) const
{
    m_shape->forAllBoxes(
        [this, &consumer](i32 x1, i32 y1, i32 z1, i32 x2, i32 y2, i32 z2) {
            consumer(get(Axis::X, x1),
                get(Axis::Y, y1),
                get(Axis::Z, z1),
                get(Axis::X, x2),
                get(Axis::Y, y2),
                get(Axis::Z, z2));
        },
        true);
}

std::vector<AxisAlignedBB> VoxelShape::toAabbs() const
{
    std::vector<AxisAlignedBB> result;
    forAllBoxes(
        [&result](f64 x1, f64 y1, f64 z1, f64 x2, f64 y2, f64 z2) { result.emplace_back(x1, y1, z1, x2, y2, z2); });
    return result;
}

// ============================================================================
// 面形状
// ============================================================================

VoxelShape VoxelShape::getFaceShape(Direction dir) const
{
    if (!isEmpty() && !Shapes::isBlock(*this)) {
        if (!m_faces) {
            m_faces = std::make_unique<VoxelShape[]>(6);
        }

        const size_t idx = ordinal(dir);
        if (!m_faces[idx].isEmpty() || m_faces[idx].m_shape->getXSize() > 0) {
            return m_faces[idx];
        }

        m_faces[idx] = _calculateFace(dir);
        return m_faces[idx];
    }

    return *this;
}

VoxelShape VoxelShape::_calculateFace(Direction dir) const
{
    const Axis axis = Directions::getAxis(dir);
    if (isCubeLikeAlong(axis)) {
        return *this;
    }

    const AxisDirection axisDir = Directions::getAxisDirection(dir);
    const f64 testCoord = (axisDir == AxisDirection::Positive) ? 0.9999999 : 1.0E-7;
    const i32 sliceIndex = findIndex(axis, testCoord);

    // 创建切片形状
    return Shapes::slice(*this, axis, sliceIndex);
}

bool VoxelShape::isCubeLikeAlong(Axis axis) const
{
    const std::vector<f64>& coords = getCoords(axis);
    return coords.size() == 2 && std::abs(coords[0]) < EPSILON && std::abs(coords[1] - 1.0) < EPSILON;
}

bool VoxelShape::isCubeLike() const
{
    return isCubeLikeAlong(Axis::X) && isCubeLikeAlong(Axis::Y) && isCubeLikeAlong(Axis::Z);
}

// ============================================================================
// 碰撞检测
// ============================================================================

namespace {

// 辅助函数：获取AABB在指定轴上的最小值
f64 getAABBMin(const AxisAlignedBB& box, Axis axis)
{
    switch (axis) {
        case Axis::X:
            return box.minX;
        case Axis::Y:
            return box.minY;
        case Axis::Z:
            return box.minZ;
    }
    return 0.0;
}

// 辅助函数：获取AABB在指定轴上的最大值
f64 getAABBMax(const AxisAlignedBB& box, Axis axis)
{
    switch (axis) {
        case Axis::X:
            return box.maxX;
        case Axis::Y:
            return box.maxY;
        case Axis::Z:
            return box.maxZ;
    }
    return 0.0;
}

} // anonymous namespace

f64 VoxelShape::collide(Axis axis, const AxisAlignedBB& entityBox, f64 movement) const
{
    return _collideX(AxisCycles::between(axis, Axis::X), entityBox, movement);
}

f64 VoxelShape::_collideX(AxisCycle cycle, const AxisAlignedBB& entityBox, f64 movement) const
{
    if (isEmpty()) {
        return movement;
    }
    if (std::abs(movement) < EPSILON) {
        return 0.0;
    }

    const AxisCycle invCycle = AxisCycles::inverse(cycle);
    const Axis axis = AxisCycles::cycle(invCycle, Axis::X);
    const Axis axis1 = AxisCycles::cycle(invCycle, Axis::Y);
    const Axis axis2 = AxisCycles::cycle(invCycle, Axis::Z);

    const f64 maxVal = getAABBMax(entityBox, axis);
    const f64 minVal = getAABBMin(entityBox, axis);

    const i32 startIndex = std::max(0, findIndex(axis, minVal + 1.0E-7));
    const i32 endIndex = std::min(m_shape->getSize(axis), findIndex(axis, maxVal - 1.0E-7) + 1);

    const i32 min1 = std::max(0, findIndex(axis1, getAABBMin(entityBox, axis1) + 1.0E-7));
    const i32 max1 = std::min(m_shape->getSize(axis1), findIndex(axis1, getAABBMax(entityBox, axis1) - 1.0E-7) + 1);

    const i32 min2 = std::max(0, findIndex(axis2, getAABBMin(entityBox, axis2) + 1.0E-7));
    const i32 max2 = std::min(m_shape->getSize(axis2), findIndex(axis2, getAABBMax(entityBox, axis2) - 1.0E-7) + 1);

    const i32 size = m_shape->getSize(axis);

    if (movement > 0.0) {
        for (i32 i = endIndex; i < size; ++i) {
            for (i32 j = min1; j < max1; ++j) {
                for (i32 k = min2; k < max2; ++k) {
                    if (m_shape->isFullWide(invCycle, i, j, k)) {
                        const f64 diff = get(axis, i) - maxVal;
                        if (diff >= -EPSILON) {
                            movement = std::min(movement, diff);
                        }
                        return movement;
                    }
                }
            }
        }
    } else if (movement < 0.0) {
        for (i32 i = startIndex - 1; i >= 0; --i) {
            for (i32 j = min1; j < max1; ++j) {
                for (i32 k = min2; k < max2; ++k) {
                    if (m_shape->isFullWide(invCycle, i, j, k)) {
                        const f64 diff = get(axis, i + 1) - minVal;
                        if (diff <= EPSILON) {
                            movement = std::max(movement, diff);
                        }
                        return movement;
                    }
                }
            }
        }
    }

    return movement;
}

// ============================================================================
// 形状比较
// ============================================================================

bool VoxelShape::equal(const VoxelShape& a, const VoxelShape& b)
{
    return !Shapes::joinIsNotEmpty(a, b, BooleanOps::NotSame());
}

// ============================================================================
// 索引查找
// ============================================================================

i32 VoxelShape::findIndex(Axis axis, f64 coord) const
{
    const std::vector<f64>& coords = getCoords(axis);
    const i32 size = m_shape->getSize(axis);

    // 二分查找
    i32 low = 0;
    i32 high = size + 1;

    while (low < high) {
        const i32 mid = (low + high) / 2;
        if (coord < get(axis, mid)) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }

    return low - 1;
}

f64 VoxelShape::min(Axis axis, f64 coord1, f64 coord2) const
{
    const Axis axis1 = AxisCycles::cycle(AxisCycle::FORWARD, axis);
    const Axis axis2 = AxisCycles::cycle(AxisCycle::BACKWARD, axis);

    const i32 index1 = findIndex(axis1, coord1);
    const i32 index2 = findIndex(axis2, coord2);

    const i32 first = m_shape->firstFull(axis, index1, index2);
    if (first >= m_shape->getSize(axis)) {
        return Shapes::LARGE_COORDINATE;
    }
    return get(axis, first);
}

f64 VoxelShape::max(Axis axis, f64 coord1, f64 coord2) const
{
    const Axis axis1 = AxisCycles::cycle(AxisCycle::FORWARD, axis);
    const Axis axis2 = AxisCycles::cycle(AxisCycle::BACKWARD, axis);

    const i32 index1 = findIndex(axis1, coord1);
    const i32 index2 = findIndex(axis2, coord2);

    const i32 last = m_shape->lastFull(axis, index1, index2);
    if (last <= 0) {
        return -Shapes::LARGE_COORDINATE;
    }
    return get(axis, last);
}

// ============================================================================
// 光线投射
// ============================================================================

std::optional<BlockHitResult> VoxelShape::clip(const Vector3& start, const Vector3& end, const BlockPos& offset) const
{
    if (isEmpty()) {
        return std::nullopt;
    }

    const Vector3 diff = end - start;
    if (diff.lengthSquared() < EPSILON * EPSILON) {
        return std::nullopt;
    }

    // 轻微偏移起点
    const Vector3 adjustedStart = start + diff * 0.001;

    // 检查起点是否在形状内
    const i32 xi = findIndex(Axis::X, adjustedStart.x - static_cast<f64>(offset.x));
    const i32 yi = findIndex(Axis::Y, adjustedStart.y - static_cast<f64>(offset.y));
    const i32 zi = findIndex(Axis::Z, adjustedStart.z - static_cast<f64>(offset.z));

    if (m_shape->isFullWide(xi, yi, zi)) {
        const Direction dir = Directions::opposite(
            Directions::fromVector(static_cast<f32>(diff.x), static_cast<f32>(diff.y), static_cast<f32>(diff.z)));
        return BlockHitResult(adjustedStart, dir, offset, true);
    }

    // 对所有AABB进行射线检测
    std::vector<AxisAlignedBB> boxes = toAabbs();
    f64 closestT = 2.0; // 超过1表示未命中
    Direction hitDir = Direction::None;

    for (const auto& box : boxes) {
        // 将盒子移动到世界坐标
        AxisAlignedBB worldBox(static_cast<f32>(box.minX + offset.x),
            static_cast<f32>(box.minY + offset.y),
            static_cast<f32>(box.minZ + offset.z),
            static_cast<f32>(box.maxX + offset.x),
            static_cast<f32>(box.maxY + offset.y),
            static_cast<f32>(box.maxZ + offset.z));

        // 计算射线与盒子的交点
        // 使用 slab 方法
        f64 tMin = 0.0;
        f64 tMax = 1.0;
        Direction tMinDir = Direction::None;

        // X轴
        if (std::abs(diff.x) > EPSILON) {
            f64 t1 = (worldBox.minX - start.x) / diff.x;
            f64 t2 = (worldBox.maxX - start.x) / diff.x;
            Direction d1 = Direction::West;
            Direction d2 = Direction::East;
            if (t1 > t2) {
                std::swap(t1, t2);
                std::swap(d1, d2);
            }
            if (t1 > tMin) {
                tMin = t1;
                tMinDir = d1;
            }
            tMax = std::min(tMax, t2);
        } else {
            if (start.x < worldBox.minX || start.x > worldBox.maxX) continue;
        }

        // Y轴
        if (std::abs(diff.y) > EPSILON) {
            f64 t1 = (worldBox.minY - start.y) / diff.y;
            f64 t2 = (worldBox.maxY - start.y) / diff.y;
            Direction d1 = Direction::Down;
            Direction d2 = Direction::Up;
            if (t1 > t2) {
                std::swap(t1, t2);
                std::swap(d1, d2);
            }
            if (t1 > tMin) {
                tMin = t1;
                tMinDir = d1;
            }
            tMax = std::min(tMax, t2);
        } else {
            if (start.y < worldBox.minY || start.y > worldBox.maxY) continue;
        }

        // Z轴
        if (std::abs(diff.z) > EPSILON) {
            f64 t1 = (worldBox.minZ - start.z) / diff.z;
            f64 t2 = (worldBox.maxZ - start.z) / diff.z;
            Direction d1 = Direction::North;
            Direction d2 = Direction::South;
            if (t1 > t2) {
                std::swap(t1, t2);
                std::swap(d1, d2);
            }
            if (t1 > tMin) {
                tMin = t1;
                tMinDir = d1;
            }
            tMax = std::min(tMax, t2);
        } else {
            if (start.z < worldBox.minZ || start.z > worldBox.maxZ) continue;
        }

        // 检查是否命中
        if (tMin <= tMax && tMin >= 0.0 && tMin <= 1.0) {
            if (tMin < closestT) {
                closestT = tMin;
                hitDir = tMinDir;
            }
        }
    }

    if (closestT <= 1.0) {
        Vector3 hitPoint(start.x + static_cast<f32>(diff.x * closestT),
            start.y + static_cast<f32>(diff.y * closestT),
            start.z + static_cast<f32>(diff.z * closestT));
        return BlockHitResult(hitPoint, hitDir, offset, false);
    }

    return std::nullopt;
}

// ============================================================================
// 最近点
// ============================================================================

std::optional<Vector3> VoxelShape::closestPointTo(const Vector3& point) const
{
    if (isEmpty()) {
        return std::nullopt;
    }

    std::optional<Vector3> closest;
    f64 closestDistSq = std::numeric_limits<f64>::max();

    forAllBoxes([&](f64 x1, f64 y1, f64 z1, f64 x2, f64 y2, f64 z2) {
        const f64 px = std::clamp(static_cast<f64>(point.x), x1, x2);
        const f64 py = std::clamp(static_cast<f64>(point.y), y1, y2);
        const f64 pz = std::clamp(static_cast<f64>(point.z), z1, z2);

        const f64 distSq =
            point.distanceSquared(Vector3(static_cast<f32>(px), static_cast<f32>(py), static_cast<f32>(pz)));

        if (distSq < closestDistSq) {
            closestDistSq = distSq;
            closest = Vector3(static_cast<f32>(px), static_cast<f32>(py), static_cast<f32>(pz));
        }
    });

    return closest;
}

// ============================================================================
// 点包含检测
// ============================================================================

bool VoxelShape::contains(f64 x, f64 y, f64 z) const
{
    if (isEmpty()) {
        return false;
    }

    const i32 xi = findIndex(Axis::X, x);
    const i32 yi = findIndex(Axis::Y, y);
    const i32 zi = findIndex(Axis::Z, z);

    return m_shape->isFullWide(xi, yi, zi);
}

// ============================================================================
// 内部方法
// ============================================================================

bool VoxelShape::_isCubePointRange(Axis axis) const
{
    const std::vector<f64>& coords = getCoords(axis);
    return coords.size() == 2 && std::abs(coords[0]) < EPSILON && std::abs(coords[1] - 1.0) < EPSILON;
}

void VoxelShape::_initFaceCache() const
{
    if (!m_faces) {
        m_faces = std::make_unique<VoxelShape[]>(6);
    }
}

} // namespace mc
