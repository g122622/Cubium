#include "server/test/runner/spawner/StructureGridSpawner.hpp"

namespace mc::test {

BlockPos StructureGridSpawner::peekOrigin() noexcept
{
    // 当前行已放满 testsPerRow 个 → 换行：Z 推进 当前行最大深度 + 行间距，X 回到行首，重置行状态。
    // 换行在 peekOrigin（取原点前）触发，保证返回的是新行首原点。
    if (m_colInRow >= m_testsPerRow) {
        m_rowBaseZ += m_rowMaxDepth + SPACE_BETWEEN_ROWS;
        m_cursorX = m_gridStart.x;
        m_rowMaxDepth = 0;
        m_colInRow = 0;
    }
    return BlockPos{m_cursorX, m_gridStart.y, m_rowBaseZ};
}

void StructureGridSpawner::advance(i32 structureSizeX, i32 structureSizeZ, i32 padding) noexcept
{
    // X 游标已由 peekOrigin 暴露为本测试原点，此处推进供行内下一个测试：
    // 本结构宽度 + 两侧 padding + 列间距。
    const i32 footprintX = structureSizeX + padding * 2;
    m_cursorX += footprintX + SPACE_BETWEEN_COLUMNS;

    // 记录本行最大 Z 深度（含 padding），换行时据此算下一行 Z 基线。
    const i32 footprintZ = structureSizeZ + padding * 2;
    if (footprintZ > m_rowMaxDepth) {
        m_rowMaxDepth = footprintZ;
    }

    ++m_colInRow;
}

} // namespace mc::test
