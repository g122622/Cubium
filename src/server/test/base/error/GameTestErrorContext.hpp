#pragma once

#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"

namespace mc::test {

/**
 * @brief GameTest 错误上下文。
 *
 * 对齐基岩版 `GameTestErrorContext`：携带三个字段——绝对方块坐标（世界坐标系）、
 * 相对方块坐标（结构内坐标系）、tick 计数。三者共同定位"错误发生在世界何处、结构内何处、
 * 第几 tick"。`GameTestError::getMessageToShowAtBlock()` 利用此上下文在游戏内错误标记处
 * 显示可读信息。
 *
 * 注意：坐标用整数 `BlockPos`（对齐基岩/Java），JS 侧暴露为 `Vector3` 由脚本绑定层转换。
 * `tickCount` 为 -1 表示未携带 tick 信息（如 setup 阶段错误）。
 */
class GameTestErrorContext {
public:
    GameTestErrorContext() noexcept = default;
    GameTestErrorContext(BlockPos absolutePosition, BlockPos relativePosition, i32 tickCount) noexcept
        : m_absolutePosition(absolutePosition)
        , m_relativePosition(relativePosition)
        , m_tickCount(tickCount)
    {}

    [[nodiscard]] const BlockPos& absolutePosition() const noexcept { return m_absolutePosition; }
    [[nodiscard]] const BlockPos& relativePosition() const noexcept { return m_relativePosition; }
    [[nodiscard]] i32 tickCount() const noexcept { return m_tickCount; }

    void setAbsolutePosition(BlockPos pos) noexcept { m_absolutePosition = pos; }
    void setRelativePosition(BlockPos pos) noexcept { m_relativePosition = pos; }
    void setTickCount(i32 tick) noexcept { m_tickCount = tick; }

private:
    BlockPos m_absolutePosition;
    BlockPos m_relativePosition;
    i32 m_tickCount = -1;
};

} // namespace mc::test
