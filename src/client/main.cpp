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

#include "application/ClientApplication.hpp"
#include "common/profiler/ProfilerManager.hpp"
#include "minecraft-reborn/version.h"

#include <exception>
#include <iostream>
#include <string>
#include <spdlog/spdlog.h>

#include "common/util/assert/AssertAll.hpp"
#include "common/util/assert/CrashHandler.hpp"

namespace {

void printBanner()
{
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

    std::cout << "  Cubium Client v" << MC_VERSION_MAJOR << "." << MC_VERSION_MINOR << "." << MC_VERSION_PATCH
              << std::endl;
    std::cout << "  ========================================\n" << std::endl;
}

/**
 * @brief 打印构建信息
 *
 * 输出版本、Git提交、构建时间、编译器等详细信息
 */
void printBuildInfo()
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

void printHelp()
{
    std::cout << "Usage: minecraft-client [options]\n\n"
              << "Options:\n"
              << "  -h, --help          Show this help message\n"
              << "  --config <path>     Use custom config file path\n"
              << "  -v, --verbose       Enable verbose logging (debug level)\n"
              << "  --skip-integrated   Skip integrated server (for external server)\n"
              << "  --quick-play <id>   Skip main menu and load world with given level ID\n"
              << "  --quick-play-new    Skip main menu and create a new world\n"
              << "  --benchmark-exit-after-initialize\n"
              << "                      Run only ClientApplication::initialize shell path, then exit\n"
              << std::endl;
}

} // namespace

int main(int argc, char* argv[])
{
    // 安装崩溃处理器：捕获 SEH 异常、信号、纯虚函数调用等，
    // 输出调用栈和局部变量信息到终端
    mc::assert::CrashHandler::install();

    // 注册崩溃清理回调：崩溃时刷新 Perfetto 跟踪数据
    mc::assert::CrashHandler::setCleanupCallback([]() {
        auto& profilerManager = mc::profiler::ProfilerManager::instance();
        profilerManager.stopTracing();
        profilerManager.shutdown();
        std::cerr << "Perfetto tracing stopped due to crash" << std::endl;
    });

    // 打印Banner
    printBanner();

    // 打印构建信息
    printBuildInfo();

    // 启动参数
    mc::client::ClientLaunchParams params;

    // 解析命令行参数
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            printHelp();
            return 0;
        }
        if (arg == "--config" && i + 1 < argc) {
            params.configPath = argv[++i];
        }
        if (arg == "-v" || arg == "--verbose") {
            // 通过设置日志级别来启用详细日志
        }
        if (arg == "--skip-integrated") {
            params.skipIntegratedServer = true;
        }
        if (arg == "--quick-play" && i + 1 < argc) {
            params.quickPlayLevelId = argv[++i];
        }
        if (arg == "--quick-play-new") {
            params.quickPlayNew = true;
        }
        if (arg == "--benchmark-exit-after-initialize") {
            params.benchmarkExitAfterInitialize = true;
        }
    }

    params.quickPlayNew = true;

    try {
        // 创建客户端实例
        mc::client::ClientApplication client;

        // 初始化
        auto initResult = client.initialize(params);
        if (initResult.failed()) {
            spdlog::error("Failed to initialize client: {}", initResult.error().toString());
            goto HANDLE_ERROR;
        }

        if (params.benchmarkExitAfterInitialize) {
            spdlog::info("Benchmark initialize-only run completed successfully");
            return 0;
        }

        // 运行主循环
        auto runResult = client.run();
        if (runResult.failed()) {
            spdlog::error("Client error: {}", runResult.error().toString());
            goto HANDLE_ERROR;
        }

        spdlog::info("Client exited successfully");
        return 0;
    }
    catch (const std::exception& e) {
        spdlog::critical("Fatal error: {}", e.what());
        goto HANDLE_ERROR;
    }

HANDLE_ERROR:
    auto& profilerManager = mc::profiler::ProfilerManager::instance();
    profilerManager.stopTracing();
    profilerManager.shutdown();
    std::cout << "Perfetto tracing stopped due to runtime exception!" << std::endl;

    return 1;
}
