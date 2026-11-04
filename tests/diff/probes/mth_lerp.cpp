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
 * @brief 差分能力：`Mth` 的插值函数族（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": "..." }
 *
 * 复刻原版 `Mth`：
 *
 * ```java
 * public static double lerp(double d, double start, double end) {
 *     return start + d * (end - start);
 * }
 *
 * public static float lerp(float d, float start, float end) {
 *     return start + d * (end - start);
 * }
 *
 * public static double clampedLerp(double d, double start, double end) {
 *     if (d < 0.0) return start;
 *     else return d > 1.0 ? end : lerp(d, start, end);
 * }
 *
 * public static int lerpInt(float d, int start, int end) {
 *     return start + floor(d * (end - start));
 * }
 *
 * public static int lerpDiscrete(float t, int start, int end) {
 *     int i = end - start;
 *     return start + floor(t * (i - 1)) + (t > 0.0F ? 1 : 0);
 * }
 *
 * public static double lerp2(double d0, double d1, double x00, double x10, double x01, double x11) {
 *     return lerp(d1, lerp(d0, x00, x10), lerp(d0, x01, x11));
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`clampedLerp` 是「越界直接返回端点」，不是「先把 d 钳到 [0,1] 再 lerp」**。
 *    在 `d = 0` 与 `d = 1` 处两者一致（`start + 0*(end-start)` 与 `start + 1*(end-start)`
 *    在浮点下**不一定**精确等于 `end`）。越界时原版返回的端点是**逐位精确**的 `start`/`end`，
 *    而先钳制再插值会引入一次乘加。这是两者唯一的差别，但足以让结果次低位不同。
 *
 * 2. **`lerpInt` 用 `floor` 而不是截断**。`d * (end - start)` 为负时，`floor` 与
 *    `static_cast<i32>` 的结果差 1。因为 `lerp` 允许 `d` 越界，负增量是合法输入。
 *
 * 3. **`lerpDiscrete` 的公式与 `lerpInt` 不同**：它用 `(i - 1)` 而不是 `i`，并且额外加
 *    `t > 0 ? 1 : 0`。这样 `t = 0` 时得 `start`、`t = 1` 时得 `end`（因为
 *    `floor(1 * (i-1)) + 1 = i-1+1 = i`），中间是阶梯状而非线性。**`t = 0` 时
 *    `t > 0` 为假，所以不加 1** —— 写成 `t >= 0` 会让 `t = 0` 的结果偏 1。
 *
 * 4. **`float` 与 `double` 两个版本不能互相代用**。`lerp(float)` 在 `float` 精度下计算，
 *    `lerp(double)` 在 `double` 下计算；用 `double` 版算完后转 `float`，结果可能差 1 ulp。
 *
 * 单元格式：`<模式>|<参数…>`，参数以逗号分隔。
 *
 * TODO: 未覆盖 `lerp3`（三维三线性插值，可由 `lerp2` 组合）、`lerp(double, Vec3, Vec3)`
 * （逐分量调用 `lerp`）、`lerpDiscrete` 之外的面板插值封装。
 */
#include <cmath>
#include <cstdio>
#include <cstdlib>
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
 * @brief 原版 `Mth.floor`：向零截断后比较。
 */
i32 mthFloor(f32 value)
{
    const i32 truncated = static_cast<i32>(value);
    return value < static_cast<f32>(truncated) ? truncated - 1 : truncated;
}

/// 原版 `Mth.floor(double)`。
i32 mthFloorD(f64 value)
{
    const i32 truncated = static_cast<i32>(value);
    return value < static_cast<f64>(truncated) ? truncated - 1 : truncated;
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

/// 原版 `Mth.lerp(double, double, double)`。
f64 lerpD(f64 d, f64 start, f64 end)
{
    return start + d * (end - start);
}

/// 原版 `Mth.lerp(float, float, float)`。
f32 lerpF(f32 d, f32 start, f32 end)
{
    return start + d * (end - start);
}

/**
 * @brief 原版 `Mth.clampedLerp(double, double, double)`。
 *
 * 越界时**直接返回端点**，不是钳制后再插值。
 */
f64 clampedLerpD(f64 d, f64 start, f64 end)
{
    if (d < 0.0) {
        return start;
    }
    return d > 1.0 ? end : lerpD(d, start, end);
}

/// 原版 `Mth.clampedLerp(float, float, float)`。
f32 clampedLerpF(f32 d, f32 start, f32 end)
{
    if (d < 0.0f) {
        return start;
    }
    return d > 1.0f ? end : lerpF(d, start, end);
}

/// 原版 `Mth.lerpInt(float, int, int)`。
i32 lerpInt(f32 d, i32 start, i32 end)
{
    return start + mthFloor(d * static_cast<f32>(end - start));
}

/// 原版 `Mth.lerpDiscrete(float, int, int)`。
i32 lerpDiscrete(f32 t, i32 start, i32 end)
{
    const i32 delta = end - start;
    return start + mthFloor(t * static_cast<f32>(delta - 1)) + (t > 0.0f ? 1 : 0);
}

/// 原版 `Mth.lerp2`。
f64 lerp2(f64 d0, f64 d1, f64 x00, f64 x10, f64 x01, f64 x11)
{
    return lerpD(d1, lerpD(d0, x00, x10), lerpD(d0, x01, x11));
}

/// 单元表。
constexpr const char* UNITS[] = {
    // lerp(double)：端点与中点
    "lerpd|0|10|20",
    "lerpd|1|10|20",
    "lerpd|0.5|10|20",
    "lerpd|0.25|100|200",
    // lerp(double)：越界
    "lerpd|-0.5|10|20",
    "lerpd|1.5|10|20",
    // lerp(double)：逆序端点
    "lerpd|0.5|20|10",
    // lerp(float)：同样的用例走单精度
    "lerpf|0|10|20",
    "lerpf|1|10|20",
    "lerpf|0.5|10|20",
    "lerpf|0.25|100|200",
    "lerpf|-0.5|10|20",
    "lerpf|1.5|10|20",
    // clampedLerp(double)：越界应精确返回端点
    "clamppd|-0.5|10|20",
    "clamppd|-1e-09|10|20",
    "clamppd|0|10|20",
    "clamppd|0.5|10|20",
    "clamppd|1|10|20",
    "clamppd|1.000000001|10|20",
    "clamppd|1.5|10|20",
    // clampedLerp：端点值不能精确表示时，越界返回的仍是精确端点
    "clamppd|-0.5|0.1|0.7",
    "clamppd|1.5|0.1|0.7",
    "clamppd|1|0.1|0.7",
    // lerpInt：floor 而非截断
    "lerpint|0.5|0|10",
    "lerpint|0|0|10",
    "lerpint|1|0|10",
    "lerpint|0.5|10|0",
    "lerpint|0.25|0|10",
    // lerpInt：负增量验证 floor
    "lerpint|0.55|10|0",
    "lerpint|-0.5|0|10",
    // lerpDiscrete：t=0 不加 1，t=1 恰好到 end
    "lerpdisc|0|0|10",
    "lerpdisc|1|0|10",
    "lerpdisc|0.5|0|10",
    "lerpdisc|0.0001|0|10",
    "lerpdisc|0.9999|0|10",
    // lerpDiscrete：起点非零
    "lerpdisc|0|100|200",
    "lerpdisc|1|100|200",
    // lerp2
    "lerp2|0|0|0|10|20|30",
    "lerp2|1|0|0|10|20|30",
    "lerp2|0|1|0|10|20|30",
    "lerp2|1|1|0|10|20|30",
    "lerp2|0.5|0.5|0|10|20|30",
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
    if (parts.size() < 2) {
        throw std::runtime_error("mth_lerp: 单元至少两段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "lerpd" && parts.size() == 4) {
        const f64 v = lerpD(std::stod(parts[1]), std::stod(parts[2]), std::stod(parts[3]));
        return json{{"unit", unit}, {"value", formatNumber(v)}}.dump();
    }
    if (mode == "lerpf" && parts.size() == 4) {
        const f32 v = lerpF(static_cast<f32>(std::stod(parts[1])),
            static_cast<f32>(std::stod(parts[2])),
            static_cast<f32>(std::stod(parts[3])));
        return json{{"unit", unit}, {"value", formatNumber(static_cast<f64>(v))}}.dump();
    }
    if (mode == "clamppd" && parts.size() == 4) {
        const f64 v = clampedLerpD(std::stod(parts[1]), std::stod(parts[2]), std::stod(parts[3]));
        return json{{"unit", unit}, {"value", formatNumber(v)}}.dump();
    }
    if (mode == "lerpint" && parts.size() == 4) {
        const i32 v = lerpInt(static_cast<f32>(std::stod(parts[1])), std::stoi(parts[2]), std::stoi(parts[3]));
        return json{{"unit", unit}, {"value", v}}.dump();
    }
    if (mode == "lerpdisc" && parts.size() == 4) {
        const i32 v = lerpDiscrete(static_cast<f32>(std::stod(parts[1])), std::stoi(parts[2]), std::stoi(parts[3]));
        return json{{"unit", unit}, {"value", v}}.dump();
    }
    if (mode == "lerp2" && parts.size() == 7) {
        const f64 v = lerp2(std::stod(parts[1]),
            std::stod(parts[2]),
            std::stod(parts[3]),
            std::stod(parts[4]),
            std::stod(parts[5]),
            std::stod(parts[6]));
        return json{{"unit", unit}, {"value", formatNumber(v)}}.dump();
    }

    throw std::runtime_error("mth_lerp: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(mth_lerp, &writeList, &runLine)

} // namespace mc::diff
