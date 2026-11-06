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
 * @brief 差分能力：`AABB` 的几何运算（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "result": [minX, minY, minZ, maxX, maxY, maxZ] } 或 { "hit": true }
 *
 * 复刻原版 `AABB` 的五个方法：
 *
 * ```java
 * public AABB inflate(double dx, double dy, double dz) {
 *     return new AABB(minX - dx, minY - dy, minZ - dz, maxX + dx, maxY + dy, maxZ + dz);
 * }
 *
 * public AABB expandTowards(double dx, double dy, double dz) {
 *     if (dx < 0.0) minX += dx; else if (dx > 0.0) maxX += dx;
 *     if (dy < 0.0) minY += dy; else if (dy > 0.0) maxY += dy;
 *     if (dz < 0.0) minZ += dz; else if (dz > 0.0) maxZ += dz;
 *     return new AABB(minX, minY, minZ, maxX, maxY, maxZ);
 * }
 *
 * public AABB intersect(AABB other) {
 *     return new AABB(max(minX, other.minX), ..., min(maxZ, other.maxZ));
 * }
 *
 * public AABB minmax(AABB other) {
 *     return new AABB(min(minX, other.minX), ..., max(maxZ, other.maxZ));
 * }
 *
 * public boolean intersects(AABB other) {
 *     return minX < other.maxX && maxX > other.minX
 *         && minY < other.maxY && maxY > other.minY
 *         && minZ < other.maxZ && maxZ > other.minZ;
 * }
 * ```
 *
 * ## 三个关键点
 *
 * 1. **`intersects` 用严格不等号**，所以「恰好接触」（一个的 `maxX` 等于另一个的 `minX`）
 *    **不算相交**。用 `<=`/`>=` 会让刷怪、碰撞、漏斗判定全部放宽一格。
 *
 * 2. **`expandTowards` 与 `inflate` 语义相反**。`inflate(1)` 让盒子**四边**各扩 1；
 *    `expandTowards(1, 0, 0)` 只把 `maxX` 加 1，`minX` 不动。负增量时反过来动 `min`。
 *    增量为 0 时**两边都不动**（不是扩 0 的意思——扩 0 结果相同，但分支不同）。
 *
 * 3. **`intersect` 与 `minmax` 是互补的两个运算**：前者取交集（`max` 下限、`min` 上限），
 *    后者取并集（`min` 下限、`max` 上限）。名字容易混——`minmax` 是并集。
 *
 * ## 浮点格式化
 *
 * 所有坐标按 `%.6g` 格式化后去掉尾随零，与其它浮点能力的契约一致。
 *
 * 单元格式：
 * - `<box>|inflate|<dx>|<dy>|<dz>`
 * - `<box>|expand|<dx>|<dy>|<dz>`
 * - `<box>|<box2>|intersect` / `<box>|<box2>|minmax` / `<box>|<box2>|intersects`
 * - `<box>|move|<dx>|<dy>|<dz>`
 *
 * 其中 `<box>` 是 `minX,minY,minZ,maxX,maxY,maxZ`。
 *
 * TODO: Cubium 目前没有 `AABB` 类（只有 `AABBShapeComponent` 数据组件，无几何运算），
 * 本能力提供的是待实现的行为基准。未覆盖 `getCenter`、`getSize`、`clip`（射线求交，
 * 那部分已有 `explosion_ray` 覆盖一个侧面）。
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
#include "common/util/AxisAlignedBB.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using mc::AxisAlignedBB;
using nlohmann::json;

struct Box {
    f64 minX = 0.0;
    f64 minY = 0.0;
    f64 minZ = 0.0;
    f64 maxX = 0.0;
    f64 maxY = 0.0;
    f64 maxZ = 0.0;
};

/**
 * @brief 复刻原版 `AABB(double, double, double, double, double, double)` 构造器。
 *
 * **构造器会归一化**：`minX = Math.min(x1, x2)`、`maxX = Math.max(x1, x2)`，三轴同理。
 * 因此任何经过 `new AABB(...)` 的运算结果都被归一化——包括 `intersect` 在盒子完全分离时
 * 产生的「min > max」结果，构造后会被翻转成 `min < max`。
 */
Box makeBox(f64 x1, f64 y1, f64 z1, f64 x2, f64 y2, f64 z2)
{
    return Box{
        std::min(x1, x2), std::min(y1, y2), std::min(z1, z2), std::max(x1, x2), std::max(y1, y2), std::max(z1, z2)};
}

std::string formatNumber(f64 value)
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.6g", value);
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

json boxToJson(const AxisAlignedBB& box)
{
    return json::array({formatNumber(box.minX),
        formatNumber(box.minY),
        formatNumber(box.minZ),
        formatNumber(box.maxX),
        formatNumber(box.maxY),
        formatNumber(box.maxZ)});
}

