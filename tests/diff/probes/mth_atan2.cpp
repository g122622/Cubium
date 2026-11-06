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

#include <cmath>
#include <cstdio>
#include <cstring>
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

/// 把 f64 的原始位模式格式化成 `0x%016llx`。
///
/// 必须比位模式而不是十进制：`Mth.atan2` 与 `std::atan2` 相差 1e-6~1e-8，
/// 用 `%.9g` 会把两者的部分结果打印成同一个字符串，从而漏掉差异。
std::string bitsOf(f64 value)
{
    u64 bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "0x%016llx", static_cast<unsigned long long>(bits));
    return std::string(buffer);
}

/// 单元表。
///
/// 设计要点：
/// - 四个象限都要覆盖（符号组合决定 `negateX` / `negateY` 两个反射分支）；
/// - `y > x` 与 `y <= x` 两侧都要有（决定 `swap` 分支）；
/// - 比值取 0.25 / 0.5 / 2 / 4 等能精确表示的值，让角度好核对；
/// - 轴的四个方向 `(1,0) (0,1) (-1,0) (0,-1)` 走的是特判路径，结果应精确；
/// - 极小、极大值检验 `fastInvSqrt` 在上下溢边界的行为。
constexpr const char* UNITS[] = {
    // 轴方向：结果应精确等于 0 / π/2 / π / -π/2
    "1|0",
    "-1|0",
    "0|1",
    "0|-1",
    // 对角（swap 两侧各取一个）
    "1|1",
    "-1|-1",
    "2|1",
    "1|2",
    "-2|1",
    "1|-2",
    // 各象限
    "1|-1",
    "-1|1",
    "2|-1",
    "-2|-1",
    "-1|-2",
    "-1|2",
    // 小角度与大角度
    "0.1|1",
    "1|0.1",
    "0.001|1",
    "1|0.001",
    // 精确可表示的比值
    "1|4",
    "4|1",
    "3|4",
    "0.75|1",
    // 极小 / 极大
    "1e-8|1",
    "1|1e-8",
    "1e8|1",
    "1|1e8",
    "1e-150|1",
    "1e150|1",
    // 零与 NaN
    "0|0",
    "nan|1",
    "1|nan",
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

/// 解析分量：接受十进制字面量与 `nan` / `inf` / `-inf`。
f64 parseComponent(const std::string& text)
{
    if (text == "nan" || text == "NaN") {
        return std::numeric_limits<f64>::quiet_NaN();
    }
    if (text == "inf" || text == "Infinity") {
        return std::numeric_limits<f64>::infinity();
    }
    if (text == "-inf" || text == "-Infinity") {
        return -std::numeric_limits<f64>::infinity();
    }
    return std::stod(text);
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() != 2) {
        throw std::runtime_error("mth_atan2: unit needs two fields 'y|x': " + unit);
    }

    const f64 y = parseComponent(parts[0]);
    const f64 x = parseComponent(parts[1]);

    // 直接调用生产实现。
    const f64 result = math::atan2(y, x);
    return json{{"unit", unit}, {"bits", bitsOf(result)}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(mth_atan2, &writeList, &runLine)

} // namespace mc::diff
