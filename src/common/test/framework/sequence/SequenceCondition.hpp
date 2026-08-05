#pragma once

#include "common/core/Types.hpp"
#include "common/test/base/error/GameTestResult.hpp"

namespace mc::test {

/**
 * @brief 序列闩锁条件（`thenTrigger` 用）。
 *
 * 对齐 Java 1.21.11 `GameTestSequence.Condition`：`thenTrigger()` 返回此对象，测试体在适当时机调用
 * `trigger(currentTick)` 标记"本 tick 已触发"；后续 `assertTriggeredThisTick(currentTick)` 检查是否在
 * 当前 tick 触发过——若未触发则返回失败。
 *
 * 典型用法（对齐 Java）：
 * ```
 * auto cond = helper.startSequence().thenTrigger();
 * // ... 某处异步逻辑中 cond.trigger(helper.currentTick());
 * cond.assertTriggeredThisTick(helper.currentTick());
 * ```
 *
 * 注：基岩版/JS 均无 `thenTrigger`，此为 Java 独有能力，C++ 框架原生层补齐（见校正 4）。
 */
class SequenceCondition {
public:
    SequenceCondition() = default;

    /**
     * @brief 标记在 `tick` 时刻触发。
     */
    void trigger(i32 tick) noexcept { m_triggerTime = tick; }

    /**
     * @brief 断言当前 tick 已触发过。
     *
     * @return nullopt=已触发（通过），非 nullopt=未触发（失败，Waiting 类型）。
     */
    [[nodiscard]] GameTestResult assertTriggeredThisTick(i32 currentTick) const
    {
        if (m_triggerTime == currentTick) {
            return std::nullopt;
        }
        return GameTestError{GameTestErrorType::Waiting,
            "Condition not triggered this tick (expected={0}, actual={1})",
            {std::to_string(currentTick), std::to_string(m_triggerTime)}};
    }

    [[nodiscard]] i32 triggerTime() const noexcept { return m_triggerTime; }

private:
    i32 m_triggerTime = -1;
};

} // namespace mc::test
