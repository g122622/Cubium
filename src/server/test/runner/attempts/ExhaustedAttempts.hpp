#pragma once

#include "common/test/base/error/GameTestError.hpp"
#include "common/test/base/error/GameTestErrorType.hpp"

#include <string>

namespace mc::test {

/**
 * @brief 重试耗尽错误。
 *
 * 对齐 Java `ExhaustedAttemptsException`：flaky 测试在 `maxAttempts` 次尝试后仍未达成 `requiredSuccesses`
 * 即构造此错误，作为测试失败原因。框架层把它包成 `GameTestError(ExhaustedAttempts)`。
 */
class ExhaustedAttempts {
public:
    ExhaustedAttempts(std::string testName, i32 attempts, i32 requiredSuccesses, i32 actualSuccesses)
        : m_testName(std::move(testName))
        , m_attempts(attempts)
        , m_requiredSuccesses(requiredSuccesses)
        , m_actualSuccesses(actualSuccesses)
    {}

    /**
     * @brief 转为 `GameTestError`（ExhaustedAttempts 类型）。
     */
    [[nodiscard]] GameTestError toGameTestError() const
    {
        return GameTestError{GameTestErrorType::ExhaustedAttempts,
            "Test '{0}' exhausted attempts: {1} tries, required {2} successes, got {3}",
            {m_testName,
                std::to_string(m_attempts),
                std::to_string(m_requiredSuccesses),
                std::to_string(m_actualSuccesses)}};
    }

    [[nodiscard]] const std::string& testName() const noexcept { return m_testName; }
    [[nodiscard]] i32 attempts() const noexcept { return m_attempts; }
    [[nodiscard]] i32 requiredSuccesses() const noexcept { return m_requiredSuccesses; }
    [[nodiscard]] i32 actualSuccesses() const noexcept { return m_actualSuccesses; }

private:
    std::string m_testName;
    i32 m_attempts;
    i32 m_requiredSuccesses;
    i32 m_actualSuccesses;
};

} // namespace mc::test
