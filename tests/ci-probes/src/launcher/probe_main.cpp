// 进程启动成本对照实验的入口（CI 临时工具，不进产品构建流程）。
//
// 背景：CI 上 ctest 每个用例都以独立进程运行，而单进程固定开销达 8~14 秒，且 CPU 时间与墙钟
// 基本相同（几乎全为用户态自旋）。本机同配置只需约 0.1 秒，无法本地复现。
//
// 该入口按 PROBE_MODE 选择要链接进进程的重型静态库与非 leaf 目标文件，用来做 A/B 对照：
//   triv    —— 只链接 libstdc++/libc 与 std::cout，交代「进程最基本的启动」在 CI 上要多久
//   fmt     —— 额外链接 fmt
//   spdlog  —— 额外链接 spdlog（编译版，依赖 fmt）
//   crasher —— 额外链接日志/断言实现（mc_assert_support 目标，含 CrashHandler.cpp）
//
// 输出格式（固定，便于 CI 侧提取）：
//   PROBE_TIME <phase> <ms>     main 入口或退出前的 steady_clock 读数
//   PROBE_DONE <wall_ms> <cpu_ms>
// 最后一个输出由静态对象析构完成，早于进程真正退出（动态链接器卸载、ASan 运行库收尾）。

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string_view>

#ifdef __linux__
#include <sys/resource.h>
#include <unistd.h>
#endif

#if PROBE_MODE >= 1
#include <fmt/format.h>
#endif

#if PROBE_MODE >= 2
#include <spdlog/spdlog.h>
#endif

#if PROBE_MODE >= 3
#include "common/util/assert/CrashHandler.hpp"
#endif

namespace {

std::chrono::steady_clock::time_point s_start;

i64 elapsedMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - s_start).count();
}

i64 cpuMs()
{
#ifdef __linux__
    struct rusage usage{};
    if (::getrusage(RUSAGE_SELF, &usage) != 0) {
        return -1;
    }
    return static_cast<i64>(usage.ru_utime.tv_sec + usage.ru_stime.tv_sec) * 1000 +
        (usage.ru_utime.tv_usec + usage.ru_stime.tv_usec) / 1000;
#else
    return -1;
#endif
}

/// 析构发生在退出序列中、C 运行库收尾之前，可区分「主逻辑耗时」与「静态析构耗时」。
struct ReportOnExit {
    ~ReportOnExit()
    {
        std::printf("PROBE_TIME exit_pre %lld\n", static_cast<long long>(elapsedMs()));
        std::fflush(stdout);
        const char* tag = std::getenv("PROBE_TAG");
        std::printf("PROBE_DONE %s %lld %lld\n",
            tag ? tag : "unknown",
            static_cast<long long>(elapsedMs()),
            static_cast<long long>(cpuMs()));
        std::fflush(stdout);
    }
};

ReportOnExit g_report;

/// 与测试进程一致地装一个捕获 SIGSEGV 的信号处理器：信号处理器与崩溃路径的符号表初始化
/// 只会在崩溃时触发，用于确认它们不是启动开销来源。
void installSignalHandler()
{
#ifdef __linux__
    static struct sigaction old{};
    struct sigaction action{};
    action.sa_handler = [](int) {
        (void)!::write(STDERR_FILENO, "probe sigsegv\n", 13);
    };
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESETHAND;
    ::sigaction(SIGSEGV, &action, &old);
#endif
}

} // namespace

int main(int argc, char** argv)
{
    s_start = std::chrono::steady_clock::now();
    std::printf("PROBE_TIME enter %lld\n", static_cast<long long>(elapsedMs()));

    installSignalHandler();
    std::printf("PROBE_TIME sigaction %lld\n", static_cast<long long>(elapsedMs()));

    // 打印一行，确保 stdout 缓冲已分配（与"什么都不输出"的进程区分开）。
    std::cout << "probe start\n";

#if PROBE_MODE == 0
    std::cout << "mode triv\n";
#elif PROBE_MODE == 1
    std::cout << fmt::format("mode fmt {}\n", argc);
#elif PROBE_MODE == 2
    spdlog::info("mode spdlog argc={}", argc);
#else
    std::cout << fmt::format("mode crasher {}\n", argc);
    // 与 mc_tests 一样安装崩溃处理器（信号处理器 + 崩溃输出准备），只安装不触发崩溃。
    mc::assert::CrashHandler::install();
#endif

    std::printf("PROBE_TIME work_done %lld\n", static_cast<long long>(elapsedMs()));
    (void)argv;
    return 0;
}
