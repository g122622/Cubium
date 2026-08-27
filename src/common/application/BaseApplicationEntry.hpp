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

#pragma once

#include "common/profiler/ProfilerManager.hpp"

#include <string>
#include <string_view>

namespace mc::application {

/**
 * @brief 应用入口抽象基类（模板方法）
 *
 * 把客户端/服务端两个进程入口（main）的公共启动流程上提为模板方法 run()，差异点下沉为
 * protected 虚函数 hook，由子类 ClientApplicationEntry / ServerApplicationEntry 覆盖。
 * main 退化为：
 * @code
 *   mc::server::ServerApplicationEntry entry;
 *   return entry.run(argc, argv);
 * @endcode
 *
 * run() 公共骨架（顺序）：
 *   1. CrashHandler::install() + 注册 profiler 清理回调（崩溃时 flush Perfetto）
 *   2. LogManager::initialize()（最早，保证后续 spdlog 调用走异步）
 *   3. onRegisterFlags()（子类 DEFINE_* 在各自 TU 全局作用域，此 hook 仅作回调）
 *   4. gflags::ParseCommandLineFlags(&argc, &argv, true)（--help 由 gflags 内建处理；
 *      未知 flag 报错退出）
 *   5. onFlagsParsed()（子类把 FLAGS_* 填进自己的 params 结构体）
 *   6. printBanner() / printBuildInfo()（用 displayName() 区分 Server/Client）
 *   7. prepareRun()（如 server 侧安装信号处理）
 *   8. 若 shouldEnableProfiler()：profilerStart()（init + setMemorySampler + startTracing
 *      + setProcessName + setThreadName），由子类虚函数提供 outputPath/processName/threadName
 *   9. try { runApplication() } catch { onErrorCleanup() + profiler stop/shutdown }
 *
 * profiler 生命周期时序约束（关键）：
 *   - ProfilerManager 的 stopTracing/shutdown 路径依赖 spdlog 全局状态
 *     （PerfettoBackend::stopTracing/shutdown 内有 spdlog::info/warn/error 调用）。
 *   - LogManager::shutdown() 会调 spdlog::shutdown() 销毁 spdlog 全局 logger/thread_pool/sink。
 *   - 故 profilerStop() 必须在 LogManager::shutdown() 之前调用，否则 ProfilerManager 的
 *     stop/shutdown 路径中的 spdlog 调用会操作已销毁资源。
 *   - 正常路径：runApplication() 返回后先 profilerStop()（若 m_profilerStarted），再
 *     LogManager::shutdown()。
 *   - 异常路径：catch 块先 profilerStopShutdown()，再 LogManager::shutdown()。
 *
 * 重复 stop 幂等：PerfettoBackend::stopTracing 有 `if (!m_initialized || !m_tracing) return`，
 * shutdown 有 `if (!m_initialized) return`。正常路径显式 profilerStop 后，Meyers 单例析构再
 * stop 是 no-op。崩溃回调 profilerStopShutdown 重复调用亦安全。
 */
class BaseApplicationEntry {
public:
    virtual ~BaseApplicationEntry() = default;

    /**
     * @brief 进程入口模板方法。
     * @return 进程退出码（0 正常，非 0 失败）。main 直接 return 此值。
     */
    int run(int argc, char* argv[]);

protected:
    // —— profiler 生命周期 hook（子类覆盖以提供差异）——

    /**
     * @brief 是否在本进程启用 profiler。默认 true。
     *
     * 子类按命令行模式门控：客户端 benchmark 模式（--benchmark-exit-after-initialize）
     * 返回 false 以纯净测 Shell 初始化耗时；服务端 gametest 模式（--gametest）返回 false
     * 保持无头测试不写 trace。须在 onFlagsParsed() 填充相关字段后调用，故 profilerStart
     * 在 run() 骨架中位于 onFlagsParsed 之后。
     */
    [[nodiscard]] virtual bool shouldEnableProfiler() const { return true; }

    /**
     * @brief profiler 输出文件路径（Perfetto trace 文件）。子类提供。
     *
     * 默认返回 ProfilerConfig.hpp 的 MC_TRACE_DEFAULT_OUTPUT。客户端/服务端子类应覆盖为
     * MC_TRACE_CLIENT_OUTPUT / MC_TRACE_SERVER_OUTPUT。
     */
    [[nodiscard]] virtual std::string profilerOutputPath() const;

    /**
     * @brief profiler 进程名（双轨 Perfetto+Tracy）。子类提供。
     *
     * 默认 "Minecraft"。客户端子类覆盖为 "MinecraftClient"，服务端子类为 "MinecraftServer"。
     */
    [[nodiscard]] virtual std::string profilerProcessName() const;

    /**
     * @brief profiler 主线程名（双轨 Perfetto+Tracy）。子类提供。
     *
     * 默认 "MainThread"。客户端子类覆盖为 "ClientMainThread"，服务端子类为 "ServerMainThread"。
     */
    [[nodiscard]] virtual std::string profilerThreadName() const;

    // —— 子类可覆盖的 hook ——
    // 注：gflags 的 DEFINE_* 宏必须在全局作用域、TU 顶层定义（不能放类内/namespace 内），
    // 故本基类不提供"注册 flag"的虚函数让子类塞 DEFINE_*；onRegisterFlags 仅作解析前回调占位，
    // 子类若有非 DEFINE 的注册逻辑可在此做。flag 定义本身直接写子类 .cpp 顶层。

    /**
     * @brief 应用显示名（如 "Server" / "Client"），用于 banner 标题。
     */
    [[nodiscard]] virtual std::string_view displayName() const = 0;

    /**
     * @brief gflags 解析完毕后的回调：子类把 FLAGS_* 值填进自己的 params 结构体。
     *        默认空实现。
     */
    virtual void onFlagsParsed() {}

    /**
     * @brief 进入核心业务前的准备。默认空实现。如 server 侧安装 SIGINT/SIGTERM 信号处理。
     */
    virtual void prepareRun() {}

    /**
     * @brief 核心业务运行：子类创建并驱动自己的 application 对象，返回退出码。
     *        失败返回非 0，正常返回 0。
     */
    [[nodiscard]] virtual int runApplication() = 0;

    /**
     * @brief 异常/错误路径的额外清理。默认空实现。
     *        在基类已做 LogManager::shutdown() + profiler stop/shutdown 之前调用，
     *        使子类（如 server 的 cleanupServerGameTest）能在脚本引擎销毁前释放 JS 句柄。
     */
    virtual void onErrorCleanup() {}

private:
    /// 启动 profiler：init + setMemorySampler + startTracing + setProcessName + setThreadName。
    /// 幂等：已初始化则跳过。须在 LogManager::initialize() 之后调用（profiler 内部用 spdlog）。
    void profilerStart();

    /// 停止 profiler：stopTracing + shutdown。幂等。须在 LogManager::shutdown() 之前调用。
    /// 崩溃清理回调路径调此（经 profilerStopShutdown），重复调用安全。
    void profilerStop();

    /// 崩溃清理回调：崩溃时 flush Perfetto 跟踪数据（与原 main 一致）。
    static void profilerStopShutdown();

    /// 打印 ASCII banner（用 displayName() 区分标题字样）。
    void printBanner() const;
    /// 打印构建信息（版本/Git/构建/编译器）。
    void printBuildInfo() const;

    /// run() 中是否已 profilerStart（用于退出路径决定是否 profilerStop）。
    bool m_profilerStarted{false};
};

} // namespace mc::application
