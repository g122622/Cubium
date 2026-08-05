#pragma once

#include "common/test/base/error/GameTestResult.hpp"

#include <memory>

namespace mc::test {

/**
 * @brief 测试函数运行结果轮询接口。
 *
 * 对齐基岩版 `IGameTestFunctionRunResult`：`BaseGameTestFunction::run(helper, ctx)` 返回此接口的
 * `unique_ptr`。框架核心（`BaseGameTestInstance`）每 tick 轮询 `isComplete()`，完成后取 `getError()`
 * 判定通过/失败。这样设计可同时支持：
 * - **原生同步测试**：`SyncGameTestRunResult`，`run()` 返回时即 `isComplete()==true`，`getError()` 携带结果。
 * - **脚本异步测试**：`ScriptAsyncGameTestRunResult`，`run()` 返回时 `isComplete()==false`，待 JS Promise
 *   resolve/reject 后才 `isComplete()==true`（第一阶段 TODO stub，事件总线未桥接）。
 *
 * `isComplete() && getError() == nullopt` = 测试通过。
 */
class IGameTestFunctionRunResult {
public:
    virtual ~IGameTestFunctionRunResult() = default;

    /**
     * @brief 测试函数是否已执行完毕。
     *
     * 同步实现恒返回 true；异步实现待 Promise 就绪后返回 true。
     */
    [[nodiscard]] virtual bool isComplete() const = 0;

    /**
     * @brief 取测试结果（仅在 `isComplete()` 后调用）。
     *
     * 返回 nullopt 表示通过，非 nullopt 表示失败并携带错误。
     */
    [[nodiscard]] virtual GameTestResult getError() = 0;
};

} // namespace mc::test
