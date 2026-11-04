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
 * @brief 差分能力：`BlockPos` 的几何变换与极值（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.core.BlockPos`：
 *
 * ```java
 * public BlockPos rotate(Rotation rot) {
 *     return switch (rot) {
 *         case CLOCKWISE_90        -> new BlockPos(-this.getZ(),  this.getY(),  this.getX());
 *         case CLOCKWISE_180       -> new BlockPos(-this.getX(),  this.getY(), -this.getZ());
 *         case COUNTERCLOCKWISE_90 -> new BlockPos( this.getZ(),  this.getY(), -this.getX());
 *         case NONE                -> this;
 *     };
 * }
 *
 * public BlockPos cross(Vec3i v) {
 *     return new BlockPos(
 *         this.getY() * v.getZ() - this.getZ() * v.getY(),
 *         this.getZ() * v.getX() - this.getX() * v.getZ(),
 *         this.getX() * v.getY() - this.getY() * v.getX());
 * }
 *
 * public static BlockPos containing(double x, double y, double z) {
 *     return new BlockPos(Mth.floor(x), Mth.floor(y), Mth.floor(z));
 * }
 *
 * public static BlockPos min(BlockPos a, BlockPos b) {
 *     return new BlockPos(Math.min(a.x, b.x), Math.min(a.y, b.y), Math.min(a.z, b.z));
 * }
 * public static BlockPos max(BlockPos a, BlockPos b) { ... Math.max ... }
 *
 * public BlockPos multiply(int factor) {
 *     if (factor == 1) return this;
 *     return factor == 0 ? ZERO : new BlockPos(x * factor, y * factor, z * factor);
 * }
 *
 * public BlockPos subtract(Vec3i v) {
 *     return v == Vec3i.ZERO ? this
 *         : new BlockPos(x - v.getX(), y - v.getY(), z - v.getZ());
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`rotate` 的 `CLOCKWISE_90` 与 `COUNTERCLOCKWISE_90` 不对称。**
 *
 *    | 旋转 | 结果 |
 *    |---|---|
 *    | `CLOCKWISE_90` | `(-z, y, x)` |
 *    | `CLOCKWISE_180` | `(-x, y, -z)` |
 *    | `COUNTERCLOCKWISE_90` | `(z, y, -x)` |
 *
 *    `CLOCKWISE_90` 是 `(-z, y, x)`，`COUNTERCLOCKWISE_90` 是 `(z, y, -x)`——**两者互为相反数**
 *    （`(-z,y,x)` 取反得 `(z,-y,-x)`，但 y 不变，所以不是简单相反）。正确的关系是：
 *    对同一坐标，`ccw` 的结果等于 `cw` 三次的结果。
 *
 *    **Y 坐标在任何旋转下都不变**（水平旋转）。
 *
 * 2. **`containing` 用 `Mth.floor`（向负无穷取整），负数会往更小的方向走。**
 *
 *    ```java
 *    return new BlockPos(Mth.floor(x), Mth.floor(y), Mth.floor(z));
 *    ```
 *
 *    - `containing(1.9, 2.1, 3.5)` → `(1, 2, 3)`
 *    - **`containing(-0.5, -1.5, -0.1)` → `(-1, -2, -1)`**
 *
 *    **`(int)` 强转（向零截断）会得 `(0, -1, 0)`，与原版不符。** 这是负坐标放置方块时最常见的
 *    1 格偏差来源。
 *
 * 3. **`multiply` 对 `1` 与 `0` 有短路，但结果与直接相乘相同。**
 *
 *    ```java
 *    if (factor == 1) return this;
 *    return factor == 0 ? ZERO : new BlockPos(x * factor, ...);
 *    ```
 *
 *    短路只影响**对象身份**（是否返回同一实例），不影响**坐标值**。`multiply(0)` 返回
 *    `BlockPos.ZERO`（共享的常量），坐标为 `(0,0,0)`。
 *
 *    **但对负数因子没有保护**：`multiply(-1)` 正常计算 `(-x, -y, -z)`。
 *
 * 4. **`cross` 是右手系叉积，但参数类型是 `Vec3i` 而非 `BlockPos`。**
 *
 *    ```java
 *    cross(v) = (y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x)
 *    ```
 *
 *    与自身叉积恒为 `(0,0,0)`；与单位向量叉积可用来求法线。**`int` 乘法可能溢出**，原版不做
 *    保护（与 `Vec3i` 的其它运算一致）。
 *
 * 单元格式（坐标用 `x,y,z` 三元组，逗号分隔）：
 * - `rotate|<旋转>|<x>,<y>,<z>` —— `BlockPos.rotate`
 * - `cross|<x1>,<y1>,<z1>|<x2>,<y2>,<z2>` —— 叉积
 * - `containing|<x>,<y>,<z>` —— 浮点转方块坐标（输入可以是小数）
 * - `min|<x1>,<y1>,<z1>|<x2>,<y2>,<z2>` / `max|...`
 * - `multiply|<x>,<y>,<z>|<因子>`
 * - `subtract|<x1>,<y1>,<z1>|<x2>,<y2>,<z2>`
 *
 * TODO: 未覆盖 `offset` 系列（`blockpos_codec` 已覆盖 `offset`）、`asLong` / `getFlatIndex`
 * （`blockpos_codec` 已覆盖 `pack`）、`getCenter` 等返回 `Vec3` 的方法（涉及浮点表示）。
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

/// 一个方块坐标。
struct Pos {
    i32 x;
    i32 y;
    i32 z;
};

/// 解析 `x,y,z` 三元组。
Pos parsePos(const std::string& text)
{
    std::vector<i64> values;
    std::string current;
    for (char ch : text) {
        if (ch == ',') {
            values.push_back(std::stoll(current));
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    values.push_back(std::stoll(current));
    if (values.size() != 3) {
        throw std::runtime_error("blockpos_geom: 坐标必须三段：" + text);
    }
    return Pos{static_cast<i32>(values[0]), static_cast<i32>(values[1]), static_cast<i32>(values[2])};
}

/// 序列化为 `x,y,z`。
std::string dumpPos(const Pos& pos)
{
    return std::to_string(pos.x) + "," + std::to_string(pos.y) + "," + std::to_string(pos.z);
}

/// 原版 `Mth.floor(double)`：向负无穷取整。
i32 mthFloor(double value)
{
    const i32 truncated = static_cast<i32>(value);
    return (value < static_cast<double>(truncated)) ? truncated - 1 : truncated;
}

/// 原版 `BlockPos.rotate(Rotation)`。
Pos rotate(const std::string& rotation, const Pos& pos)
{
    if (rotation == "CLOCKWISE_90") {
        return Pos{-pos.z, pos.y, pos.x};
    }
    if (rotation == "CLOCKWISE_180") {
        return Pos{-pos.x, pos.y, -pos.z};
    }
    if (rotation == "COUNTERCLOCKWISE_90") {
        return Pos{pos.z, pos.y, -pos.x};
    }
    if (rotation == "NONE") {
        return pos;
    }
    throw std::runtime_error("blockpos_geom: 未知旋转：" + rotation);
}

/// 原版 `BlockPos.cross(Vec3i)`。
Pos cross(const Pos& a, const Pos& b)
{
    return Pos{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

/// 原版 `BlockPos.min`。
Pos minPos(const Pos& a, const Pos& b)
{
    return Pos{std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)};
}

/// 原版 `BlockPos.max`。
Pos maxPos(const Pos& a, const Pos& b)
{
    return Pos{std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)};
}

/// 原版 `BlockPos.multiply(int)`。
Pos multiply(const Pos& pos, i32 factor)
{
    if (factor == 0) {
        return Pos{0, 0, 0};
    }
    return Pos{pos.x * factor, pos.y * factor, pos.z * factor};
}

/// 原版 `BlockPos.subtract(Vec3i)`。
Pos subtract(const Pos& a, const Pos& b)
{
    return Pos{a.x - b.x, a.y - b.y, a.z - b.z};
}

/// 解析浮点三元组（用于 `containing`）。
std::vector<double> parseDoubleTriple(const std::string& text)
{
    std::vector<double> values;
    std::string current;
    for (char ch : text) {
        if (ch == ',') {
            values.push_back(std::stod(current));
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    values.push_back(std::stod(current));
    if (values.size() != 3) {
        throw std::runtime_error("blockpos_geom: 坐标必须三段：" + text);
    }
    return values;
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // rotate：全部 4 种旋转 × 4 个象限点
        "rotate|NONE|5,3,7",
        "rotate|CLOCKWISE_90|5,3,7",
        "rotate|CLOCKWISE_180|5,3,7",
        "rotate|COUNTERCLOCKWISE_90|5,3,7",
        "rotate|CLOCKWISE_90|-5,3,7",
        "rotate|CLOCKWISE_90|5,3,-7",
        "rotate|CLOCKWISE_90|0,0,0",
        "rotate|COUNTERCLOCKWISE_90|1,2,3",
        // rotate：Y 不变
        "rotate|CLOCKWISE_90|0,99,0",
        "rotate|CLOCKWISE_180|0,-99,0",
        // cross
        "cross|1,0,0|0,1,0",
        "cross|1,0,0|0,0,1",
        "cross|0,1,0|0,0,1",
        "cross|1,2,3|1,2,3",
        "cross|2,3,4|5,6,7",
        "cross|0,0,0|1,2,3",
        // containing：正数与负数
        "containing|1.9,2.1,3.5",
        "containing|-0.5,-1.5,-0.1",
        "containing|0.0,0.0,0.0",
        "containing|-0.0,-0.0,-0.0",
        "containing|5.999,6.001,-3.5",
        "containing|-1.0,-2.0,-3.0",
        // min / max
        "min|5,3,7|2,8,4",
        "max|5,3,7|2,8,4",
        "min|1,1,1|1,1,1",
        "max|-5,-3,-7|-2,-8,-4",
        // multiply
        "multiply|5,3,7|0",
        "multiply|5,3,7|1",
        "multiply|5,3,7|2",
        "multiply|5,3,7|-1",
        "multiply|-5,3,-7|-3",
        // subtract
        "subtract|5,3,7|2,8,4",
        "subtract|5,3,7|5,3,7",
        "subtract|0,0,0|1,2,3",
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
    if (parts.size() < 2) {
        throw std::runtime_error("blockpos_geom: 单元至少两段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "rotate" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", dumpPos(rotate(parts[1], parsePos(parts[2])))}}.dump();
    }
    if (mode == "cross" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", dumpPos(cross(parsePos(parts[1]), parsePos(parts[2])))}}.dump();
    }
    if (mode == "containing" && parts.size() == 2) {
        const auto values = parseDoubleTriple(parts[1]);
        const Pos pos{mthFloor(values[0]), mthFloor(values[1]), mthFloor(values[2])};
        return json{{"unit", unit}, {"value", dumpPos(pos)}}.dump();
    }
    if (mode == "min" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", dumpPos(minPos(parsePos(parts[1]), parsePos(parts[2])))}}.dump();
    }
    if (mode == "max" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", dumpPos(maxPos(parsePos(parts[1]), parsePos(parts[2])))}}.dump();
    }
    if (mode == "multiply" && parts.size() == 3) {
        const i32 factor = static_cast<i32>(std::stoll(parts[2]));
        return json{{"unit", unit}, {"value", dumpPos(multiply(parsePos(parts[1]), factor))}}.dump();
    }
    if (mode == "subtract" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", dumpPos(subtract(parsePos(parts[1]), parsePos(parts[2])))}}.dump();
    }

    throw std::runtime_error("blockpos_geom: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(blockpos_geom, &writeList, &runLine)

} // namespace mc::diff
