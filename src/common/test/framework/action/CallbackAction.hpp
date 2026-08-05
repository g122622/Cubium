#pragma once

#include "common/test/framework/action/BaseGameTestAction.hpp"

#include <functional>

namespace mc::test {

/**
 * @brief 持 `std::function` 回调的序列动作。
 *
 * 对齐基岩版 `NativeFunctionGameTestAction`：原生 C++ 测试序列步骤的默认实现，持一个
 * `std::function<GameTestResult()>` 回调。
 *
 * 两种语义（由 `isWait` 区分，`GameTestSequence` 据此调度）：
 * - **execute 型**（`thenExecute`/`thenExecuteAfter`/`thenExecuteFor`）：到达该 tick 时调用一次 `run()`，
 *   返回通过则立即进入下一步，返回失败则序列终止。
 * - **wait 型**（`thenWait`/`thenWaitAfter`）：从该 tick 起**每 tick** 调用 `run()`，直到返回通过才进入下一步
 *  （对齐基岩 `thenWaitAfter` 的轮询语义）。返回失败不终止，仅继续等待（除非超时）。
 */
class CallbackAction final : public BaseGameTestAction {
public:
    /**
     * @param callback 动作回调。
     * @param isWait 是否为 wait 型（每 tick 轮询直到通过）。
     */
    CallbackAction(std::function<GameTestResult()> callback, bool isWait)
        : m_callback(std::move(callback))
        , m_isWait(isWait)
    {}

    [[nodiscard]] GameTestResult run() override { return m_callback(); }

    [[nodiscard]] bool isWait() const noexcept { return m_isWait; }

private:
    std::function<GameTestResult()> m_callback;
    bool m_isWait;
};

} // namespace mc::test
