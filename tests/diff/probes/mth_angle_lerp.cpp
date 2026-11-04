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
 * @brief 差分能力：`Mth` 的角度插值与 Catmull-Rom 样条（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.util.Mth`：
 *
 * ```java
 * public static float wrapDegrees(float degrees) {
 *     float f = degrees % 360.0F;
 *     if (f >= 180.0F)  f -= 360.0F;
 *     if (f < -180.0F)  f += 360.0F;
 *     return f;
 * }
 *
 * public static float rotLerp(float delta, float start, float end) {
 *     return start + delta * wrapDegrees(end - start);
 * }
 *
 * public static double rotLerp(double delta, double start, double end) {
 *     return start + delta * wrapDegrees(end - start);
 * }
 *
 * public static float rotLerpRad(float delta, float start, float end) {
 *     float f = end - start;
 *     while (f < (float) -Math.PI)    f += (float) (Math.PI * 2);
 *     while (f >= (float) Math.PI)    f -= (float) (Math.PI * 2);
 *     return start + delta * f;
 * }
 *
 * public static float catmullrom(float delta, float p0, float p1, float p2, float p3) {
 *     return 0.5F * (
 *         2.0F * p1
 *         + (p2 - p0) * delta
 *         + (2.0F * p0 - 5.0F * p1 + 4.0F * p2 - p3) * delta * delta
 *         + (3.0F * p1 - p0 - 3.0F * p2 + p3) * delta * delta * delta);
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`wrapDegrees` 的结果区间是 `(-180, 180]`，`rotLerpRad` 是 `[-π, π)`——两者的开闭端相反。**
 *
 *    `wrapDegrees` 用 `>= 180` 减、`< -180` 加，所以：
 *
 *    - **`wrapDegrees(180)` → `-180`**（180 被减成 -180）
 *    - `wrapDegrees(-180)` → `-180`（不变）
 *    - `wrapDegrees(360)` → `0`
 *
 *    `rotLerpRad` 用 `while (f >= π) f -= 2π`，所以 `π` 会被减成 `-π`——同样是「上界归到下界」。
 *    两侧都是**左闭右开**（`rotLerpRad`）与**左开右闭**（`wrapDegrees` 输出 `(-180,180]`）
 *    的微妙差别。
 *
 * 2. **`rotLerp` 走的是「先 wrap 差值再插值」的短路路径，不是「插值后 wrap」。**
 *
 *    ```java
 *    return start + delta * wrapDegrees(end - start);
 *    ```
 *
 *    对 `rotLerp(0.5, 170, -170)`：
 *    - 直接插值得 `0`（(170 + (-170))/2 = 0），但这是**反向绕路**
 *    - 原版：`wrapDegrees(-170 - 170) = wrapDegrees(-340) = 20` → `170 + 0.5*20 = 180`
 *
 *    **结果 `180` 而不是 `0`**——玩家从 170° 转到 -170° 只转 20°（顺时针），不是 340°。
 *    这是角度插值的核心语义。
 *
 * 3. **`rotLerpRad` 的 `while` 循环在极大输入下会迭代很多次。**
 *
 *    ```java
 *    while (f < (float) -Math.PI) f += (float) (Math.PI * 2);
 *    while (f >= (float) Math.PI) f -= (float) (Math.PI * 2);
 *    ```
 *
 *    若 `end - start = 1e6`，循环要跑约 `1.6e5` 次。**原版没有用 `%` 优化**——这是性能陷阱，
 *    但行为上仍等价于取模。Cubium 若用 `%` 实现，结果应相同（需验证浮点精度）。
 *
 *    注意 `(float) Math.PI` 与 `(float)(Math.PI * 2)` 都是 `float` 且**不精确**：
 *    `π` 的 `float` 是 `3.1415927F`，`2π` 的 `float` 是 `6.2831855F`。**不是精确的 2 倍关系**
 *    （`3.1415927F * 2 = 6.2831854F` ≠ `6.2831855F`），因此循环边界有微妙的非对称。
 *
 * 4. **`catmullrom` 是四点样条，不是两点插值。**
 *
 *    ```java
 *    catmullrom(delta, p0, p1, p2, p3)
 *    ```
 *
 *    - `p1`、`p2` 是**实际端点**（`delta=0` 得 `p1`，`delta=1` 得 `p2`）
 *    - `p0`、`p3` 是**切向量参考点**（来自相邻段），决定曲线在端点处的斜率
 *
 *    `delta` 超出 `[0, 1]` 时**会外推**，不做 clamp。
 *
 *    **`catmullrom` 不保证单调**：`p0 > p1 < p2 > p3` 之类的配置会产生过冲。这与线性插值
 *    `lerp` 有本质区别。
 *
 * 单元格式：
 * - `wrap|degrees` —— `wrapDegrees`
 * - `rotLerp|<delta>|<start>|<end>` —— `float` 版角度插值
 * - `rotLerpRad|<delta>|<start>|<end>` —— 弧度版
 * - `catmull|<delta>|<p0>|<p1>|<p2>|<p3>`
 *
 * 全部是 `float` 运算，输出按 `%.9g` 格式化后去尾零（`%.6g` 会掩盖 `float` 末位差异）。
 *
 * TODO: 未覆盖 `triangleWave`、`smoothstep`、`square`（同属 `Mth` 的波形/缓动族）。
 */
#include <cmath>
#include <cstdio>
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

/// 格式化 `float`：`%.9g` 并去尾零。
std::string formatNumber(f32 value)
{
    if (std::isnan(value)) {
        return "NaN";
    }
    if (std::isinf(value)) {
        return value > 0 ? "Infinity" : "-Infinity";
    }
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.9g", static_cast<f64>(value));
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

/// 原版 `Mth.wrapDegrees(float)`。
f32 wrapDegrees(f32 degrees)
{
    f32 f = std::fmod(degrees, 360.0F);
    if (f >= 180.0F) {
        f -= 360.0F;
    }
    if (f < -180.0F) {
        f += 360.0F;
    }
    return f;
}

/// 原版 `Mth.rotLerp(float, float, float)`。
f32 rotLerp(f32 delta, f32 start, f32 end)
{
    return start + delta * wrapDegrees(end - start);
}

/// 原版 `Mth.rotLerpRad`：用 while 循环回绕，边界 `[-π, π)`。
f32 rotLerpRad(f32 delta, f32 start, f32 end)
{
    constexpr f32 PI = 3.1415927F;
    constexpr f32 TWO_PI = 6.2831855F;
    f32 f = end - start;
    while (f < -PI) {
        f += TWO_PI;
    }
    while (f >= PI) {
        f -= TWO_PI;
    }
    return start + delta * f;
}

/// 原版 `Mth.catmullrom`：四点样条。
f32 catmullrom(f32 delta, f32 p0, f32 p1, f32 p2, f32 p3)
{
    return 0.5F *
        (2.0F * p1 + (p2 - p0) * delta + (2.0F * p0 - 5.0F * p1 + 4.0F * p2 - p3) * delta * delta +
            (3.0F * p1 - p0 - 3.0F * p2 + p3) * delta * delta * delta);
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // wrap：边界与常规
        "wrap|0",
        "wrap|90",
        "wrap|179",
        "wrap|180",
        "wrap|-180",
        "wrap|181",
        "wrap|360",
        "wrap|-360",
        "wrap|540",
        "wrap|-540",
        // wrapInt：int 重载
        "wrapInt|180",
        "wrapInt|-180",
        // rotLerp：常规
        "rotLerp|0|0|90",
        "rotLerp|1|0|90",
        "rotLerp|0.5|0|90",
        // rotLerp：跨越 ±180 边界（关键）
        "rotLerp|0.5|170|-170",
        "rotLerp|0.5|-170|170",
        "rotLerp|0|170|-170",
        "rotLerp|1|170|-170",
        // rotLerp：超出 [0,1] 的外推
        "rotLerp|1.5|0|90",
        "rotLerp|-0.5|0|90",
        // rotLerpRad：边界 [-π, π)
        "rotLerpRad|0.5|0|1.5707964",
        "rotLerpRad|0.5|3.0|-3.0",
        "rotLerpRad|0.5|-3.0|3.0",
        "rotLerpRad|0|3.0|-3.0",
        // catmull：端点取值
        "catmull|0|1|2|3|4",
        "catmull|1|1|2|3|4",
        "catmull|0.5|1|2|3|4",
        // catmull：线性配置（p0,p1,p2,p3 等差时退化为线性）
        "catmull|0.5|0|1|2|3",
        "catmull|0.25|0|1|2|3",
        // catmull：过冲配置
        "catmull|0.5|0|0|1|1",
        "catmull|0.25|0|0|1|1",
        // catmull：外推
        "catmull|1.5|1|2|3|4",
        "catmull|-0.5|1|2|3|4",
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

f32 parseF(const std::string& text)
{
    return static_cast<f32>(std::stod(text));
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("mth_angle_lerp: 单元至少两段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "wrap" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", formatNumber(wrapDegrees(parseF(parts[1])))}}.dump();
    }
    if (mode == "wrapInt" && parts.size() == 2) {
        const f32 result = wrapDegrees(static_cast<f32>(std::stoi(parts[1])));
        return json{{"unit", unit}, {"value", formatNumber(result)}}.dump();
    }
    if (mode == "rotLerp" && parts.size() == 4) {
        const f32 result = rotLerp(parseF(parts[1]), parseF(parts[2]), parseF(parts[3]));
        return json{{"unit", unit}, {"value", formatNumber(result)}}.dump();
    }
    if (mode == "rotLerpRad" && parts.size() == 4) {
        const f32 result = rotLerpRad(parseF(parts[1]), parseF(parts[2]), parseF(parts[3]));
        return json{{"unit", unit}, {"value", formatNumber(result)}}.dump();
    }
    if (mode == "catmull" && parts.size() == 6) {
        const f32 result =
            catmullrom(parseF(parts[1]), parseF(parts[2]), parseF(parts[3]), parseF(parts[4]), parseF(parts[5]));
        return json{{"unit", unit}, {"value", formatNumber(result)}}.dump();
    }

    throw std::runtime_error("mth_angle_lerp: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(mth_angle_lerp, &writeList, &runLine)

} // namespace mc::diff
