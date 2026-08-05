#pragma once

#include "common/test/base/error/GameTestError.hpp"

#include <optional>

namespace mc::test {

/**
 * @brief GameTest 断言结果类型。
 *
 * `= std::optional<GameTestError>`：`std::nullopt` 表示通过（无错误），非空表示失败并携带错误信封。
 * 全框架统一以错误即值（error-as-value）风格表达断言结果——无任何异常，符合项目"no exceptions"规范。
 *
 * 对齐基岩版 `IGameTestFunctionRunResult::getError()` 返回的 `std::optional<GameTestError>` 语义，
 * 以及 Java 版"通过=无异常 / 失败=GameTestException"二元结果，但用 optional 替代异常。
 *
 * 提供 `pass()`/`fail(...)` 工厂便于在序列步骤回调和断言方法中构造结果。
 */
using GameTestResult = std::optional<GameTestError>;

/**
 * @brief 构造"通过"结果。
 */
[[nodiscard]] inline GameTestResult pass() noexcept
{
    return std::nullopt;
}

/**
 * @brief 构造"失败"结果（无上下文）。
 */
[[nodiscard]] inline GameTestResult fail(GameTestErrorType type, std::string message)
{
    return GameTestError{type, std::move(message)};
}

/**
 * @brief 构造"失败"结果（带占位符参数）。
 */
[[nodiscard]] inline GameTestResult fail(GameTestErrorType type, std::string message, std::vector<std::string> params)
{
    return GameTestError{type, std::move(message), std::move(params)};
}

/**
 * @brief 构造"失败"结果（带占位符参数与坐标上下文）。
 */
[[nodiscard]] inline GameTestResult fail(
    GameTestErrorType type, std::string message, std::vector<std::string> params, GameTestErrorContext context)
{
    return GameTestError{type, std::move(message), std::move(params), std::move(context)};
}

/**
 * @brief 便利判定：结果是否为通过。
 */
[[nodiscard]] inline bool isPass(const GameTestResult& result) noexcept
{
    return !result.has_value();
}

} // namespace mc::test
