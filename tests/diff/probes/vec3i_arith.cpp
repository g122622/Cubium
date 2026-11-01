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
 * @brief 差分能力：`Vec3i` 的整数算术、排序与距离（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.core.Vec3i`（`BlockPos` 的父类）：
 *
 * ```java
 * public int compareTo(Vec3i other) {
 *     if (this.getY() == other.getY()) {
 *         return this.getZ() == other.getZ()
 *             ? this.getX() - other.getX()
 *             : this.getZ() - other.getZ();
 *     }
 *     return this.getY() - other.getY();
 * }
 *
 * public int distManhattan(Vec3i other) {
 *     float f  = Math.abs(other.getX() - this.getX());
 *     float f1 = Math.abs(other.getY() - this.getY());
 *     float f2 = Math.abs(other.getZ() - this.getZ());
 *     return (int) (f + f1 + f2);          // 注意：float 中间量
 * }
 *
 * public int distChessboard(Vec3i other) {
 *     int i = Math.abs(this.getX() - other.getX());
 *     int j = Math.abs(this.getY() - other.getY());
 *     int k = Math.abs(this.getZ() - other.getZ());
 *     return Math.max(i, Math.max(j, k));  // 纯整数
 * }
 *
 * public Vec3i relative(Direction dir, int steps) {
 *     return steps == 0 ? this
 *         : new Vec3i(x + dir.getStepX() * steps,
 *                     y + dir.getStepY() * steps,
 *                     z + dir.getStepZ() * steps);
 * }
 *
 * public Vec3i multiply(int f) { return f == 1 ? this : new Vec3i(x*f, y*f, z*f); }
 * public Vec3i multiply(int fx, int fy, int fz) { ... 逐分量 ... }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`compareTo` 的优先级是 Y → Z → X，不是常见的 X → Y → Z。**
 *
 *    ```java
 *    if (Y 相同) return (Z 相同) ? X 之差 : Z 之差;
 *    else        return Y 之差;
 *    ```
 *
 *    **Y 是第一关键字，X 反而最后。** 这决定了方块坐标集合的排序结果，进而影响结构生成、
 *    方块实体更新、随机刻遍历的**迭代顺序**。若 Cubium 用 `(x,y,z)` 字典序，所有依赖排序的
 *    迭代都会与原版不同。
 *
 * 2. **`distManhattan` 内部用 `float`，大数会丢精度。**
 *
 *    ```java
 *    float f = Math.abs(other.getX() - this.getX());   // int → float
 *    ...
 *    return (int) (f + f1 + f2);
 *    ```
 *
 *    `float` 只有 24 位尾数，大于 `2^24 = 16777216` 的整数无法精确表示。三个分量相加后若超过
 *    该阈值，结果会**偏离纯整数计算**。
 *
 *    `distChessboard` 则**全程用 `int`**，没有这个问题。两者实现风格不同。
 *
 * 3. **`Vec3i.distSqr` 返回 `double`，不是 `int`。**
 *
 *    本能力**不覆盖**它（涉及浮点表示，留待 `vec3i_dist` 或类似能力）。注意这与
 *    [`blockpos_dist`](./blockpos_dist.md) 中可能的整数重载不同，调用时要看清返回类型。
 *
 * 4. **`relative` / `multiply` 对 `0` 与 `1` 有短路，但结果与计算一致。**
 *
 *    ```java
 *    relative(dir, 0)  -> this      // 返回同一实例
 *    multiply(1)       -> this      // 返回同一实例
 *    ```
 *
 *    短路只影响**对象身份**，不影响坐标值。
 *
 * 单元格式（坐标用 `x,y,z` 逗号分隔）：
 * - `compare|<x1>,<y1>,<z1>|<x2>,<y2>,<z2>` —— `compareTo` 结果（负/零/正）
 * - `manhattan|<x1>,<y1>,<z1>|<x2>,<y2>,<z2>` —— 曼哈顿距离
 * - `chessboard|<x1>,<y1>,<z1>|<x2>,<y2>,<z2>` —— 棋盘距离
 * - `relative|<x>,<y>,<z>|<方向>|<步数>`
 * - `multiply|<x>,<y>,<z>|<因子>` / `multiply3|<x>,<y>,<z>|<fx>,<fy>,<fz>`
 * - `offset|<x>,<y>,<z>|<dx>,<dy>,<dz>`
 * - `subtract|<x1>,<y1>,<z1>|<x2>,<y2>,<z2>`
 *
 * TODO: 未覆盖 `distSqr` / `distToCenterSqr` / `distToLowCornerSqr`（返回 `double`）、
 * `closerThan`、`closestToCenter`、`relative(Axis, int)`、`offsetCodec`。
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

/// 一个整数向量。
struct Vec {
    i32 x;
    i32 y;
    i32 z;
};

/// 解析 `x,y,z`。
Vec parseVec(const std::string& text)
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
        throw std::runtime_error("vec3i_arith: 坐标必须三段：" + text);
    }
    return Vec{static_cast<i32>(values[0]), static_cast<i32>(values[1]), static_cast<i32>(values[2])};
}

std::string dumpVec(const Vec& v)
{
    return std::to_string(v.x) + "," + std::to_string(v.y) + "," + std::to_string(v.z);
}

/// 原版 `Vec3i.compareTo`：优先级 Y → Z → X。
i32 compareTo(const Vec& a, const Vec& b)
{
    if (a.y == b.y) {
        if (a.z == b.z) {
            return a.x - b.x;
        }
        return a.z - b.z;
    }
    return a.y - b.y;
}

