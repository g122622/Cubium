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
 * @brief 差分能力：`PositionMoveRotation.calculateAbsolute`（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": "pos@delta@yRot@xRot" }
 *
 * 复刻原版 `net.minecraft.world.entity.PositionMoveRotation`：
 *
 * ```java
 * public static PositionMoveRotation calculateAbsolute(
 *         PositionMoveRotation p1, PositionMoveRotation p2, Set<Relative> relatives) {
 *     double d0 = relatives.contains(X) ? p1.position.x : 0.0;
 *     double d1 = relatives.contains(Y) ? p1.position.y : 0.0;
 *     double d2 = relatives.contains(Z) ? p1.position.z : 0.0;
 *     float f  = relatives.contains(Y_ROT) ? p1.yRot : 0.0F;
 *     float f1 = relatives.contains(X_ROT) ? p1.xRot : 0.0F;
 *
 *     Vec3 pos = new Vec3(d0 + p2.position.x, d1 + p2.position.y, d2 + p2.position.z);
 *     float yRot = f + p2.yRot;
 *     float xRot = Mth.clamp(f1 + p2.xRot, -90.0F, 90.0F);   // 只有 xRot 被钳
 *
 *     Vec3 delta = p1.deltaMovement;
 *     if (relatives.contains(ROTATE_DELTA)) {
 *         float f4 = p1.yRot - yRot;
 *         float f5 = p1.xRot - xRot;
 *         delta = delta.xRot((float) Math.toRadians(f5));
 *         delta = delta.yRot((float) Math.toRadians(f4));
 *     }
 *
 *     Vec3 newDelta = new Vec3(
 *         calculateDelta(delta.x, p2.deltaMovement.x, relatives, DELTA_X),
 *         calculateDelta(delta.y, p2.deltaMovement.y, relatives, DELTA_Y),
 *         calculateDelta(delta.z, p2.deltaMovement.z, relatives, DELTA_Z));
 *
 *     return new PositionMoveRotation(pos, newDelta, yRot, xRot);
 * }
 *
 * private static double calculateDelta(double a, double b, Set<Relative> r, Relative which) {
 *     return r.contains(which) ? a + b : b;
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **位置与旋转都遵循「`Relative` 含该位则取 `p1`，否则取 `0`」，然后与 `p2` 相加。**
 *
 *    ```java
 *    double d0 = relatives.contains(X) ? p1.position.x : 0.0;
 *    Vec3 pos = new Vec3(d0 + p2.position.x, ...);
 *    ```
 *
 *    **含 `X` 是「相对移动」语义**（`p1` 是增量）；**不含 `X` 是「绝对设置」语义**
 *    （`p1` 被丢弃，结果就是 `p2`）。
 *
 * 2. **只有 `xRot` 被钳到 `[-90, 90]`，`yRot` 不钳。**
 *
 *    ```java
 *    float f2 = f + p2.yRot;                                 // 不钳
 *    float f3 = Mth.clamp(f1 + p2.xRot, -90.0F, 90.0F);      // 钳
 *    ```
 *
 *    `xRot` 是**俯仰角**（pitch），物理上不能超过 ±90°；`yRot` 是**偏航角**（yaw），可以绕圈。
 *
 * 3. **`ROTATE_DELTA` 只影响速度向量，不影响位置或朝向。**
 *
 *    ```java
 *    if (relatives.contains(ROTATE_DELTA)) {
 *        float f4 = p1.yRot - yRot;                      // 注意：p1 - 新值
 *        float f5 = p1.xRot - xRot;
 *        delta = delta.xRot((float) Math.toRadians(f5));  // 先 xRot
 *        delta = delta.yRot((float) Math.toRadians(f4));  // 再 yRot
 *    }
 *    ```
 *
 *    **顺序是「先 xRot 再 yRot」**，且角度是 `(float) Math.toRadians(delta)`——
 *    先转 `double` 再降为 `float`。
 *
 * 4. **`calculateDelta` 的三轴逻辑与位置完全一致。**
 *
 *    ```java
 *    return relatives.contains(which) ? a + b : b;
 *    ```
 *
 *    注意这里的 `a` 是**旋转后的** `delta`（若 `ROTATE_DELTA` 生效），`b` 是 `p2.deltaMovement`。
 *
 * 单元格式（24 段）：
 * - `p1`：`posX|posY|posZ|deltaX|deltaY|deltaZ|yRot|xRot`（8 段）
 * - `p2`：同上（8 段）
 * - `relatives`：逗号分隔的名字，空集写 `-`（1 段）
 * - 合计 8 + 8 + 1 = 17 段
 *
 * 输出 `posX,posY,posZ|deltaX,deltaY,deltaZ|yRot|xRot`，浮点用 `%.9g`。
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

constexpr const char* RELATIVE_NAMES[] = {
    "x", "y", "z", "y_rot", "x_rot", "delta_x", "delta_y", "delta_z", "rotate_delta"};

constexpr i32 RELATIVE_COUNT = 9;

/// 与原版 `Math.PI` 一致的圆周率常量。
constexpr f64 PI = 3.141592653589793;

/// 原版 `Mth` 的 65536 项正弦查表。
struct SinTable {
    f32 values[65536];

    SinTable()
    {
        for (i32 i = 0; i < 65536; ++i) {
            values[i] = static_cast<f32>(std::sin(static_cast<f64>(i) / 10430.378350470453));
        }
    }
};

const SinTable& sinTable()
{
    static const SinTable table;
    return table;
}

/// 原版 `Mth.sin(double)`。
f32 mthSin(f64 value)
{
    const i64 index = static_cast<i64>(value * 10430.378350470453) & 65535LL;
    return sinTable().values[static_cast<i32>(index)];
}

/// 原版 `Mth.cos(double)`。
f32 mthCos(f64 value)
{
    const i64 index = static_cast<i64>(value * 10430.378350470453 + 16384.0) & 65535LL;
    return sinTable().values[static_cast<i32>(index)];
}

/// 原版 `Mth.clamp(float, float, float)`。
f32 clampF(f32 value, f32 min, f32 max)
{
    return value < min ? min : (value > max ? max : value);
}

/// 原版 `Vec3.xRot(float)`。
void applyXRot(f64& x, f64& y, f64& z, f32 radians)
{
    const f32 cosValue = mthCos(radians);
    const f32 sinValue = mthSin(radians);
    const f64 newY = y * cosValue + z * sinValue;
    const f64 newZ = z * cosValue - y * sinValue;
    y = newY;
    z = newZ;
    (void)x;
}

/// 原版 `Vec3.yRot(float)`。
void applyYRot(f64& x, f64& y, f64& z, f32 radians)
{
    const f32 cosValue = mthCos(radians);
    const f32 sinValue = mthSin(radians);
    const f64 newX = x * cosValue + z * sinValue;
    const f64 newZ = z * cosValue - x * sinValue;
    x = newX;
    z = newZ;
    (void)y;
}

/// 一个 `PositionMoveRotation`。
struct Pmr {
    f64 posX = 0.0;
    f64 posY = 0.0;
    f64 posZ = 0.0;
    f64 deltaX = 0.0;
    f64 deltaY = 0.0;
    f64 deltaZ = 0.0;
    f32 yRot = 0.0F;
    f32 xRot = 0.0F;
};

i32 relativeIndex(const std::string& name)
{
    for (i32 i = 0; i < RELATIVE_COUNT; ++i) {
        if (name == RELATIVE_NAMES[i]) {
            return i;
        }
    }
    throw std::runtime_error("pmr: 未知 Relative：" + name);
}

i32 parseMask(std::string text)
{
    if (text.empty() || text == "-") {
        return 0;
    }
    i32 mask = 0;
    std::string token;
    for (std::size_t i = 0; i <= text.size(); ++i) {
        if (i == text.size() || text[i] == ',') {
            if (!token.empty()) {
                mask |= 1 << relativeIndex(token);
                token.clear();
            }
        } else {
            token.push_back(text[i]);
        }
    }
    return mask;
}

bool has(i32 mask, i32 bit)
{
    return (mask & (1 << bit)) != 0;
}

std::string formatNumber(f64 value)
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.9g", value);
    return buffer;
}

void writeList(FILE* out)
{
    // p1 = pos|delta|yRot|xRot, p2 同, relatives
    constexpr const char* UNITS[] = {
        // 空集合：结果完全等于 p2
        "0|0|0|0|0|0|0|0|1|2|3|0.5|0.25|0.125|10|20|-",
        // 位置全相对
        "1|2|3|0|0|0|0|0|10|20|30|0|0|0|0|0|x,y,z",
        // 位置全绝对（含 X 但 p1 为 0 的对照）
        "0|0|0|0|0|0|0|0|10|20|30|0|0|0|0|0|x,y,z",
        // 只有 X 相对
        "5|0|0|0|0|0|0|0|10|20|30|0|0|0|0|0|x",
        // yRot 相对 / 绝对
        "0|0|0|0|0|0|45|0|0|0|0|0|0|0|10|0|y_rot",
        "0|0|0|0|0|0|45|0|0|0|0|0|0|0|10|0|-",
        // xRot 被钳：70 + 40 = 110 -> 90
        "0|0|0|0|0|0|0|70|0|0|0|0|0|0|0|40|x_rot",
        // xRot 被钳：-70 + -40 = -110 -> -90
        "0|0|0|0|0|0|0|-70|0|0|0|0|0|0|0|-40|x_rot",
        // yRot 不钳：170 + 40 = 210
        "0|0|0|0|0|0|170|0|0|0|0|0|0|0|40|0|y_rot",
        // delta 相对 / 绝对
        "0|0|0|1|2|3|0|0|0|0|0|10|20|30|0|0|delta_x,delta_y,delta_z",
        "0|0|0|1|2|3|0|0|0|0|0|10|20|30|0|0|-",
        // ROTATE_DELTA
        "0|0|0|1|0|0|30|0|0|0|0|10|0|0|0|0|rotate_delta",
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

Pmr parsePmr(const std::vector<std::string>& parts, std::size_t offset)
{
    Pmr pmr;
    pmr.posX = std::stod(parts[offset]);
    pmr.posY = std::stod(parts[offset + 1]);
    pmr.posZ = std::stod(parts[offset + 2]);
    pmr.deltaX = std::stod(parts[offset + 3]);
    pmr.deltaY = std::stod(parts[offset + 4]);
    pmr.deltaZ = std::stod(parts[offset + 5]);
    // yRot / xRot 是 float。
    pmr.yRot = std::stof(parts[offset + 6]);
    pmr.xRot = std::stof(parts[offset + 7]);
    return pmr;
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() != 17) {
        throw std::runtime_error("pmr: 单元需要 17 段：" + unit);
    }

    const Pmr p1 = parsePmr(parts, 0);
    const Pmr p2 = parsePmr(parts, 8);
    const i32 mask = parseMask(parts[16]);

    // 位置：含该轴则取 p1，否则 0；再与 p2 相加。
    const f64 baseX = has(mask, 0) ? p1.posX : 0.0;
    const f64 baseY = has(mask, 1) ? p1.posY : 0.0;
    const f64 baseZ = has(mask, 2) ? p1.posZ : 0.0;
    const f64 posX = baseX + p2.posX;
    const f64 posY = baseY + p2.posY;
    const f64 posZ = baseZ + p2.posZ;

    // 旋转：yRot 不钳，xRot 钳到 [-90, 90]。
    const f32 yRotBase = has(mask, 3) ? p1.yRot : 0.0F;
    const f32 xRotBase = has(mask, 4) ? p1.xRot : 0.0F;
    const f32 yRot = yRotBase + p2.yRot;
    const f32 xRot = clampF(xRotBase + p2.xRot, -90.0F, 90.0F);

    // 速度：ROTATE_DELTA 时先按角度差旋转。
    f64 deltaX = p1.deltaX;
    f64 deltaY = p1.deltaY;
    f64 deltaZ = p1.deltaZ;
    if (has(mask, 8)) {
        const f32 yawDelta = p1.yRot - yRot;
        const f32 pitchDelta = p1.xRot - xRot;
        // 原版 Math.toRadians(deg) = deg / 180.0 * PI，先以 double 计算再降为 float。
        applyXRot(deltaX, deltaY, deltaZ, static_cast<f32>(static_cast<f64>(pitchDelta) / 180.0 * PI));
        applyYRot(deltaX, deltaY, deltaZ, static_cast<f32>(static_cast<f64>(yawDelta) / 180.0 * PI));
    }

    const f64 newDeltaX = has(mask, 5) ? deltaX + p2.deltaX : p2.deltaX;
    const f64 newDeltaY = has(mask, 6) ? deltaY + p2.deltaY : p2.deltaY;
    const f64 newDeltaZ = has(mask, 7) ? deltaZ + p2.deltaZ : p2.deltaZ;

    const std::string value = formatNumber(posX) + "," + formatNumber(posY) + "," + formatNumber(posZ) + "|" +
        formatNumber(newDeltaX) + "," + formatNumber(newDeltaY) + "," + formatNumber(newDeltaZ) + "|" +
        formatNumber(static_cast<f64>(yRot)) + "|" + formatNumber(static_cast<f64>(xRot));
    return json{{"unit", unit}, {"value", value}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("position_move_rotation", &writeList, &runLine)

} // namespace mc::diff
