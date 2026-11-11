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

// ============================================================================
// 断言异常机制（测试专用）
//
// 生产 target 以 -fno-exceptions 编译，断言一律走 defaultAssertHandler（打印并 abort），
// 与异常相关的 AssertException / throwAssertHandler 因此不参与生产编译
// （见 common/util/assert/Assert.hpp 的 MC_ASSERT_WITH_EXCEPTIONS 守卫）。
// 测试需要用 EXPECT_THROW 验证断言路径，故由测试 target 自己提供这两个定义。
// ============================================================================

#include "common/util/assert/AssertAll.hpp"

#include <sstream>

namespace mc::assert {

[[noreturn]] void throwAssertHandler(const AssertFailure& failure)
{
    throw AssertException(failure);
}

AssertException::AssertException(const AssertFailure& failure)
    : m_failure(failure)
{
    std::ostringstream oss;
    oss << "Assertion failed: " << failure.expression;

    if (!failure.message.empty()) {
        oss << " - " << failure.message;
    }

    oss << " at " << failure.file << ":" << failure.line << " in " << failure.function;

    m_what = oss.str();
}

} // namespace mc::assert
