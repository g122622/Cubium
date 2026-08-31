/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "server/test/runner/reporter/TestReporter.hpp"
#include "server/test/runner/tracker/MultipleTestTracker.hpp"

#include <string>
#include <vector>

namespace mc::test {

class BaseGameTestInstance;

/**
 * @brief 失败测试收集器：TestReporter 子类，收集失败测试的 testName 列表。
 *
 * 用途：GameTestServer::run() 结束后，提取失败 testName 列表，供外层脚本/CLI 构造
 * 重跑过滤（testsFilter）做隔离重跑。
 *
 * 生命周期：挂在 GlobalTestReporter 单例上，随 GameTestServer::stop() 中 clear() 一起移除。
 */
class FailedTestCollector final : public TestReporter {
public:
    FailedTestCollector() = default;
    ~FailedTestCollector() override = default;

    void onTestPassed(const BaseGameTestInstance& test) override;
    void onTestFailed(const BaseGameTestInstance& test) override;
    void onBatchFinished(const MultipleTestTracker& tracker) override;
    void onAllFinished(const MultipleTestTracker& tracker) override;

    /**
     * @brief 获取失败测试的 testName 列表（按失败时间排序）。
     */
    [[nodiscard]] const std::vector<std::string>& failedTestNames() const noexcept { return m_failedTestNames; }

    /**
     * @brief 获取失败测试的错误信息（与 failedTestNames 同序）。
     */
    [[nodiscard]] const std::vector<std::string>& failureMessages() const noexcept { return m_failureMessages; }

    /**
     * @brief 清空收集结果（复用 collector 跑新一轮时用）。
     */
    void reset() noexcept
    {
        m_failedTestNames.clear();
        m_failureMessages.clear();
    }

private:
    std::vector<std::string> m_failedTestNames;
    std::vector<std::string> m_failureMessages;
};

} // namespace mc::test
