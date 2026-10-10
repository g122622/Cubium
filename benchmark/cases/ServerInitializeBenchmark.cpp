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
#include "ServerExecutablePath.hpp"

#include "common/profiler/TraceEvents.hpp"

#include <benchmark/benchmark.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#ifndef _WIN32
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

using namespace mc;
using namespace mc::trace;

namespace {

#ifdef _WIN32

/// ASCII 字面量（被测可执行文件路径、benchmark flag 名）→ 宽字符。
[[nodiscard]] std::wstring widenAscii(std::string_view ascii)
{
    return std::wstring(ascii.begin(), ascii.end());
}
#endif

// 单次启动超时（毫秒）。冷启动 + 出生区域（23×23 区块）生成在低端机上可能超过 1 分钟。
constexpr i32 LAUNCH_TIMEOUT_MS = 300000;

// 被测进程返回码约定：124 = 超时被杀（与 GNU timeout 一致）。
constexpr i32 EXIT_CODE_TIMEOUT = 124;

// 被测进程控制台日志文件名（落在本次启动专用的临时世界目录内，随目录一起清理）。
constexpr const char* SERVER_CONSOLE_LOG_NAME = "server_console.log";

// 失败回显日志时头/尾各保留的行数上限：服务端启动日志通常上千行，全量回显会刷屏，
// 而关键信息既可能在开头（gflags/配置解析报错）也可能在结尾（崩溃前的最后动作）。
constexpr std::size_t LOG_ECHO_HEAD_LINES = 80;
constexpr std::size_t LOG_ECHO_TAIL_LINES = 120;

/// 回显被测进程的控制台日志（仅在启动失败或非零退出时调用）。
/// \param logPath 日志文件路径
/// \param exitCode 被测进程退出码
void echoServerConsoleLog(const std::filesystem::path& logPath, i32 exitCode)
{
    std::ifstream input(logPath, std::ios::binary);
    if (!input.is_open()) {
        spdlog::warn(
            "benchmark: server exited with code {} and no console log was captured ({})", exitCode, logPath.string());
        return;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(input, line)) {
        lines.push_back(std::move(line));
    }

    spdlog::error("benchmark: server exited with code {}; console log: {}", exitCode, logPath.string());
    if (lines.empty()) {
        std::cerr << "[mc_benchmark] (server console log is empty)" << std::endl;
        return;
    }

    const std::size_t total = lines.size();
    const std::size_t headCount = std::min(total, LOG_ECHO_HEAD_LINES);
    // 未超上限时把剩余行全部放在尾部一起回显，保证短日志不被截断。
    const std::size_t tailCount =
        total > headCount ? std::min(total - headCount, LOG_ECHO_TAIL_LINES) : static_cast<std::size_t>(0);
    const std::size_t omitted = total - headCount - tailCount;

    std::cerr << "[mc_benchmark] ----- server console log begin -----\n";
    for (std::size_t i = 0; i < headCount; ++i) {
        std::cerr << lines[i] << '\n';
    }
    if (omitted > 0) {
        std::cerr << "[mc_benchmark] ... " << omitted << " lines omitted ...\n";
    }
    for (std::size_t i = total - tailCount; i < total; ++i) {
        std::cerr << lines[i] << '\n';
    }
    std::cerr << "[mc_benchmark] ----- server console log end -----" << std::endl;
}

/// 启动一次被测服务端并阻塞等待其退出。
///
/// 被测进程的 stdout/stderr 一律重定向到 `<worldDir>/server_console.log`（两个平台行为
/// 一致）：既能拿到启动失败的真实原因（gflags 报错、存档初始化失败等），又不会让多次
/// 重复的服务端日志与基准输出交错刷屏。非零退出时由 echoServerConsoleLog 回显日志。
///
/// \param modeFlag 服务端 benchmark 退出 flag（--benchmark-exit-after-shell-init /
///                 --benchmark-exit-after-world-init）
/// \param worldDir 本次启动专用的新临时世界目录（配置与控制台日志都落在其中）
/// \return 进程退出码；启动失败返回 -1
[[nodiscard]] i32 launchServerOnce(const std::string& modeFlag, const std::filesystem::path& worldDir)
{
    std::error_code pathError;
    const auto serverExecutable =
        mc::benchmark::detail::resolveServerExecutable(MC_BENCHMARK_SERVER_FILE_NAME, pathError);
    if (pathError) {
        spdlog::error("benchmark: failed to resolve server executable path: {}", pathError.message());
        return -1;
    }
    const std::filesystem::path logPath = worldDir / SERVER_CONSOLE_LOG_NAME;
    i32 exitCode = -1;

#ifdef _WIN32
    // 与 POSIX 分支参数完全对齐：--config 指向本次启动专用的临时配置，服务端由配置路径
    // 推导游戏目录（saves/<worldName> 落在临时目录内），保证每次都是全新世界冷启动；
    // --profiler_enabled=false 关闭被测进程侧的 Perfetto。
    //
    // 命令行整体以宽字符拼装：std::filesystem::path::string() 在 Windows 上按 ANSI 代码页
    // 编码（并非 UTF-8），经窄字符串中转会让含非 ASCII 的临时目录路径失真，必须直接用
    // path::wstring()。
    // TODO: 被测服务端走窄字符 main()（gflags 解析 char** argv），MSVC CRT 会把宽命令行按
    // 当前 ANSI 代码页转回窄 argv，因此临时目录路径中超出该代码页的字符仍无法传递；如需
    // 完全 Unicode 支持，须服务端改用 wmain / UTF-8 argv（app manifest activeCodePage）。
    const std::filesystem::path configPath = worldDir / "server_options.json";
    const std::wstring wideExecutable = serverExecutable.wstring();
    std::wstring wideCommandLine = L"\"" + wideExecutable + L"\" " + widenAscii(modeFlag) + L" --config \"" +
        configPath.wstring() + L"\" --profiler_enabled=false";

    // 子进程 std 句柄指向日志文件（STARTF_USESTDHANDLES + 可继承句柄）。
    // 仍保留 CREATE_NO_WINDOW：子进程的控制台窗口不显示，但 std 输出已被文件接管。
    const HANDLE logHandle = CreateFileW(logPath.wstring().c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (logHandle == INVALID_HANDLE_VALUE) {
        spdlog::warn(
            "benchmark: failed to create server console log {} (GetLastError={})", logPath.string(), GetLastError());
        return -1;
    }
    if (SetHandleInformation(logHandle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT) == FALSE) {
        spdlog::warn("benchmark: failed to make console log handle inheritable (GetLastError={})", GetLastError());
        CloseHandle(logHandle);
        return -1;
    }

    // stdin 透传父进程的标准输入（被测服务端当前不读 stdin，留作后续交互式诊断的入口）。
    const HANDLE stdInput = GetStdHandle(STD_INPUT_HANDLE);

    STARTUPINFOW startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.dwFlags = STARTF_USESTDHANDLES;
    startupInfo.hStdInput = (stdInput == nullptr || stdInput == INVALID_HANDLE_VALUE) ? nullptr : stdInput;
    startupInfo.hStdOutput = logHandle;
    startupInfo.hStdError = logHandle;
    PROCESS_INFORMATION processInfo{};

    const BOOL createResult = CreateProcessW(wideExecutable.c_str(),
        wideCommandLine.data(),
        nullptr,
        nullptr,
        TRUE, // bInheritHandles：STARTF_USESTDHANDLES 指定的句柄必须可继承
        CREATE_NO_WINDOW,
        nullptr,
        nullptr, // 工作目录 = 继承 mc_benchmark 的 CWD（仓库根）
        &startupInfo,
        &processInfo);
    CloseHandle(logHandle); // 子进程已持有自己的副本，父进程不再需要
    if (createResult == FALSE) {
        spdlog::warn("benchmark: failed to launch server process (GetLastError={})", GetLastError());
        return -1;
    }

    const DWORD waitResult = WaitForSingleObject(processInfo.hProcess, static_cast<DWORD>(LAUNCH_TIMEOUT_MS));
    if (waitResult == WAIT_TIMEOUT) {
        TerminateProcess(processInfo.hProcess, EXIT_CODE_TIMEOUT);
        WaitForSingleObject(processInfo.hProcess, 5000);
    }
    DWORD processExitCode = 0;
    GetExitCodeProcess(processInfo.hProcess, &processExitCode);
    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    exitCode = static_cast<i32>(processExitCode);
#else
    // 临时存档隔离：每次启动用全新游戏目录（--config 指向临时目录内的空配置，
    // 服务端从配置路径推导游戏目录与 saves/），保证每次启动都是全新世界冷启动。
    const std::string configPath = (worldDir / "server_options.json").string();
    const std::string executablePath = serverExecutable.string();

    // 控制台输出重定向到日志文件（与 Windows 分支行为一致）。
    const std::string logPathString = logPath.string();
    const i32 logFd = ::open(logPathString.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (logFd < 0) {
        spdlog::warn("benchmark: failed to create server console log {} (errno={})", logPathString, errno);
        return -1;
    }

    const pid_t pid = fork();
    if (pid < 0) {
        ::close(logFd);
        return -1;
    }
    if (pid == 0) {
        // 子进程：stdout/stderr 指向日志文件。超时保护用外部方式不可用，这里以 alarm 兜底
        // （启动超过 LAUNCH_TIMEOUT_MS 秒后 SIGALRM 终止子进程）。
        if (::dup2(logFd, STDOUT_FILENO) < 0 || ::dup2(logFd, STDERR_FILENO) < 0) {
            _exit(127);
        }
        if (logFd > STDERR_FILENO) {
            ::close(logFd);
        }

        std::signal(SIGALRM, [](i32) { _exit(EXIT_CODE_TIMEOUT); });
        alarm(static_cast<unsigned>(LAUNCH_TIMEOUT_MS / 1000));

        execl(executablePath.c_str(),
            executablePath.c_str(),
            modeFlag.c_str(),
            "--config",
            configPath.c_str(),
            "--profiler_enabled=false",
            static_cast<char*>(nullptr));
        _exit(127); // exec 失败
    }
    ::close(logFd); // 父进程不再持有日志文件描述符

    i32 status = 0;
    const pid_t waited = waitpid(pid, &status, 0);
    if (waited != pid) {
        return -1;
    }
    if (WIFEXITED(status)) {
        exitCode = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        exitCode = 128 + WTERMSIG(status);
    }
#endif

    if (exitCode != 0) {
        echoServerConsoleLog(logPath, exitCode);
    }
    return exitCode;
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

        const std::filesystem::path worldDir = prepareTempWorldDir("shell", static_cast<i32>(state.iterations()));
        const i32 exitCode = launchServerOnce(modeFlag, worldDir);

        if (exitCode != 0) {
            // 失败时保留临时世界目录（内含 server_console.log，launchServerOnce 已回显头尾），
            // 供事后排查；同名目录会在下一次运行前被 prepareTempWorldDir 清理。
            state.SkipWithError(
                fmt::format("server exited with code {} (world dir kept: {})", exitCode, worldDir.string()));
            return;
        }
        cleanupTempWorldDir(worldDir);
    }
}

void serverInitializeWorld(::benchmark::State& state)
{
    const std::string modeFlag = "--benchmark-exit-after-world-init";

    for (auto _ : state) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Benchmark.Run, "ServerInitialize::world");

        // 设置本用例的 Perfetto trace 文件名主干（每次重复一个文件）。
        mc::benchmark::PerfettoProfilerAdapter::setCaseName("server_initialize_world");

        const std::filesystem::path worldDir = prepareTempWorldDir("world", static_cast<i32>(state.iterations()));
        const i32 exitCode = launchServerOnce(modeFlag, worldDir);

        if (exitCode != 0) {
            // 失败时保留临时世界目录（内含 server_console.log，launchServerOnce 已回显头尾），
            // 供事后排查；同名目录会在下一次运行前被 prepareTempWorldDir 清理。
            state.SkipWithError(
                fmt::format("server exited with code {} (world dir kept: {})", exitCode, worldDir.string()));
            return;
        }
        cleanupTempWorldDir(worldDir);
    }
}

} // namespace

// 每次重复恰一次迭代；5 次重复取 mean/median/stddev。
BENCHMARK(serverInitializeShell)->Repetitions(5)->Iterations(1)->Unit(::benchmark::kMillisecond);
BENCHMARK(serverInitializeWorld)->Repetitions(5)->Iterations(1)->Unit(::benchmark::kMillisecond);
