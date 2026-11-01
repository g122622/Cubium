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
 * @brief 差分能力：`AABB` 的工厂方法与基础实例方法（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.world.phys.AABB`：
 *
 * ```java
 * public static AABB encapsulatingFullBlocks(BlockPos a, BlockPos b) {
 *     return new AABB(
 *         Math.min(a.getX(), b.getX()), Math.min(a.getY(), b.getY()), Math.min(a.getZ(), b.getZ()),
 *         Math.max(a.getX(), b.getX()) + 1, Math.max(a.getY(), b.getY()) + 1,
 *         Math.max(a.getZ(), b.getZ()) + 1);
 * }
 *
 * public static AABB ofSize(Vec3 center, double dx, double dy, double dz) {
 *     return new AABB(
 *         center.x - dx / 2.0, center.y - dy / 2.0, center.z - dz / 2.0,
 *         center.x + dx / 2.0, center.y + dy / 2.0, center.z + dz / 2.0);
 * }
 *
 * public Vec3 getCenter() {
 *     return new Vec3(Mth.lerp(0.5, minX, maxX), Mth.lerp(0.5, minY, maxY),
 *                     Mth.lerp(0.5, minZ, maxZ));
 * }
 *
 * // 注意：原版 getSize() 返回单轴尺寸的算术平均，不是 Vec3！
 * public double getSize() { return (getXsize() + getYsize() + getZsize()) / 3.0; }
 *
 * public AABB inflate(double x, double y, double z) {
 *     return new AABB(minX - x, minY - y, minZ - z, maxX + x, maxY + y, maxZ + z);
 * }
 *
 * public AABB move(double x, double y, double z) {
 *     return new AABB(minX + x, minY + y, minZ + z, maxX + x, maxY + y, maxZ + z);
 * }
 *
 * public AABB intersect(AABB other) {
 *     return new AABB(Math.max(minX, other.minX), Math.max(minY, other.minY),
 *                     Math.max(minZ, other.minZ), Math.min(maxX, other.maxX),
 *                     Math.min(maxY, other.maxY), Math.min(maxZ, other.maxZ));
 * }
 *
 * public boolean hasNaN() { return Double.isNaN(minX) || ... ; }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`encapsulatingFullBlocks` 的 `+1` 只加在 `max` 一侧。**
 *
 *    ```java
 *    Math.min(a.x, b.x), ..., Math.max(a.x, b.x) + 1, ...
 *    ```
 *
 *    所以 `(0,0,0)..(0,0,0)`（单方块）得到 `(0,0,0)..(1,1,1)`——一个**边长 1 的完整方块**。
 *    这是「包含整格」语义：方块 `(0,0,0)` 占据 `[0,1)³`。
 *
 * 2. **`getCenter()` 用的是 `Mth.lerp(0.5, min, max)`，不是 `(min + max) / 2`。**
 *
 *    ```java
 *    Mth.lerp(delta, start, end) = start + delta * (end - start);
 *    ```
 *
 *    **两者在浮点上不等价**：`(a + b) / 2` 可能溢出或产生不同的舍入。
 *    `lerp(0.5, a, b) = a + 0.5 * (b - a)`——先做减法再缩放。
 *
 *    对 `a = 1e308, b = 1e308`，`(a+b)/2` 会溢出成 `Infinity`，而 `lerp` 得 `1e308`。
 *
 * 3. **`intersect` 不相交时，`AABB` 构造器会把两端交换，得到一个正尺寸盒。**
 *
 *    ```java
 *    // intersect 先算出 max(minX)=5, min(maxX)=1
 *    // 但构造器随即归一化：
 *    this.minX = Math.min(p_82295_, p_82298_);   // min(5, 1) = 1
 *    this.maxX = Math.max(p_82295_, p_82298_);   // max(5, 1) = 5
 *    ```
 *
 *    **构造器对 min/max 无条件归一化**——所以 `intersect` 两个不相交的盒子不会得到「负尺寸盒」，
 *    而是得到 `(1,1,1)..(5,5,5)` 这个**跨越两者的正尺寸盒**。
 *
 *    **原版不返回 `null` 也不抛异常**，调用方必须自行用 `intersects` 判断是否真的相交——
 *    否则会误以为它们有重叠区域。
 *
 * 4. **`ofSize` 用 `center ± size / 2.0`，`size` 可以为负（得到反向盒）。**
 *
 *    没有 `Math.abs`，负尺寸会翻转 min/max。
 *
 * 单元格式：
 * - `encap|<ax>|<ay>|<az>|<bx>|<by>|<bz>` —— `encapsulatingFullBlocks`
 * - `ofSize|<cx>|<cy>|<cz>|<dx>|<dy>|<dz>` —— `ofSize`（浮点输入用 `%.17g` 字符串）
 * - `center|<minX>|<minY>|<minZ>|<maxX>|<maxY>|<maxZ>` —— `getCenter`
 * - `size|<minX>|<minY>|<minZ>|<maxX>|<maxY>|<maxZ>` —— `getSize`（三轴尺寸的算术平均）
 * - `axisSizes|<minX>|...|<maxZ>` —— `getXsize` / `getYsize` / `getZsize`
 * - `inflate|<minX>|<minY>|<minZ>|<maxX>|<maxY>|<maxZ>|<dx>|<dy>|<dz>` —— `inflate`
 * - `move|<minX>|<minY>|<minZ>|<maxX>|<maxY>|<maxZ>|<dx>|<dy>|<dz>` —— `move`
 * - `intersect|<a6 段>|<b6 段>` —— 两个盒子求交
 * - `hasNaN|<minX>|...|<maxZ>` —— `hasNaN`（`nan` 表示 NaN）
 *
 * 输出浮点用 `%.9g` 格式化（float 契约），`nan` / `inf` 统一小写。
 */
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
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

