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
 * @brief 差分能力：`PortalShape.getRelativePosition` 的传送门相对位置（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": "x,y,z" }
 *
 * 复刻原版 `net.minecraft.world.level.portal.PortalShape`：
 *
 * ```java
 * public static Vec3 getRelativePosition(
 *         BlockUtil.FoundRectangle rectangle,      // { minCorner, axis1Size, axis2Size }
 *         Direction.Axis axis,                     // 传送门的「宽度轴」
 *         Vec3 entityPos,
 *         EntityDimensions dimensions) {
 *     double widthSpace = rectangle.axis1Size - dimensions.width();
 *     double heightSpace = rectangle.axis2Size - dimensions.height();
 *     BlockPos corner = rectangle.minCorner;
 *
 *     double d2;
 *     if (widthSpace > 0.0) {
 *         double d3 = corner.get(axis) + dimensions.width() / 2.0;
 *         d2 = Mth.clamp(Mth.inverseLerp(entityPos.get(axis) - d3, 0.0, widthSpace), 0.0, 1.0);
 *     } else {
 *         d2 = 0.5;                                  // 空间不足时居中对齐
 *     }
 *
 *     double d5;
 *     if (heightSpace > 0.0) {
 *         d5 = Mth.clamp(
 *             Mth.inverseLerp(entityPos.y - corner.y, 0.0, heightSpace),
 *             0.0, 1.0);
 *     } else {
 *         d5 = 0.0;                                  // 注意：高度不足时是 0，不是 0.5
 *     }
 *
 *     Direction.Axis other = axis == Direction.Axis.X ? Direction.Axis.Z : Direction.Axis.X;
 *     double d4 = entityPos.get(other) - (corner.get(other) + 0.5);
 *     return new Vec3(d2, d5, d4);
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **宽度不足与高度不足的兜底值不同：`0.5` vs `0.0`。**
 *
 *    ```java
 *    if (widthSpace > 0.0) { ... } else { d2 = 0.5; }    // 水平居中
 *    if (heightSpace > 0.0) { ... } else { d5 = 0.0; }   // 贴底
 *    ```
 *
 *    **这是极易写错的一处**——两者都「空间不足」，但行为不对称。
 *
 * 2. **返回的 `Vec3` 三个分量语义完全不同，不是世界坐标。**
 *
 *    | 分量 | 语义 |
 *    |---|---|
 *    | `x` | 沿宽度轴的**归一化插值** `[0, 1]` |
 *    | `y` | 沿高度的**归一化插值** `[0, 1]` |
 *    | `z` | 沿**垂直轴**的相对偏移（**带符号的绝对差值**，不是归一化）|
 *
 *    **`z` 分量是 `entityPos.get(other) - (corner.get(other) + 0.5)`**——一个以「传送门中心平面」
 *    为原点的绝对偏移，**没有除以任何尺寸**。
 *
 * 3. **`other` 轴的选择：`X` 与 `Z` 互换，其他轴保持。**
 *
 *    ```java
 *    Direction.Axis other = axis == Direction.Axis.X ? Direction.Axis.Z : Direction.Axis.X;
 *    ```
 *
 *    若 `axis == Y`（下界传送门不会这样，但代码允许），`other` 得 `X`——**不是 Z**。
 *
 * 4. **`Mth.inverseLerp(value, 0.0, span)` 等价于 `value / span`，但除零行为不同。**
 *
 *    ```java
 *    Mth.inverseLerp(value, min, max) = (value - min) / (max - min);
 *    ```
 *
 *    本能力中 `widthSpace` / `heightSpace` 已保证 `> 0`，所以不会除零。
 *
 * 单元格式（7 + 8 段）：
 * - `<cornerX>|<cornerY>|<cornerZ>|<axis1Size>|<axis2Size>|<axis>|<width>|<height>|<posX>|<posY>|<posZ>`
 *
 * `axis` 取 `x` / `y` / `z`。
 * 输出 `x,y,z` 用 `%.9g` 格式化（去尾零）。
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

/// 原版 `Mth.clamp(value, min, max)`。
f64 clamp(f64 value, f64 min, f64 max)
{
    return value < min ? min : (value > max ? max : value);
}

/// 原版 `Mth.inverseLerp(value, min, max)` = `(value - min) / (max - min)`。
f64 inverseLerp(f64 value, f64 min, f64 max)
{
    return (value - min) / (max - min);
}

/// 格式化浮点：`%.9g`，去掉尾随零（与 C 的 `%g` 一致）。
std::string formatNumber(f64 value)
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.9g", value);
    return buffer;
}

/// 按轴取坐标分量。
f64 axisValue(i32 axis, f64 x, f64 y, f64 z)
{
    if (axis == 0) {
        return x;
    }
    if (axis == 1) {
        return y;
    }
    return z;
}

i32 parseAxis(const std::string& text)
{
    if (text == "x") {
        return 0;
    }
    if (text == "y") {
        return 1;
    }
    if (text == "z") {
        return 2;
    }
    throw std::runtime_error("portal_relative: 未知轴：" + text);
}

/// 原版 `PortalShape.getRelativePosition`。
void getRelativePosition(f64 cornerX,
    f64 cornerY,
    f64 cornerZ,
    f64 axis1Size,
    f64 axis2Size,
    i32 axis,
    f64 width,
    f64 height,
    f64 posX,
    f64 posY,
    f64 posZ,
    f64& outX,
    f64& outY,
    f64& outZ)
{
    const f64 widthSpace = axis1Size - width;
    const f64 heightSpace = axis2Size - height;

    // 宽度轴：空间不足时取 0.5（水平居中）。
    f64 along;
    if (widthSpace > 0.0) {
        const f64 center = axisValue(axis, cornerX, cornerY, cornerZ) + width / 2.0;
        along = clamp(inverseLerp(axisValue(axis, posX, posY, posZ) - center, 0.0, widthSpace), 0.0, 1.0);
    } else {
        along = 0.5;
    }

    // 高度轴：空间不足时取 0.0（贴底）。
    f64 up;
    if (heightSpace > 0.0) {
        up = clamp(inverseLerp(posY - cornerY, 0.0, heightSpace), 0.0, 1.0);
    } else {
        up = 0.0;
    }

    // 垂直轴：X 与 Z 互换。
    const i32 other = axis == 0 ? 2 : 0;
    const f64 across = axisValue(other, posX, posY, posZ) - (axisValue(other, cornerX, cornerY, cornerZ) + 0.5);

    outX = along;
    outY = up;
    outZ = across;
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // X 轴传送门，实体在中心
        "0|0|0|2|3|x|0.6|1.8|1|0|0",
        // X 轴：实体偏左 / 偏右
        "0|0|0|2|3|x|0.6|1.8|-1|0|0",
        "0|0|0|2|3|x|0.6|1.8|3|0|0",
        // Z 轴传送门（other 轴为 X）
        "0|0|0|2|3|z|0.6|1.8|0|0|1",
        "0|0|0|2|3|z|0.6|1.8|1|0|0",
        // 宽度不足（widthSpace <= 0）→ 0.5
        "0|0|0|1|3|x|2.0|1.8|0|0|0",
        "0|0|0|0|3|x|1.0|1.8|0|0|0",
        // 高度不足（heightSpace <= 0）→ 0.0
        "0|0|0|2|1|x|0.6|2.0|0|0|0",
        "0|0|0|2|0|x|0.6|1.8|0|0|0",
        // 两者都不足
        "0|0|0|1|1|x|2.0|2.0|0|0|0",
        // Y 轴（other 得 X，非 Z）
        "0|0|0|2|3|y|0.6|1.8|0|1|2",
        // 非原点角
        "10|64|-20|3|4|x|0.6|1.8|12|64|-20",
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
    if (parts.size() != 11) {
        throw std::runtime_error("portal_relative: 单元需要 11 段：" + unit);
    }

    f64 outX = 0.0;
    f64 outY = 0.0;
    f64 outZ = 0.0;
    // FoundRectangle 的两个尺寸字段是 int，不是 double。
    getRelativePosition(static_cast<f64>(std::stoi(parts[0])),
        static_cast<f64>(std::stoi(parts[1])),
        static_cast<f64>(std::stoi(parts[2])),
        static_cast<f64>(std::stoi(parts[3])),
        static_cast<f64>(std::stoi(parts[4])),
        parseAxis(parts[5]),
        // EntityDimensions 的宽高是 float，必须先降到 float 再提升，否则精度不同。
        static_cast<f64>(std::stof(parts[6])),
        static_cast<f64>(std::stof(parts[7])),
        std::stod(parts[8]),
        std::stod(parts[9]),
        std::stod(parts[10]),
        outX,
        outY,
        outZ);

    const std::string value = formatNumber(outX) + "," + formatNumber(outY) + "," + formatNumber(outZ);
    return json{{"unit", unit}, {"value", value}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("portal_relative", &writeList, &runLine)

} // namespace mc::diff
