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
 * @brief 差分能力：缓动函数 `EasingType` / `Ease`（纯计算）。
 *
 * `list`：一组「<缓动名>|<输入>」单元
 * `run` ：同上一行 → { "result": "..." }
 *
 * 原版 32 个具名缓动全部委托给 `net.minecraft.util.Ease` 的静态方法：
 *
 * ```java
 * EasingType IN_BACK     = registerSimple("in_back",      Ease::inBack);
 * EasingType OUT_ELASTIC = registerSimple("out_elastic",  Ease::outElastic);
 * ...
 * EasingType CONSTANT    = registerSimple("constant", t -> 0.0F);
 * EasingType LINEAR      = registerSimple("linear",   t -> t);
 * ```
 *
 * ## 三个关键点
 *
 * 1. **全程 float 运算，但 `Math.pow` / `Math.sqrt` / `Math.sin` 是 double**。原版写法是
 *    `(float) Math.pow(2.0, 10.0 * t - 10.0)`——参数先被提升成 double，算完再截回 float。
 *    若在 C++ 里用 `std::pow(float, float)`，会走 float 重载，末位可能不同。
 *
 * 2. **`inElastic` 的两个端点有硬编码短路**：`t == 0` 返回 0、`t == 1` 返回 1。
 *    公式本身在 `t = 1` 时算出来是 `-1`（负值），必须靠短路纠正。少了短路的实现会在
 *    端点给出完全错误的结果。
 *
 * 3. **`outBounce` 是分段函数**，阈值 `1/2.75`、`2/2.75`、`2.5/2.75` 都是 float 常量。
 *    `inBounce` / `inOutBounce` 都是对它的组合调用，误差会逐层放大。
 *
 * ## 已知差异：`-ffast-math` 导致的 FMA 收缩
 *
 * 有 13 个单元存在 1 个 ulp
 * 级别的差异（`out_quad|0.1`、`out_cubic|0.1`、
 * `in_back|0.6667`
 * 等）。经实测确认，**不是实现错误**，而是编译选项的浮点语义差异：
 *
 * 项目在 RelWithDebInfo 下启用 `-march=native
 * -mavx2 -ffast-math`，`-ffast-math` 允许
 * 编译器把 `1.0f - x * x` 收缩成单条 FMA 指令，舍入点因此改变：
 *
 * ```
 *
 * clang++ -O2 -march=native -mavx2               -> 0.190000057   （原版值）
 * clang++ -O2 -march=native -mavx2
 * -ffast-math   -> 0.190000042   （Cubium 值）
 * ```
 *
 * 判定依据：在探针里对同一输入手算 `1.0f - inner * inner`
 * 得到原版值，而
 * `EasingType::OUT_QUAD.apply()` 得到另一个值——同一份代码、同一个函数，说明差异来自
 *
 * 编译期变换而非源码。
 *
 * `TODO: 若要彻底消除这类差异，需要为浮点敏感的源文件单独关闭 -ffast-math 与 FMA 收缩
 *
 * （-ffp-contract=off）。这是全局编译选项变更，影响面需单独评估。`
 *
 * ## 采样点
 *
 * 输入取 `0`、`1`、`0.5` 三个端点与中点，再加上 `0.25` / `0.75` / `0.1` / `0.9` /
 * `0.3333` / `0.6667`，覆盖分段函数的各个分支与端点短路。
 */

#include "common/core/Types.hpp"
#include "common/world/timeline/EasingType.hpp"
#include "probe.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>
#include <nlohmann/json.hpp>

