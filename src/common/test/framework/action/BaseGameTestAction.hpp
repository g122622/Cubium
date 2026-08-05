#pragma once

#include "common/test/base/error/GameTestResult.hpp"

namespace mc::test {

/**
 * @brief 序列步骤动作抽象基类。
 *
 * 对齐基岩版 `BaseGameTestAction`：`GameTestSequence` 的每一步（`thenExecute`/`thenWait`/`thenExecuteAfter`
 * 等）持有一个 `BaseGameTestAction`，在到达该步骤的 tick 时调用 `run()` 执行并返回 `GameTestResult`。
 * 抽象基类允许：
 * - `CallbackAction`（持 `std::function<GameTestResult()>`）：原生 C++ 回调。
 * - 脚本侧 `ScriptAction`（持 JS Closure）：1H 阶段。
 *
 * `run()` 返回 nullopt=本步通过（序列继续），非 nullopt=本步失败（序列终止，测试失败）。
 */
class BaseGameTestAction {
public:
    virtual ~BaseGameTestAction() = default;

    /**
     * @brief 执行本步骤动作。
     *
     * @return nullopt=通过，非 nullopt=失败。
     */
    [[nodiscard]] virtual GameTestResult run() = 0;
};

} // namespace mc::test
