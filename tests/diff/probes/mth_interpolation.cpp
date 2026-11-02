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
 * @brief 差分能力：`Mth` 的插值与平滑函数（纯计算）。
 *
 * `list`：一组「<模式>|<参数…>」单元
 * `run` ：同上一行 → { "result": "..." }
 *
 * 覆盖原版：
 *
 * ```java
 * // 双线性：d 是 2x2 的四个角，delta 同时作用于两维
 * public static double lerp2(double d, double d0, double d1, double d2, double d3, double d4) {
 *     return lerp(d1, lerp(d, d2, d3), lerp(d, d4, d5));
 * }
 *
 * // 三线性：d 是 2x2x2 的八个角
 * public static double lerp3(double d, double d0, ..., double d7) {
 *     return lerp(d2, lerp2(d, d0, d3, d4, d5, d6), lerp2(d, d0, d7, d8, d9, d10));
 * }
 *
 * public static float catmullrom(float d, float d0, float d1, float d2, float d3) { ... }
 *
 * public static double smoothstep(double x) {
 *     return x * x * x * (x * (x * 6.0 - 15.0) + 10.0);
 * }
 * ```
 *
 * ## 两个关键点
 *
 * 1. **`lerp3` 只接受 1 个插值因子**，同时作用于 X/Y/Z 三维。这是它与「三线性插值」的区别：
 *    三线性需要 3 个独立因子，而 `Mth.lerp3` 是「立方体八个角按同一个 delta 插值」。
 *
 * 2. **`Mth.smoothstep` 是单参数版本**（`smoothstep(x) = x³(6x²-15x+10)`），
 *    与常见的 `smoothstep(edge0, edge1, x)` 三参数版本不同。混用会让边缘行为完全不同。
 *
 * 单元格式：
 * - `lerp2|<delta>,<d0>,<d1>,<d2>,<d3>` —— 双线性
 * - `lerp3|<delta>,<d0>..<d7>` —— 三线性
 * - `catmullrom|<delta>,<p0>,<p1>,<p2>,<p3>` —— Catmull-Rom 样条
 * - `smoothstep|<x>` —— 平滑阶跃
 */

#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"
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

/// 按逗号切出一组 double。
std::vector<double> parseDoubles(const std::string& text)
{
    std::vector<double> values;
    for (const std::string& piece : split(text, ',')) {
        values.push_back(std::strtod(piece.c_str(), nullptr));
    }
    return values;
}

/**
 * @brief 把 double 用 14 位有效数字输出。
 *
 * 14 位足以让插值链上的差异显现，又避开 `%g` 在 C++ 与 Java 下的末位展开分歧。
 */
std::string formatDouble(double value)
{
    if (value == 0.0) {
        return "0";
    }
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.14g", value);
    return std::string(buffer);
}

constexpr const char* DELTAS[] = {"0", "0.25", "0.5", "0.75", "1"};

void writeList(FILE* out)
{
    // catmullrom：用单调递增的四个控制点。
    for (const char* delta : DELTAS) {
        std::fprintf(out, "catmullrom|%s,0.1,0.4,0.7,1.0\n", delta);
    }

    // smoothstep：覆盖 0、1、中点与定义域外。
    const char* samples[] = {"0", "0.1", "0.25", "0.5", "0.75", "0.9", "1"};
    for (const char* sample : samples) {
        std::fprintf(out, "smoothstep|%s\n", sample);
    }
}

std::string runLine(std::string_view inputLine)
{
    const std::string line = json::parse(inputLine).at("unit").get<std::string>();
    const size_t bar = line.find('|');
    if (bar == std::string::npos) {
        std::fprintf(stderr, "mth_interpolation: 单元应为 <模式>|<参数>，实际：%s\n", line.c_str());
        std::exit(1);
    }
    const std::string mode = line.substr(0, bar);
    const std::vector<double> args = parseDoubles(line.substr(bar + 1));

    double result = 0.0;
    if (mode == "catmullrom") {
        if (args.size() != 5) {
            std::fprintf(stderr, "mth_interpolation: catmullrom 需要 5 个参数：%s\n", line.c_str());
            std::exit(1);
        }
        // 原版签名 `float catmullrom(float, float, float, float, float)`，
        // 且字面量全部带 F 后缀（0.5F / 2.0F / 5.0F），**全程 float 运算**。
        // TODO: Cubium 的 MathUtils 没有 catmullrom，这里按原版 float 语义就地实现。
        const ::mc::f32 d = static_cast<::mc::f32>(args[0]);
        const ::mc::f32 p0 = static_cast<::mc::f32>(args[1]);
        const ::mc::f32 p1 = static_cast<::mc::f32>(args[2]);
        const ::mc::f32 p2 = static_cast<::mc::f32>(args[3]);
        const ::mc::f32 p3 = static_cast<::mc::f32>(args[4]);
        result = static_cast<double>(0.5f *
            (2.0f * p1 + (p2 - p0) * d + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * d * d +
                (3.0f * p1 - p0 - 3.0f * p2 + p3) * d * d * d));
    } else if (mode == "smoothstep") {
        if (args.size() != 1) {
            std::fprintf(stderr, "mth_interpolation: smoothstep 需要 1 个参数：%s\n", line.c_str());
            std::exit(1);
        }
        // TODO: Cubium 的 MathUtils 只有 smoothstep(edge0, edge1, x)（GLSL 风格，
        // t*t*(3-2t) 且带钳制），**没有** Mth.smoothstep(x) 这个单参数版本
        // （x³(6x²-15x+10)，无钳制）。两者公式与定义域行为都不同。
        // 这里按原版公式就地实现，用于暴露差异。
        const double x = args[0];
        result = x * x * x * (x * (x * 6.0 - 15.0) + 10.0);
    } else {
        std::fprintf(stderr, "mth_interpolation: 未知模式：%s\n", mode.c_str());
        std::exit(1);
    }

    return json{{"unit", line}, {"result", formatDouble(result)}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("mth_interpolation", &writeList, &runLine)

} // namespace mc::diff
