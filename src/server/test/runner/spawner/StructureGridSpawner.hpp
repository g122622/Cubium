#pragma once

#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"

#include <cstddef>

namespace mc::test {

/**
 * @brief 结构网格布局器：把多个测试结构按网格排列，算每个测试的原点。
 *
 * 对齐 Java `GameTestRunner.StructureSpawner` + 基岩 `StructureGridSpawner`：
 * - `SPACE_BETWEEN_COLUMNS = 5`：同列相邻测试 X 方向间距（含 padding）。
 * - `SPACE_BETWEEN_ROWS = 6`：行间 Z 方向间距。
 * - `testsPerRow`：每行测试数（默认 8，来自 `TestParameters.testsPerRow`）。
 *
 * `originOf(index, structureSizeX, structureSizeZ)` 返回第 `index` 个测试的原点（相对网格起点）。
 * 旋转后包围盒尺寸由调用方（`MinecraftGameTestBatchRunner`）传入。
 *
 * 不对外——由 runner 内部使用。`MinecraftGameTestBatchRunner` 第一阶段用线性递增，完整网格待切换到此 spawner。
 */
class StructureGridSpawner {
public:
    static constexpr i32 SPACE_BETWEEN_COLUMNS = 5;
    static constexpr i32 SPACE_BETWEEN_ROWS = 6;

    StructureGridSpawner(BlockPos gridStart, i32 testsPerRow) noexcept
        : m_gridStart(gridStart)
        , m_testsPerRow(testsPerRow > 0 ? testsPerRow : 8)
    {}

    /**
     * @brief 计算第 `index` 个测试的原点。
     *
     * @param index 测试在批次中的序号（0-based）。
     * @param structureSizeX 该测试结构旋转后 X 跨度。
     * @param structureSizeZ 该测试结构旋转后 Z 跨度。
     */
    [[nodiscard]] BlockPos originOf(std::size_t index, i32 structureSizeX, i32 structureSizeZ) const noexcept;

    [[nodiscard]] const BlockPos& gridStart() const noexcept { return m_gridStart; }
    [[nodiscard]] i32 testsPerRow() const noexcept { return m_testsPerRow; }

private:
    BlockPos m_gridStart;
    i32 m_testsPerRow;
};

} // namespace mc::test
