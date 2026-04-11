#include "DiscreteVoxelShape.hpp"
#include <algorithm>
#include <cmath>

namespace mc {

// ============================================================================
// 构造函数
// ============================================================================

DiscreteVoxelShape::DiscreteVoxelShape()
    : m_xSize(0), m_ySize(0), m_zSize(0) {
}

DiscreteVoxelShape::DiscreteVoxelShape(i32 xSize, i32 ySize, i32 zSize)
    : m_xSize(std::max(0, xSize)),
      m_ySize(std::max(0, ySize)),
      m_zSize(std::max(0, zSize)) {
    if (m_xSize > 0 && m_ySize > 0 && m_zSize > 0) {
        m_storage.resize(static_cast<size_t>(m_xSize) * m_ySize * m_zSize, false);
    }
    m_xMin = m_xSize;
    m_yMin = m_ySize;
    m_zMin = m_zSize;
    m_xMax = 0;
    m_yMax = 0;
    m_zMax = 0;
    m_boundsDirty = false;
}

DiscreteVoxelShape::DiscreteVoxelShape(const DiscreteVoxelShape& other)
    : m_xSize(other.m_xSize),
      m_ySize(other.m_ySize),
      m_zSize(other.m_zSize),
      m_storage(other.m_storage),
      m_xMin(other.m_xMin),
      m_yMin(other.m_yMin),
      m_zMin(other.m_zMin),
      m_xMax(other.m_xMax),
      m_yMax(other.m_yMax),
      m_zMax(other.m_zMax),
      m_boundsDirty(other.m_boundsDirty) {
}

DiscreteVoxelShape& DiscreteVoxelShape::operator=(const DiscreteVoxelShape& other) {
    if (this != &other) {
        m_xSize = other.m_xSize;
        m_ySize = other.m_ySize;
        m_zSize = other.m_zSize;
        m_storage = other.m_storage;
        m_xMin = other.m_xMin;
        m_yMin = other.m_yMin;
        m_zMin = other.m_zMin;
        m_xMax = other.m_xMax;
        m_yMax = other.m_yMax;
        m_zMax = other.m_zMax;
        m_boundsDirty = other.m_boundsDirty;
    }
    return *this;
}

DiscreteVoxelShape::DiscreteVoxelShape(DiscreteVoxelShape&& other) noexcept
    : m_xSize(other.m_xSize),
      m_ySize(other.m_ySize),
      m_zSize(other.m_zSize),
      m_storage(std::move(other.m_storage)),
      m_xMin(other.m_xMin),
      m_yMin(other.m_yMin),
      m_zMin(other.m_zMin),
      m_xMax(other.m_xMax),
      m_yMax(other.m_yMax),
      m_zMax(other.m_zMax),
      m_boundsDirty(other.m_boundsDirty) {
    other.m_xSize = 0;
    other.m_ySize = 0;
    other.m_zSize = 0;
}

DiscreteVoxelShape& DiscreteVoxelShape::operator=(DiscreteVoxelShape&& other) noexcept {
    if (this != &other) {
        m_xSize = other.m_xSize;
        m_ySize = other.m_ySize;
        m_zSize = other.m_zSize;
        m_storage = std::move(other.m_storage);
        m_xMin = other.m_xMin;
        m_yMin = other.m_yMin;
        m_zMin = other.m_zMin;
        m_xMax = other.m_xMax;
        m_yMax = other.m_yMax;
        m_zMax = other.m_zMax;
        m_boundsDirty = other.m_boundsDirty;
        other.m_xSize = 0;
        other.m_ySize = 0;
        other.m_zSize = 0;
    }
    return *this;
}

// ============================================================================
// 尺寸查询
// ============================================================================

i32 DiscreteVoxelShape::getSize(Axis axis) const {
    switch (axis) {
        case Axis::X: return m_xSize;
        case Axis::Y: return m_ySize;
        case Axis::Z: return m_zSize;
    }
    return 0;
}

// ============================================================================
// 体素操作
// ============================================================================

bool DiscreteVoxelShape::isFull(i32 x, i32 y, i32 z) const {
    if (x < 0 || x >= m_xSize || y < 0 || y >= m_ySize || z < 0 || z >= m_zSize) {
        return false;
    }
    return m_storage[static_cast<size_t>(getIndex(x, y, z))];
}

bool DiscreteVoxelShape::isFullWide(i32 x, i32 y, i32 z) const {
    if (x < 0 || y < 0 || z < 0) {
        return false;
    }
    if (x >= m_xSize || y >= m_ySize || z >= m_zSize) {
        return false;
    }
    return m_storage[static_cast<size_t>(getIndex(x, y, z))];
}

bool DiscreteVoxelShape::isFullWide(AxisCycle cycle, i32 x, i32 y, i32 z) const {
    return isFullWide(
        AxisCycles::cycle(cycle, x, y, z, Axis::X),
        AxisCycles::cycle(cycle, x, y, z, Axis::Y),
        AxisCycles::cycle(cycle, x, y, z, Axis::Z)
    );
}

bool DiscreteVoxelShape::isFull(AxisCycle cycle, i32 x, i32 y, i32 z) const {
    return isFull(
        AxisCycles::cycle(cycle, x, y, z, Axis::X),
        AxisCycles::cycle(cycle, x, y, z, Axis::Y),
        AxisCycles::cycle(cycle, x, y, z, Axis::Z)
    );
}

void DiscreteVoxelShape::fill(i32 x, i32 y, i32 z) {
    fillUpdateBounds(x, y, z, true);
}

void DiscreteVoxelShape::clear(i32 x, i32 y, i32 z) {
    if (x >= 0 && x < m_xSize && y >= 0 && y < m_ySize && z >= 0 && z < m_zSize) {
        m_storage[static_cast<size_t>(getIndex(x, y, z))] = false;
        m_boundsDirty = true;
    }
}

bool DiscreteVoxelShape::isEmpty() const {
    if (m_boundsDirty) {
        recalculateBounds();
    }
    // 如果边界缓存显示范围无效，则为空
    return m_xMin >= m_xMax || m_yMin >= m_yMax || m_zMin >= m_zMax;
}

// ============================================================================
// 边界查询
// ============================================================================

i32 DiscreteVoxelShape::firstFull(Axis axis) const {
    if (m_boundsDirty) {
        recalculateBounds();
    }
    switch (axis) {
        case Axis::X: return m_xMin;
        case Axis::Y: return m_yMin;
        case Axis::Z: return m_zMin;
    }
    return 0;
}

i32 DiscreteVoxelShape::lastFull(Axis axis) const {
    if (m_boundsDirty) {
        recalculateBounds();
    }
    switch (axis) {
        case Axis::X: return m_xMax;
        case Axis::Y: return m_yMax;
        case Axis::Z: return m_zMax;
    }
    return 0;
}

i32 DiscreteVoxelShape::firstFull(Axis axis, i32 slice1, i32 slice2) const {
    const i32 size = getSize(axis);
    if (slice1 < 0 || slice2 < 0) {
        return size;
    }

    const Axis axis1 = AxisCycles::cycle(AxisCycle::FORWARD, axis);
    const Axis axis2 = AxisCycles::cycle(AxisCycle::BACKWARD, axis);

    if (slice1 >= getSize(axis1) || slice2 >= getSize(axis2)) {
        return size;
    }

    const AxisCycle cycle = AxisCycles::between(Axis::X, axis);

    for (i32 i = 0; i < size; ++i) {
        if (isFull(cycle, i, slice1, slice2)) {
            return i;
        }
    }

    return size;
}

i32 DiscreteVoxelShape::lastFull(Axis axis, i32 slice1, i32 slice2) const {
    if (slice1 < 0 || slice2 < 0) {
        return 0;
    }

    const Axis axis1 = AxisCycles::cycle(AxisCycle::FORWARD, axis);
    const Axis axis2 = AxisCycles::cycle(AxisCycle::BACKWARD, axis);

    if (slice1 >= getSize(axis1) || slice2 >= getSize(axis2)) {
        return 0;
    }

    const i32 size = getSize(axis);
    const AxisCycle cycle = AxisCycles::between(Axis::X, axis);

    for (i32 i = size - 1; i >= 0; --i) {
        if (isFull(cycle, i, slice1, slice2)) {
            return i + 1;
        }
    }

    return 0;
}

// ============================================================================
// 遍历
// ============================================================================

void DiscreteVoxelShape::forAllEdges(const IntLineConsumer& consumer, bool simplify) {
    forAllAxisEdges(consumer, AxisCycle::NONE, simplify);
    forAllAxisEdges(consumer, AxisCycle::FORWARD, simplify);
    forAllAxisEdges(consumer, AxisCycle::BACKWARD, simplify);
}

void DiscreteVoxelShape::forAllBoxes(const IntLineConsumer& consumer, bool simplify) {
    // 简化版本：遍历所有体素并生成盒子
    // 完整实现应该合并相邻体素

    if (simplify) {
        // TODO: 实现完整的盒子合并算法
        // 参考MC BitSetDiscreteVoxelShape.forAllBoxes
    }

    // 简单实现：每个填充的体素生成一个1x1x1盒子
    for (i32 x = 0; x < m_xSize; ++x) {
        for (i32 y = 0; y < m_ySize; ++y) {
            for (i32 z = 0; z < m_zSize; ++z) {
                if (isFull(x, y, z)) {
                    consumer(x, y, z, x + 1, y + 1, z + 1);
                }
            }
        }
    }
}

void DiscreteVoxelShape::forAllFaces(const IntFaceConsumer& consumer) {
    forAllAxisFaces(consumer, AxisCycle::NONE);
    forAllAxisFaces(consumer, AxisCycle::FORWARD);
    forAllAxisFaces(consumer, AxisCycle::BACKWARD);
}

// ============================================================================
// 填充
// ============================================================================

void DiscreteVoxelShape::fillAll() {
    std::fill(m_storage.begin(), m_storage.end(), true);
    m_xMin = 0;
    m_yMin = 0;
    m_zMin = 0;
    m_xMax = m_xSize;
    m_yMax = m_ySize;
    m_zMax = m_zSize;
    m_boundsDirty = false;
}

void DiscreteVoxelShape::fillRange(i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ) {
    for (i32 x = minX; x < maxX; ++x) {
        for (i32 y = minY; y < maxY; ++y) {
            for (i32 z = minZ; z < maxZ; ++z) {
                fillUpdateBounds(x, y, z, false);
            }
        }
    }
    // 更新边界
    m_xMin = std::min(m_xMin, minX);
    m_yMin = std::min(m_yMin, minY);
    m_zMin = std::min(m_zMin, minZ);
    m_xMax = std::max(m_xMax, maxX);
    m_yMax = std::max(m_yMax, maxY);
    m_zMax = std::max(m_zMax, maxZ);
}

// ============================================================================
// 工厂方法
// ============================================================================

DiscreteVoxelShape DiscreteVoxelShape::withFilledBounds(
    i32 xSize, i32 ySize, i32 zSize,
    i32 minX, i32 minY, i32 minZ,
    i32 maxX, i32 maxY, i32 maxZ) {

    DiscreteVoxelShape shape(xSize, ySize, zSize);
    shape.fillRange(minX, minY, minZ, maxX, maxY, maxZ);
    return shape;
}

// ============================================================================
// 内部方法
// ============================================================================

i32 DiscreteVoxelShape::getIndex(i32 x, i32 y, i32 z) const {
    return (x * m_ySize + y) * m_zSize + z;
}

void DiscreteVoxelShape::updateBounds(i32 x, i32 y, i32 z) {
    m_xMin = std::min(m_xMin, x);
    m_yMin = std::min(m_yMin, y);
    m_zMin = std::min(m_zMin, z);
    m_xMax = std::max(m_xMax, x + 1);
    m_yMax = std::max(m_yMax, y + 1);
    m_zMax = std::max(m_zMax, z + 1);
}

void DiscreteVoxelShape::fillUpdateBounds(i32 x, i32 y, i32 z, bool updateBounds) {
    if (x >= 0 && x < m_xSize && y >= 0 && y < m_ySize && z >= 0 && z < m_zSize) {
        m_storage[static_cast<size_t>(getIndex(x, y, z))] = true;
        if (updateBounds) {
            this->updateBounds(x, y, z);
        }
    }
}

void DiscreteVoxelShape::recalculateBounds() const {
    m_xMin = m_xSize;
    m_yMin = m_ySize;
    m_zMin = m_zSize;
    m_xMax = 0;
    m_yMax = 0;
    m_zMax = 0;

    for (i32 x = 0; x < m_xSize; ++x) {
        for (i32 y = 0; y < m_ySize; ++y) {
            for (i32 z = 0; z < m_zSize; ++z) {
                if (m_storage[static_cast<size_t>(getIndex(x, y, z))]) {
                    m_xMin = std::min(m_xMin, x);
                    m_yMin = std::min(m_yMin, y);
                    m_zMin = std::min(m_zMin, z);
                    m_xMax = std::max(m_xMax, x + 1);
                    m_yMax = std::max(m_yMax, y + 1);
                    m_zMax = std::max(m_zMax, z + 1);
                }
            }
        }
    }

    m_boundsDirty = false;
}

void DiscreteVoxelShape::forAllAxisEdges(const IntLineConsumer& consumer, AxisCycle cycle, bool simplify) {
    const AxisCycle invCycle = AxisCycles::inverse(cycle);
    const i32 jSize = getSize(AxisCycles::cycle(invCycle, Axis::X));
    const i32 kSize = getSize(AxisCycles::cycle(invCycle, Axis::Y));
    const i32 lSize = getSize(AxisCycles::cycle(invCycle, Axis::Z));

    for (i32 i1 = 0; i1 <= jSize; ++i1) {
        for (i32 j1 = 0; j1 <= kSize; ++j1) {
            i32 start = -1;

            for (i32 k1 = 0; k1 <= lSize; ++k1) {
                i32 count = 0;
                i32 xorSum = 0;

                for (i32 dj = 0; dj <= 1; ++dj) {
                    for (i32 dk = 0; dk <= 1; ++dk) {
                        if (isFullWide(invCycle, i1 + dj - 1, j1 + dk - 1, k1)) {
                            ++count;
                            xorSum ^= dj ^ dk;
                        }
                    }
                }

                // 检查是否是边的端点
                if (count == 1 || count == 3 || (count == 2 && (xorSum & 1) == 0)) {
                    if (simplify) {
                        if (start == -1) {
                            start = k1;
                        }
                    } else {
                        consumer(
                            AxisCycles::cycle(cycle, i1, j1, k1, Axis::X),
                            AxisCycles::cycle(cycle, i1, j1, k1, Axis::Y),
                            AxisCycles::cycle(cycle, i1, j1, k1, Axis::Z),
                            AxisCycles::cycle(cycle, i1, j1, k1 + 1, Axis::X),
                            AxisCycles::cycle(cycle, i1, j1, k1 + 1, Axis::Y),
                            AxisCycles::cycle(cycle, i1, j1, k1 + 1, Axis::Z)
                        );
                    }
                } else if (start != -1) {
                    consumer(
                        AxisCycles::cycle(cycle, i1, j1, start, Axis::X),
                        AxisCycles::cycle(cycle, i1, j1, start, Axis::Y),
                        AxisCycles::cycle(cycle, i1, j1, start, Axis::Z),
                        AxisCycles::cycle(cycle, i1, j1, k1, Axis::X),
                        AxisCycles::cycle(cycle, i1, j1, k1, Axis::Y),
                        AxisCycles::cycle(cycle, i1, j1, k1, Axis::Z)
                    );
                    start = -1;
                }
            }
        }
    }
}

void DiscreteVoxelShape::forAllAxisFaces(const IntFaceConsumer& consumer, AxisCycle cycle) {
    const AxisCycle invCycle = AxisCycles::inverse(cycle);
    const Axis zAxis = AxisCycles::cycle(invCycle, Axis::Z);
    const i32 iSize = getSize(AxisCycles::cycle(invCycle, Axis::X));
    const i32 jSize = getSize(AxisCycles::cycle(invCycle, Axis::Y));
    const i32 kSize = getSize(zAxis);

    const Direction negDir = Directions::fromAxisAndDirection(zAxis, AxisDirection::Negative);
    const Direction posDir = Directions::fromAxisAndDirection(zAxis, AxisDirection::Positive);

    for (i32 i = 0; i < iSize; ++i) {
        for (i32 j = 0; j < jSize; ++j) {
            bool wasFull = false;

            for (i32 k = 0; k <= kSize; ++k) {
                const bool isFull = (k != kSize) && DiscreteVoxelShape::isFull(invCycle, i, j, k);

                if (!wasFull && isFull) {
                    // 开始的面（负方向）
                    consumer(
                        negDir,
                        AxisCycles::cycle(cycle, i, j, k, Axis::X),
                        AxisCycles::cycle(cycle, i, j, k, Axis::Y),
                        AxisCycles::cycle(cycle, i, j, k, Axis::Z)
                    );
                }

                if (wasFull && !isFull) {
                    // 结束的面（正方向）
                    consumer(
                        posDir,
                        AxisCycles::cycle(cycle, i, j, k - 1, Axis::X),
                        AxisCycles::cycle(cycle, i, j, k - 1, Axis::Y),
                        AxisCycles::cycle(cycle, i, j, k - 1, Axis::Z)
                    );
                }

                wasFull = isFull;
            }
        }
    }
}

} // namespace mc