/// 一个轴对齐包围盒。
struct Box {
    f64 minX = 0.0;
    f64 minY = 0.0;
    f64 minZ = 0.0;
    f64 maxX = 0.0;
    f64 maxY = 0.0;
    f64 maxZ = 0.0;
};

/// 与原版 `Mth.lerp` 一致：`start + delta * (end - start)`。
f64 lerp(f64 delta, f64 start, f64 end)
{
    return start + delta * (end - start);
}

/// 格式化浮点：`%.9g`，并把 `nan` / `inf` 统一成小写。
std::string formatNumber(f64 value)
{
    if (std::isnan(value)) {
        return "nan";
    }
    if (std::isinf(value)) {
        return value > 0 ? "inf" : "-inf";
    }
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.9g", value);
    return buffer;
}

/// 渲染盒子为 `minX,minY,minZ,maxX,maxY,maxZ`。
std::string render(const Box& box)
{
    return formatNumber(box.minX) + "," + formatNumber(box.minY) + "," + formatNumber(box.minZ) + "," +
        formatNumber(box.maxX) + "," + formatNumber(box.maxY) + "," + formatNumber(box.maxZ);
}

/// 渲染三维点为 `x,y,z`。
std::string renderPoint(f64 x, f64 y, f64 z)
{
    return formatNumber(x) + "," + formatNumber(y) + "," + formatNumber(z);
}

/// 解析浮点参数，支持 `nan` / `inf` 字面量。
f64 parseNumber(const std::string& text)
{
    if (text == "nan") {
        return std::nan("");
    }
    if (text == "inf") {
        return std::numeric_limits<f64>::infinity();
    }
    if (text == "-inf") {
        return -std::numeric_limits<f64>::infinity();
    }
    return std::stod(text);
}

