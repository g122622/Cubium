#include "Shapes.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

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

void Shapes::ensureInitialized()
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
        {-std::numeric_limits<f64>::infinity(), std::numeric_limits<f64>::infinity()},
        {-std::numeric_limits<f64>::infinity(), std::numeric_limits<f64>::infinity()},
        {-std::numeric_limits<f64>::infinity(), std::numeric_limits<f64>::infinity()});
    s_infinity.m_shape->fill(0, 0, 0);

    s_initialized = true;
}

// ============================================================================
// 基础形状
// ============================================================================

VoxelShape Shapes::empty()
{
    ensureInitialized();
    return s_empty;
}

VoxelShape Shapes::block()
{
    ensureInitialized();
    return s_block;
}

VoxelShape Shapes::infinity()
{
    ensureInitialized();
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

    // 创建立方体坐标点
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
    // 特殊情况处理
    if (!op.apply(false, false)) {
        // FALSE 操作会导致无限体积，不允许
        return empty();
    }

    if (a.isEmpty()) {
        return op.apply(false, true) ? b : empty();
    }

    if (b.isEmpty()) {
        return op.apply(true, false) ? a : empty();
    }

    // 简化实现：收集所有盒子并应用布尔运算
    // 完整实现需要使用 IndexMerger 和 BitSetDiscreteVoxelShape.join

    // 对于 OR 操作，简单合并盒子
    if (op.apply(true, true)) {
        std::vector<AxisAlignedBB> boxes = a.toAabbs();
        std::vector<AxisAlignedBB> bBoxes = b.toAabbs();
        boxes.insert(boxes.end(), bBoxes.begin(), bBoxes.end());

        // 使用第一个盒子作为基础
        if (boxes.empty()) {
            return empty();
        }

        VoxelShape result = create(boxes[0]);
        for (size_t i = 1; i < boxes.size(); ++i) {
            result = or_(result, create(boxes[i]));
        }
        return result;
    }

    // 对于 AND 操作
    if (op.apply(true, true) && !op.apply(true, false) && !op.apply(false, true) && !op.apply(false, false)) {
        // 简化实现：只检查边界相交
        const AxisAlignedBB aBounds = a.bounds();
        const AxisAlignedBB bBounds = b.bounds();

        if (!aBounds.intersects(bBounds)) {
            return empty();
        }

        // 创建相交区域
        const AxisAlignedBB intersection(std::max(aBounds.minX, bBounds.minX),
            std::max(aBounds.minY, bBounds.minY),
            std::max(aBounds.minZ, bBounds.minZ),
            std::min(aBounds.maxX, bBounds.maxX),
            std::min(aBounds.maxY, bBounds.maxY),
            std::min(aBounds.maxZ, bBounds.maxZ));

        return create(intersection);
    }

    // 默认返回空
    return empty();
}

