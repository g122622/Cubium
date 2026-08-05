#pragma once

namespace mc::test {

/**
 * @brief 测试实例运行状态。
 *
 * 对齐 Java `GameTestInfo` 的标志位推导状态，但用显式枚举（更清晰，见校正 D.5 差异）。
 * 状态流转：
 * - `NotStarted` → `Running`（`startExecution` 后，setup 阶段结束进入正式 tick）
 * - `Running` → `Succeeded`（`succeed` 或全部序列完成且 succeedIf 通过）
 * - `Running` → `Failed`（`fail` 或序列返回错误或超时）
 * - `Succeeded`/`Failed` → `Stopped`（清理完成）
 *
 * `isDone()` = Succeeded/Failed/Stopped（已结束，不再 tick）。
 * `hasSucceeded()` = Succeeded。
 * `hasFailed()` = Failed/Stopped（Stopped 视为失败后清理）。
 */
enum class GameTestState {
    NotStarted, // 尚未开始（结构放置前/setup 阶段）
    Running,    // 正在运行（tick 推进中）
    Succeeded,  // 已成功
    Failed,     // 已失败
    Stopped,    // 已停止（清理完成）
};

[[nodiscard]] inline constexpr bool isDone(GameTestState state) noexcept
{
    return state == GameTestState::Succeeded || state == GameTestState::Failed || state == GameTestState::Stopped;
}

[[nodiscard]] inline constexpr bool hasSucceeded(GameTestState state) noexcept
{
    return state == GameTestState::Succeeded;
}

[[nodiscard]] inline constexpr bool hasFailed(GameTestState state) noexcept
{
    return state == GameTestState::Failed || state == GameTestState::Stopped;
}

} // namespace mc::test
