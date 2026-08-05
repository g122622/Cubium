#pragma once

#include "common/test/framework/function/IGameTestRunResult.hpp"

namespace mc::test {

/**
 * @brief 同步测试函数运行结果（立即完成）。
 *
 * 对齐基岩版 `SyncGameTestFunctionRunResult`：原生 C++ 测试函数 `void(GameTestHelper&)` 在 `run()` 返回时
 * 即完成，结果已在构造时携带。`isComplete()` 恒 true，`getError()` 返回构造时存入的 `GameTestResult`。
 */
class SyncGameTestRunResult final : public IGameTestFunctionRunResult {
public:
    explicit SyncGameTestRunResult(GameTestResult result) noexcept
        : m_result(std::move(result))
    {}

    [[nodiscard]] bool isComplete() const noexcept override { return true; }
    [[nodiscard]] GameTestResult getError() override { return std::move(m_result); }

private:
    GameTestResult m_result;
};

} // namespace mc::test
