#pragma once

#include "common/util/Direction.hpp" // Rotation
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp" // StructureBoundingBox

namespace mc::test {

// 全限定别名，规避 mc::test 内非限定名两段查找不回退 mc::world 的遮蔽坑（见 BossBarState 内存）
using StructureBoundingBox = mc::world::gen::structure::StructureBoundingBox;

/**
 * @brief 测试结构的旋转后包围盒计算工具。
 *
 * 持有结构原点、原始尺寸（未旋转的 x/y/z 跨度）、旋转角度，提供：
 * - `bounds()`：旋转后的 `StructureBoundingBox`（minX..maxX/minY..maxY/minZ..maxZ）。
 * - `rotatedSize()`：旋转后的尺寸（90/270 度 X、Z 互换）。
 * - `paddingBounds(padding)`：含周边清理范围的外扩包围盒（供 `MinecraftStructurePlacer` 清屏）。
 *
 * 旋转后包围盒由尺寸与旋转推导（无需实际放置结构），供 `StructureGridSpawner` 算网格间距、
 * `MinecraftStructurePlacer` 算屏障范围。`Template::getBoundingBox(settings, origin)` 是权威来源，
 * 但放置前（仅注册期 `TestData`）无 PlacementSettings，故此处按旋转公式本地推算，二者须一致
 *（见 `base/coords/TestTransform::rotatedSize`）。
 */
class StructureBounds {
public:
    StructureBounds(BlockPos origin, BlockPos size, Rotation rotation) noexcept
        : m_origin(origin)
        , m_size(size)
        , m_rotation(rotation)
    {}

    /**
     * @brief 旋转后尺寸：90/270 度 X、Z 互换，0/180 度不变。
     */
    [[nodiscard]] BlockPos rotatedSize() const noexcept;

    /**
     * @brief 旋转后在世界中的包围盒（min=origin，max=origin+rotatedSize-1）。
     *
     * 对齐方块坐标的"左闭右闭"语义：结构占据 [origin, origin+size-1]。
     */
    [[nodiscard]] StructureBoundingBox bounds() const noexcept;

    /**
     * @brief 外扩 padding 格的清理包围盒（结构四周各扩 padding 格，供放置前清屏）。
     */
    [[nodiscard]] StructureBoundingBox paddingBounds(i32 padding) const noexcept;

    [[nodiscard]] const BlockPos& origin() const noexcept { return m_origin; }
    [[nodiscard]] const BlockPos& size() const noexcept { return m_size; }
    [[nodiscard]] Rotation rotation() const noexcept { return m_rotation; }

private:
    BlockPos m_origin;
    BlockPos m_size;
    Rotation m_rotation;
};

} // namespace mc::test
