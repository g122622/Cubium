#pragma once

#include "common/core/Types.hpp"

namespace mc::test {

/**
 * @brief 重试选项。
 *
 * 对齐 Java 版 `RetryOptions`：`numberOfTries`（最大尝试次数，0=不重试只跑一次的语义由调用方约定，
 * 这里采用 `numberOfTries>=1` 表示至少跑 1 次）+ `haltOnFailure`（某次失败后是否立即停止后续重试，
 * 对齐基岩 `TestParameters.stopOnFailure`）。
 *
 * `noRetries()` = 只跑一次、失败即停（默认行为）。
 * `unlimitedTries()` = 无限重试、失败不停（用于 `--verify` 压测）。
 * `hasTriesLeft(used)` = 已用 `used` 次后是否还能再试。
 *
 * 与 `TestData::maxAttempts`/`requiredSuccesses` 的关系：`maxAttempts` 是注册期元数据（数据驱动），
 * `RetryOptions` 是运行期实例（每次重试生成新实例），由 `MinecraftGameTestBatchRunner` 在重试时构造。
 */
class RetryOptions {
public:
    RetryOptions() noexcept = default;
    RetryOptions(i32 numberOfTries, bool haltOnFailure) noexcept
        : m_numberOfTries(numberOfTries)
        , m_haltOnFailure(haltOnFailure)
    {}

    [[nodiscard]] i32 numberOfTries() const noexcept { return m_numberOfTries; }
    [[nodiscard]] bool haltOnFailure() const noexcept { return m_haltOnFailure; }

    void setNumberOfTries(i32 tries) noexcept { m_numberOfTries = tries; }
    void setHaltOnFailure(bool halt) noexcept { m_haltOnFailure = halt; }

    /**
     * @brief 默认：只跑一次，失败即停。
     */
    [[nodiscard]] static RetryOptions noRetries() noexcept { return RetryOptions{1, true}; }

    /**
     * @brief 无限重试，失败不停（用于 `--verify` 旋转压测）。
     *
     * `numberOfTries` 取 `INT32_MAX` 作为"无限"的工程近似。
     */
    [[nodiscard]] static RetryOptions unlimitedTries() noexcept { return RetryOptions{0x7FFFFFFF, false}; }

    /**
     * @brief 已用 `used` 次尝试后是否还能再试。
     */
    [[nodiscard]] bool hasTriesLeft(i32 used) const noexcept { return used < m_numberOfTries; }

private:
    i32 m_numberOfTries = 1;
    bool m_haltOnFailure = true;
};

} // namespace mc::test
