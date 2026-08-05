#pragma once

#include "common/test/base/error/GameTestErrorContext.hpp"
#include "common/test/base/error/GameTestErrorType.hpp"

#include <optional>
#include <string>
#include <vector>

namespace mc::test {

/**
 * @brief GameTest 错误信封。
 *
 * 对齐基岩版 `GameTestError`：由 `type`（错误类别）+ `message`（可含 `{0}`/`{1}` 占位符的可读模板）+
 * `params`（占位符实参列表）+ `context`（绝对/相对 BlockPos + tick）四部分组成。整个框架统一以
 * `GameTestResult = std::optional<GameTestError>` 表达断言结果，nullopt=通过，非 nullopt=失败。
 *
 * `message` 中的占位符 `{0}`/`{1}`/... 在 `formattedMessage()` 中按 `params` 顺序替换，
 * 供日志与游戏内错误标记显示。这与基岩版 `GameTestError::getFullDescriptionMessage` 语义一致。
 *
 * 本类型是值类型（可拷贝/可移动），作为门面方法的返回值对外可见（`GameTestHelper` 的断言方法、
 * `GameTestSequence` 的步骤回调均返回 `std::optional<GameTestError>`）。
 */
class GameTestError {
public:
    GameTestError() = default;

    explicit GameTestError(GameTestErrorType type, std::string message)
        : m_type(type)
        , m_message(std::move(message))
    {}

    GameTestError(GameTestErrorType type, std::string message, std::vector<std::string> params)
        : m_type(type)
        , m_message(std::move(message))
        , m_params(std::move(params))
    {}

    GameTestError(GameTestErrorType type,
        std::string message,
        std::vector<std::string> params,
        std::optional<GameTestErrorContext> context)
        : m_type(type)
        , m_message(std::move(message))
        , m_params(std::move(params))
        , m_context(std::move(context))
    {}

    [[nodiscard]] GameTestErrorType type() const noexcept { return m_type; }
    [[nodiscard]] const std::string& message() const noexcept { return m_message; }
    [[nodiscard]] const std::vector<std::string>& params() const noexcept { return m_params; }
    [[nodiscard]] const std::optional<GameTestErrorContext>& context() const noexcept { return m_context; }

    void setContext(std::optional<GameTestErrorContext> context) { m_context = std::move(context); }

    /**
     * @brief 返回替换占位符后的完整可读消息。
     *
     * 将 `message` 中的 `{0}`/`{1}`/... 按 `params` 顺序替换；缺参的占位符保留原样。
     */
    [[nodiscard]] std::string formattedMessage() const;

    /**
     * @brief 返回用于游戏内错误标记处显示的简短文本。
     *
     * 格式："(<相对坐标>) <formattedMessage>"，若无上下文则只返回 formattedMessage。
     * 供 `WorldVisualizationListener` 在错误方块上方信标/讲台处展示。
     */
    [[nodiscard]] std::string getMessageToShowAtBlock() const;

private:
    GameTestErrorType m_type = GameTestErrorType::Unknown;
    std::string m_message;
    std::vector<std::string> m_params;
    std::optional<GameTestErrorContext> m_context;
};

} // namespace mc::test
