#pragma once

#include "common/core/Types.hpp"
#include "common/util/Direction.hpp" // Rotation
#include "common/world/block/BlockPos.hpp"

namespace mc::math {
template <typename>
class Vector3;
using Vector3d = Vector3<f64>;
} // namespace mc::math

namespace mc::test {

/**
 * @brief 结构相对坐标 ↔ 世界绝对坐标的变换工具。
 *
 * 持有三个量：结构放置原点（世界绝对方块坐标 `m_origin`）、结构尺寸（`m_size`，来自
 * `Template::getSize()`）、施加的旋转（`m_rotation`）。提供 `relativeToWorld`/`worldToRelative`
 * 两个方向的整数 BlockPos 变换，以及 `relativeToWorldF`/`worldToRelativeF` 浮点 Vector3 变换
 * （供实体位置用）。
 *
 * 旋转语义对齐 Java `StructureTemplate`/`PlacementSettings`：以结构尺寸的局部坐标系绕 Y 轴顺时针旋转，
 * 相对坐标 (rx, ry, rz) 在旋转 r 后映射到新的局部坐标，再加原点得到世界坐标。具体公式：
 * - None:        (rx, ry, rz)
 * - Clockwise90: (sizeZ-1-rz, ry, rx)
 * - Clockwise180:(sizeX-1-rx, ry, sizeZ-1-rz)
 * - Cw270(Ccw90):(rz, ry, sizeX-1-rx)
 *
 * 这是 `GameTestHelper::worldPosition`/`relativePosition` 与 `MinecraftStructurePlacer` 计算
 * 旋转后包围盒的基础工具，放 base/coords/ 作为依赖链末梢。
 */
class TestTransform {
public:
    TestTransform() noexcept = default;
    TestTransform(BlockPos origin, BlockPos size, Rotation rotation) noexcept
        : m_origin(origin)
        , m_size(size)
        , m_rotation(rotation)
    {}

    [[nodiscard]] const BlockPos& origin() const noexcept { return m_origin; }
    [[nodiscard]] const BlockPos& size() const noexcept { return m_size; }
    [[nodiscard]] Rotation rotation() const noexcept { return m_rotation; }

    void setOrigin(BlockPos origin) noexcept { m_origin = origin; }
    void setSize(BlockPos size) noexcept { m_size = size; }
    void setRotation(Rotation rotation) noexcept { m_rotation = rotation; }

    /**
     * @brief 相对方块坐标 → 世界绝对方块坐标。
     */
    [[nodiscard]] BlockPos relativeToWorld(BlockPos relativePos) const noexcept
    {
        const BlockPos rotated = rotateRelative(relativePos);
        return BlockPos{m_origin.x + rotated.x, m_origin.y + rotated.y, m_origin.z + rotated.z};
    }

    /**
     * @brief 世界绝对方块坐标 → 相对方块坐标。
     */
    [[nodiscard]] BlockPos worldToRelative(BlockPos worldPos) const noexcept
    {
        const BlockPos local{worldPos.x - m_origin.x, worldPos.y - m_origin.y, worldPos.z - m_origin.z};
        return unrotateRelative(local);
    }

    /**
     * @brief 相对浮点坐标 → 世界绝对浮点坐标（供实体位置用）。
     *
     * 浮点旋转不取 size-1 偏移（实体坐标是连续的，结构局部坐标 [0,size] 而非 [0,size-1]）。
     */
    [[nodiscard]] math::Vector3d relativeToWorldF(math::Vector3d relativePos) const noexcept;

    /**
     * @brief 世界绝对浮点坐标 → 相对浮点坐标。
     */
    [[nodiscard]] math::Vector3d worldToRelativeF(math::Vector3d worldPos) const noexcept;

    /**
     * @brief 计算旋转后的结构世界包围盒最小角（放置原点）与最大角。
     *
     * 旋转后结构尺寸的 X/Z 维度可能互换（90°/270° 时），据此算世界包围盒。
     * 返回 {minCorner, maxCorner}，minCorner 即考虑旋转后的实际放置原点。
     */
    [[nodiscard]] BlockPos rotatedSize() const noexcept
    {
        // 90°/270° 旋转时 X 与 Z 维度互换
        if (m_rotation == Rotation::Clockwise90 || m_rotation == Rotation::CounterClockwise90) {
            return BlockPos{m_size.z, m_size.y, m_size.x};
        }
        return m_size;
    }

private:
    // 相对坐标按旋转映射到"旋转前"的局部坐标（用于 +原点 得世界坐标）
    [[nodiscard]] BlockPos rotateRelative(BlockPos p) const noexcept
    {
        switch (m_rotation) {
            case Rotation::None:
                return p;
            case Rotation::Clockwise90:
                return BlockPos{m_size.z - 1 - p.z, p.y, p.x};
            case Rotation::Clockwise180:
                return BlockPos{m_size.x - 1 - p.x, p.y, m_size.z - 1 - p.z};
            case Rotation::CounterClockwise90:
                return BlockPos{p.z, p.y, m_size.x - 1 - p.x};
        }
        return p;
    }

    // 旋转前局部坐标 → 相对坐标（worldToRelative 的逆）
    [[nodiscard]] BlockPos unrotateRelative(BlockPos p) const noexcept
    {
        switch (m_rotation) {
            case Rotation::None:
                return p;
            case Rotation::Clockwise90:
                return BlockPos{p.z, p.y, m_size.z - 1 - p.x};
            case Rotation::Clockwise180:
                return BlockPos{m_size.x - 1 - p.x, p.y, m_size.z - 1 - p.z};
            case Rotation::CounterClockwise90:
                return BlockPos{m_size.x - 1 - p.z, p.y, p.x};
        }
        return p;
    }

    BlockPos m_origin;
    BlockPos m_size;
    Rotation m_rotation = Rotation::None;
};

} // namespace mc::test
