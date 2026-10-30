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

#pragma once

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

/**
 * @file probe.hpp
 * @brief 差分探针（differential probe）的公共接口。
 *
 * 一个 probe 是一段「读输入 → 调用被测能力 → 写输出」的纯函数式小程序，它只链接
 * mc_common，不启动服务端、不连网络、不创建世界。TypeScript 编排层（harness/runner.ts）
 * 负责给它生成输入、运行它，并与参照实现（原版数据或 Java oracle）的输出逐字节比对。
 *
 * 协议（两个子命令）：
 *
 *   diff_<domain> list
 *       输出该能力可枚举的全部单元（每个方块状态、每个物品……），每行一个 ID。
 *       编排层据此生成输入，从而保证两侧喂的是**同一集合**，而不是各自枚举一次。
 *
 *   diff_<domain> run <input.jsonl> <output.jsonl>
 *       逐行读输入（JSON），每行产出一行结果（JSON），顺序与输入一致。
 *
 * 确定性要求：同一输入必须永远产出同一输出。凡涉及无序容器、随机数、时间、地址，
 * probe 必须在写入前自行收敛（排序、固定种子、去掉地址），否则差分没有意义。
 */

namespace mc::diff {

/**
 * @brief 枚举模式：本能力可被枚举的全部单元 ID，按确定顺序写入 out。
 *
 * 顺序由 probe 负责稳定（通常是字典序），因为编排层会把两侧的列表直接比对。
 */
using ListFn = void (*)(FILE* out);

/**
 * @brief 执行模式：处理一行输入，返回一行输出（不含换行）。
 *
 * 输入是调用方给的 JSON 行；输出必须是**一行** JSON（不允许内嵌换行）。
 */
using RunLineFn = std::string (*)(std::string_view inputLine);

/**
 * @brief 进程退出前的清理回调（可选）。
 *
 * 持有全局状态的探针必须在 `main` 返回前显式释放这些状态。交给静态析构会因
 * 「构造/析构顺序」问题在退出阶段访问已销毁的单例（表现为 0xC0000005），
 * 编排层会把成功的运行误判成环境问题。
 */
using ShutdownFn = void (*)();

/**
 * @brief 一个能力领域的探针实现。
 */
struct Probe {
    /** 能力名，用于错误信息与报告，如 "text_encode"。 */
    std::string_view domain;
    /** 枚举全部单元。 */
    ListFn list;
    /** 执行一行输入。 */
    RunLineFn runLine;
    /** 退出前清理；nullptr 表示无需清理。 */
    ShutdownFn shutdown = nullptr;
};

/**
 * @brief 由各 probe 提供的实现入口（每个 probe 源文件各自定义）。
 *
 * 使用宏而非 extern 函数名，是为了让 runner_main.cpp 能按 MC_DIFF_PROBE_DOMAIN
 * 选择能力，新增能力时只改 CMake 的能力清单，不必再改 runner_main.cpp。
 *
 * 宏不自己开 namespace：各 probe 源文件在 `namespace mc::diff` 内调用它。
 */
#define MC_DIFF_DEFINE_PROBE(domainName, listFunc, runLineFunc)                \
    const Probe& currentProbe()                                                \
    {                                                                          \
        static const Probe probe{#domainName, listFunc, runLineFunc, nullptr}; \
        return probe;                                                          \
    }

/**
 * @brief 同上，但带退出清理回调。
 *
 * 适用于持有实体、世界等需要显式释放的探针；`shutdownFunc` 在 `main` 的每条
 * 退出路径上都会被调用一次。
 */
#define MC_DIFF_DEFINE_PROBE_WITH_SHUTDOWN(domainName, listFunc, runLineFunc, shutdownFunc) \
    const Probe& currentProbe()                                                             \
    {                                                                                       \
        static const Probe probe{#domainName, listFunc, runLineFunc, shutdownFunc};         \
        return probe;                                                                       \
    }

} // namespace mc::diff
