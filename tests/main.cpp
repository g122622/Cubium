/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, without limitation the use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, IN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

// 测试程序入口：安装 CrashHandler 后运行所有 GoogleTest 用例。
// 崩溃（SEH 异常、信号、纯虚调用、std::terminate、MC_ASSERT_RELEASE 触发的 abort）时
// 输出调用栈，便于定位测试失败/挂起根因。参考 src/client/main.cpp 与 src/server/main.cpp。

#include "common/util/assert/CrashHandler.hpp"

#include <gtest/gtest.h>

#include <cstdlib>

int main(int argc, char* argv[])
{
    // 安装崩溃处理器：捕获 SEH 异常、信号、纯虚函数调用、std::terminate 等，
    // 输出调用栈和局部变量信息到终端。多次调用安全（只安装一次）。
    mc::assert::CrashHandler::install();

    ::testing::InitGoogleTest(&argc, argv);
    const int result = RUN_ALL_TESTS();

    // CrashHandler 不需要 uninstall：进程即将退出，操作系统回收所有资源。
    return result;
}
