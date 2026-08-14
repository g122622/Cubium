#pragma once

#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"

namespace mc::test {

/**
 * @brief 结构网格布局器：把多个测试结构按网格排列，算每个测试的原点。
 *
 * 对齐 Java `GameTestRunner.StructureSpawner` + 基岩 `StructureGridSpawner`：有状态逐个推进。
 * 使用两步协议适配"先放结构再算下一原点"的 runner 流程：
 * 1. `peekOrigin()`：返回当前游标位置作为本测试原点。若当前行已满（达 testsPerRow），先换行再返回。
 * 2. `advance(sizeX, sizeZ, padding)`：用本测试旋转后真实尺寸推进游标（X 累加 + 行深度记录 + 计数）。
 *
 * 布局规则：
 * - 行内沿 X 方向依次排列，每个结构占 `sizeX + 2*padding + SPACE_BETWEEN_COLUMNS`。
 * - 每行放满 `testsPerRow` 个后换行：Z 推进 `当前行最大 (sizeZ + 2*padding) + SPACE_BETWEEN_ROWS`，
 *   X 回到行首。
 * - SPACE_BETWEEN_COLUMNS/ROWS 须大于实体 FOLLOW_RANGE（默认 16，部分实体如僵尸/铁傀儡变体可达 32），
 *   否则相邻结构间距小于目标搜索半径，实体目标选择 goal（NearestAttackableTargetGoal 等）会跨结构
 *   搜索污染——checkSight 射线穿过无围墙开放结构被邻结构方块/实体阻挡，致实体选到本结构目标后
 *   canSee 失败而放弃，表现为"单独跑通过、同 batch 并行跑必失败"。取 32 覆盖 vanilla 最大常见
 *   FOLLOW_RANGE，从框架层根除跨结构目标搜索污染。
 *
 * 游标状态跨 batch 累积（不每 batch 重置），整个运行连续编号，对齐 vanilla 全局网格。
 *
 * 不对外——由 `MinecraftGameTestBatchRunner` 持有使用。
 */
class StructureGridSpawner {
public:
    static constexpr i32 SPACE_BETWEEN_COLUMNS = 32;
    static constexpr i32 SPACE_BETWEEN_ROWS = 32;

    StructureGridSpawner(BlockPos gridStart, i32 testsPerRow) noexcept
        : m_gridStart(gridStart)
        , m_testsPerRow(testsPerRow > 0 ? testsPerRow : 8)
        , m_cursorX(gridStart.x)
        , m_rowBaseZ(gridStart.z)
        , m_rowMaxDepth(0)
        , m_colInRow(0)
    {}

    /**
     * @brief 返回当前游标位置作为下一个测试的原点。
     *
     * 若当前行已放满（m_colInRow >= testsPerRow），先换行（Z 推进行最大深度 + 行间距，X 回行首，
     * 重置行状态）再返回。Y 取网格起点 Y。调用方应在放置结构前调此方法取得原点，放置后用真实尺寸
     * 调 `advance` 推进。
     */
    [[nodiscard]] BlockPos peekOrigin() noexcept;

    /**
     * @brief 用本测试旋转后真实尺寸 + padding 推进游标（供下一个测试的 peekOrigin）。
     *
     * @param structureSizeX 该测试结构旋转后 X 跨度。
     * @param structureSizeZ 该测试结构旋转后 Z 跨度。
     * @param padding 该测试结构周边清理格数（两侧各 padding，计入间距）。
     */
    void advance(i32 structureSizeX, i32 structureSizeZ, i32 padding) noexcept;

    /** @brief 网格起点（只读，诊断/单测用）。 */
    [[nodiscard]] const BlockPos& gridStart() const noexcept { return m_gridStart; }
    /** @brief 每行测试数（只读，诊断/单测用）。 */
    [[nodiscard]] i32 testsPerRow() const noexcept { return m_testsPerRow; }

private:
    BlockPos m_gridStart;
    i32 m_testsPerRow;

    // 游标状态（跨 peekOrigin/advance 调用累积）
    i32 m_cursorX;     // 当前行下一个测试的 X 原点
    i32 m_rowBaseZ;    // 当前行 Z 基线（行内所有测试共享）
    i32 m_rowMaxDepth; // 当前行已放置结构的最大 (sizeZ + 2*padding)，换行时据此推进 Z
    i32 m_colInRow;    // 当前行已放置测试数（达 testsPerRow 触发换行）
};

} // namespace mc::test
