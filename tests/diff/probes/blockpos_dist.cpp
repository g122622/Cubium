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
 * @brief 差分能力：`Vec3i` / `BlockPos` 的距离与包围盒迭代（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... } 或 { "order": [...] }
 *
 * 复刻原版 `Vec3i` 与 `BlockPos`：
 *
 * ```java
 * // Vec3i
 * public int distManhattan(Vec3i o) {
 *     float f  = Math.abs(o.getX() - this.getX());
 *     float f1 = Math.abs(o.getY() - this.getY());
 *     float f2 = Math.abs(o.getZ() - this.getZ());
 *     return (int)(f + f1 + f2);          // 注意：float 累加后转 int
 * }
 *
 * public double distToLowCornerSqr(double x, double y, double z) {
 *     double d0 = this.getX() - x;
 *     double d1 = this.getY() - y;
 *     double d2 = this.getZ() - z;
 *     return d0 * d0 + d1 * d1 + d2 * d2;
 * }
 *
 * // BlockPos.betweenClosed(int, int, int, int, int, int)
 * int i = maxX - minX + 1;   // X 方向长度
 * int j = maxY - minY + 1;   // Y 方向长度
 * int k = maxZ - minZ + 1;   // Z 方向长度
 * // computeNext:
 *   int i1 = index % i;          // X 偏移（最内层）
 *   int j1 = index / i;
 *   int k1 = j1 % j;             // Y 偏移（中层）
 *   int l1 = j1 / j;             // Z 偏移（最外层）
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`distManhattan` 用 `float` 做累加**，最后才 `(int)` 转换。坐标差的绝对值本身不超过
 *    `int` 范围，但 **`float` 只有 24 位有效位**——差值之和超过 `2^24`（约 1680 万）时，
 *    低位会丢失，结果比真实值偏大或偏小。用 `int` 直接累加会与它不同。
 *
 * 2. **`betweenClosed` 的迭代顺序是 X 最内层、Z 最外层**（列优先）。写成 Z 最内层会让所有
 *    依赖遍历顺序的逻辑（方块更新的传播次序、随机数的消耗次序）与原版错位。
 *
 * 3. **`betweenClosed` 的两个角点会自动排序**——它内部先取 `Math.min` / `Math.max`，所以
 *    「从高到低」传参也能得到同样的盒子，但**迭代顺序仍是从 min 开始**。
 *
 * 4. **`distToLowCornerSqr` 返回 `double`**，而 `distManhattan` 返回 `int`。前者是精确的
 *    双精度平方和，后者有 `float` 精度损失——同名系列的两个函数精度语义完全不同。
 *
 * 单元格式：
 * - `manhattan|<x1>|<y1>|<z1>|<x2>|<y2>|<z2>`
 * - `sqr|<x1>|<y1>|<z1>|<x2>|<y2>|<z2>`
 * - `box|<x1>|<y1>|<z1>|<x2>|<y2>|<z2>` —— 输出前 N 个坐标（N 由盒体积决定，最多 8）
 * - `boxsize|<x1>|<y1>|<z1>|<x2>|<y2>|<z2>` —— 只输出体积
 *
 * TODO: 未覆盖 `spiralAround`（螺旋迭代，逻辑复杂需单独能力）、`randomBetweenClosed`
 * （依赖随机源）、`betweenClosed(AABB)`（依赖 `Mth.floor`）。
 */
#include <algorithm>
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

std::string formatNumber(f64 value)
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.9g", value);
    std::string text(buffer);

    // 指数形式也要裁掉尾零，否则 `1e+14` 与 `1.00000000e+14` 会被当成不同。
    const size_t expPos = text.find_first_of("eE");
    std::string mantissa = expPos == std::string::npos ? text : text.substr(0, expPos);
    const std::string suffix = expPos == std::string::npos ? std::string() : text.substr(expPos);
    if (mantissa.find('.') != std::string::npos) {
        while (!mantissa.empty() && mantissa.back() == '0') {
            mantissa.pop_back();
        }
        if (!mantissa.empty() && mantissa.back() == '.') {
            mantissa.pop_back();
        }
    }
    return mantissa + suffix;
}

/// 原版 `Vec3i.distManhattan`：`float` 累加后转 `int`。
i32 distManhattan(i32 x1, i32 y1, i32 z1, i32 x2, i32 y2, i32 z2)
{
    const f32 dx = std::fabs(static_cast<f32>(x2 - x1));
    const f32 dy = std::fabs(static_cast<f32>(y2 - y1));
    const f32 dz = std::fabs(static_cast<f32>(z2 - z1));
    return static_cast<i32>(dx + dy + dz);
}

/// 原版 `Vec3i.distToLowCornerSqr`：`double` 平方和。
f64 distToLowCornerSqr(i32 x1, i32 y1, i32 z1, i32 x2, i32 y2, i32 z2)
{
    const f64 dx = static_cast<f64>(x1) - static_cast<f64>(x2);
    const f64 dy = static_cast<f64>(y1) - static_cast<f64>(y2);
    const f64 dz = static_cast<f64>(z1) - static_cast<f64>(z2);
    return dx * dx + dy * dy + dz * dz;
}

/**
 * @brief 复刻 `betweenClosed` 的迭代：X 最内层、Z 最外层。
 *
 * 前 `limit` 个坐标按原版顺序输出。
 */
std::vector<std::vector<i32>> iterateBox(i32 x1, i32 y1, i32 z1, i32 x2, i32 y2, i32 z2, i32 limit)
{
    const i32 minX = std::min(x1, x2);
    const i32 minY = std::min(y1, y2);
    const i32 minZ = std::min(z1, z2);
    const i32 maxX = std::max(x1, x2);
    const i32 maxY = std::max(y1, y2);
    const i32 maxZ = std::max(z1, z2);

    const i32 sizeX = maxX - minX + 1;
    const i32 sizeY = maxY - minY + 1;
    const i32 sizeZ = maxZ - minZ + 1;

    std::vector<std::vector<i32>> result;
    const i64 total = static_cast<i64>(sizeX) * sizeY * sizeZ;
    const i64 count = std::min<i64>(total, limit);
    for (i64 index = 0; index < count; ++index) {
        const i32 i1 = static_cast<i32>(index % sizeX);
        const i32 j1 = static_cast<i32>(index / sizeX);
        const i32 k1 = j1 % sizeY;
        const i32 l1 = j1 / sizeY;
        result.push_back({minX + i1, minY + k1, minZ + l1});
    }
    return result;
}

/// 盒体积。
i64 boxVolume(i32 x1, i32 y1, i32 z1, i32 x2, i32 y2, i32 z2)
{
    const i64 sizeX = std::abs(static_cast<i64>(x2) - x1) + 1;
    const i64 sizeY = std::abs(static_cast<i64>(y2) - y1) + 1;
    const i64 sizeZ = std::abs(static_cast<i64>(z2) - z1) + 1;
    return sizeX * sizeY * sizeZ;
}

/// 单元表。
constexpr const char* UNITS[] = {
    // distManhattan：常规
    "manhattan|0|0|0|1|2|3",
    "manhattan|0|0|0|0|0|0",
    "manhattan|10|20|30|10|20|30",
    "manhattan|-5|-5|-5|5|5|5",
    // distManhattan：单轴
    "manhattan|0|0|0|100|0|0",
    "manhattan|0|0|0|0|100|0",
    "manhattan|0|0|0|0|0|100",
    // distManhattan：大坐标（float 精度损失区）
    "manhattan|0|0|0|10000000|0|0",
    "manhattan|0|0|0|16777216|0|0",
    "manhattan|0|0|0|20000000|0|0",
    "manhattan|-10000000|0|0|10000000|0|0",
    // distToLowCornerSqr：常规
    "sqr|0|0|0|3|4|0",
    "sqr|0|0|0|0|0|0",
    "sqr|1|2|3|4|6|3",
    "sqr|-5|-5|-5|5|5|5",
    // distToLowCornerSqr：大坐标（精确，无 float 损失）
    "sqr|0|0|0|10000000|0|0",
    // 包围盒迭代：3x2x1
    "box|0|0|0|2|1|0",
    // 包围盒迭代：1x1x3（只有 Z 变化）
    "box|0|0|0|0|0|2",
    // 包围盒迭代：3x1x1（只有 X 变化）
    "box|0|0|0|2|0|0",
    // 包围盒迭代：2x2x2
    "box|0|0|0|1|1|1",
    // 包围盒迭代：角点反序传入（应自动归一）
    "box|2|1|0|0|0|0",
    // 包围盒迭代：负坐标
    "box|-1|-1|-1|0|0|0",
    // 体积
    "boxsize|0|0|0|2|1|0",
    "boxsize|0|0|0|0|0|0",
    "boxsize|0|0|0|1|1|1",
    "boxsize|-5|-5|-5|5|5|5",
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
    if (parts.size() != 7) {
        throw std::runtime_error("blockpos_dist: 单元应为 7 段 '模式|x1|y1|z1|x2|y2|z2'：" + unit);
    }

    const std::string mode = parts[0];
    const i32 x1 = std::stoi(parts[1]);
    const i32 y1 = std::stoi(parts[2]);
    const i32 z1 = std::stoi(parts[3]);
    const i32 x2 = std::stoi(parts[4]);
    const i32 y2 = std::stoi(parts[5]);
    const i32 z2 = std::stoi(parts[6]);

    if (mode == "manhattan") {
        return json{{"unit", unit}, {"value", distManhattan(x1, y1, z1, x2, y2, z2)}}.dump();
    }
    if (mode == "sqr") {
        return json{{"unit", unit}, {"value", formatNumber(distToLowCornerSqr(x1, y1, z1, x2, y2, z2))}}.dump();
    }
    if (mode == "boxsize") {
        return json{{"unit", unit}, {"volume", std::to_string(boxVolume(x1, y1, z1, x2, y2, z2))}}.dump();
    }
    if (mode == "box") {
        // 输出前 8 个（或全部，若不足 8 个），足够暴露迭代顺序。
        const auto coords = iterateBox(x1, y1, z1, x2, y2, z2, 8);
        json order = json::array();
        for (const auto& c : coords) {
            order.push_back(json::array({c[0], c[1], c[2]}));
        }
        return json{{"unit", unit}, {"order", order}, {"volume", std::to_string(boxVolume(x1, y1, z1, x2, y2, z2))}}
            .dump();
    }

    throw std::runtime_error("blockpos_dist: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(blockpos_dist, &writeList, &runLine)

} // namespace mc::diff
