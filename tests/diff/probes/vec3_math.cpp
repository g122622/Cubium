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
 * @brief 差分能力：`Vec3` 的向量运算（纯计算）。
 *
 * `list`：一组「模式|x,y,z|参数…」单元
 * `run` ：同上一行 → { "result": [...] } 或 { "value": "..." }
 *
 * 复刻原版 `Vec3`：
 *
 * ```java
 * public Vec3 normalize() {
 *     double d0 = Math.sqrt(this.x * this.x + this.y * this.y + this.z * this.z);
 *     return d0 < 1.0E-5F ? ZERO : new Vec3(this.x / d0, this.y / d0, this.z / d0);
 * }
 *
 * public double dot(Vec3 o) {
 *     return this.x * o.x + this.y * o.y + this.z * o.z;
 * }
 *
 * public Vec3 cross(Vec3 o) {
 *     return new Vec3(this.y * o.z - this.z * o.y,
 *                     this.z * o.x - this.x * o.z,
 *                     this.x * o.y - this.y * o.x);
 * }
 *
 * public Vec3 scale(double s)       { return this.multiply(s, s, s); }
 * public double length()            { return Math.sqrt(x*x + y*y + z*z); }
 * public double lengthSqr()         { return x*x + y*y + z*z; }
 * public double horizontalDistance()    { return Math.sqrt(x*x + z*z); }
 * public double horizontalDistanceSqr() { return x*x + z*z; }
 * ```
 *
 * ## 三个关键点
 *
 * 1. **`normalize()` 的短路阈值是 `1.0E-5F`**（`float` 字面量，被提升为 `double` 后比较）。
 *    长度小于该值时直接返回 `ZERO`，**不做除法**。这与「长度为 0 才返回零」不同：长度在
 *    `[1e-5, +∞)` 之外的任何向量都会被静默地归零。
 *
 * 2. **归一化用除法 `x / d0`，不是倒数乘 `x * (1/d0)`**。两者在浮点下可能差 1 ulp。本能力
 *    固定使用除法，与实现对齐后才能逐位比较。
 *
 * 3. **`horizontalDistance` 与 `length` 的差别只在是否含 `y`**。`horizontalDistance*` 完全
 *    忽略 `y`——所以 `(3, 4, 0)` 的 `length` 是 5、`horizontalDistance` 是 3。
 *
 * ## 与 Cubium 的两处已知口径差异
 *
 * - Cubium 的 `Vector3::normalized()` 用 `len > EPSILON`（`EPSILON = 1e-6f`）判断，原版是
 *   `d0 < 1.0E-5F`。**阈值差 10 倍**：长度落在 `[1e-6, 1e-5)` 的向量，原版归零、Cubium
 *   正常归一化。
 * - Cubium 用倒数乘 `x * (1/len)`，原版用除法 `x / d0`。
 *
 * 本探针按**原版口径**复算，因此这两处差异会体现为 diff。它们记录在
 * `docs/test/diff/vec3_math.md` 中，作为待对齐项而非豁免项。
 *
 * 单元格式：`<模式>|<x>,<y>,<z>|<参数…>`
 *
 * TODO: 未覆盖 `clip`（与 AABB 求交，已有 `explosion_ray` 覆盖一个侧面）、`subtract`/
 * `add` 的逐分量形式、`multiply` 的逐分量形式。
 */
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 原版 `Vec3.normalize()` 的短路阈值（`1.0E-5F`，float 字面量）。
constexpr f64 NORMALIZE_EPSILON = 1.0e-5;

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

json vecToJson(const Vector3d& v)
{
    return json::array({formatNumber(v.x), formatNumber(v.y), formatNumber(v.z)});
}

/// 单元表。
constexpr const char* UNITS[] = {
    // normalize：常规向量
    "normalize|3,4,0",
    "normalize|1,0,0",
    "normalize|1,1,1",
    // normalize：零向量 -> ZERO
    "normalize|0,0,0",
    // normalize：长度恰在阈值以下（1e-6，远小于 1e-5）-> ZERO
    "normalize|1e-06,0,0",
    // normalize：长度刚好低于阈值（9.9e-6）-> ZERO
    "normalize|9.9e-06,0,0",
    // normalize：长度刚好等于阈值（1e-5）-> 不归零（严格小于才归零）
    "normalize|1e-05,0,0",
    // normalize：长度略高于阈值（1.1e-5）-> 归一化
    "normalize|1.1e-05,0,0",
    // normalize：长度在 [1e-6, 1e-5) 之间 —— Cubium 的 EPSILON 区间
    "normalize|5e-06,0,0",
    // normalize：负分量
    "normalize|-3,-4,0",
    // dot
    "dot|1,2,3|4,5,6",
    "dot|0,0,0|1,1,1",
    "dot|-1,-2,-3|4,5,6",
    "dot|1,0,0|0,1,0",
    // cross
    "cross|1,0,0|0,1,0",
    "cross|1,2,3|4,5,6",
    "cross|0,0,0|1,2,3",
    "cross|1,1,1|1,1,1",
    // length / lengthSqr
    "length|3,4,0",
    "length|1,1,1",
    "length|0,0,0",
    "lengthsqr|3,4,0",
    "lengthsqr|1,1,1",
    // horizontalDistance / horizontalDistanceSqr：注意 y 被忽略
    "hdist|3,4,0",
    "hdist|3,100,4",
    "hdist|0,5,0",
    "hdistsqr|3,100,4",
    // scale
    "scale|1,2,3|2",
    "scale|1,2,3|0",
    "scale|1,2,3|-1",
    "scale|1,2,3|0.5",
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

/// 原版 `Vec3.horizontalDistanceSqr()`。
f64 horizontalDistanceSqr(const Vector3d& v)
{
    return v.x * v.x + v.z * v.z;
}

Vector3d parseVec(const std::string& text)
{
    std::vector<std::string> parts;
    std::string current;
    for (char ch : text) {
        if (ch == ',') {
            parts.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    parts.push_back(current);
    if (parts.size() != 3) {
        throw std::runtime_error("vec3_math: 向量应为 3 个分量：" + text);
    }
    return Vector3d{std::stod(parts[0]), std::stod(parts[1]), std::stod(parts[2])};
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("vec3_math: 单元至少需要「模式|向量」：" + unit);
    }

    const std::string mode = parts[0];
    const Vector3d v = parseVec(parts[1]);

    if (mode == "normalize" && parts.size() == 2) {
        return json{{"unit", unit}, {"result", vecToJson(v.normalized())}}.dump();
    }
    if (mode == "dot" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", formatNumber(v.dot(parseVec(parts[2])))}}.dump();
    }
    if (mode == "cross" && parts.size() == 3) {
        return json{{"unit", unit}, {"result", vecToJson(v.cross(parseVec(parts[2])))}}.dump();
    }
    if (mode == "length" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", formatNumber(v.length())}}.dump();
    }
    if (mode == "lengthsqr" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", formatNumber(v.lengthSquared())}}.dump();
    }
    if (mode == "hdist" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", formatNumber(v.lengthHorizontal())}}.dump();
    }
    if (mode == "hdistsqr" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", formatNumber(v.lengthHorizontal() * v.lengthHorizontal())}}.dump();
    }
    if (mode == "scale" && parts.size() == 3) {
        return json{{"unit", unit}, {"result", vecToJson(v * std::stod(parts[2]))}}.dump();
    }

    throw std::runtime_error("vec3_math: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(vec3_math, &writeList, &runLine)

} // namespace mc::diff
