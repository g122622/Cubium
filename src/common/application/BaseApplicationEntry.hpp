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
 *   8. try { runApplication() } catch { onErrorCleanup() + profiler stop/shutdown }
 *
 * LogManager::shutdown() 须在 ProfilerManager shutdown 之前调用（见 LogManager 文档），
 * 故正常/异常路径都先 LogManager::shutdown() 再 profilerStopShutdown()。
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
    /// 崩溃清理回调：崩溃时 flush Perfetto 跟踪数据（与原 main 一致）。
    static void profilerStopShutdown();

    /// 打印 ASCII banner（用 displayName() 区分标题字样）。
    void printBanner() const;
    /// 打印构建信息（版本/Git/构建/编译器）。
    void printBuildInfo() const;
};

} // namespace mc::application
