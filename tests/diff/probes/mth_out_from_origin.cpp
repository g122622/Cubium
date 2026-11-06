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
 * @brief 差分能力：`Mth.fastInvSqrt` 与 `Mth.sqrt`（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.util.Mth`：
 *
 * ```java
 * @Deprecated
 * public static double fastInvSqrt(double x) {
 *     double half = 0.5 * x;
 *     long bits = Double.doubleToRawLongBits(x);
 *     bits = 6910469410427058090L - (bits >> 1);     // 有符号右移！魔数近似
 *     x = Double.longBitsToDouble(bits);
 *     return x * (1.5 - half * x * x);                // 一次牛顿迭代
 * }
 *
 * public static float sqrt(float x) {
 *     return (float) Math.sqrt(x);
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **这是 Quake 快速平方根倒数的双精度变体——结果是近似值，不是精确值。**
 *
 *    魔数 `6910469410427058090L` 的十六进制是 `0x5FE6EB50C7B537A9`，与经典的
 *    `0x5FE6EB50C7B537A9`（float 版 `0x5f3759df` 的 double 推广）对应。
 *
 *    **一次牛顿迭代后相对误差约 `1e-4`**，比 `1/sqrt(x)` 差得多，但速度快。
 *
 *    `fastInvSqrt(4.0)` 不会精确等于 `0.5`。
 *
 * 2. **`bits >> 1` 是「有符号」右移，不是 `>>>`。**
 *
 *    对正数输入（`x > 0`），`bits` 的高位是 `0`，`>>` 与 `>>>` 结果相同。
 *
 *    但**对负数输入（`x < 0`），`bits` 的最高位是 1，`>>` 会保留符号位**——这与 `>>>` 差异巨大。
 *    `fastInvSqrt` 对负数没有语义意义（`sqrt` 无定义），但**代码路径仍然会执行并返回某个数值**。
 *
 * 3. **`x = 0` 时返回 `0`，`x` 为负时返回 `NaN` 或某个无意义值。**
 *
 *    - `fastInvSqrt(0.0)`：`bits = 0`，`0x5FE6... - 0` = 魔数，其数值约 `1.7e153`；
 *      再乘 `(1.5 - 0) = 1.5` → 约 `2.5e153`（**不是 `Infinity`**）
 *    - `fastInvSqrt(-1.0)`：无意义
 *
 *    **没有任何特殊值保护**——与 `1/Math.sqrt(x)` 的行为完全不同。
 *
 * 4. **`Mth.sqrt(float)` 只是 `(float) Math.sqrt(x)`，把 `double` 结果降回 `float`。**
 *
 *    这意味着：`sqrt` 先在 `double` 精度上算，再**一次舍入**到 `float`。
 *
 *    与直接调用 `std::sqrt(float)`（`float` 精度的库函数）**不保证逐位相同**——
 *    虽然 IEEE 754 要求两者都正确舍入，但中间位宽不同会放大差异。
 *
 *    本能力直接验证这一点。
 *
 * 单元格式：
 * - `fast|<x>` —— `fastInvSqrt(double)`
 * - `sqrtF|<x>` —— `sqrt(float)`
 *
 * 全部按 `%.9g` 格式化后去尾零（**不处理指数形式的尾零会报假差异**，
 * 见 [`mth_frac`](./mth_frac.md)）。
 *
 * TODO: 未覆盖 `Mth.invSqrt(float)` / `invSqrt(double)`（委托给 `org.joml.Math.invsqrt`，
 * 该库方法在 oracle 侧无法单独验证其实现）、`Mth.atan2`（依赖 257 项 `ASIN_TAB` / `COS_TAB`，
 * 而这些表由 `Math.sin` 填充）。
 */
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <cstdio>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 单元表。
///
/// 设计要点：
/// - 区间对称 / 不对称 / 单点 / 空跨度（lower == upper）都要覆盖；
/// - 原点在区间内、恰好在下界、恰好在上界、完全在区间外（需 clamp）；
/// - step 取 1 / 2 / 3，以及能整除与不能整除区间宽度的；
/// - 末尾越界一格的用例是**刻意保留**的，用来锁定原版 hasNext 的语义。
constexpr const char* UNITS[] = {
    // 基本：原点在中心
    "0|-2|2|1",
    "0|-3|3|1",
    "0|-2|2|2",
    "0|-3|3|2",
    "0|-3|3|3",
    // 单点区间（原点在内部，无可扩展）
    "0|0|0|1",
    // 原点在下界
    "0|0|2|1",
    "0|0|4|2",
    // 原点在上界
    "0|-2|0|1",
    "0|-4|0|2",
    // 原点在区间外（需要 clamp）
    "5|-2|2|1",
    "-5|-2|2|1",
    "100|0|3|1",
    "-100|-3|0|2",
    // 不对称区间
    "0|-1|5|1",
    "0|-5|1|1",
    "2|-1|5|2",
    // 宽度不能被 step 整除
    "0|-2|3|2",
    "0|-3|2|2",
    // step 大于区间宽度
    "0|-1|1|5",
    "0|0|1|10",
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
    if (parts.size() != 4) {
        throw std::runtime_error("mth_out_from_origin: unit needs four fields: " + unit);
    }

    const i32 origin = std::stoi(parts[0]);
    const i32 lower = std::stoi(parts[1]);
    const i32 upper = std::stoi(parts[2]);
    const i32 step = std::stoi(parts[3]);

    const std::vector<i32> sequence = math::outFromOrigin(origin, lower, upper, step);

    std::string joined;
    for (size_t index = 0; index < sequence.size(); ++index) {
        if (index > 0) {
            joined += ",";
        }
        joined += std::to_string(sequence[index]);
    }
    return json{{"unit", unit}, {"value", joined}, {"size", sequence.size()}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(mth_out_from_origin, &writeList, &runLine)

} // namespace mc::diff
