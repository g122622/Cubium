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
 * @brief 差分能力：`Mth` 的整数除法与 2 的幂工具（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.util.Mth`：
 *
 * ```java
 * private static final int[] MULTIPLY_DE_BRUIJN_BIT_POSITION = new int[]{
 *     0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
 *     31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
 * };
 *
 * public static int floorDiv(int x, int y) { return Math.floorDiv(x, y); }
 *
 * public static boolean isPowerOfTwo(int value) {
 *     return value != 0 && (value & value - 1) == 0;
 * }
 *
 * public static int smallestEncompassingPowerOfTwo(int value) {
 *     int i = value - 1;
 *     i |= i >> 1;
 *     i |= i >> 2;
 *     i |= i >> 4;
 *     i |= i >> 8;
 *     i |= i >> 16;
 *     return i + 1;
 * }
 *
 * public static int ceillog2(int value) {
 *     value = isPowerOfTwo(value) ? value : smallestEncompassingPowerOfTwo(value);
 *     return MULTIPLY_DE_BRUIJN_BIT_POSITION[(int)(value * 125613361L >> 27) & 31];
 * }
 *
 * public static int log2(int value) {
 *     return ceillog2(value) - (isPowerOfTwo(value) ? 0 : 1);
 * }
 *
 * public static int positiveCeilDiv(int x, int y) {
 *     return -Math.floorDiv(-x, y);
 * }
 *
 * public static int roundToward(int value, int divisor) {
 *     return positiveCeilDiv(value, divisor) * divisor;
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`floorDiv` 是向下取整，不是向零截断。**
 *
 *    `Math.floorDiv(-7, 2) == -4`，而 `-7 / 2`（Java 整除）是 `-3`。负被除数时两者差 1。
 *
 *    除数也是负数时不报错（除非 `y == 0`）：`floorDiv(7, -2) == -4`。
 *
 * 2. **`isPowerOfTwo(0)` 是 `false`，`isPowerOfTwo(负数)` 也是 `false`。**
 *
 *    ```java
 *    return value != 0 && (value & value - 1) == 0;
 *    ```
 *
 *    `0` 被第一个条件挡住；负数因为符号位而为非零，`value & (value - 1)` 也不为 0。所以
 *    **只有正的真 2 的幂返回 `true`**。
 *
 * 3. **`smallestEncompassingPowerOfTwo` 对 `value <= 0` 返回 0 或负数，不加保护。**
 *
 *    ```java
 *    int i = value - 1;      // value = 0 时 i = -1（全 1）
 *    i |= i >> 1; ...        // 全 1 扩散后仍是 -1
 *    return i + 1;           // -1 + 1 = 0
 *    ```
 *
 *    - `value = 0` → 返回 **0**
 *    - `value = 1` → `i = 0`，扩散后 0，返回 **1**
 *    - `value = 2` → `i = 1`，扩散后 1，返回 **2**
 *    - `value = 3` → `i = 2`，扩散后 3，返回 **4**
 *
 *    **`0` 返回 `0` 而不是 `1`**——这是很多人会预期错的地方。负数输入返回 `0` 或负数，
 *    函数不做校验。
 *
 * 4. **`ceillog2` 用 De Bruijn 乘法查表，`log2 = ceillog2 - (是否 2 的幂 ? 0 : 1)`。**
 *
 *    ```java
 *    MULTIPLY_DE_BRUIJN_BIT_POSITION[(int)(value * 125613361L >> 27) & 31]
 *    ```
 *
 *    `value * 125613361` 用 `long` 计算（避免 32 位溢出），右移 27 位取高 5 位作为索引。
 *    这是经典的「乘常数 + 移位」求 2 的幂的指数技巧，**比循环移位快**。
 *
 *    `log2(x)` 对非 2 的幂会**减 1**，即向下取整。`log2(3) = ceillog2(3) - 1 = 2 - 1 = 1`。
 *
 * 单元格式：
 * - `floorDiv|<x>|<y>` —— `Mth.floorDiv`
 * - `isPow2|<value>` —— `Mth.isPowerOfTwo`
 * - `smallest|<value>` —— `Mth.smallestEncompassingPowerOfTwo`
 * - `ceillog2|<value>` / `log2|<value>`
 * - `posCeilDiv|<x>|<y>` / `roundToward|<value>|<divisor>`
 *
 * `MIN` / `MAX` 表示 `Integer.MIN_VALUE` / `MAX_VALUE`。
 *
 * TODO: 未覆盖 `Mth.nextInt(RandomSource, int, int)`（需要随机源）、`Mth.frac`（浮点版本，
 * 与 `mth_rounding` 部分重叠）。
 */
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 原版 `MULTIPLY_DE_BRUIJN_BIT_POSITION`。
constexpr i32 DE_BRUIJN[] = {0,
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

/// 原版 `Mth.floorDiv`：向下取整。
i32 floorDiv(i32 x, i32 y)
{
    if (y == 0) {
        throw std::runtime_error("mth_int_div: 除数为 0");
    }
    i32 q = x / y;
    if ((x % y != 0) && ((x ^ y) < 0)) {
        --q;
    }
    return q;
}

/// 原版 `Mth.isPowerOfTwo`。
bool isPowerOfTwo(i32 value)
{
    return value != 0 && (value & (value - 1)) == 0;
}

/// 原版 `Mth.smallestEncompassingPowerOfTwo`：位扩散。
i32 smallestEncompassingPowerOfTwo(i32 value)
{
    i32 i = static_cast<i32>(static_cast<u32>(value) - 1u);
    i |= i >> 1;
    i |= i >> 2;
    i |= i >> 4;
    i |= i >> 8;
    i |= i >> 16;
    return static_cast<i32>(static_cast<u32>(i) + 1u);
}

/// 原版 `Mth.ceillog2`：De Bruijn 乘法查表。
i32 ceillog2(i32 value)
{
    const i32 adjusted = isPowerOfTwo(value) ? value : smallestEncompassingPowerOfTwo(value);
    const i64 product = static_cast<i64>(adjusted) * 125613361LL;
    return DE_BRUIJN[static_cast<i32>(product >> 27) & 31];
}

/// 原版 `Mth.log2`。
i32 log2Of(i32 value)
{
    return ceillog2(value) - (isPowerOfTwo(value) ? 0 : 1);
}

/// 原版 `Mth.positiveCeilDiv`。
i32 positiveCeilDiv(i32 x, i32 y)
{
    return -floorDiv(-x, y);
}

/// 原版 `Mth.roundToward`。
i32 roundToward(i32 value, i32 divisor)
{
    return static_cast<i32>(static_cast<u32>(positiveCeilDiv(value, divisor)) * static_cast<u32>(divisor));
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // floorDiv：正负组合
        "floorDiv|7|2",
        "floorDiv|-7|2",
        "floorDiv|7|-2",
        "floorDiv|-7|-2",
        "floorDiv|6|2",
        "floorDiv|-6|2",
        "floorDiv|0|5",
        "floorDiv|-1|5",
        // isPow2
        "isPow2|0",
        "isPow2|1",
        "isPow2|2",
        "isPow2|3",
        "isPow2|4",
        "isPow2|-4",
        "isPow2|16384",
        "isPow2|16383",
        // smallest
        "smallest|0",
        "smallest|1",
        "smallest|2",
        "smallest|3",
        "smallest|4",
        "smallest|5",
        "smallest|17",
        "smallest|-1",
        // ceillog2
        "ceillog2|1",
        "ceillog2|2",
        "ceillog2|3",
        "ceillog2|4",
        "ceillog2|5",
        "ceillog2|16",
        "ceillog2|17",
        "ceillog2|1024",
        // log2
        "log2|1",
        "log2|2",
        "log2|3",
        "log2|4",
        "log2|5",
        "log2|16",
        "log2|17",
        // posCeilDiv
        "posCeilDiv|7|2",
        "posCeilDiv|6|2",
        "posCeilDiv|1|5",
        "posCeilDiv|0|5",
        "posCeilDiv|-1|5",
        // roundToward
        "roundToward|7|2",
        "roundToward|6|2",
        "roundToward|1|5",
        "roundToward|0|5",
        "roundToward|-1|5",
        "roundToward|15|16",
        "roundToward|16|16",
    };
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

i32 parseInt(const std::string& text)
{
    if (text == "MIN") {
        return std::numeric_limits<i32>::min();
    }
    if (text == "MAX") {
        return std::numeric_limits<i32>::max();
    }
    return static_cast<i32>(std::stoll(text));
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("mth_int_div: 单元至少两段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "floorDiv" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", floorDiv(parseInt(parts[1]), parseInt(parts[2]))}}.dump();
    }
    if (mode == "isPow2" && parts.size() == 2) {
        const bool result = isPowerOfTwo(parseInt(parts[1]));
        return json{{"unit", unit}, {"value", result ? "true" : "false"}}.dump();
    }
    if (mode == "smallest" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", smallestEncompassingPowerOfTwo(parseInt(parts[1]))}}.dump();
    }
    if (mode == "ceillog2" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", ceillog2(parseInt(parts[1]))}}.dump();
    }
    if (mode == "log2" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", log2Of(parseInt(parts[1]))}}.dump();
    }
    if (mode == "posCeilDiv" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", positiveCeilDiv(parseInt(parts[1]), parseInt(parts[2]))}}.dump();
    }
    if (mode == "roundToward" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", roundToward(parseInt(parts[1]), parseInt(parts[2]))}}.dump();
    }

    throw std::runtime_error("mth_int_div: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE("mth_int_div", &writeList, &runLine)

} // namespace mc::diff
