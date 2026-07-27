/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/ sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

// 测试用唯一临时目录工具。
//
// 为什么需要它：CTest 并行（-j16）下，每个用例是独立进程，多个进程可能同一秒启动。
// 早期测试夹具用 std::time(nullptr)（秒级精度）+ 进程局部 static atomic 计数器生成"唯一"
// 临时目录，结果同秒启动的不同进程计数器都从 0 开始，目录 token 完全相同，互相覆盖文件，
// 导致 SingleLevelStorageManager / RocksDB 在 Windows 上抢不到文件锁（ERROR_SHARING_VIOLATION），
// 整批用例竞态失败（单独跑全过）。
//
// 解决：token 由四个分量组合，跨进程 / 跨线程 / 同秒同线程都唯一：
//   <steady_clock 纳秒>_<进程ID>_<线程ID>_<进程内自增计数>
// steady_clock 单调且亚纳秒精度；进程 ID 区分 CTest 并行进程（线程 ID 可跨进程复用，不能替代）；
// 线程 ID 兜底同进程多线程；计数器兜底同进程同 tick 同线程的极高频调用。

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <sstream>
#include <string_view>
#include <thread>

// 进程 ID 的获取只依赖一个系统调用，为避免 <windows.h> 的宏污染（near/far/min/max 等）
// 渗漏进所有包含本头的测试 TU（曾导致 GameEventServerTest.cpp 用 near 作变量名编译失败），
// 这里直接前向声明所需函数，不拉入 <windows.h>。POSIX 侧 <unistd.h> 无此问题。
#if defined(_WIN32)
extern "C" __declspec(dllimport) unsigned long __stdcall GetCurrentProcessId();
#else
#include <unistd.h>
#endif

namespace mc::test {

// 跨平台进程 ID。Windows 用 GetCurrentProcessId，POSIX 用 getpid。
// 放在 helper 内 inline，避免为这点测试专用功能新增 src/ 工具头。
inline std::uint64_t processId()
{
#if defined(_WIN32)
    return static_cast<std::uint64_t>(::GetCurrentProcessId());
#else
    return static_cast<std::uint64_t>(::getpid());
#endif
}

// 生成唯一目录 token：<steady_ns>_<pid>_<tid>_<counter>。
inline std::string uniqueTempDirToken()
{
    const auto steadyNs = std::chrono::steady_clock::now().time_since_epoch().count();
    static std::atomic<std::uint64_t> counter{0};
    std::ostringstream tidOss;
    tidOss << std::this_thread::get_id();
    std::ostringstream oss;
    oss << steadyNs << '_' << processId() << '_' << tidOss.str() << '_' << counter.fetch_add(1);
    return oss.str();
}

// 仅计算唯一路径（不创建）。供 SetUp 后才创建子目录、或需自定义创建时机的测试使用。
inline std::filesystem::path uniqueTestDirPath(std::string_view prefix)
{
    return std::filesystem::temp_directory_path() / std::string(prefix) / uniqueTempDirToken();
}

// 生成并创建唯一临时目录：temp / <prefix> / <token>。prefix 用作测试族名便于人工排查。
inline std::filesystem::path makeUniqueTestDir(std::string_view prefix)
{
    const auto dir = uniqueTestDirPath(prefix);
    std::filesystem::create_directories(dir);
    return dir;
}

// 删除临时目录，带重试。Windows 上 RocksDB 等后台线程可能延迟释放文件句柄，单次 remove_all
// 会因 ERROR_SHARING_VIOLATION 失败；重试 10 次、每次间隔 100ms 可基本覆盖句柄释放窗口。
// error_code 版本吞掉失败，重试结束后不抛异常。
inline void removeTestDir(const std::filesystem::path& dir)
{
    for (int i = 0; i < 10; ++i) {
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
        if (!ec) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

} // namespace mc::test
