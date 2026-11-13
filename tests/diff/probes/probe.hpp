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

#include <cmath>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Result.hpp"
#include "common/util/assert/AssertAll.hpp"

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
 *   diff_runner <domain> list
 *       输出该能力可枚举的全部单元（每个方块状态、每个物品……），每行一个 ID。
 *       编排层据此生成输入，从而保证两侧喂的是**同一集合**，而不是各自枚举一次。
 *
 *   diff_runner <domain> run <input.jsonl> <output.jsonl>
 *       逐行读输入（JSON），每行产出一行结果（JSON），顺序与输入一致。
 *
 * 全部能力共用一个可执行文件：每个 probe 只提供自己的 `Probe` 常量，由链接期收集进
 * {@link registeredProbes()}。此前每个能力单独链接一份 mc_common，257 个可执行文件
 * 各自携带 40-90 MB 的调试信息（合计 11 GB 的 .pdb），且改一行公共代码要重链接 257 次。
 *
 * 确定性要求：同一输入必须永远产出同一输出。凡涉及无序容器、随机数、时间、地址，
 * probe 必须在写入前自行收敛（排序、固定种子、去掉地址），否则差分没有意义。
 */

/**
 * @brief 把一个浮点结果编码成两侧都能安全表达的 JSON 值。
 *
 * 有限值原样返回（JSON number），让编排层按容差比较——项目不对齐浮点精度，
 * 末位差异不是缺陷。
 *
 * 非有限值返回字符串："nan" / "inf" / "-inf"。不能直接交给 JSON 数字：
 * nlohmann 会写 `null`，Gson 会写裸 `NaN`/`Infinity`（**不是合法 JSON**，编排层
 * 解析时直接报错）。两侧统一用这三个字符串，NaN 与 NaN 才算相等。
 */
inline nlohmann::json encodeFloat(double value)
{
    if (std::isnan(value)) {
        return "nan";
    }
    if (std::isinf(value)) {
        return value > 0 ? "inf" : "-inf";
    }
    return value;
}

namespace mc::diff {

/**
 * @brief 从解析结果取值（差分探针的输入都是人工给定的合法样本）
 *
 * 解析器统一返回 `Result`；探针遇到失败说明「样本 / 探针」与解析器脱节（API 变更、
 * 样本过期），属于探针自身的缺陷，直接断言暴露，不要静默退化成 nullptr 让差异报告
 * 变成"两块地形的差别"。
 */
template <typename T, typename E>
[[nodiscard]] T expectParsed(const mc::Result<T, E>& result)
{
    MC_ASSERT_RELEASE_MSG(result.success(), "diff probe input failed to parse");
    return result.value();
}

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
 * @brief 把 probe 登记进全局注册表。
 *
 * 各 probe 源文件定义自己的 `Probe` 常量并构造一个静态 registrar；`runner_main.cpp`
 * 在启动时遍历注册表按域名查找能力。这样全部能力共用同一个可执行文件，
 * 新增能力只需在 CMake 清单里加一行，不必改 `runner_main.cpp`。
 *
 * 宏不自己开 namespace：各 probe 源文件在 `namespace mc::diff` 内调用它。
 */
class ProbeRegistrar {
public:
    explicit ProbeRegistrar(const Probe& probe);
};

/** 构造顺序确定的全部已登记 probe（按域名排序）。 */
[[nodiscard]] const std::vector<const Probe*>& registeredProbes();

/** 按域名查找；找不到返回 nullptr。 */
[[nodiscard]] const Probe* findProbe(std::string_view domain);

/**
 * @brief 声明一个能力领域的注册对象。
 *
 * `id` 由调用方显式给出，**不使用 `__LINE__`**：`__LINE__` 作为实参传入内层宏时
 * 会先被展开成行号，两个不同文件里同名的注册对象本身不冲突（都带内链接），但
 * 调试时符号名读出能力名远比读出行号有用。
 *
 * 域名同样只传**符号**（不经 `#` 字符串化）：某些 clang 版本对「实参本身就是
 * 字符串字面量、再经 `#`」的组合会多套一层转义，产出 `"\"aabb_ops\""`，使
 * `diff_runner aabb_ops list` 查表失败。改由 {@link MC_DIFF_DOMAIN} 把符号摇身
 * 变成字符串：`aabb_ops` → `"aabb_ops"`，全程只经一次 `STR` 展开，绕开该问题。
 */
#define MC_DIFF_STR_INNER(name) #name
#define MC_DIFF_STR(name) MC_DIFF_STR_INNER(name)

#define MC_DIFF_DECLARE_PROBE(id, listFunc, runLineFunc, shutdownFunc)                    \
    constexpr Probe diffProbe_##id{MC_DIFF_STR(id), listFunc, runLineFunc, shutdownFunc}; \
    static const ProbeRegistrar diffRegistrar_##id(diffProbe_##id);

/**
 * @brief 定义一个能力领域（无清理回调）。
 *
 * 用法：`MC_DIFF_DEFINE_PROBE(aabb_ops, &writeList, &runLine)`，必须写在
 * `namespace mc::diff` 内、文件作用域（不能放在匿名 namespace 里）。
 * 域名就是 `id` 本身，不需要再写一遍字符串。
 */
#define MC_DIFF_DEFINE_PROBE(id, listFunc, runLineFunc) MC_DIFF_DECLARE_PROBE(id, listFunc, runLineFunc, nullptr)

/**
 * @brief 定义一个能力领域（带退出清理回调）。
 *
 * 适用于持有实体、世界等需要显式释放的探针；`shutdownFunc` 在 `main` 的每条
 * 退出路径上都会被调用一次。
 */
#define MC_DIFF_DEFINE_PROBE_WITH_SHUTDOWN(id, listFunc, runLineFunc, shutdownFunc) \
    MC_DIFF_DECLARE_PROBE(id, listFunc, runLineFunc, shutdownFunc)

} // namespace mc::diff
