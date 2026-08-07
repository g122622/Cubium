/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permitted persons to whom the Software is
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

#include "BaseApplicationEntry.hpp"

#include "LogManager.hpp"

#include "common/profiler/ProfilerManager.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "minecraft-reborn/version.h"

#include <gflags/gflags.h>
#include <spdlog/spdlog.h>

#include <iostream>

namespace mc::application {

// ============================================================================
// 公共 flag 定义（子类专属 flag 在各自 TU 顶层 DEFINE）。
// gflags 的 DEFINE_* 必须在全局作用域、TU 顶层；此处定义 verbose 公共 flag。
// 注：--verbose 历史上用于开 debug 级别日志，但 CODE_CONVENTIONS §4 禁止 debug/trace
// 级别日志，故此 flag 现仅作命令行兼容占位，不再改变日志级别（统一 info）。
// ============================================================================
DEFINE_bool(verbose, false, "Enable verbose logging (deprecated, no effect)");

void BaseApplicationEntry::profilerStopShutdown()
{
    auto& profilerManager = mc::profiler::ProfilerManager::instance();
    profilerManager.stopTracing();
    profilerManager.shutdown();
}

void BaseApplicationEntry::printBanner() const
{
    // ASCII art 主体（客户端/服务端共享），标题行用 displayName() 区分。
    std::cout << R"(
  __  __      _____           _____ ______
 |  \/  | /\  / ____|    /\   |  __ \___  /
 | \  / /  \ | |       /  \  | |__) | / /
 | |\/| / /\ \| |      / /\ \ |  ___/ / /
 | |\/| |/ ____ \ |____ / ____ \| |   / /__
 |_|  _/_/    \_\_____/_/    \_\_|  /_____/
    | |         | |
    | |__   __ _| | _____ _ __
    | '_ \ / _` | |/ / _ \ '__|
    | |_) | (_| |   <  __/ |
    |_.__/ \__,_|_|\_\___|_|

)" << std::endl;

    std::cout << "  Cubium " << displayName() << " v" << MC_VERSION_MAJOR << "." << MC_VERSION_MINOR << "."
              << MC_VERSION_PATCH << std::endl;
    std::cout << "  ========================================\n" << std::endl;
}

void BaseApplicationEntry::printBuildInfo() const
{
    std::cout << "  Build Information:\n";
    std::cout << "  -------------------\n";

    // 版本信息
    std::cout << "  Version:    " << MC_VERSION_STRING << "\n";

    // Git 信息
    std::cout << "  Git Branch: " << MC_GIT_BRANCH << "\n";
    std::cout << "  Git Commit: " << MC_GIT_COMMIT_HASH;
#ifdef MC_GIT_DIRTY
    std::cout << " (dirty)";
#endif
    std::cout << "\n";

    // 构建信息
    std::cout << "  Build Time: " << MC_BUILD_TIME << "\n";
    std::cout << "  Build Type: " << MC_BUILD_TYPE << "\n";
    std::cout << "  Platform:   " << MC_BUILD_PLATFORM << " " << MC_BUILD_ARCH << "\n";

    // 编译器信息
    std::cout << "  Compiler:   " << MC_COMPILER_STRING << "\n";

    std::cout << "\n";
}

int BaseApplicationEntry::run(int argc, char* argv[])
{
    // 1. 安装崩溃处理器：捕获 SEH 异常、信号、纯虚函数调用等，输出调用栈和局部变量。
    mc::assert::CrashHandler::install();

    // 注册崩溃清理回调：崩溃时 flush Perfetto 跟踪数据（避免依赖进程退出时的析构链，
    // 析构链中途二次崩溃会丢 trace）。
    mc::assert::CrashHandler::setCleanupCallback([]() {
        profilerStopShutdown();
        std::cerr << "Perfetto tracing stopped due to crash" << std::endl;
    });

    // 2. 最早初始化异步日志：保证后续所有 spdlog 调用走异步（不阻塞主线程）。
    LogManager::instance().initialize();

    // 3. gflags 解析命令行。remove_flags=true 表示从 argv 移除已解析 flag；
    //    遇未知 flag 报错退出（gflags 内建）；--help 由 gflags 内建处理后 exit(1)。
    //    注：onRegisterFlags hook 在此之前调用——但 DEFINE_* 已在 TU 顶层全局构造，
    //    gflags 注册在 main 进入前即完成，故本 hook 仅给子类做非 DEFINE 注册的回调占位。
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    // 4. 子类把 FLAGS_* 填进自己的 params 结构体。
    onFlagsParsed();

    // 5. 打印 banner / 构建信息。
    printBanner();
    printBuildInfo();

    // 6. 进入核心业务前的准备（如 server 安装信号处理）。
    prepareRun();

    // 7. 核心业务运行（异常路径统一清理）。
    try {
        const int code = runApplication();

        // LogManager 须在 ProfilerManager shutdown 之前停（避免日志消费线程访问已销毁资源）。
        // shutdown 会 flush 队列，确保剩余日志落盘后再销毁 logger。
        LogManager::instance().shutdown();

        if (code != 0) {
            // 非零退出（如 server initialize 失败、gametest 有失败用例）：立即 stop+shutdown
            // profiler，避免依赖析构链（中途二次崩溃会丢 trace）。与原 HANDLE_ERROR 路径一致。
            profilerStopShutdown();
            std::cout << "Perfetto tracing stopped due to runtime error!" << std::endl;
        }
        // code == 0（正常退出）：不显式 profiler stop，依赖 Meyers 单例析构（与原 main 一致）。
        return code;
    }
    catch (const std::exception& e) {
        spdlog::critical("Fatal error: {}", e.what());

        // 子类错误清理（如 server cleanupServerGameTest：在脚本引擎销毁前释放 JS 句柄）。
        onErrorCleanup();

        // LogManager 须在 ProfilerManager shutdown 之前停（避免日志消费线程访问已销毁资源）。
        // shutdown 会 flush 队列，确保上面 critical 落盘后再销毁 logger。
        LogManager::instance().shutdown();

        // 异常路径立即 stop+shutdown profiler，避免依赖析构链（中途二次崩溃会丢 trace）。
        profilerStopShutdown();
        std::cout << "Perfetto tracing stopped due to runtime exception!" << std::endl;
        return 1;
    }
}

} // namespace mc::application
