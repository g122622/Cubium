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
 * @brief 差分能力：`AABB` 的包含判定、尺寸与收缩（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.world.phys.AABB` 中**未被 `aabb_ops` 覆盖**的方法：
 *
 * ```java
 * public boolean contains(double x, double y, double z) {
 *     return x >= this.minX && x < this.maxX
 *         && y >= this.minY && y < this.maxY
 *         && z >= this.minZ && z < this.maxZ;
 * }
 *
 * public double getSize() {
 *     return (this.getXsize() + this.getYsize() + this.getZsize()) / 3.0;
 * }
 * public double getXsize() { return this.maxX - this.minX; }
 *
 * public AABB deflate(double dx, double dy, double dz) {
 *     return this.inflate(-dx, -dy, -dz);
 * }
 *
 * public AABB contract(double dx, double dy, double dz) {
 *     double d0 = this.minX, d1 = this.minY, d2 = this.minZ;
 *     double d3 = this.maxX, d4 = this.maxY, d5 = this.maxZ;
 *     if (dx < 0.0) d0 -= dx; else if (dx > 0.0) d3 -= dx;
 *     if (dy < 0.0) d1 -= dy; else if (dy > 0.0) d4 -= dy;
 *     if (dz < 0.0) d2 -= dz; else if (dz > 0.0) d5 -= dz;
 *     return new AABB(d0, d1, d2, d3, d4, d5);
 * }
 *
 * public boolean hasNaN() { return Double.isNaN(minX) || ...; }
 *
 * public double distanceToSqr(AABB other) {
 *     // 与原版一致的分量累加
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`contains` 是半开区间：`min` 包含，`max` 不包含。**
 *
 *    ```java
 *    x >= this.minX && x < this.maxX
 *    ```
 *
 *    对 `(0,0,0,1,1,1)`：
 *
 *    - `contains(0,0,0)` → **`true`**（`min` 边界算在内）
 *    - **`contains(1,1,1)` → `false`**（`max` 边界**不算**）
 *    - `contains(0.999,0.999,0.999)` → `true`
 *
 *    **这是 `[min, max)` 的方块判定约定**，与 `intersects`（闭区间重叠判定）不同。若把
 *    `contains` 写成 `<=`，所有站在方块边界上的实体都会被多算一格。
 *
 * 2. **`contract` 只改 min 或 max 中的一个，`deflate` 改两个。**
 *
 *    | 方法 | 参数为正 | 参数为负 |
 *    |---|---|---|
 *    | `deflate(d)` | 收缩（min 增、max 减）| 膨胀 |
 *    | **`contract(d)`** | **只减 `max`** | **只增 `min`** |
 *
 *    `contract(0.5, 0, 0)` 对 `(0,0,0,1,1,1)` 得 `(0,0,0,0.5,1,1)`——**只有 `maxX` 变小，
 *    `minX` 不动**。这用于「从某侧收缩包围盒」，例如玩家贴墙时的判定框。
 *
 *    **`contract` 参数为 0 时该轴完全不变**（`if/else if` 都不进）。若实现成
 *    `min += d; max -= d`，参数为 0 时行为相同，但为正/负时不同。
 *
 * 3. **`getSize` 是三个边长的算术平均，不是体积也不是对角线。**
 *
 *    ```java
 *    return (xsize + ysize + zsize) / 3.0;
 *    ```
 *
 *    对 `(0,0,0,1,1,1)` 得 `1.0`；对 `(0,0,0,1,2,3)` 得 `2.0`。
 *
 * 4. **`hasNaN` 检查全部六个分量。**
 *
 *    任一为 `NaN` 就返回 `true`。这是防御性检查——`NaN` 参与任何比较都返回 `false`，会让
 *    `contains` / `intersects` 静默失效。
 *
 * 单元格式：
 * - `<box>|contains|<x>|<y>|<z>` —— 包含判定
 * - `<box>|getSize` / `<box>|getXsize` / `<box>|getYsize` / `<box>|getZsize`
 * - `<box>|deflate|<dx>|<dy>|<dz>` —— 收缩（内部取负后 inflate）
 * - `<box>|contract|<dx>|<dy>|<dz>` —— 单侧收缩
 * - `<box>|hasNaN`
 * - `<box>|distAabb|<other>` —— 到另一个盒的距离平方
 *
 * `<box>` 形如 `minX,minY,minZ,maxX,maxY,maxZ`。
 * 所有浮点按 `%.6g` 格式化后去掉尾随零，与其它浮点能力的契约一致。
 *
 * TODO: 未覆盖 `clip`（返回 `Optional<Vec3>`，涉及射线求交）、`collidedAlongVector`、
 * `encapsulatingFullBlocks`、`unitCubeFromLowerCorner`、`of(BoundingBox)`。
 */
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>
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
using mc::Vector3;
using nlohmann::json;

/// 一个轴对齐包围盒。
struct Box {
    f64 minX;
    f64 minY;
    f64 minZ;
    f64 maxX;
    f64 maxY;
    f64 maxZ;
};

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

AxisAlignedBB parseBox(const std::string& text)
{
    f64 values[6] = {0, 0, 0, 0, 0, 0};
    i32 count = 0;
    std::string current;
    for (char ch : text) {
        if (ch == ',') {
            if (count >= 6) {
                throw std::runtime_error("aabb_contains: 包围盒必须六段：" + text);
            }
            values[count++] = std::stod(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    if (count != 5) {
        throw std::runtime_error("aabb_contains: 包围盒必须六段：" + text);
    }
    values[count++] = std::stod(current);
    return AxisAlignedBB(static_cast<f32>(values[0]),
        static_cast<f32>(values[1]),
        static_cast<f32>(values[2]),
        static_cast<f32>(values[3]),
        static_cast<f32>(values[4]),
        static_cast<f32>(values[5]));
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // contains：min 边界在内、max 边界不在
        "0,0,0,1,1,1|contains|0|0|0",
        "0,0,0,1,1,1|contains|1|1|1",
        "0,0,0,1,1,1|contains|0.999|0.999|0.999",
        "0,0,0,1,1,1|contains|0.5|0.5|0.5",
        "0,0,0,1,1,1|contains|-0.001|0.5|0.5",
        "0,0,0,1,1,1|contains|0.5|-0.001|0.5",
        "0,0,0,1,1,1|contains|0.5|0.5|-0.001",
        "0,0,0,1,1,1|contains|1|0.5|0.5",
        "0.5,0.5,0.5,1.5,2.5,3.5|contains|0.5|0.5|0.5",
        "0.5,0.5,0.5,1.5,2.5,3.5|contains|1.5|2.5|3.5",
        // getSize 与三个边长
        "0,0,0,1,1,1|getSize",
        "0,0,0,1,2,3|getSize",
        "0,0,0,1,2,3|getXsize",
        "0,0,0,1,2,3|getYsize",
        "0,0,0,1,2,3|getZsize",
        "0.5,0.5,0.5,1.5,2.5,3.5|getSize",
        // deflate：同时收缩两侧
        "0,0,0,1,1,1|deflate|0.25|0.25|0.25",
        "0,0,0,1,1,1|deflate|-0.5|-0.5|-0.5",
        "0,0,0,2,2,2|deflate|0.5|0|0",
        // contract：只改单侧
        "0,0,0,1,1,1|contract|0.5|0|0",
        "0,0,0,1,1,1|contract|-0.5|0|0",
        "0,0,0,1,1,1|contract|0|0.25|0",
        "0,0,0,2,2,2|contract|0.5|0.5|0.5",
        "0,0,0,2,2,2|contract|-0.5|-0.5|-0.5",
        "0,0,0,1,1,1|contract|0|0|0",
        // hasNaN
        "0,0,0,1,1,1|hasNaN",
        // distAabb：盒间距离
        "0,0,0,1,1,1|distAabb|0.5,0.5,0.5,1.5,1.5,1.5",
        "0,0,0,1,1,1|distAabb|2,0,0,3,1,1",
        "0,0,0,1,1,1|distAabb|2,2,2,3,3,3",
        "0,0,0,1,1,1|distAabb|5,5,5,6,6,6",
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
        throw std::runtime_error("aabb_contains: 单元至少两段：" + unit);
    }

    const AxisAlignedBB box = parseBox(parts[0]);
    const std::string mode = parts[1];

    if (mode == "contains" && parts.size() == 5) {
        const bool hit = box.contains(Vector3(static_cast<f32>(std::stod(parts[2])),
            static_cast<f32>(std::stod(parts[3])),
            static_cast<f32>(std::stod(parts[4]))));
        return json{{"unit", unit}, {"value", hit ? "true" : "false"}}.dump();
    }
    if (mode == "getSize" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", formatNumber(box.getSize())}}.dump();
    }
    if (mode == "getXsize" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", formatNumber(box.getXsize())}}.dump();
    }
    if (mode == "getYsize" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", formatNumber(box.getYsize())}}.dump();
    }
    if (mode == "getZsize" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", formatNumber(box.getZsize())}}.dump();
    }
    if (mode == "deflate" && parts.size() == 5) {
        const AxisAlignedBB result = box.deflate(static_cast<f32>(std::stod(parts[2])),
            static_cast<f32>(std::stod(parts[3])),
            static_cast<f32>(std::stod(parts[4])));
        return json{{"unit", unit}, {"value", boxToJson(result)}}.dump();
    }
    if (mode == "contract" && parts.size() == 5) {
        const AxisAlignedBB result = box.contract(static_cast<f32>(std::stod(parts[2])),
            static_cast<f32>(std::stod(parts[3])),
            static_cast<f32>(std::stod(parts[4])));
        return json{{"unit", unit}, {"value", boxToJson(result)}}.dump();
    }
    if (mode == "hasNaN" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", box.hasNaN() ? "true" : "false"}}.dump();
    }
    if (mode == "distAabb" && parts.size() == 3) {
        const f32 result = box.distanceToSqr(parseBox(parts[2]));
        return json{{"unit", unit}, {"value", formatNumber(result)}}.dump();
    }

    throw std::runtime_error("aabb_contains: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(aabb_contains, &writeList, &runLine)

} // namespace mc::diff