bool Shapes::joinIsNotEmpty(const VoxelShape& a, const VoxelShape& b, const BooleanOp& op)
{
    if (!op.apply(false, false)) {
        return false; // FALSE 操作
    }

    const bool aEmpty = a.isEmpty();
    const bool bEmpty = b.isEmpty();

    if (!aEmpty && !bEmpty) {
        if (&a == &b) {
            return op.apply(true, true);
        }

        const bool aOnly = op.apply(true, false);
        const bool bOnly = op.apply(false, true);

        // 快速边界检查
        for (Axis axis : {Axis::X, Axis::Y, Axis::Z}) {
            if (a.max(axis) < b.min(axis) - EPSILON) {
                return aOnly || bOnly;
            }
            if (b.max(axis) < a.min(axis) - EPSILON) {
                return aOnly || bOnly;
            }
        }

        // 详细检查：遍历所有盒子
        std::vector<AxisAlignedBB> aBoxes = a.toAabbs();
        std::vector<AxisAlignedBB> bBoxes = b.toAabbs();

        for (const auto& boxA : aBoxes) {
            for (const auto& boxB : bBoxes) {
                if (boxA.intersects(boxB)) {
                    if (op.apply(true, true)) {
                        return true;
                    }
                } else {
                    if (op.apply(true, false) || op.apply(false, true)) {
                        return true;
                    }
                }
            }
        }

        return aOnly || bOnly;
    }

    return op.apply(!aEmpty, !bEmpty);
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
    const VoxelShape merged = or_(nearFace, farFace);
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
    const VoxelShape merged = or_(faceShape1, faceShape2);
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
    const std::vector<f64>& xCoords = shape.getCoords(Axis::X);
    const std::vector<f64>& yCoords = shape.getCoords(Axis::Y);
    const std::vector<f64>& zCoords = shape.getCoords(Axis::Z);

    std::vector<f64> newXCoords, newYCoords, newZCoords;
    static const std::vector<f64> sliceCoords = {0.0, 1.0};

    switch (axis) {
        case Axis::X:
            newXCoords = sliceCoords;
            newYCoords.assign(yCoords.begin() + startY, yCoords.begin() + endY);
            newZCoords.assign(zCoords.begin() + startZ, zCoords.begin() + endZ);
            break;
        case Axis::Y:
            newXCoords.assign(xCoords.begin() + startX, xCoords.begin() + endX);
            newYCoords = sliceCoords;
            newZCoords.assign(zCoords.begin() + startZ, zCoords.begin() + endZ);
            break;
        case Axis::Z:
            newXCoords.assign(xCoords.begin() + startX, xCoords.begin() + endX);
            newYCoords.assign(yCoords.begin() + startY, yCoords.begin() + endY);
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
    ensureInitialized();
    return shape.isCubeLike();
}

bool Shapes::isEmpty(const VoxelShape& shape)
{
    return shape.isEmpty();
}

// ============================================================================
// IndexMerger 实现（内部类）
// ============================================================================

namespace {

class SimpleIndexMerger : public Shapes::IndexMerger {
public:
    SimpleIndexMerger(std::vector<f64> list, std::vector<std::pair<i32, i32>> pairs)
        : m_list(std::move(list))
        , m_pairs(std::move(pairs))
    {}

    const std::vector<f64>& getList() const override { return m_list; }

    bool forMergedIndexes(const std::function<bool(i32, i32, i32)>& consumer) const override
    {
        for (size_t i = 0; i < m_pairs.size(); ++i) {
            if (!consumer(m_pairs[i].first, m_pairs[i].second, static_cast<i32>(i))) {
                return false;
            }
        }
        return true;
    }

    i32 size() const override { return static_cast<i32>(m_list.size()); }

private:
    std::vector<f64> m_list;
    std::vector<std::pair<i32, i32>> m_pairs;
};

} // anonymous namespace

std::unique_ptr<Shapes::IndexMerger> Shapes::createIndexMerger(
    i32 size, const std::vector<f64>& a, const std::vector<f64>& b, bool aIncluded, bool bIncluded)
{

    std::set<f64> allCoords;
    for (f64 v : a)
        allCoords.insert(v);
    for (f64 v : b)
        allCoords.insert(v);

    std::vector<f64> merged(allCoords.begin(), allCoords.end());
    std::vector<std::pair<i32, i32>> pairs;

    for (size_t i = 0; i < merged.size(); ++i) {
        i32 idxA = -1, idxB = -1;

        // 找到a中的索引
        for (size_t j = 0; j < a.size() - 1; ++j) {
            if (a[j] <= merged[i] && merged[i] <= a[j + 1]) {
                idxA = static_cast<i32>(j);
                break;
            }
        }

        // 找到b中的索引
        for (size_t j = 0; j < b.size() - 1; ++j) {
            if (b[j] <= merged[i] && merged[i] <= b[j + 1]) {
                idxB = static_cast<i32>(j);
                break;
            }
        }

        pairs.emplace_back(idxA, idxB);
    }

    return std::make_unique<SimpleIndexMerger>(std::move(merged), std::move(pairs));
}

} // namespace mc
