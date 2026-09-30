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

#include "PerfettoProfilerAdapter.hpp"

#include "common/profiler/TraceEvents.hpp"

#include <benchmark/benchmark.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#else
#include <csignal>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
#endif

using namespace mc;
using namespace mc::trace;

namespace {

// 被测服务端可执行文件路径（相对当前工作目录，通常在仓库根目录运行 mc_benchmark）。
// 与 mc_benchmark 二进制的相对位置关系由构建布局决定（build/bin/RelWithDebInfo/）。
constexpr const char* SERVER_EXECUTABLE = "build/bin/RelWithDebInfo/minecraft-server";
#ifdef _WIN32
constexpr const char* SERVER_EXECUTABLE_WITH_EXT = "build/bin/RelWithDebInfo/minecraft-server.exe";
#endif

// 单次启动超时（毫秒）。冷启动 + 出生区域（23×23 区块）生成在低端机上可能超过 1 分钟。
constexpr i32 LAUNCH_TIMEOUT_MS = 300000;

// 被测进程返回码约定：124 = 超时被杀（与 GNU timeout 一致）。
constexpr int EXIT_CODE_TIMEOUT = 124;

/// 启动一次被测服务端并阻塞等待其退出。
/// \param modeFlag 服务端 benchmark 退出 flag（--benchmark-exit-after-shell-init /
///                 --benchmark-exit-after-world-init）
/// \param worldDir 本次启动专用的新临时世界目录
/// \return 进程退出码；启动失败返回 -1
[[nodiscard]] int launchServerOnce(const std::string& modeFlag, const std::filesystem::path& worldDir)
{
#ifdef _WIN32
    std::string commandLine = fmt::format(
        "\"{}\" {} --world-name \"{}\"", SERVER_EXECUTABLE_WITH_EXT, modeFlag, worldDir.filename().string());
    // TODO: --world-name 仅影响存档名，隔离需配合 --config 指向临时游戏目录；
    // Windows 路径处理与 CreateProcessW 工作目录参数待补全后移除此 TODO。
    std::vector<char> commandBuffer(commandLine.begin(), commandLine.end());
    commandBuffer.push_back('\0');

    STARTUPINFOA startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo{};

    const BOOL createResult = CreateProcessA(nullptr,
        commandBuffer.data(),
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr, // 工作目录 = 继承 mc_benchmark 的 CWD（仓库根）
        &startupInfo,
        &processInfo);
    if (createResult == FALSE) {
        return -1;
    }

    const DWORD waitResult = WaitForSingleObject(processInfo.hProcess, static_cast<DWORD>(LAUNCH_TIMEOUT_MS));
    if (waitResult == WAIT_TIMEOUT) {
        TerminateProcess(processInfo.hProcess, EXIT_CODE_TIMEOUT);
        WaitForSingleObject(processInfo.hProcess, 5000);
    }
    DWORD exitCode = 0;
    GetExitCodeProcess(processInfo.hProcess, &exitCode);
    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    return static_cast<int>(exitCode);
#else
    // 临时存档隔离：每次启动用全新游戏目录（--config 指向临时目录内的空配置，
    // 服务端从配置路径推导游戏目录与 saves/），保证每次启动都是全新世界冷启动。
    const std::string configPath = (worldDir / "server_options.json").string();

    const pid_t pid = fork();
    if (pid < 0) {
        return -1;
    }
    if (pid == 0) {
        // 子进程：启动被测服务端。超时保护用外部方式不可用，这里以 alarm 兜底
        // （启动超过 LAUNCH_TIMEOUT_MS 秒后 SIGALRM 终止子进程）。
        std::signal(SIGALRM, [](int) { _exit(EXIT_CODE_TIMEOUT); });
        alarm(static_cast<unsigned>(LAUNCH_TIMEOUT_MS / 1000));

        // 掩蔽服务端 stdout/stderr 之外的额外 fd 继承（保持 0/1/2 原样输出日志）。
        execl(SERVER_EXECUTABLE,
            SERVER_EXECUTABLE,
            modeFlag.c_str(),
            "--config",
            configPath.c_str(),
            "--profiler_enabled=false",
            static_cast<char*>(nullptr));
        _exit(127); // exec 失败
    }

    int status = 0;
    const pid_t waited = waitpid(pid, &status, 0);
    if (waited != pid) {
        return -1;
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }
    return -1;
#endif
}

/// 为一次启动准备全新的临时世界目录（空目录，内含空配置文件）。
[[nodiscard]] std::filesystem::path prepareTempWorldDir(const std::string& caseName, i32 repetition)
{
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path base = fs::temp_directory_path(ec) / "mc_benchmark_server_init";
    fs::create_directories(base, ec);

    // 目录名带用例名与重复序号，跨用例/跨重复完全隔离；先清理同名残留。
    fs::path dir = base / fmt::format("{}_{}", caseName, repetition);
    fs::remove_all(dir, ec);
    fs::create_directories(dir, ec);

    // 写一个最小配置：隔离世界目录名（防止污染默认存档）+ 随机端口避免与
    // 开发中服务端冲突。服务端从配置路径推导游戏目录（saves/<worldName>）。
    // JSON 结构与 ServerSettings 的分组注册一致：world.worldName / network.serverPort。
    std::ofstream options(dir / "server_options.json");
    options << "{\n"
            << "  \"world\": {\n"
            << "    \"worldName\": \"" << dir.filename().string() << "\"\n"
            << "  },\n"
            << "  \"network\": {\n"
            << "    \"serverPort\": 0\n"
            << "  }\n"
            << "}\n";
    return dir;
}

void cleanupTempWorldDir(const std::filesystem::path& dir)
{
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

/**
 * @brief 服务端启动基准（一次性事件测量）
 *
 * 每次重复：新建临时世界目录 → 启动外部 minecraft-server 进程（带 benchmark
 * 退出 flag）→ 阻塞等待其退出 → 删除临时目录。库对每个重复恰好执行一次迭代，
 * 时间即被测进程从 fork/exec 到退出的总耗时（含进程创建与回收开销，量级
 * 数毫秒，相对秒级启动耗时可忽略）。
 *
 * 迭代模型：->Repetitions(5)->Iterations(1)，禁止库自动校准迭代次数
 * （自动迭代会启动数百个服务端进程）。统计（mean/median/stddev）由
 * --benchmark_repetitions 聚合机制在 5 次重复上计算。
 *
 * trace 范围：仅基准进程侧（启停/等待事件）。被测服务端自身不写 trace
 * （启动命令显式传 --profiler_enabled=false，与原 client benchmark 模式一致）。
 *
 * 被测 flag：
 * - server_initialize_shell:  --benchmark-exit-after-shell-init
 *   （子系统初始化 + 网络监听就绪即退出）
 * - server_initialize_world:  --benchmark-exit-after-world-init
 *   （再加世界创建 + SPAWN_CHUNK_RADIUS 出生区块全部生成 FULL 后退出）
 */
void serverInitializeShell(::benchmark::State& state)
{
    const std::string modeFlag = "--benchmark-exit-after-shell-init";

    for (auto _ : state) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Benchmark.Run, "ServerInitialize::shell");

        // 设置本用例的 Perfetto trace 文件名主干（每次重复一个文件）。
        mc::benchmark::PerfettoProfilerAdapter::setCaseName("server_initialize_shell");

        const std::filesystem::path worldDir =
            prepareTempWorldDir("shell", static_cast<i32>(state.iterations()));
        const int exitCode = launchServerOnce(modeFlag, worldDir);
        cleanupTempWorldDir(worldDir);

        if (exitCode != 0) {
            state.SkipWithError(fmt::format("server exited with code {}", exitCode));
            return;
        }
    }
}

void serverInitializeWorld(::benchmark::State& state)
{
    const std::string modeFlag = "--benchmark-exit-after-world-init";

    for (auto _ : state) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Benchmark.Run, "ServerInitialize::world");

        // 设置本用例的 Perfetto trace 文件名主干（每次重复一个文件）。
        mc::benchmark::PerfettoProfilerAdapter::setCaseName("server_initialize_world");

        const std::filesystem::path worldDir =
            prepareTempWorldDir("world", static_cast<i32>(state.iterations()));
        const int exitCode = launchServerOnce(modeFlag, worldDir);
        cleanupTempWorldDir(worldDir);

        if (exitCode != 0) {
            state.SkipWithError(fmt::format("server exited with code {}", exitCode));
            return;
        }
    }
}

} // namespace

// 每次重复恰一次迭代；5 次重复取 mean/median/stddev。
BENCHMARK(serverInitializeShell)->Repetitions(5)->Iterations(1)->Unit(::benchmark::kMillisecond);
BENCHMARK(serverInitializeWorld)->Repetitions(5)->Iterations(1)->Unit(::benchmark::kMillisecond);
