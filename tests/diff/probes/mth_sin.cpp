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
 * @brief 差分能力：`Mth.sin` / `Mth.cos` 的查表实现（纯计算）。
 *
 * `list`：一组待求值角度（弧度字符串），以及若干「表项」单元
 * `run` ：{ "unit": "3.14159" } → { "sin": "1.22465e-16", "cos": "-1" }
 *
 * 复刻原版 `Mth`：
 *
 * ```java
 * private static final float[] SIN = Util.make(new float[65536], a -> {
 *     for (int i = 0; i < a.length; i++) {
 *         a[i] = (float) Math.sin(i / 10430.378350470453);
 *     }
 * });
 *
 * public static float sin(double v) {
 *     return SIN[(int)((long)(v * 10430.378350470453) & 65535L)];
 * }
 *
 * public static float cos(double v) {
 *     return SIN[(int)((long)(v * 10430.378350470453 + 16384.0) & 65535L)];
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **这是查表，不是 `Math.sin`。** 表的量化精度是 `2π / 65536 ≈ 9.6e-5` 弧度，所以结果
 *    只能精确到该量级。直接调 `std::sin` 会在大部分输入上与表项差 1 ulp 以上。
 *
 * 2. **索引计算用 `& 65535L`，等价于对 65536 取模**，因此**负角度也能正确处理**（`long`
 *    的按位与非负），不需要额外的符号处理。但注意是 `(long)` 强转**之后**再掩码——
 *    先掩码再转会是另一种行为。
 *
 * 3. **`cos` 复用同一张表**，索引加 `16384.0`（四分之一周期 = 65536/4）。这样 `cos` 与
 *    `sin` 的量化误差分布完全一致，不会出现「sin 对上了但 cos 没对上」的独立误差源。
 *
 * 4. **表项本身由 `Math.sin` 计算后转 `float`。** Java 的 `Math.sin` 与 C 的 `std::sin`
 *    在**末位可能不同**（JDK 用 fdlibm，glibc/MSVC 用各自实现）。这是本能力真正要验证的
 *    地方：如果某个表项差 1 ulp，`sin(v)` 在该索引上的结果就会分叉。
 *
 * 单元格式：
 * - 直接给弧度值（十进制字符串），如 `3.14159`
 * - `tab|<i>` —— 直接取第 `i` 个表项，用于定位表构造本身的差异
 *
 * TODO: 未覆盖 `Mth.COS_TAB`（`asin` 用的 257 项表）与 `Mth.sqrt`（直接转 `std::sqrt`）。
 */
#include <array>
#include <cmath>
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

/// 原版 `Mth.SIN_SCALE`。
constexpr f64 SIN_SCALE = 10430.378350470453;

/// 原版 `Mth.SIN` 的长度（即 `SIN_MASK + 1`）。
constexpr i32 SIN_QUANTIZATION = 65536;

/// 原版 `Mth.SIN_MASK`。
constexpr i64 SIN_MASK = 65535LL;

/// `cos` 相对 `sin` 的四分之一周期偏移。
constexpr f64 COS_QUARTER_OFFSET = 16384.0;

/**
 * @brief 重建原版 `Mth.SIN` 表。
 *
 * 表项 = `(float)Math.sin(i / SIN_SCALE)`。这里用 `std::sin` —— 若两者在某个下标上末位
 * 不同，对应索引的 `sin`/`cos` 就会分叉，这正是本能力要检测的。
 */
const std::array<f32, SIN_QUANTIZATION>& sinTable()
{
    static const std::array<f32, SIN_QUANTIZATION> table = [] {
        std::array<f32, SIN_QUANTIZATION> built{};
        for (i32 i = 0; i < SIN_QUANTIZATION; ++i) {
            built[static_cast<size_t>(i)] = static_cast<f32>(std::sin(static_cast<f64>(i) / SIN_SCALE));
        }
        return built;
    }();
    return table;
}

/// 原版 `Mth.sin`。
f32 mthSin(f64 value)
{
    const i64 scaled = static_cast<i64>(value * SIN_SCALE);
    return sinTable()[static_cast<size_t>(scaled & SIN_MASK)];
}

/// 原版 `Mth.cos`。
f32 mthCos(f64 value)
{
    const i64 scaled = static_cast<i64>(value * SIN_SCALE + COS_QUARTER_OFFSET);
    return sinTable()[static_cast<size_t>(scaled & SIN_MASK)];
}

std::string formatNumber(f64 value)
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.9g", value);
    std::string text(buffer);
    if (text.find('.') != std::string::npos && text.find('e') == std::string::npos &&
        text.find('E') == std::string::npos) {
        while (!text.empty() && text.back() == '0') {
            text.pop_back();
        }
        if (!text.empty() && text.back() == '.') {
            text.pop_back();
        }
    }
    return text;
}

/**
 * @brief 单元表。
 *
 * - 表项单元：抽样覆盖整张表的关键位置（0、四分之一、半、四分之三、整周期，以及表尾）
 * - 角度单元：常规值、负值、多圈、量化边界
 */
constexpr const char* UNITS[] = {
    // 表项抽样：正弦表的关键相位
    "tab|0",
    "tab|1",
    "tab|16384",
    "tab|32768",
    "tab|49152",
    "tab|65535",
    // 表项抽样：均匀取若干位置，覆盖不同象限的曲率
    "tab|100",
    "tab|1000",
    "tab|10000",
    "tab|20000",
    "tab|30000",
    "tab|40000",
    "tab|50000",
    "tab|60000",
    // 角度：关键弧度值
    "0",
    "1.5707963267948966",
    "3.141592653589793",
    "4.71238898038469",
    "6.283185307179586",
    // 角度：小值与负值
    "0.1",
    "-0.1",
    "-1.5707963267948966",
    "-3.141592653589793",
    // 角度：多圈
    "12.566370614359172",
    "-12.566370614359172",
    "100",
    "-100",
    // 角度：量化边界附近（步长约 9.587e-5）
    "0.00001",
    "0.0001",
    "0.0002",
    // 角度：常见游戏内角度
    "0.7853981633974483",
    "0.5235987755982988",
    "1.0471975511965976",
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

    // 表项单元：`tab|<i>`
    if (unit.size() > 4 && unit.compare(0, 4, "tab|") == 0) {
        const i32 index = std::stoi(unit.substr(4));
        if (index < 0 || index >= SIN_QUANTIZATION) {
            throw std::runtime_error("mth_sin: 表下标越界：" + unit);
        }
        const f32 entry = sinTable()[static_cast<size_t>(index)];
        return json{{"unit", unit}, {"kind", "tab"}, {"value", formatNumber(static_cast<f64>(entry))}}.dump();
    }

    const f64 angle = std::stod(unit);
    return json{{"unit", unit},
        {"kind", "angle"},
        {"sin", formatNumber(static_cast<f64>(mthSin(angle)))},
        {"cos", formatNumber(static_cast<f64>(mthCos(angle)))}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("mth_sin", &writeList, &runLine)

} // namespace mc::diff
