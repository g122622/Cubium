#pragma once

#include "common/test/framework/function/BaseGameTestFunction.hpp"
#include "common/test/framework/function/IGameTestFunctionContext.hpp"
#include "common/test/framework/function/IGameTestRunResult.hpp"
#include "common/test/framework/helper/IGameTestHelper.hpp"

#include <functional>
#include <memory>
#include <string>

namespace mc::test {

/**
 * @brief 原生 C++ 测试函数。
 *
 * 对齐基岩版 `NativeFunctionGameTestAction`/`NativeGameTestFunction`：持一个 `std::function` 测试体，
 * `run()` 同步执行并返回 `SyncGameTestRunResult`。
 *
 * 测试体签名是 `GameTestResult(IGameTestHelper&)`（framework 层接口）——`GameTestHelper`（facade，
 * 1F）实现 `IGameTestHelper`，故作者经 `MC_REGISTER_GAME_TEST` 宏注册的 `void(GameTestHelper&)` 体由宏
 * 包装为 `IGameTestHelper&` 适配闭包（内含 `static_cast<GameTestHelper&>`），在服务器 EXE（1F builtin）内编译。
 *
 * 这样 `mc_test` 库（含 native/）不依赖 facade/server 类型，仅依赖 framework 接口；facade 在服务器 EXE 内。
 *
 * 注册：经 `GameTestRegistrar::register()`（facade）→ `NativeTestRegistrationBuilder` → 提交到 `GameTestRegistry`。
 */
class NativeGameTestFunction final : public BaseGameTestFunction {
public:
    using TestBody = std::function<GameTestResult(IGameTestHelper&)>;

    NativeGameTestFunction(
        std::string batchName, std::string testName, std::string structureName, TestData data, TestBody body)
        : BaseGameTestFunction(std::move(batchName), std::move(testName), std::move(structureName), std::move(data))
        , m_body(std::move(body))
    {}

    [[nodiscard]] std::unique_ptr<IGameTestFunctionContext> createContext(IGameTestHelper& helper) const override;
    [[nodiscard]] std::unique_ptr<IGameTestFunctionRunResult> run(
        IGameTestHelper& helper, IGameTestFunctionContext& context) const override;

private:
    TestBody m_body;
};

} // namespace mc::test