constexpr const char* UNITS[] = {
    // inflate：四边各扩
    "0,0,0,1,1,1|inflate|1|1|1",
    "0,0,0,1,1,1|inflate|0.5|0|0",
    // inflate：负增量收缩
    "0,0,0,1,1,1|inflate|-0.25|-0.25|-0.25",
    // expandTowards：正增量只动 max
    "0,0,0,1,1,1|expand|1|0|0",
    // expandTowards：负增量只动 min
    "0,0,0,1,1,1|expand|-1|0|0",
    // expandTowards：零增量不动
    "0,0,0,1,1,1|expand|0|0|0",
    // expandTowards：三轴混合正负
    "0,0,0,1,1,1|expand|2|-3|0.5",
    // move
    "0,0,0,1,1,1|move|5|5|5",
    "0,0,0,1,1,1|move|-0.5|2.25|0",
    // intersect：部分重叠
    "0,0,0,2,2,2|0.5,0.5,0.5,3,3,3|intersect",
    // intersect：完全分离（结果 min > max）
    "0,0,0,1,1,1|5,5,5,6,6,6|intersect",
    // minmax：并集
    "0,0,0,2,2,2|0.5,0.5,0.5,3,3,3|minmax",
    "0,0,0,1,1,1|5,5,5,6,6,6|minmax",
    // intersects：部分重叠 -> true
    "0,0,0,2,2,2|1,1,1,3,3,3|intersects",
    // intersects：恰好接触 -> false（严格不等号）
    "0,0,0,1,1,1|1,1,1,2,2,2|intersects",
    // intersects：完全分离 -> false
    "0,0,0,1,1,1|5,5,5,6,6,6|intersects",
    // intersects：包含关系 -> true
    "0,0,0,10,10,10|2,2,2,3,3,3|intersects",
    // intersects：仅一轴接触，其余重叠 -> false
    "0,0,0,1,1,1|1,0.2,0.2,2,0.8,0.8|intersects",
    // intersects：一轴分离，其余重叠 -> false
    "0,0,0,1,1,1|0.2,0.2,5,0.8,0.8,6|intersects",
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

std::vector<std::string> splitComma(const std::string& text)
{
    std::vector<std::string> tokens;
    std::string current;
    for (char ch : text) {
        if (ch == ',') {
            tokens.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    tokens.push_back(current);
    return tokens;
}

AxisAlignedBB parseBox(const std::string& text)
{
    const auto parts = splitComma(text);
    if (parts.size() != 6) {
        throw std::runtime_error("aabb_ops: 盒子应为 6 个分量：" + text);
    }
    return AxisAlignedBB(static_cast<f32>(std::stod(parts[0])),
        static_cast<f32>(std::stod(parts[1])),
        static_cast<f32>(std::stod(parts[2])),
        static_cast<f32>(std::stod(parts[3])),
        static_cast<f32>(std::stod(parts[4])),
        static_cast<f32>(std::stod(parts[5])));
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("aabb_ops: 单元至少需要「盒子|模式」：" + unit);
    }

    const AxisAlignedBB box = parseBox(parts[0]);

    // 双盒子形式：`<box>|<box2>|<op>`，模式在最后一段。
    if (parts.size() == 3) {
        const AxisAlignedBB other = parseBox(parts[1]);
        if (parts[2] == "intersect") {
            return json{{"unit", unit}, {"result", boxToJson(box.intersect(other))}}.dump();
        }
        if (parts[2] == "minmax") {
            return json{{"unit", unit}, {"result", boxToJson(box.minmax(other))}}.dump();
        }
        if (parts[2] == "intersects") {
            return json{{"unit", unit}, {"hit", box.intersects(other)}}.dump();
        }
    }
    if (parts.size() == 5) {
        const f64 dx = std::stod(parts[2]);
        const f64 dy = std::stod(parts[3]);
        const f64 dz = std::stod(parts[4]);
        if (parts[1] == "inflate") {
            return json{{"unit", unit},
                {"result", boxToJson(box.inflate(static_cast<f32>(dx), static_cast<f32>(dy), static_cast<f32>(dz)))}}
                .dump();
        }
        if (parts[1] == "expand") {
            return json{{"unit", unit},
                {"result",
                    boxToJson(box.expandTowards(static_cast<f32>(dx), static_cast<f32>(dy), static_cast<f32>(dz)))}}
                .dump();
        }
        if (parts[1] == "move") {
            return json{{"unit", unit},
                {"result", boxToJson(box.offsetted(static_cast<f32>(dx), static_cast<f32>(dy), static_cast<f32>(dz)))}}
                .dump();
        }
    }

    throw std::runtime_error("aabb_ops: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(aabb_ops, &writeList, &runLine)

} // namespace mc::diff
