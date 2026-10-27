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

/**
 * @file runner_main.cpp
 * @brief 全部差分探针共享的进程入口。
 *
 * 具体能力由编译期宏 MC_DIFF_PROBE_DOMAIN 选择：各 probe 源文件用
 * MC_DIFF_DEFINE_PROBE 定义 mc::diff::currentProbe()，本文件只负责命令行、
 * 文件读写与错误处理。
 *
 * 用法：
 *   diff_<domain> list
 *   diff_<domain> run <input.jsonl> <output.jsonl>
 */

// 探针是内部工具，文件路径全部来自编排层，不存在不受信输入。MSVC 的 CRT 把 fopen
// 标为 deprecated（建议 fopen_s），用 fopen_s 又会让非 Windows 平台多一层分支，
// 因此这里保留 fopen 并关闭该组弃用告警。该宏必须在使用 CRT 头文件之前定义。
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _CRT_SECURE_NO_WARNINGS

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "probe.hpp"

namespace mc::diff {
const Probe& currentProbe();
}

namespace {

/**
 * @brief 打开文件；失败时把 errno 一并报出。
 *
 * 文件名也要进错误信息——编排层同时跑多个 probe，只报 "open failed" 无法定位。
 * 不用 std::strerror：它返回指向共享缓冲区的指针，被 clang-tidy 判为并发不安全；
 * 探针是单线程，但没必要为此留一个可移植性隐患，直接用 errno 数值。
 */
std::FILE* openFile(const char* path, const char* mode)
{
    std::FILE* file = std::fopen(path, mode);
    if (file == nullptr) {
        std::fprintf(stderr, "diff probe: cannot open '%s' (errno %d)\n", path, errno);
    }
    return file;
}

int runList(const mc::diff::Probe& probe)
{
    // 只往 stdout 写，由编排层重定向落盘。
    probe.list(stdout);
    return std::fflush(stdout) == 0 ? 0 : 1;
}

int runInput(const mc::diff::Probe& probe, const char* inputPath, const char* outputPath)
{
    std::FILE* in = openFile(inputPath, "rb");
    if (in == nullptr) {
        return 1;
    }
    std::FILE* out = openFile(outputPath, "wb");
    if (out == nullptr) {
        std::fclose(in);
        return 1;
    }

    std::string line;
    line.reserve(512);
    char buffer[4096];
    unsigned long long lineNumber = 0;
    while (std::fgets(buffer, sizeof(buffer), in) != nullptr) {
        line.assign(buffer);
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }
        ++lineNumber;

        std::string result;
        try {
            result = probe.runLine(line);
        }
        catch (const std::exception& error) {
            // 异常必须变成**这一行**的错误输出而不是中断整个运行：中断会让两侧的行号
            // 错位，报出来的差异就无从定位了。
            result = nlohmann::json{{"error", error.what()}, {"line", lineNumber}}.dump();
        }
        std::fprintf(out, "%s\n", result.c_str());
    }

    std::fclose(in);
    const bool ok = std::fclose(out) == 0;
    return ok ? 0 : 1;
}

} // namespace

int main(int argc, char** argv)
{
    // stdout 是**协议通道**（list 每行一个单元），任何日志写进去都会被当成数据。
    // 方块注册表初始化会打 info 日志，这里在初始化前就把级别压到 error 及以上。
    spdlog::set_level(spdlog::level::err);

    const mc::diff::Probe& probe = mc::diff::currentProbe();

    // 探针内部可能抛异常（JSON 解析、方块查找）。异常逃出 main 会触发 std::terminate，
    // 编排层只能看到进程被杀、拿不到原因；这里统一转成一个可读的错误与退出码 1。
    //
    // 退出码统一在 `status` 里，最后**先**跑探针的清理回调再返回：持有实体的探针
    // 如果等到静态析构才释放，实体会在注册表单例被销毁之后才析构，退出阶段崩溃。
    int status = 2;
    try {
        if (argc >= 2 && std::string_view(argv[1]) == "list") {
            status = runList(probe);
        } else if (argc == 4 && std::string_view(argv[1]) == "run") {
            status = runInput(probe, argv[2], argv[3]);
        } else {
            std::fprintf(stderr,
                "usage: diff_%.*s list\n"
                "       diff_%.*s run <input.jsonl> <output.jsonl>\n",
                static_cast<int>(probe.domain.size()),
                probe.domain.data(),
                static_cast<int>(probe.domain.size()),
                probe.domain.data());
        }
    }
    catch (const std::exception& error) {
        std::fprintf(stderr, "diff probe: %s\n", error.what());
        status = 1;
    }

    if (probe.shutdown != nullptr) {
        probe.shutdown();
    }
    return status;
}
