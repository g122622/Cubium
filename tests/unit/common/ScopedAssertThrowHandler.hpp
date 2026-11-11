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

#include "common/util/assert/AssertAll.hpp"

#if !MC_ASSERT_WITH_EXCEPTIONS
#error "ScopedAssertThrowHandler requires C++ exceptions; tests must be built with exceptions enabled"
#endif

namespace mc::test {

/**
 * @brief 把断言处理器临时替换为「抛出 AssertException」的版本
 *
 * 生产代码的断言失败会直接终止进程，测试无法用 `EXPECT_THROW` 验证。
 * 构造本对象后，`MC_ASSERT_*` 失败改为抛出 `assert::AssertException`；
 * 析构时恢复原处理器（断言在 `EXPECT_THROW` 内抛出时也能正确复原）。
 *
 * 用法：
 * @code
 * mc::test::ScopedAssertThrowHandler guard;
 * EXPECT_THROW(MC_UNUSED(Noises::get("minecraft:nonexistent")), mc::assert::AssertException);
 * @endcode
 *
 * @note 依赖测试 target 保留 C++ 异常；生产 target 的断言行为不受影响。
 */
class ScopedAssertThrowHandler {
public:
    ScopedAssertThrowHandler()
        : m_originalConfig(assert::AssertManager::instance().config())
    {
        assert::AssertConfig config;
        config.handler = [](const assert::AssertFailure& failure) {
            throw assert::AssertException(failure);
        };
        assert::AssertManager::instance().setConfig(config);
    }

    ~ScopedAssertThrowHandler()
    {
        assert::AssertManager::instance().setConfig(m_originalConfig);
    }

    ScopedAssertThrowHandler(const ScopedAssertThrowHandler&) = delete;
    ScopedAssertThrowHandler& operator=(const ScopedAssertThrowHandler&) = delete;
    ScopedAssertThrowHandler(ScopedAssertThrowHandler&&) = delete;
    ScopedAssertThrowHandler& operator=(ScopedAssertThrowHandler&&) = delete;

private:
    assert::AssertConfig m_originalConfig;
};

} // namespace mc::test
