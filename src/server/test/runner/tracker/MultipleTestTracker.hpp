#pragma once

#include "common/core/Types.hpp"

#include <cstddef>

namespace mc::test {

class BaseGameTestInstance;

/**
 * @brief 多测试进度跟踪器。
 *
 * 对齐 Java `MultipleTestTracker`：聚合一批测试的进度计数（总/通过/失败/进行中），供报告器输出进度条。
 * runner 每 tick 查询计数，完成时触发最终报告。
 *
 * 不对外——由 `GameTestRunner` 内部持有，报告器读取。
 */
class MultipleTestTracker {
public:
    MultipleTestTracker() = default;

    void setTotal(std::size_t total) noexcept { m_total = total; }
    void onPassed() noexcept { ++m_passed; }
    void onFailed() noexcept { ++m_failed; }

    [[nodiscard]] std::size_t total() const noexcept { return m_total; }
    [[nodiscard]] std::size_t passed() const noexcept { return m_passed; }
    [[nodiscard]] std::size_t failed() const noexcept { return m_failed; }
    [[nodiscard]] std::size_t done() const noexcept { return m_passed + m_failed; }
    [[nodiscard]] std::size_t remaining() const noexcept { return m_total > done() ? m_total - done() : 0; }
    [[nodiscard]] bool allDone() const noexcept { return done() >= m_total && m_total > 0; }

private:
    std::size_t m_total = 0;
    std::size_t m_passed = 0;
    std::size_t m_failed = 0;
};

} // namespace mc::test
