// 进程启动成本对照实验：分阶段装配 mc_tests 的入口（CI 临时工具，不进产品构建流程）。
//
// 背景见 docs/CI.md「单元测试单进程固定开销」。CI 上每个 ctest 用例都是独立进程，单进程固定
// 开销 8~14 秒且 CPU 时间≈墙钟；ctest 只给每个用例一个总时长，没有拆分。本入口按 PROBE_LEVEL
// 逐级装配 mc_tests 入口的各个组成部分，对照各阶段对总时长的贡献：
//
//   PROBE_LEVEL=0  空 main，仅输出阶段时刻
//   PROBE_LEVEL=1  安装 CrashHandler（与 tests/unit/main.cpp 同序的第一步）
//   PROBE_LEVEL=2  1 + InitGoogleTest + WorldGenRegistryEnvironment 注册（但不运行用例）
//   PROBE_LEVEL=3  1 + InitGoogleTest + WorldGenRegistryEnvironment，并运行全部用例
//   PROBE_LEVEL=4  3 + 注册 TestIsolationListener（tests/unit/main.cpp 的完整路径）
//
// WorldGenRegistryEnvironment / TestIsolationListener 直接复用 tests/unit/test_main_entry.hpp，
// 与真实测试进程是同一份实现。
//
// 输出（固定格式）：PROBE_TIME <phase> <ms>、PROBE_DONE <level> <wall_ms> <cpu_ms>。

#include <chrono>
#include <cstdio>
#include <cstdlib>

#ifdef __linux__
#include <sys/resource.h>
#endif

#include "common/core/Types.hpp"
#include "common/util/assert/CrashHandler.hpp"
#include "test_main_entry.hpp"

namespace {

std::chrono::steady_clock::time_point s_start;
mc::i64 s_level = 0;

mc::i64 elapsedMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - s_start).count();
}

mc::i64 cpuMs()
{
#ifdef __linux__
    struct rusage usage{};
    if (::getrusage(RUSAGE_SELF, &usage) != 0) {
        return -1;
    }
    return static_cast<mc::i64>(usage.ru_utime.tv_sec + usage.ru_stime.tv_sec) * 1000 +
        (usage.ru_utime.tv_usec + usage.ru_stime.tv_usec) / 1000;
#else
    return -1;
#endif
}

void phase(const char* name)
{
    std::printf("PROBE_TIME %s %lld\n", name, static_cast<long long>(elapsedMs()));
    std::fflush(stdout);
}

struct ReportOnExit {
    ~ReportOnExit()
    {
        phase("exit_pre");
        std::printf("PROBE_DONE %lld %lld %lld\n",
            static_cast<long long>(s_level),
            static_cast<long long>(elapsedMs()),
            static_cast<long long>(cpuMs()));
        std::fflush(stdout);
    }
};

ReportOnExit g_report;

} // namespace

int main(int argc, char** argv)
{
    s_start = std::chrono::steady_clock::now();
    const char* levelText = std::getenv("PROBE_LEVEL");
    s_level = levelText != nullptr ? std::strtoll(levelText, nullptr, 10) : 0;
    phase("enter");

    if (s_level >= 1) {
        mc::assert::CrashHandler::install();
        phase("crash_handler");
    }

    if (s_level >= 2) {
        ::testing::InitGoogleTest(&argc, argv);
        phase("init_gtest");
        ::testing::AddGlobalTestEnvironment(new WorldGenRegistryEnvironment());
        phase("add_env");
    }

    if (s_level >= 3) {
        if (s_level >= 4) {
            ::testing::UnitTest::GetInstance()->listeners().Append(new TestIsolationListener());
            phase("add_listener");
        }
        const int result = RUN_ALL_TESTS();
        phase("run_tests");
        (void)result;
    }

    return 0;
}
