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
 * @brief 差分能力：`LegacyRandomSource` 的确定性随机序列（纯计算）。
 *
 * `list`：一组「<seed>|<op>;<op>;…」单元
 * `run` ：同上一行 → { "result": "..." }
 *
 * 原版 `LegacyRandomSource` 就是 Java 的 48 位 LCG：
 *
 * ```java
 * protected int next(int bits) {
 *     long i = this.seed.get();
 *     long j = i * 25214903917L + 11L & 281474976710655L;
 *     this.seed.set(j);
 *     return (int)(j >> 48 - bits);
 * }
 * ```
 *
 * ## 三个关键点
 *
 * 1. **`nextInt(bound)` 是拒绝采样，不是简单取模**。非 2 的幂走
 *    `do { bits = next(31); val = bits % bound; } while (bits - val + (bound - 1) < 0)`，
 *    判定式依赖 i32 回绕。简单 `nextInt() % bound` 会引入偏置，序列完全不同。
 *
 * 2. **2 的幂走快速路径**：`(bound * next(31)) >> 31`，一次抽取；非 2 的幂可能抽多次。
 *    因此**同一 seed 下，取不同 bound 会推进不同的步数**。
 *
 * 3. **`setSeed` 先与乘数异或再掩码**：`(seed ^ 25214903917L) & 281474976710655L`。
 *    少了这步异或，序列会整体错位。
 *
 * ## 单元格式
 *
 * `<seed>|<op>;<op>;…`，每个 op 是：
 * - `i:<bound>` —— `nextInt(bound)`
 * - `I` —— `nextInt()`（int，全范围）
 * - `l` —— `nextLong()`
 * - `b` —— `nextBoolean()`
 * - `f` —— `nextFloat()`
 * - `d` —— `nextDouble()`
 * - `c:<count>` —— `consumeCount(count)`
 *
 * 输出是各操作结果用 `,` 连接的字符串。
 */

#include "common/core/Types.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "probe.hpp"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>
#include <nlohmann/json.hpp>

namespace mc::diff {

namespace {

using nlohmann::json;

/// 把字符串按分隔符切成若干段。
std::vector<std::string> split(std::string_view text, char separator)
{
    std::vector<std::string> parts;
    std::string current;
    for (const char ch : text) {
        if (ch == separator) {
            parts.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    parts.push_back(current);
    return parts;
}

/// 把 double 用 17 位有效数字输出（随机序列需要完整精度才能看出分叉）。
std::string formatDouble(double value)
{
    char buffer[64];
    // 用 16 位有效数字：17 位的精确展开在 C++ 与 Java 的 %g 实现间会给出不同的末位，
    // 那是格式化分歧而非数值分歧。16 位仍足以看出一位以上的序列分叉。
    std::snprintf(buffer, sizeof(buffer), "%.16g", value);
    return std::string(buffer);
}

/// 把 float 用 9 位有效数字输出（float 的往返位数）。
std::string formatFloat(float value)
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.9g", static_cast<double>(value));
    return std::string(buffer);
}

const char* UNITS[] = {
    // 基础序列：连续 nextInt(bound)
    "0|i:10;i:10;i:10;i:10;i:10",
    "1|i:10;i:10;i:10;i:10;i:10",
    "12345|i:100;i:100;i:100;i:100;i:100",
    // 2 的幂（快速路径）
    "42|i:8;i:8;i:8;i:16;i:32;i:64",
    // 非 2 的幂（拒绝采样）
    "42|i:3;i:5;i:7;i:9;i:11",
    // bound = 1 的退化情形
    "7|i:1;i:1;i:1",
    // 大 bound
    "99|i:1000000;i:2147483647;i:1000000007",
    // 混合序列
    "2024|I;I;l;l;b;b;f;f;d;d",
    // consumeCount
    "555|c:10;i:100",
    "555|c:1;i:100",
    // 负种子
    "-12345|i:100;i:100;i:100",
    // 极大种子
    "9223372036854775807|i:1000;i:1000",
};

void writeList(FILE* out)
{
    for (const char* unit : UNITS) {
        std::fprintf(out, "%s\n", unit);
    }
}

std::string runLine(std::string_view inputLine)
{
    const std::string line = json::parse(inputLine).at("unit").get<std::string>();
    const size_t bar = line.find('|');
    if (bar == std::string::npos) {
        std::fprintf(stderr, "legacy_random: 单元应为 <seed>|<ops>，实际：%s\n", line.c_str());
        std::exit(1);
    }

    const i64 seed = static_cast<i64>(std::strtoll(line.substr(0, bar).c_str(), nullptr, 10));
    ::mc::math::JavaLegacyRandom random;
    random.setSeed(static_cast<u64>(seed));

    std::string output;
    auto append = [&output](const std::string& text) {
        if (!output.empty()) {
            output.push_back(',');
        }
        output += text;
    };

    for (const std::string& op : split(line.substr(bar + 1), ';')) {
        if (op.empty()) {
            continue;
        }
        const char kind = op[0];
        if (kind == 'i') {
            if (op.size() < 3) {
                std::fprintf(stderr, "legacy_random: i 需要 <bound>：%s\n", op.c_str());
                std::exit(1);
            }
            append(std::to_string(random.nextInt(std::atoi(op.substr(2).c_str()))));
        } else if (kind == 'I') {
            append(std::to_string(random.nextInt()));
        } else if (kind == 'l') {
            append(std::to_string(random.nextLong()));
        } else if (kind == 'b') {
            append(random.nextBoolean() ? "1" : "0");
        } else if (kind == 'f') {
            append(formatFloat(random.nextFloat()));
        } else if (kind == 'd') {
            append(formatDouble(random.nextDouble()));
        } else if (kind == 'c') {
            if (op.size() < 3) {
                std::fprintf(stderr, "legacy_random: c 需要 <count>：%s\n", op.c_str());
                std::exit(1);
            }
            random.consumeCount(std::atoi(op.substr(2).c_str()));
        } else {
            std::fprintf(stderr, "legacy_random: 未知操作：%s\n", op.c_str());
            std::exit(1);
        }
    }

    return json{{"unit", line}, {"result", output}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("legacy_random", &writeList, &runLine)

} // namespace mc::diff
