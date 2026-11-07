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

// 测试程序入口：安装 CrashHandler 后运行所有 GoogleTest 用例。全局环境（数据包驱动的注册表
// 装载）与用例隔离监听器在 test_main_entry.hpp 中。
// 崩溃（SEH 异常、信号、纯虚调用、std::terminate、MC_ASSERT_RELEASE 触发的 abort）时
// 输出调用栈，便于定位测试失败/挂起根因。参考 src/client/main.cpp 与 src/server/main.cpp。

#include "common/util/assert/CrashHandler.hpp"
#include "test_main_entry.hpp"

#include <gtest/gtest.h>

#include <spdlog/spdlog.h>

int main(int argc, char* argv[])
{
    // 供 OnTestProgramEnd 统计进程墙钟的上界余量（见该类注释）。取值早于任何初始化。
    TestIsolationListener::s_processStartMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count();

    spdlog::info("[SetupTiming] main_enter");
    WorldGenRegistryEnvironment::recordTiming("main_enter", 0);

    // 安装崩溃处理器：捕获 SEH 异常、信号、纯虚函数调用、std::terminate 等，
    // 输出调用栈和局部变量信息到终端。多次调用安全（只安装一次）。
    mc::assert::CrashHandler::install();
    spdlog::info("[SetupTiming] main_after_crash_handler");

    ::testing::InitGoogleTest(&argc, argv);
    spdlog::info("[SetupTiming] main_after_init_gtest");
    // 注册全局环境：在 RUN_ALL_TESTS 之前加载世界生成注册表。
    ::testing::AddGlobalTestEnvironment(new WorldGenRegistryEnvironment());
    spdlog::info("[SetupTiming] main_after_add_env");
    // 注册跨用例隔离监听器：每用例结束重置 thread_local 调度上下文，根除全二进制
    // 直跑时的跨用例共享状态污染（详见 docs/test/「跨测试隔离模型」）。
    ::testing::UnitTest::GetInstance()->listeners().Append(new TestIsolationListener());
    const int result = RUN_ALL_TESTS();

    // CrashHandler 不需要 uninstall：进程即将退出，操作系统回收所有资源。
    return result;
}
