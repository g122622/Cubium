#include "server/test/runner/spawner/StructureGridSpawner.hpp"

namespace mc::test {

BlockPos StructureGridSpawner::originOf(std::size_t index, i32 structureSizeX, i32 structureSizeZ) const noexcept
{
    // 行列：每 testsPerRow 个测试换行；X 方向按列累加（结构宽度 + 列间距），Z 方向按行累加（结构深度 + 行间距）
    const i32 col = static_cast<i32>(index % static_cast<std::size_t>(m_testsPerRow));
    const i32 row = static_cast<i32>(index / static_cast<std::size_t>(m_testsPerRow));

    i32 xOffset = 0;
    for (i32 c = 0; c < col; ++c) {
        // 简化：每列按固定平均宽度（结构宽度 + 列间距）累加；精确值需遍历前列实际尺寸
        xOffset += structureSizeX + SPACE_BETWEEN_COLUMNS;
    }
    const i32 zOffset = row * (structureSizeZ + SPACE_BETWEEN_ROWS);

    return BlockPos{m_gridStart.x + xOffset, m_gridStart.y, m_gridStart.z + zOffset};
}

} // namespace mc::test
