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
 * @brief 差分能力：`Mth` 的整数位运算工具（纯计算）。
 *
 * `list`：一组「模式|参数」单元
 * `run` ：同上一行 → { "value": 8 }
 *
 * 复刻原版 `Mth`：
 *
 * ```java
 * public static int smallestEncompassingPowerOfTwo(int x) {
 *     int i = x - 1;
 *     i |= i >> 1;  i |= i >> 2;  i |= i >> 4;  i |= i >> 8;  i |= i >> 16;
 *     return i + 1;
 * }
 *
 * public static boolean isPowerOfTwo(int x) {
 *     return x != 0 && (x & (x - 1)) == 0;
 * }
 *
 * public static int ceillog2(int x) {
 *     x = isPowerOfTwo(x) ? x : smallestEncompassingPowerOfTwo(x);
 *     return MULTIPLY_DE_BRUIJN_BIT_POSITION[(int)(x * 125613361L >> 27) & 31];
 * }
 *
 * public static int log2(int x) {
 *     return ceillog2(x) - (isPowerOfTwo(x) ? 0 : 1);
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`smallestEncompassingPowerOfTwo` 用位传播实现，对非正输入反直觉。**
 *    输入 `0` 时 `x - 1 = -1`，`-1` 或任何数都是 `-1`，再 `+1` 得 **0**——返回 0 而
 *    不是 1。负数同理：`-1` 返回 0、`-2` 返回 0、`-5` 返回 **0**。这不是"向上取到 2 的
 *    幂"的错误处理，而是位传播的直接结果，必须原样复刻。
 *
 * 2. **`isPowerOfTwo(0)` 返回 `false`**（因为有 `x != 0` 守卫），而 `-1` 的 `(-1 & -2)`
 *    是 `-2 != 0`，所以也返回 `false`。只有**正的** 2 的幂返回 `true`。
 *
 * 3. **`ceillog2` 依赖 De Bruijn 常数 `125613361L`**，32 项查找表。这个常数的作用是把
 *    「2 的幂」映射到唯一索引，写成别的做法（循环移位）结果相同但性能不同；写成 `log2`
 *    或 `Integer.numberOfTrailingZeros` 会让某些输入差 1。
 *
 * 4. **`log2(x)` 对非 2 的幂是「向下取整」**：`ceillog2` 先向上取到 2 的幂再查表，最后
 *    减 1。例如 `log2(5)` = `ceillog2(5) - 1` = `3 - 1` = **2**（因为 2^2=4 ≤ 5 < 8）。
 *
 * 单元格式：`<模式>|<参数>`，参数为十进制整数。
 *
 * TODO: 未覆盖 `ceillog2` 的负数行为（依赖 `smallestEncompassingPowerOfTwo` 的负输入，
 * 会与 De Bruijn 表的越界访问交互，属未定义区间）；也未覆盖 `smallestSquareSide`（依赖
 * `ceil(Math.sqrt())`，浮点误差另议）。
 */
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 原版 `Mth.MULTIPLY_DE_BRUIJN_BIT_POSITION`。
constexpr i32 DE_BRUIJN_BIT_POSITION[32] = {0,
    1,
    28,
    2,
    29,
    14,
    24,
    3,
    30,
    22,
    20,
    15,
    25,
    17,
    4,
    8,
    31,
    27,
    13,
    23,
    21,
    19,
    16,
    7,
    26,
    12,
    18,
    6,
    11,
    5,
    10,
    9};

/// 原版 `Mth.smallestEncompassingPowerOfTwo`。
i32 smallestEncompassingPowerOfTwo(i32 value)
{
    i32 result = value - 1;
    result |= result >> 1;
    result |= result >> 2;
    result |= result >> 4;
    result |= result >> 8;
    result |= result >> 16;
    return result + 1;
}

/// 原版 `Mth.isPowerOfTwo`。
bool isPowerOfTwo(i32 value)
{
    return value != 0 && (value & (value - 1)) == 0;
}

/// 原版 `Mth.ceillog2`。
i32 ceillog2(i32 value)
{
    const i32 adjusted = isPowerOfTwo(value) ? value : smallestEncompassingPowerOfTwo(value);
    return DE_BRUIJN_BIT_POSITION[static_cast<i32>((static_cast<i64>(adjusted) * 125613361LL) >> 27) & 31];
}

/// 原版 `Mth.log2`。
i32 log2Of(i32 value)
{
    return ceillog2(value) - (isPowerOfTwo(value) ? 0 : 1);
}

/// 单元表。
constexpr const char* UNITS[] = {
    // smallestEncompassingPowerOfTwo：2 的幂原样返回
    "sep2|1",
    "sep2|2",
    "sep2|4",
    "sep2|8",
    "sep2|16",
    // 非 2 的幂向上取
    "sep2|3",
    "sep2|5",
    "sep2|6",
    "sep2|7",
    "sep2|9",
    "sep2|17",
    "sep2|31",
    "sep2|33",
    // 大值
    "sep2|1000",
    "sep2|30000000",
    "sep2|1073741824",
    // 边界：0 与负数返回 0（反直觉）
    "sep2|0",
    "sep2|-1",
    "sep2|-2",
    "sep2|-5",
    // isPowerOfTwo
    "ispot|1",
    "ispot|2",
    "ispot|3",
    "ispot|4",
    "ispot|0",
    "ispot|-1",
    "ispot|-2",
    // log2：2 的幂
    "log2|1",
    "log2|2",
    "log2|4",
    "log2|8",
    "log2|1024",
    // log2：非 2 的幂向下取整
    "log2|3",
    "log2|5",
    "log2|7",
    "log2|9",
    "log2|31",
    "log2|1023",
    "log2|1025",
    // ceillog2：非 2 的幂向上取整
    "ceillog2|1",
    "ceillog2|2",
    "ceillog2|3",
    "ceillog2|5",
    "ceillog2|9",
    "ceillog2|1025",
};

void writeList(FILE* out)
{
    for (const auto* unit : UNITS) {
        std::fprintf(out, "%s\n", unit);
    }
}

std::vector<std::string> splitPipe(const std::string& text)
{
    std::vector<std::string> tokens;
    std::string current;
    for (char ch : text) {
        if (ch == '|') {
            tokens.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    tokens.push_back(current);
    return tokens;
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() != 2) {
        throw std::runtime_error("mth_bit: 单元应为两段 '模式|参数'：" + unit);
    }

    const std::string mode = parts[0];
    const i32 value = std::stoi(parts[1]);

    if (mode == "sep2") {
        return json{{"unit", unit}, {"value", smallestEncompassingPowerOfTwo(value)}}.dump();
    }
    if (mode == "ispot") {
        return json{{"unit", unit}, {"result", isPowerOfTwo(value)}}.dump();
    }
    if (mode == "log2") {
        return json{{"unit", unit}, {"value", log2Of(value)}}.dump();
    }
    if (mode == "ceillog2") {
        return json{{"unit", unit}, {"value", ceillog2(value)}}.dump();
    }

    throw std::runtime_error("mth_bit: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE("mth_bit", &writeList, &runLine)

} // namespace mc::diff
