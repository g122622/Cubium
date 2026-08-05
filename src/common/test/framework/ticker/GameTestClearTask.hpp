#pragma once

#include "common/world/gen/structure/StructureBoundingBox.hpp"

namespace mc::test {

/**
 * @brief 测试后清理任务抽象基类。
 *
 * 对齐基岩版 `GameTestClearTask`：测试结束后，分多 tick 逐步把结构区域内的方块清回空气（避免单 tick
 * 大量 setBlock 卡顿）。`GameTestTicker` 持有一组清理任务，每 tick 调用 `tick()`，返回 true 表示清理完成
 * 可移除。
 *
 * 抽象基类（framework 层引擎无关）：具体世界交互（`ServerWorld::setBlockState`）由
 * `MinecraftGameTestClearTask`（`minecraft/` 层）实现。此处仅定义接口与包围盒。
 */
class GameTestClearTask {
public:
    virtual ~GameTestClearTask() = default;

    /**
     * @brief 推进一 tick 清理。
     *
     * @return true=清理完成（ticker 可移除），false=仍需继续。
     */
    [[nodiscard]] virtual bool tick() = 0;

    [[nodiscard]] const mc::world::gen::structure::StructureBoundingBox& bounds() const noexcept { return m_bounds; }

protected:
    explicit GameTestClearTask(mc::world::gen::structure::StructureBoundingBox bounds) noexcept
        : m_bounds(std::move(bounds))
    {}

    mc::world::gen::structure::StructureBoundingBox m_bounds;
};

} // namespace mc::test
