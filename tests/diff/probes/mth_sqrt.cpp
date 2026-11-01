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

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 格式化浮点：`%.9g` 并去尾零（尾数与指数分开处理）。
std::string formatNumber(f64 value)
{
    if (std::isnan(value)) {
        return "NaN";
    }
    if (std::isinf(value)) {
        return value > 0 ? "Infinity" : "-Infinity";
    }
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.9g", value);
    std::string text(buffer);

    const std::size_t exponentAt = text.find('e') != std::string::npos ? text.find('e') : text.find('E');
    const std::string mantissa = exponentAt == std::string::npos ? text : text.substr(0, exponentAt);
    const std::string exponent = exponentAt == std::string::npos ? std::string() : text.substr(exponentAt);

    std::string trimmed = mantissa;
    if (trimmed.find('.') != std::string::npos) {
        while (!trimmed.empty() && trimmed.back() == '0') {
            trimmed.pop_back();
        }
        if (!trimmed.empty() && trimmed.back() == '.') {
            trimmed.pop_back();
        }
    }
    return trimmed + exponent;
}

/// 把 `double` 的原始位模式读出为 `long`（对应 `Double.doubleToRawLongBits`）。
i64 doubleToRawLongBits(f64 value)
{
    i64 bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

/// 把 `long` 的原始位模式写回 `double`（对应 `Double.longBitsToDouble`）。
f64 longBitsToDouble(i64 bits)
{
    f64 value = 0.0;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

/// 原版 `Mth.fastInvSqrt`：魔数 + 一次牛顿迭代。
f64 fastInvSqrt(f64 x)
{
    const f64 half = 0.5 * x;
    i64 bits = doubleToRawLongBits(x);
    // 注意：这里是有符号右移 `>>`，与 Java 的 `>>` 一致（非 `>>>`）。
    bits = 6910469410427058090LL - (bits >> 1);
    const f64 approx = longBitsToDouble(bits);
    return approx * (1.5 - half * approx * approx);
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // fast：完全平方数
        "fast|1",
        "fast|4",
        "fast|9",
        "fast|16",
        "fast|100",
        // fast：非平方数
        "fast|2",
        "fast|3",
        "fast|10",
        "fast|0.5",
        "fast|0.25",
        // fast：边界
        "fast|0",
        "fast|-1",
        "fast|-4",
        // fast：极大极小
        "fast|1e100",
        "fast|1e-100",
        // sqrtF：常规
        "sqrtF|0",
        "sqrtF|1",
        "sqrtF|2",
        "sqrtF|4",
        "sqrtF|9",
        "sqrtF|0.25",
        // sqrtF：边界
        "sqrtF|-1",
        "sqrtF|1e-40",
        "sqrtF|3.4028235e38",
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

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() != 2) {
        throw std::runtime_error("mth_sqrt: 单元需要 2 段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "fast") {
        return json{{"unit", unit}, {"value", formatNumber(fastInvSqrt(std::stod(parts[1])))}}.dump();
    }
    if (mode == "sqrtF") {
        // 原版 `Mth.sqrt(float)`：先把输入降为 float，再提升为 double 求根，最后降回 float。
        const f32 input = std::stof(parts[1]);
        const f64 root = std::sqrt(static_cast<f64>(input));
        const f32 result = static_cast<f32>(root);
        return json{{"unit", unit}, {"value", formatNumber(static_cast<f64>(result))}}.dump();
    }

    throw std::runtime_error("mth_sqrt: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE("mth_sqrt", &writeList, &runLine)

} // namespace mc::diff