Box parseBox(const std::vector<std::string>& parts, std::size_t offset)
{
    Box box;
    box.minX = parseNumber(parts[offset]);
    box.minY = parseNumber(parts[offset + 1]);
    box.minZ = parseNumber(parts[offset + 2]);
    box.maxX = parseNumber(parts[offset + 3]);
    box.maxY = parseNumber(parts[offset + 4]);
    box.maxZ = parseNumber(parts[offset + 5]);
    return box;
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // encapsulatingFullBlocks：单方块 +1
        "encap|0|0|0|0|0|0",
        "encap|0|0|0|2|3|1",
        "encap|2|3|1|0|0|0",
        "encap|-1|-2|-3|1|2|3",
        // ofSize
        "ofSize|0|0|0|1|1|1",
        "ofSize|10|64|-20|2|4|6",
        "ofSize|0|0|0|0|0|0",
        // center
        "center|0|0|0|1|1|1",
        "center|10|64|-20|12|68|-16",
        "center|-1|-1|-1|1|1|1",
        // size
        "size|0|0|0|1|1|1",
        "size|-1|-2|-3|1|2|3",
        "size|0|0|0|3|1|1",
        "axisSizes|0|0|0|1|1|1",
        "axisSizes|-1|-2|-3|1|2|3",
        // inflate
        "inflate|0|0|0|1|1|1|0.5|0.5|0.5",
        "inflate|0|0|0|1|1|1|-0.1|-0.1|-0.1",
        // move
        "move|0|0|0|1|1|1|10|64|-20",
        "move|0|0|0|1|1|1|-0.5|-0.5|-0.5",
        // intersect：相交
        "intersect|0|0|0|2|2|2|1|1|1|3|3|3",
        // intersect：不相交（产出负尺寸盒）
        "intersect|0|0|0|1|1|1|5|5|5|6|6|6",
        // intersect：包含
        "intersect|0|0|0|10|10|10|2|2|2|3|3|3",
        // hasNaN
        "hasNaN|0|0|0|1|1|1",
        "hasNaN|nan|0|0|1|1|1",
        "hasNaN|0|0|0|inf|1|1",
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
        throw std::runtime_error("aabb_factory: 单元至少两段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "encap" && parts.size() == 7) {
        const i64 ax = std::stoll(parts[1]);
        const i64 ay = std::stoll(parts[2]);
        const i64 az = std::stoll(parts[3]);
        const i64 bx = std::stoll(parts[4]);
        const i64 by = std::stoll(parts[5]);
        const i64 bz = std::stoll(parts[6]);
        Box box;
        box.minX = static_cast<f64>(std::min(ax, bx));
        box.minY = static_cast<f64>(std::min(ay, by));
        box.minZ = static_cast<f64>(std::min(az, bz));
        box.maxX = static_cast<f64>(std::max(ax, bx) + 1);
        box.maxY = static_cast<f64>(std::max(ay, by) + 1);
        box.maxZ = static_cast<f64>(std::max(az, bz) + 1);
        return json{{"unit", unit}, {"value", render(box)}}.dump();
    }
    if (mode == "ofSize" && parts.size() == 7) {
        const f64 cx = parseNumber(parts[1]);
        const f64 cy = parseNumber(parts[2]);
        const f64 cz = parseNumber(parts[3]);
        const f64 dx = parseNumber(parts[4]);
        const f64 dy = parseNumber(parts[5]);
        const f64 dz = parseNumber(parts[6]);
        Box box;
        box.minX = std::min(cx - dx / 2.0, cx + dx / 2.0);
        box.minY = std::min(cy - dy / 2.0, cy + dy / 2.0);
        box.minZ = std::min(cz - dz / 2.0, cz + dz / 2.0);
        box.maxX = std::max(cx - dx / 2.0, cx + dx / 2.0);
        box.maxY = std::max(cy - dy / 2.0, cy + dy / 2.0);
        box.maxZ = std::max(cz - dz / 2.0, cz + dz / 2.0);
        return json{{"unit", unit}, {"value", render(box)}}.dump();
    }
    if (mode == "center" && parts.size() == 7) {
        const Box box = parseBox(parts, 1);
        return json{{"unit", unit},
            {"value",
                renderPoint(
                    lerp(0.5, box.minX, box.maxX), lerp(0.5, box.minY, box.maxY), lerp(0.5, box.minZ, box.maxZ))}}
            .dump();
    }
    if (mode == "size" && parts.size() == 7) {
        // 原版 getSize() 返回三轴尺寸的算术平均；getXsize() 等才是单轴差值。
        const Box box = parseBox(parts, 1);
        const f64 sizeX = box.maxX - box.minX;
        const f64 sizeY = box.maxY - box.minY;
        const f64 sizeZ = box.maxZ - box.minZ;
        return json{{"unit", unit}, {"value", formatNumber((sizeX + sizeY + sizeZ) / 3.0)}}.dump();
    }
    if (mode == "axisSizes" && parts.size() == 7) {
        const Box box = parseBox(parts, 1);
        return json{
            {"unit", unit}, {"value", renderPoint(box.maxX - box.minX, box.maxY - box.minY, box.maxZ - box.minZ)}}
            .dump();
    }
    if (mode == "inflate" && parts.size() == 10) {
        const Box box = parseBox(parts, 1);
        const f64 dx = parseNumber(parts[7]);
        const f64 dy = parseNumber(parts[8]);
        const f64 dz = parseNumber(parts[9]);
        Box out;
        out.minX = std::min(box.minX - dx, box.maxX + dx);
        out.minY = std::min(box.minY - dy, box.maxY + dy);
        out.minZ = std::min(box.minZ - dz, box.maxZ + dz);
        out.maxX = std::max(box.minX - dx, box.maxX + dx);
        out.maxY = std::max(box.minY - dy, box.maxY + dy);
        out.maxZ = std::max(box.minZ - dz, box.maxZ + dz);
        return json{{"unit", unit}, {"value", render(out)}}.dump();
    }
    if (mode == "move" && parts.size() == 10) {
        const Box box = parseBox(parts, 1);
        const f64 dx = parseNumber(parts[7]);
        const f64 dy = parseNumber(parts[8]);
        const f64 dz = parseNumber(parts[9]);
        Box out;
        out.minX = box.minX + dx;
        out.minY = box.minY + dy;
        out.minZ = box.minZ + dz;
        out.maxX = box.maxX + dx;
        out.maxY = box.maxY + dy;
        out.maxZ = box.maxZ + dz;
        return json{{"unit", unit}, {"value", render(out)}}.dump();
    }
    if (mode == "intersect" && parts.size() == 13) {
        const Box a = parseBox(parts, 1);
        const Box b = parseBox(parts, 7);
        // 原版 AABB 构造器会归一化：min = Math.min(a, b), max = Math.max(a, b)。
        // 所以 intersect 在不相交时并不会返回负尺寸盒，而是把两端交换。
        Box raw;
        raw.minX = std::max(a.minX, b.minX);
        raw.minY = std::max(a.minY, b.minY);
        raw.minZ = std::max(a.minZ, b.minZ);
        raw.maxX = std::min(a.maxX, b.maxX);
        raw.maxY = std::min(a.maxY, b.maxY);
        raw.maxZ = std::min(a.maxZ, b.maxZ);
        Box out;
        out.minX = std::min(raw.minX, raw.maxX);
        out.minY = std::min(raw.minY, raw.maxY);
        out.minZ = std::min(raw.minZ, raw.maxZ);
        out.maxX = std::max(raw.minX, raw.maxX);
        out.maxY = std::max(raw.minY, raw.maxY);
        out.maxZ = std::max(raw.minZ, raw.maxZ);
        return json{{"unit", unit}, {"value", render(out)}}.dump();
    }
    if (mode == "hasNaN" && parts.size() == 7) {
        const Box box = parseBox(parts, 1);
        const bool hasNan = std::isnan(box.minX) || std::isnan(box.minY) || std::isnan(box.minZ) ||
            std::isnan(box.maxX) || std::isnan(box.maxY) || std::isnan(box.maxZ);
        return json{{"unit", unit}, {"value", hasNan}}.dump();
    }

    throw std::runtime_error("aabb_factory: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE("aabb_factory", &writeList, &runLine)

} // namespace mc::diff
