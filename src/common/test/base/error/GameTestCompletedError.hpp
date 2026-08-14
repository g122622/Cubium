#pragma once

#include "common/test/base/error/GameTestError.hpp"

#include <string>

namespace mc::test {

/**
 * @brief 测试已完成的原因（Done=正常完成，CleanUp=清理阶段）。
 *
 * 对齐基岩版 `GameTestCompletedErrorReason`（int 枚举）。用于 `GameTestCompletedError` 标识
 * "测试已结束"信号发生在哪个阶段，供脚本绑定层在 JS 侧抛出对应类型的 Error。
 */
enum class GameTestCompletedErrorReason : i32 {
    Done = 0,    // 测试已正常结束（succeed/fail 已调用）
    CleanUp = 1, // 测试已进入清理阶段
};

// 前向声明：GameTestCompletedError::toGameTestError() 内联引用此函数（定义在本文件类之后），
// 须先声明使内联成员函数体可解析。
[[nodiscard]] inline const char* gameTestCompletedErrorReasonName(GameTestCompletedErrorReason reason) noexcept;

/**
 * @brief "测试已结束"信号错误。
 *
 * 对齐基岩版 `GameTestCompletedError`：当测试已通过/失败/进入清理后，若仍尝试操作世界（如再 spawn、
 * 再 assert），则产生此信号。在原生 C++ 侧，门面方法（`GameTestHelper`/`GameTestSequence`）在已完成
 * 状态下调用会返回携带此语义的 `GameTestResult`（不破坏错误即值约定）。
 *
 * 在脚本绑定层，此类型与普通 `GameTestError` 区分——JS 侧分别抛出 `GameTestCompletedError` 与
 * `GameTestError` 两个不同的 Error 子类（基岩官方 JS 文档：`idle`/`until`/`removeSimulatedPlayer`/
 * `getTestDirection`/`startSequence` 仅抛 `GameTestCompletedError`）。
 *
 * 构造携带：reason（阶段）、gameTestName（测试名）、methodName（被调方法名），供诊断信息。
 */
class GameTestCompletedError {
public:
    GameTestCompletedError() = default;

    GameTestCompletedError(GameTestCompletedErrorReason reason, std::string gameTestName, std::string methodName)
        : m_reason(reason)
        , m_gameTestName(std::move(gameTestName))
        , m_methodName(std::move(methodName))
    {}

    [[nodiscard]] GameTestCompletedErrorReason reason() const noexcept { return m_reason; }
    [[nodiscard]] const std::string& gameTestName() const noexcept { return m_gameTestName; }
    [[nodiscard]] const std::string& methodName() const noexcept { return m_methodName; }

    /**
     * @brief 转为 `GameTestError`（用于在错误即值通道中传递）。
     *
     * 类型映射为 `MethodNotImplemented`（语义最接近："在错误阶段调用方法"），消息携带 reason/测试名/方法名。
     * 脚本绑定层若需区分两种错误，应在调用原生方法前先用 `GameTestHelper::isCompleted()` 判定，
     * 而非依赖此处转换后的 `GameTestError` 类型。
     */
    [[nodiscard]] GameTestError toGameTestError() const
    {
        std::vector<std::string> params{gameTestCompletedErrorReasonName(m_reason), m_gameTestName, m_methodName};
        return GameTestError{GameTestErrorType::MethodNotImplemented,
            "GameTest already completed (reason={0}, test={1}, method={2})",
            std::move(params)};
    }

private:
    GameTestCompletedErrorReason m_reason = GameTestCompletedErrorReason::Done;
    std::string m_gameTestName;
    std::string m_methodName;
};

/**
 * @brief 将完成原因枚举转为字符串名（对齐 JS `GameTestCompletedErrorReason`）。
 */
[[nodiscard]] inline const char* gameTestCompletedErrorReasonName(GameTestCompletedErrorReason reason) noexcept
{
    switch (reason) {
        case GameTestCompletedErrorReason::Done:
            return "Done";
        case GameTestCompletedErrorReason::CleanUp:
            return "Cleanup";
    }
    return "Done";
}

} // namespace mc::test