namespace mc::diff {

namespace {

using nlohmann::json;

/// 32 个具名缓动：名字与探针/原版一致。
struct EasingEntry {
    const char* name;
    const ::mc::world::timeline::EasingType* easing;
};

const EasingEntry EASINGS[] = {
    {"constant", &::mc::world::timeline::EasingType::CONSTANT},
    {"linear", &::mc::world::timeline::EasingType::LINEAR},
    {"in_back", &::mc::world::timeline::EasingType::IN_BACK},
    {"in_bounce", &::mc::world::timeline::EasingType::IN_BOUNCE},
    {"in_circ", &::mc::world::timeline::EasingType::IN_CIRC},
    {"in_cubic", &::mc::world::timeline::EasingType::IN_CUBIC},
    {"in_elastic", &::mc::world::timeline::EasingType::IN_ELASTIC},
    {"in_expo", &::mc::world::timeline::EasingType::IN_EXPO},
    {"in_quad", &::mc::world::timeline::EasingType::IN_QUAD},
    {"in_quart", &::mc::world::timeline::EasingType::IN_QUART},
    {"in_quint", &::mc::world::timeline::EasingType::IN_QUINT},
    {"in_sine", &::mc::world::timeline::EasingType::IN_SINE},
    {"in_out_back", &::mc::world::timeline::EasingType::IN_OUT_BACK},
    {"in_out_bounce", &::mc::world::timeline::EasingType::IN_OUT_BOUNCE},
    {"in_out_circ", &::mc::world::timeline::EasingType::IN_OUT_CIRC},
    {"in_out_cubic", &::mc::world::timeline::EasingType::IN_OUT_CUBIC},
    {"in_out_elastic", &::mc::world::timeline::EasingType::IN_OUT_ELASTIC},
    {"in_out_expo", &::mc::world::timeline::EasingType::IN_OUT_EXPO},
    {"in_out_quad", &::mc::world::timeline::EasingType::IN_OUT_QUAD},
    {"in_out_quart", &::mc::world::timeline::EasingType::IN_OUT_QUART},
    {"in_out_quint", &::mc::world::timeline::EasingType::IN_OUT_QUINT},
    {"in_out_sine", &::mc::world::timeline::EasingType::IN_OUT_SINE},
    {"out_back", &::mc::world::timeline::EasingType::OUT_BACK},
    {"out_bounce", &::mc::world::timeline::EasingType::OUT_BOUNCE},
    {"out_circ", &::mc::world::timeline::EasingType::OUT_CIRC},
    {"out_cubic", &::mc::world::timeline::EasingType::OUT_CUBIC},
    {"out_elastic", &::mc::world::timeline::EasingType::OUT_ELASTIC},
    {"out_expo", &::mc::world::timeline::EasingType::OUT_EXPO},
    {"out_quad", &::mc::world::timeline::EasingType::OUT_QUAD},
    {"out_quart", &::mc::world::timeline::EasingType::OUT_QUART},
    {"out_quint", &::mc::world::timeline::EasingType::OUT_QUINT},
    {"out_sine", &::mc::world::timeline::EasingType::OUT_SINE},
};

/// 采样点：端点、中点、分段边界附近。
const char* SAMPLES[] = {"0", "0.1", "0.25", "0.3333", "0.5", "0.6667", "0.75", "0.9", "1"};

/**
 * @brief 把 f32 输出为其 32 位二进制模式的十六进制。
 *
 * 缓动函数全程 float 运算，两侧必须按 float 精度逐位比较；十进制格式化会把
 * 最后一位的差异掩盖或放大成格式化器噪声。
 */
std::string formatFloat(float value)
{
    // 与 oracle 一致：输出 float 的 32 位二进制模式（8 位十六进制）。十进制格式化
    // （C 的 %.9g 与 Java 的 %.9g）在「恰好卡在第 9 位中点」的值上舍入方向不同，
    // 那属于格式化器差异；按位比较才能把被测算术的差异原样暴露出来。
    u32 bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%08x", bits);
    return std::string(buffer);
}

void writeList(FILE* out)
{
    for (const auto& entry : EASINGS) {
        for (const char* sample : SAMPLES) {
            std::fprintf(out, "%s|%s\n", entry.name, sample);
        }
    }
}

std::string runLine(std::string_view inputLine)
{
    // 入参是整行 JSON：{"unit":"<缓动名>|<输入>"}；先取出 unit 再按 | 切分。
    const std::string line = json::parse(inputLine).at("unit").get<std::string>();
    const size_t bar = line.find('|');
    if (bar == std::string::npos) {
        std::fprintf(stderr, "easing_type: 单元应为 <name>|<input>，实际：%s\n", line.c_str());
        std::exit(1);
    }
    const std::string name = line.substr(0, bar);
    const float input = std::strtof(line.substr(bar + 1).c_str(), nullptr);

    const ::mc::world::timeline::EasingType* easing = nullptr;
    for (const auto& entry : EASINGS) {
        if (name == entry.name) {
            easing = entry.easing;
            break;
        }
    }
    if (easing == nullptr) {
        std::fprintf(stderr, "easing_type: 未知缓动名：%s\n", name.c_str());
        std::exit(1);
    }

    return json{{"unit", line}, {"result", formatFloat(easing->apply(input))}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("easing_type", &writeList, &runLine)

} // namespace mc::diff