/// 原版 `Vec3i.distManhattan`：**内部用 float 累加**。
i32 distManhattan(const Vec& a, const Vec& b)
{
    const float fx = std::abs(static_cast<float>(b.x) - static_cast<float>(a.x));
    const float fy = std::abs(static_cast<float>(b.y) - static_cast<float>(a.y));
    const float fz = std::abs(static_cast<float>(b.z) - static_cast<float>(a.z));
    return static_cast<i32>(fx + fy + fz);
}

/// 原版 `Vec3i.distChessboard`：纯整数。
i32 distChessboard(const Vec& a, const Vec& b)
{
    const i32 dx = std::abs(a.x - b.x);
    const i32 dy = std::abs(a.y - b.y);
    const i32 dz = std::abs(a.z - b.z);
    return std::max(dx, std::max(dy, dz));
}

/// 方向的步进。
struct Step {
    const char* name;
    i32 dx;
    i32 dy;
    i32 dz;
};

constexpr Step STEPS[] = {
    {"DOWN", 0, -1, 0},
    {"UP", 0, 1, 0},
    {"NORTH", 0, 0, -1},
    {"SOUTH", 0, 0, 1},
    {"WEST", -1, 0, 0},
    {"EAST", 1, 0, 0},
};

/// 原版 `Vec3i.relative(Direction, int)`。
Vec relative(const Vec& pos, const std::string& dir, i32 steps)
{
    if (steps == 0) {
        return pos;
    }
    for (const auto& step : STEPS) {
        if (dir == step.name) {
            return Vec{pos.x + step.dx * steps, pos.y + step.dy * steps, pos.z + step.dz * steps};
        }
    }
    throw std::runtime_error("vec3i_arith: 未知方向：" + dir);
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // compare：优先级 Y → Z → X
        "compare|0,0,0|0,0,1",
        "compare|0,0,1|0,0,0",
        "compare|1,0,0|0,0,0",
        "compare|0,0,0|1,0,0",
        "compare|5,0,0|0,1,0",
        "compare|0,1,0|5,0,0",
        "compare|0,0,5|0,1,0",
        "compare|1,2,3|1,2,3",
        "compare|-1,-1,-1|0,0,0",
        "compare|0,0,0|-1,-1,-1",
        // manhattan：小数与大数
        "manhattan|0,0,0|1,2,3",
        "manhattan|0,0,0|-1,-2,-3",
        "manhattan|5,3,7|2,8,4",
        "manhattan|1,2,3|1,2,3",
        // manhattan：float 精度边界（分量和 > 2^24）
        "manhattan|0,0,0|8388608,0,0",
        "manhattan|0,0,0|9000000,9000000,9000000",
        "manhattan|0,0,0|16777216,0,0",
        "manhattan|0,0,0|16777217,0,0",
        // chessboard：纯整数
        "chessboard|0,0,0|1,2,3",
        "chessboard|0,0,0|-1,-2,-3",
        "chessboard|5,3,7|2,8,4",
        "chessboard|0,0,0|9000000,9000000,9000000",
        // relative
        "relative|0,0,0|UP|1",
        "relative|0,0,0|UP|0",
        "relative|0,0,0|UP|-1",
        "relative|5,3,7|NORTH|2",
        "relative|5,3,7|EAST|-3",
        "relative|0,0,0|DOWN|5",
        // multiply
        "multiply|5,3,7|0",
        "multiply|5,3,7|1",
        "multiply|5,3,7|2",
        "multiply|5,3,7|-1",
        "multiply3|5,3,7|2,-1,0",
        // offset / subtract
        "offset|5,3,7|1,2,3",
        "offset|5,3,7|-1,-2,-3",
        "subtract|5,3,7|2,8,4",
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
        throw std::runtime_error("vec3i_arith: 单元至少两段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "compare" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", compareTo(parseVec(parts[1]), parseVec(parts[2]))}}.dump();
    }
    if (mode == "manhattan" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", distManhattan(parseVec(parts[1]), parseVec(parts[2]))}}.dump();
    }
    if (mode == "chessboard" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", distChessboard(parseVec(parts[1]), parseVec(parts[2]))}}.dump();
    }
    if (mode == "relative" && parts.size() == 4) {
        const i32 steps = static_cast<i32>(std::stoll(parts[3]));
        return json{{"unit", unit}, {"value", dumpVec(relative(parseVec(parts[1]), parts[2], steps))}}.dump();
    }
    if (mode == "multiply" && parts.size() == 3) {
        const Vec v = parseVec(parts[1]);
        const i32 f = static_cast<i32>(std::stoll(parts[2]));
        return json{{"unit", unit}, {"value", dumpVec(Vec{v.x * f, v.y * f, v.z * f})}}.dump();
    }
    if (mode == "multiply3" && parts.size() == 3) {
        const Vec v = parseVec(parts[1]);
        const Vec f = parseVec(parts[2]);
        return json{{"unit", unit}, {"value", dumpVec(Vec{v.x * f.x, v.y * f.y, v.z * f.z})}}.dump();
    }
    if (mode == "offset" && parts.size() == 3) {
        const Vec v = parseVec(parts[1]);
        const Vec d = parseVec(parts[2]);
        return json{{"unit", unit}, {"value", dumpVec(Vec{v.x + d.x, v.y + d.y, v.z + d.z})}}.dump();
    }
    if (mode == "subtract" && parts.size() == 3) {
        const Vec a = parseVec(parts[1]);
        const Vec b = parseVec(parts[2]);
        return json{{"unit", unit}, {"value", dumpVec(Vec{a.x - b.x, a.y - b.y, a.z - b.z})}}.dump();
    }

    throw std::runtime_error("vec3i_arith: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE("vec3i_arith", &writeList, &runLine)

} // namespace mc::diff
