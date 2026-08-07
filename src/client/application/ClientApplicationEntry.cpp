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

#include "ClientApplicationEntry.hpp"

#include <gflags/gflags.h>
#include <spdlog/spdlog.h>

namespace mc::client {

// ============================================================================
// client 专属 gflags flag 定义（TU 顶层全局作用域）。
// gflags 把连字符规范化为下划线：--skip-integrated 命中 skip_integrated、
// --quick-play 命中 quick_play、--quick-play-new 命中 quick_play_new、
// --benchmark-exit-after-initialize 命中 benchmark_exit_after_initialize。
// ============================================================================
DEFINE_string(config, "", "Use custom config file path");
DEFINE_bool(skip_integrated, false, "Skip integrated server (for external server)");
DEFINE_string(quick_play, "", "Skip main menu and load world with given level ID");
DEFINE_bool(quick_play_new, false, "Skip main menu and create a new world");
DEFINE_bool(benchmark_exit_after_initialize, false, "Run only ClientApplication::initialize shell path, then exit");

void ClientApplicationEntry::onFlagsParsed()
{
    // config：空串 → nullopt（用默认路径）；非空 → 指定路径。
    if (!FLAGS_config.empty()) {
        m_params.configPath = FLAGS_config;
    }

    m_params.skipIntegratedServer = FLAGS_skip_integrated;

    // quick_play：空串 → nullopt；非空 → 指定世界 level id。
    if (!FLAGS_quick_play.empty()) {
        m_params.quickPlayLevelId = FLAGS_quick_play;
    }

    m_params.benchmarkExitAfterInitialize = FLAGS_benchmark_exit_after_initialize;

    // 历史遗留硬编码：无条件强制启用 quick-play-new（原 client/main.cpp:160）。
    // 故 --quick-play-new flag 实际无效，仅占位。TODO: 未来若需命令行控制 quick-play-new，
    // 移除此行并改用 FLAGS_quick_play_new。
    m_params.quickPlayNew = true;
}

int ClientApplicationEntry::runApplication()
{
    // 创建客户端实例
    ClientApplication client;

    // 初始化
    auto initResult = client.initialize(m_params);
    if (initResult.failed()) {
        spdlog::error("Failed to initialize client: {}", initResult.error().toString());
        return 1;
    }

    if (m_params.benchmarkExitAfterInitialize) {
        spdlog::info("Benchmark initialize-only run completed successfully");
        return 0;
    }

    // 运行主循环（阻塞到窗口关闭）
    auto runResult = client.run();
    if (runResult.failed()) {
        spdlog::error("Client error: {}", runResult.error().toString());
        return 1;
    }

    spdlog::info("Client exited successfully");
    return 0;
}

} // namespace mc::client
