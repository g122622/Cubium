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

#include "server/test/runner/reporter/FailedTestCollector.hpp"

#include "common/test/framework/instance/BaseGameTestInstance.hpp"
#include "common/util/assert/AssertMacros.hpp" // MC_UNUSED

namespace mc::test {

void FailedTestCollector::onTestPassed(const BaseGameTestInstance& test)
{
    MC_UNUSED(test);
    // 通过的测试不收集
}

void FailedTestCollector::onTestFailed(const BaseGameTestInstance& test)
{
    m_failedTestNames.push_back(test.function().testName());
    const auto& err = test.error();
    m_failureMessages.push_back(err.has_value() ? err->formattedMessage() : std::string{"unknown failure"});
}

void FailedTestCollector::onBatchFinished(const MultipleTestTracker& tracker)
{
    MC_UNUSED(tracker);
}

void FailedTestCollector::onAllFinished(const MultipleTestTracker& tracker)
{
    MC_UNUSED(tracker);
}

} // namespace mc::test
