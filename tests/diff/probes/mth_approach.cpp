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
 * @brief 差分能力：`Mth.approach` / `approachDegrees` 与 `clamp`（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": "..." }
 *
 * 复刻原版 `Mth`：
 *
 * ```java
 * public static float approach(float value, float limit, float step) {
 *     step = abs(step);
 *     return value < limit ? clamp(value + step, value, limit)
 *                          : clamp(value - step, limit, value);
 * }
 *
 * public static float approachDegrees(float from, float to, float step) {
 *     float f = degreesDifference(from, to);
 *     return approach(from, from + f, step);
 * }
 *
 * public static float degreesDifference(float from, float to) {
 *     return wrapDegrees(to - from);
 * }
 *
 * public static float clamp(float value, float min, float max) {
 *     return value < min ? min : Math.min(value, max);
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`approach` 第一行就把 `step` 取绝对值。** 传负步长不会让逼近反向——它变成正步长照常
 *    逼近。想反向只能交换 `value` 与 `limit`。这处「防御性 abs」静默吞掉了调用方的符号错误。
 *
 * 2. **`clamp` 不校验 `min <= max`。** 它的实现是 `value < min ? min : min(value, max)`：
 *    - 若 `value < min`，直接返回 `min`（此时 `max` 完全不参与）
 *    - 否则返回 `min(value, max)`
 *
 *    所以当 `min > max` 时，结果是「以 min 为主、max 只裁剪上界」，而不是「夹在两者之间」。
 *    `approach` 在 `value < limit` 时调用 `clamp(value + step, value, limit)`，两者顺序正常；
 *    但 `approachDegrees` 传入的 `from + f` 可能使上界小于下界（见第 3 点）。
 *
 * 3. **`approachDegrees` 的关键是「先算差值再逼近」**，而不是「直接朝 `to` 逼近」。它把
 *    `to` 换成 `from + wrapDegrees(to - from)`——一个与 `from` 相差不超过 180 度的目标，
 *    再对这个目标做普通逼近。这样**跨 0/360 度时会走短弧**。
 *
 * 4. **`approach` 的返回值可能超出 `[value, limit]` 之外的中间区间吗？** 不会——`clamp`
 *    保证结果在 `[value, limit]`（或 `[limit, value]`）内。但 `approachDegrees` 的返回值
 *    可以**超出 `[0, 360)`**，因为 `from + f` 与 `from` 都可能在外。
 *
 * 单元格式：`<模式>|<参数…>`
 *
 * TODO: 未覆盖 `Mth.approach(int, int, int)`（整数版不存在，原版只有 float/double）。
 */
#include <cmath>
#include <cstdio>
#include <cstdlib>
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

/// 单元表。
constexpr const char* UNITS[] = {
    // approach：正向逼近
    "approach|0|100|10",
    "approach|95|100|10",
    "approach|100|100|10",
    // approach：反向逼近
    "approach|100|0|10",
    "approach|5|0|10",
    // approach：步长为负（会被取绝对值）
    "approach|0|100|-10",
    "approach|100|0|-10",
    // approach：步长为 0
    "approach|0|100|0",
    "approach|50|100|0",
    // approach：一步到位
    "approach|0|100|1000",
    "approach|0|100|100",
    // approach：value == limit
    "approach|50|50|10",
    // clamp：常规
    "clamp|5|0|10",
    "clamp|-5|0|10",
    "clamp|15|0|10",
    "clamp|0|0|10",
    "clamp|10|0|10",
    // clamp：min > max（反直觉）
    "clamp|5|10|0",
    "clamp|15|10|0",
    "clamp|0|10|0",
    // degreesDifference
    "degdiff|0|90",
    "degdiff|0|180",
    "degdiff|0|270",
    "degdiff|350|10",
    "degdiff|10|350",
    "degdiff|0|540",
    // approachDegrees：常规
    "appdeg|0|90|10",
    "appdeg|85|90|10",
    "appdeg|90|90|10",
    // approachDegrees：跨 0 度（应走短弧 +20）
    "appdeg|350|10|5",
    "appdeg|10|350|5",
    // approachDegrees：一步到位
    "appdeg|0|90|1000",
    // approachDegrees：跨 0 度且步长足够
    "appdeg|350|10|100",
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
        throw std::runtime_error("mth_approach: 单元至少两段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "approach" && parts.size() == 4) {
        const f32 v = math::approach(static_cast<f32>(std::stod(parts[1])),
            static_cast<f32>(std::stod(parts[2])),
            static_cast<f32>(std::stod(parts[3])));
        return json{{"unit", unit}, {"value", formatNumber(static_cast<f64>(v))}}.dump();
    }
    if (mode == "clamp" && parts.size() == 4) {
        const f32 v = math::clamp(static_cast<f32>(std::stod(parts[1])),
            static_cast<f32>(std::stod(parts[2])),
            static_cast<f32>(std::stod(parts[3])));
        return json{{"unit", unit}, {"value", formatNumber(static_cast<f64>(v))}}.dump();
    }
    if (mode == "degdiff" && parts.size() == 3) {
        const f32 v =
            math::degreesDifference(static_cast<f32>(std::stod(parts[1])), static_cast<f32>(std::stod(parts[2])));
        return json{{"unit", unit}, {"value", formatNumber(static_cast<f64>(v))}}.dump();
    }
    if (mode == "appdeg" && parts.size() == 4) {
        const f32 v = math::approachDegrees(static_cast<f32>(std::stod(parts[1])),
            static_cast<f32>(std::stod(parts[2])),
            static_cast<f32>(std::stod(parts[3])));
        return json{{"unit", unit}, {"value", formatNumber(static_cast<f64>(v))}}.dump();
    }

    throw std::runtime_error("mth_approach: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(mth_approach, &writeList, &runLine)

} // namespace mc::diff
