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
 * @brief 差分能力：`Mth.floor` / `Mth.ceil` 的取整语义（纯计算）。
 *
 * `list`：一组待取整的数值（十进制字符串）
 * `run` ：{ "value": "-0.5" }
 *         → { "floorF": -1, "floorD": -1, "ceilF": 0, "ceilD": 0 }
 *
 * 复刻原版 `Mth`：
 *
 * ```java
 * public static int floor(float f) { int i = (int) f; return f < i ? i - 1 : i; }
 * public static int floor(double d) { int i = (int) d; return d < i ? i - 1 : i; }
 * public static int ceil(float f)  { int i = (int) f; return f > i ? i + 1 : i; }
 * public static int ceil(double d) { int i = (int) d; return d > i ? i + 1 : i; }
 * ```
 *
 * ## 不是 `std::floor`
 *
 * 原版**不用** `Math.floor`，而是「先向零截断成 int，再比较」。两者在绝大多数输入上
 * 结果相同，但在**超出 int 范围**时语义完全不同：
 *
 * - Java 的 `(int)` 转换对超界值会**饱和**到 `Integer.MIN_VALUE` / `MAX_VALUE`
 * - C 的 `static_cast<i32>(double)` 对超界值是**未定义行为**
 *
 * Cubium 的 `MathUtils::floorTo<T>` 用的是 `static_cast<T>(std::floor(x))`，同样在超界
 * 值时是未定义行为。本能力把超界输入也纳入单元表——不是为了要求实现复刻 Java 的饱和
 * 语义（那是 JVM 特性），而是为了**记录这一边界两边都没有明确定义**。
 *
 * ## 负数的取整方向
 *
 * `-0.5` 的两个函数结果不同：`floor` 得 -1、`ceil` 得 0。检查一个常见写法
 * `static_cast<i32>(x)`——它对负数向零截断，等价于 `ceil` 而非 `floor`，把 `floor` 写成
 * 它就是错。
 *
 * TODO: 本能力只输出 `float` / `double` 两个重载的结果。原版还有 `lfloor` /
 * `ceilLong` 两个 long 版本，Cubium 目前没有对应函数，故未纳入。
 */
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/**
 * @brief 原版 `Mth.floor(float)`：先向零截断，再比较。
 */
i32 mthFloorF(f32 value)
{
    const i32 truncated = static_cast<i32>(value);
    return value < static_cast<f32>(truncated) ? truncated - 1 : truncated;
}

/**
 * @brief 原版 `Mth.floor(double)`。
 */
i32 mthFloorD(f64 value)
{
    const i32 truncated = static_cast<i32>(value);
    return value < static_cast<f64>(truncated) ? truncated - 1 : truncated;
}

/**
 * @brief 原版 `Mth.ceil(float)`。
 */
i32 mthCeilF(f32 value)
{
    const i32 truncated = static_cast<i32>(value);
    return value > static_cast<f32>(truncated) ? truncated + 1 : truncated;
}

/**
 * @brief 原版 `Mth.ceil(double)`。
 */
i32 mthCeilD(f64 value)
{
    const i32 truncated = static_cast<i32>(value);
    return value > static_cast<f64>(truncated) ? truncated + 1 : truncated;
}

/**
 * @brief 单元表：待取整的数值。
 *
 * 覆盖：
 * - 正负整数；
 * - 正负小数（含 0.5 这类「恰好半个」的点，检验取整方向）；
 * - 极小正值与极大负值（接近 0 的边界）；
 * - 超出 int 范围的値（记录两边都未定义的情形）；
 * - 能精确表示与不能精确表示的十进制值。
 */
constexpr const char* UNITS[] = {
    "0",
    "1",
    "-1",
    "0.5",
    "-0.5",
    "1.5",
    "-1.5",
    "2.9",
    "-2.9",
    "0.1",
    "-0.1",
    "100.0",
    "-100.0",
    "123456.789",
    "-123456.789",
    // 极小正值 / 极接近 0 的负值
    "0.0001",
    "-0.0001",
    // int 边界附近
    "2147483520",
    "-2147483648",
    // 超出 int 范围（两边都未定义，仅用于记录）
    "3000000000",
    "-3000000000",
};

void writeList(FILE* out)
{
    for (const auto* unit : UNITS) {
        std::fprintf(out, "%s\n", unit);
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    const f32 valueF = static_cast<f32>(std::stod(unit));
    const f64 valueD = std::stod(unit);

    return json{{"unit", unit},
        {"floorF", mthFloorF(valueF)},
        {"floorD", mthFloorD(valueD)},
        {"ceilF", mthCeilF(valueF)},
        {"ceilD", mthCeilD(valueD)}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(mth_rounding, &writeList, &runLine)

} // namespace mc::diff
