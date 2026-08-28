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

#include "common/profiler/ProfilerConfig.hpp"
#include "common/profiler/ProfilerManager.hpp"
#include "common/util/PlatformInfo.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "minecraft-reborn/version.h"

#include <gflags/gflags.h>
#include <spdlog/spdlog.h>

#include <iostream>

namespace mc::application {

namespace {
// RAII 守卫：在进程入口提升 Windows 定时器分辨率到 1ms，使 std::this_thread::sleep_for
// 精度从默认 ~15.6ms 提升到 ~1ms（覆盖内存采样线程 1ms 间隔、服务端 tick 节流、客户端
// 帧率限制三处对 sleep 精度敏感的主循环）。析构时归还。进程级单次调用，覆盖本进程所有线程。
// 非 Windows 平台 request/release 为空实现。
class _HighResTimerGuard {
public:
    _HighResTimerGuard() { mc::util::PlatformInfo::requestHighResTimer(); }
    ~_HighResTimerGuard() { mc::util::PlatformInfo::releaseHighResTimer(); }

    _HighResTimerGuard(const _HighResTimerGuard&) = delete;
    _HighResTimerGuard& operator=(const _HighResTimerGuard&) = delete;
    _HighResTimerGuard(_HighResTimerGuard&&) = delete;
    _HighResTimerGuard& operator=(_HighResTimerGuard&&) = delete;
};
} // namespace

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

std::string BaseApplicationEntry::profilerOutputPath() const
{
    return MC_TRACE_DEFAULT_OUTPUT;
}

std::string BaseApplicationEntry::profilerProcessName() const
{
    return "Minecraft";
}

std::string BaseApplicationEntry::profilerThreadName() const
{
    return "MainThread";
}

void BaseApplicationEntry::profilerStart()
{
    auto& profilerManager = mc::profiler::ProfilerManager::instance();

    // 构造 TraceConfig：仅 outputPath 差异（客户端/服务端），bufferSizeKb 固定 65536*8（512MB）。
    mc::profiler::TraceConfig traceConfig;
    traceConfig.outputPath = profilerOutputPath();
    traceConfig.bufferSizeKb = 65536 * 8;
    profilerManager.initialize(traceConfig);

    // 注入进程内存采样回调（须在 startTracing 之前）：ProfilerManager 处于比 PlatformInfo
    // 更底层的 mc_profiler 库，不能直接依赖 PlatformInfo，故由本层注入。返回 {工作集MB, 提交量MB}。
    profilerManager.setMemorySampler([]() -> std::pair<i64, i64> {
        return {static_cast<i64>(util::PlatformInfo::getProcessMemoryMB()),
            static_cast<i64>(util::PlatformInfo::getProcessCommitMB())};
    });

    profilerManager.startTracing();

    // 设置进程和主线程名称（双轨 Perfetto+Tracy）
    profilerManager.setProcessName(profilerProcessName());
    profilerManager.setThreadName(profilerThreadName());
    spdlog::info("Perfetto tracing initialized");
}

void BaseApplicationEntry::profilerStop()
{
    auto& profilerManager = mc::profiler::ProfilerManager::instance();
    profilerManager.stopTracing();
    profilerManager.shutdown();
    spdlog::info("Perfetto tracing stopped");
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

    // 提升进程定时器分辨率到 1ms（仅 Windows）。放在 CrashHandler::install 之后：崩溃时
    // OS 进程终止会自动回收分辨率计数，无需在 crash cleanup 回调里 release。RAII 守卫
    // 覆盖本函数全部退出路径（含 catch 异常）。覆盖内存采样线程/服务端 tick/客户端帧率
    // 限制三处对 sleep_for 精度敏感的主循环。
    _HighResTimerGuard highResTimerGuard;

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

    // 7. 启动 profiler（若子类门控允许）。须在 onFlagsParsed 之后：shouldEnableProfiler()
    // 依赖子类已填充的 benchmark/gametest 字段。profilerStop() 须在 LogManager::shutdown()
    // 之前（profiler 的 stop/shutdown 路径内部用 spdlog）。
    if (shouldEnableProfiler()) {
        profilerStart();
        m_profilerStarted = true;
    }

    // 8. 核心业务运行（异常路径统一清理）。
    try {
        const int code = runApplication();

        // 先 stop profiler（若已启动），再 shutdown LogManager。
        // 顺序原因：ProfilerManager 的 stopTracing/shutdown 路径内部调用 spdlog::info/warn/error，
        // 需要 spdlog 全局 logger/thread_pool/sink 仍然有效；LogManager::shutdown() 会调
        // spdlog::shutdown() 销毁这些全局状态。若颠倒顺序，profiler stop 路径的 spdlog 调用会
        // 操作已销毁资源。
        if (m_profilerStarted) {
            profilerStop();
        }

        LogManager::instance().shutdown();
        return code;
    }
    catch (const std::exception& e) {
        spdlog::critical("Fatal error: {}", e.what());

        // 子类错误清理（如 server cleanupServerGameTest：在脚本引擎销毁前释放 JS 句柄）。
        onErrorCleanup();

        // 先 stop profiler，再 shutdown LogManager（理由同正常路径）。
        if (m_profilerStarted) {
            profilerStopShutdown();
        }

        LogManager::instance().shutdown();
        return 1;
    }
}

} // namespace mc::application
