#include "server/test/minecraft/structure/StructureBounds.hpp"

namespace mc::test {

BlockPos StructureBounds::rotatedSize() const noexcept
{
    // 90/270 度 X、Z 互换（对齐 TestTransform::rotatedSize 公式）
    switch (m_rotation) {
        case Rotation::Clockwise90:
        case Rotation::CounterClockwise90:
            return BlockPos{m_size.z, m_size.y, m_size.x};
        case Rotation::None:
        case Rotation::Clockwise180:
        default:
            return m_size;
    }
}

StructureBoundingBox StructureBounds::bounds() const noexcept
{
    const BlockPos rs = rotatedSize();
    // 方块坐标左闭右闭：结构占据 [origin, origin+rotatedSize-1]
    const BlockPos max{
        m_origin.x + rs.x - 1,
        m_origin.y + rs.y - 1,
        m_origin.z + rs.z - 1,
    };
    return StructureBoundingBox(m_origin.x, m_origin.y, m_origin.z, max.x, max.y, max.z);
}

StructureBoundingBox StructureBounds::paddingBounds(i32 padding) const noexcept
{
    const auto bb = bounds();
    return StructureBoundingBox(bb.minX() - padding,
        bb.minY() - padding,
        bb.minZ() - padding,
        bb.maxX() + padding,
        bb.maxY() + padding,
        bb.maxZ() + padding);
}

} // namespace mc::test
