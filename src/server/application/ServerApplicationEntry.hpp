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

#include "common/application/BaseApplicationEntry.hpp"

#include "server/application/StandaloneServer.hpp"

#include <atomic>

namespace mc::server {

/**
 * @brief 服务端进程入口（BaseApplicationEntry 子类）
 *
 * 把原 server/main.cpp 的过程式启动流程封装为对象：
 * - onFlagsParsed：把 gflags 的 --config / --gametest 填进 StandaloneServerParams + m_gametestMode。
 * - prepareRun：安装 SIGINT/SIGTERM 信号处理（置 g_shouldExit，主循环轮询退出）。
 * - runApplication：--gametest 走 GameTestServer 无头门面；否则构造 StandaloneServer 完整生命周期
 *   （initialize → initializeServerGameTest → run → 轮询 isRunning → cleanupServerGameTest → stop）。
 * - onErrorCleanup：异常路径调 cleanupServerGameTest（在脚本引擎销毁前释放 JS 句柄）。
 *
 * GameTest 接入代码（initializeServerGameTest/cleanupServerGameTest）保留在本 TU——mc_test 仅
 * minecraft-server exe 链接，client exe 不链接，故不能上提到 common 的 mc_common。
 */
class ServerApplicationEntry : public mc::application::BaseApplicationEntry {
public:
    ServerApplicationEntry() = default;
    ~ServerApplicationEntry() override = default;

    ServerApplicationEntry(const ServerApplicationEntry&) = delete;
    ServerApplicationEntry& operator=(const ServerApplicationEntry&) = delete;

protected:
    [[nodiscard]] std::string_view displayName() const override { return "Server"; }

    void onFlagsParsed() override;
    void prepareRun() override;
    [[nodiscard]] int runApplication() override;
    void onErrorCleanup() override;

private:
    /// 启动参数（由 onFlagsParsed 从 gflags 填充）。
    StandaloneServerParams m_params;

    /// --gametest：走 GameTestServer 无头批量自动跑门面。
    bool m_gametestMode{false};

    /// --gametest-report：JUnit XML 输出路径（空=不写文件，仅 stdout 日志）。
    std::string m_gametestReportPath;

    /// --gametest-tests：测试名过滤通配符（空=跑全部非 manualOnly 非 broken 测试）。
    std::string m_gametestTestsFilter;

    /// 收到 SIGINT/SIGTERM 后置 true，runApplication 主循环轮询此标志优雅退出。
    static std::atomic<bool> s_shouldExit;

    /// 信号处理函数（SIGINT/SIGTERM）。
    static void _signalHandler(int signal);

    /// 接入 GameTest 框架到生产服务器（/gametest 命令 + GameTestTicker 驱动 + JS gametest 模块）。
    /// 仅 minecraft-server exe 调用，须在 server.initialize 成功后调用。
    static void _initializeServerGameTest(MinecraftServer& server);

    /// 关闭前清理 GameTest 框架（须在 server.stop 销毁脚本引擎之前调用）。
    static void _cleanupServerGameTest();
};

} // namespace mc::server
