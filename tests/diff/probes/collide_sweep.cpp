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
 * @brief 差分能力：实体碰撞扫描的轴序与实际位移（纯计算）。
 *
 * `list`：一组「轴序」单元与「碰撞」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版：
 *
 * ```java
 * // Direction.axisStepOrder
 * private static final List<Axis> YXZ_AXIS_ORDER = List.of(Axis.Y, Axis.X, Axis.Z);
 * private static final List<Axis> YZX_AXIS_ORDER = List.of(Axis.Y, Axis.Z, Axis.X);
 *
 * public static List<Axis> axisStepOrder(Vec3 delta) {
 *     return Math.abs(delta.x) < Math.abs(delta.z) ? YZX_AXIS_ORDER : YXZ_AXIS_ORDER;
 * }
 *
 * // Entity.collideWithShapes
 * Vec3 result = Vec3.ZERO;
 * for (Axis axis : Direction.axisStepOrder(delta)) {
 *     double d = delta.get(axis);
 *     if (d != 0.0) {
 *         double moved = Shapes.collide(axis, box.move(result), shapes, d);
 *         result = result.with(axis, moved);
 *     }
 * }
 *
 * // Shapes.collide
 * for (VoxelShape shape : shapes) {
 *     if (Math.abs(d) < 1.0E-7) { return 0.0; }
 *     d = shape.collide(axis, box, d);
 * }
 * ```
 *
 * 单盒碰撞（`AABB` 与一组轴对齐盒）按以下规则解得实际位移：
 *
 * ```
 * 沿正方向：对每个在另两轴上与运动盒重叠的碰撞盒，
 *           若其起始面 >= 运动盒末尾面 - 1e-7，
 *           位移 = min(位移, 碰撞盒起始面 - 运动盒末尾面)
 * 沿负方向：对称，取 max
 * ```
 *
 * ## 四个关键点
 *
 * 1. **Y 轴永远第一个处理**，X 与 Z 的先后由位移决定：`|x| < |z|` 时走 `YZX`（先 Z 后 X），
 *    否则走 `YXZ`（先 X 后 Z）。**主轴（位移分量大的那条）最后推进**，这样侧向位移不会
 *    提前把实体推离主轴路径。
 *
 * 2. **`|d| < 1e-7` 时直接返回 0**，且这个判断在**每个 shape 的循环内**——一旦某个 shape
 *    把位移压到 1e-7 以下，后续 shape 全部跳过并返回 0。这意味着「贴墙滑行」时微小位移会被
 *    完全吃掉，而不是累积成亚像素移动。
 *
 * 3. **`1e-7` 容差同时出现在两处**：碰撞判定（`d2 >= -1e-7`）与提前返回。前者让「恰好贴合
 *    的面上沿」不被判为碰撞，后者抹掉微小残余。两处都用固定绝对容差，不随坐标量级缩放。
 *
 * 4. **首次命中即 `return`**，不是继续搜更近的。原版依赖 shape 按坐标排序，`collideX` 用
 *    `findIndex` 把搜索范围收窄到与运动盒相交的区间，所以首个命中就是最近的。
 *
 * 单元格式：
 * - `axis|<dx>|<dy>|<dz>` —— 只输出轴序
 * - `collide|<dx>|<dy>|<dz>|<盒子>|<碰撞盒1>[|<碰撞盒2>...]`
 *
 * 盒子与碰撞盒的格式为 `minX,minY,minZ,maxX,maxY,maxZ`（逗号分隔 6 个值）。
 *
 * TODO: 未覆盖 `VoxelShape` 的多盒形状（`isFullWide` 的分层索引）、世界边界、方块碰撞收集。
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

/// 原版碰撞判定的固定容差。
constexpr f64 EPSILON = 1.0E-7;

enum class Axis { X, Y, Z };

std::string axisName(Axis axis)
{
    switch (axis) {
        case Axis::X:
            return "x";
        case Axis::Y:
            return "y";
        default:
            return "z";
    }
}

/// 原版 `Direction.axisStepOrder`：`|x| < |z|` 时 YZX，否则 YXZ。
std::vector<Axis> axisStepOrder(f64 dx, f64 dy, f64 dz)
{
    if (std::fabs(dx) < std::fabs(dz)) {
        return {Axis::Y, Axis::Z, Axis::X};
    }
    return {Axis::Y, Axis::X, Axis::Z};
}

/// 轴对齐盒。
struct Box {
    f64 minX = 0.0;
    f64 minY = 0.0;
    f64 minZ = 0.0;
    f64 maxX = 0.0;
    f64 maxY = 0.0;
    f64 maxZ = 0.0;

    f64 min(Axis axis) const
    {
        switch (axis) {
            case Axis::X:
                return minX;
            case Axis::Y:
                return minY;
            default:
                return minZ;
        }
    }

    f64 max(Axis axis) const
    {
        switch (axis) {
            case Axis::X:
                return maxX;
            case Axis::Y:
                return maxY;
            default:
                return maxZ;
        }
    }

    /// 沿某轴平移。
    Box moved(f64 dx, f64 dy, f64 dz) const
    {
        return Box{minX + dx, minY + dy, minZ + dz, maxX + dx, maxY + dy, maxZ + dz};
    }

    /// 两个盒在某轴上是否重叠（含容差）。
    bool overlapsOn(Axis axis, const Box& other) const
    {
        return min(axis) < other.max(axis) - EPSILON && max(axis) > other.min(axis) + EPSILON;
    }
};

/**
 * @brief 单盒沿单轴的碰撞位移。
 *
 * 对应原版 `VoxelShape.collideX` 在「单盒形状」上的语义。
 */
f64 collideAxis(Axis axis, const Box& moving, f64 d, const std::vector<Box>& shapes)
{
    if (std::fabs(d) < EPSILON) {
        return 0.0;
    }

    Axis first = Axis::X;
    Axis second = Axis::Z;
    if (axis == Axis::X) {
        first = Axis::Y;
        second = Axis::Z;
    } else if (axis == Axis::Y) {
        first = Axis::X;
        second = Axis::Z;
    } else {
        first = Axis::X;
        second = Axis::Y;
    }

    f64 result = d;
    for (const auto& shape : shapes) {
        if (std::fabs(result) < EPSILON) {
            return 0.0;
        }
        // 另两轴必须与运动盒重叠，否则该 shape 不可能挡住。
        if (!moving.overlapsOn(first, shape) || !moving.overlapsOn(second, shape)) {
            continue;
        }

        if (result > 0.0) {
            const f64 gap = shape.min(axis) - moving.max(axis);
            if (gap >= -EPSILON) {
                result = std::min(result, gap);
                return result;
            }
        } else {
            const f64 gap = shape.max(axis) - moving.min(axis);
            if (gap <= EPSILON) {
                result = std::max(result, gap);
                return result;
            }
        }
    }
    return result;
}

/**
 * @brief 复刻 `Entity.collideWithShapes`：按轴序逐轴推进。
 */
Box collide(const Box& start, f64 dx, f64 dy, f64 dz, const std::vector<Box>& shapes)
{
    Box current = start;
    if (shapes.empty()) {
        return start.moved(dx, dy, dz);
    }

    for (Axis axis : axisStepOrder(dx, dy, dz)) {
        f64 delta = 0.0;
        if (axis == Axis::X) {
            delta = dx;
        } else if (axis == Axis::Y) {
            delta = dy;
        } else {
            delta = dz;
        }
        if (delta == 0.0) {
            continue;
        }
        const f64 moved = collideAxis(axis, current, delta, shapes);
        if (axis == Axis::X) {
            current = current.moved(moved, 0.0, 0.0);
        } else if (axis == Axis::Y) {
            current = current.moved(0.0, moved, 0.0);
        } else {
            current = current.moved(0.0, 0.0, moved);
        }
    }
    return current;
}

std::string formatNumber(f64 value)
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.9g", value);
    std::string text(buffer);
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

std::vector<std::string> splitBy(const std::string& text, char separator)
{
    std::vector<std::string> tokens;
    std::string current;
    for (char ch : text) {
        if (ch == separator) {
            tokens.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    tokens.push_back(current);
    return tokens;
}

Box parseBox(const std::string& text)
{
    const auto parts = splitBy(text, ',');
    if (parts.size() != 6) {
        throw std::runtime_error("collide_sweep: 盒子应为 6 个逗号分隔的值：" + text);
    }
    return Box{std::stod(parts[0]),
        std::stod(parts[1]),
        std::stod(parts[2]),
        std::stod(parts[3]),
        std::stod(parts[4]),
        std::stod(parts[5])};
}

/// 单元表。实体盒统一为边长 0.6 的立方体，放在原点。
constexpr const char* UNITS[] = {
    // 轴序：|x| 与 |z| 的大小关系
    "axis|1|0|0",
    "axis|0|0|1",
    "axis|1|0|1",
    "axis|0.5|0|1",
    "axis|1|0|0.5",
    "axis|-1|0|0",
    "axis|0|0|-1",
    "axis|-1|0|1",
    "axis|1|0|-1",
    // 轴序：纯垂直移动
    "axis|0|1|0",
    "axis|0|0|0",
    // 碰撞：无碰撞盒（自由移动）
    "collide|1|0|0|0,0,0,0.6,1.8,0.6",
    // 碰撞：正面撞墙（X+）
    "collide|1|0|0|0,0,0,0.6,1.8,0.6|1,0,-1,2,2,2",
    // 碰撞：撞墙但距离足够（不碰）
    "collide|0.3|0|0|0,0,0,0.6,1.8,0.6|1,0,-1,2,2,2",
    // 碰撞：贴墙（间隙 0）
    "collide|1|0|0|0,0,0,0.6,1.8,0.6|0.6,0,-1,2,2,2",
    // 碰撞：落地（Y-）
    "collide|0|-1|0|0,0,0,0.6,1.8,0.6|-2,-1,-2,2,0,2",
    // 碰撞：向上撞天花板
    "collide|0|1|0|0,0,0,0.6,1.8,0.6|-2,1.8,-2,2,4,2",
    // 碰撞：对角移动，X 先被挡住
    "collide|1|0|1|0,0,0,0.6,1.8,0.6|1,0,-2,2,2,2",
    // 碰撞：对角移动，Z 先被挡住
    "collide|1|0|1|0,0,0,0.6,1.8,0.6|-2,0,1,2,2,2",
    // 碰撞：两个碰撞盒，最近的生效
    "collide|2|0|0|0,0,0,0.6,1.8,0.6|1,0,-1,2,2,2|3,0,-1,4,2,2",
    // 碰撞：碰撞盒在侧方但不重叠（应忽略）
    "collide|1|0|0|0,0,0,0.6,1.8,0.6|1,0,5,2,2,6",
    // 碰撞：微小位移（低于 1e-7 阈值）
    "collide|0.0000001|0|0|0,0,0,0.6,1.8,0.6|1,0,-1,2,2,2",
    // 碰撞：负向撞墙
    "collide|-1|0|0|0,0,0,0.6,1.8,0.6|-2,0,-1,-1,2,2",
};

void writeList(FILE* out)
{
    for (const auto* unit : UNITS) {
        std::fprintf(out, "%s\n", unit);
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitBy(unit, '|');
    if (parts.size() < 2) {
        throw std::runtime_error("collide_sweep: 单元至少两段：" + unit);
    }

    if (parts[0] == "axis" && parts.size() == 4) {
        const auto order = axisStepOrder(std::stod(parts[1]), std::stod(parts[2]), std::stod(parts[3]));
        std::string text;
        for (const auto axis : order) {
            text += axisName(axis);
        }
        return json{{"unit", unit}, {"value", text}}.dump();
    }

    if (parts[0] == "collide" && parts.size() >= 5) {
        const f64 dx = std::stod(parts[1]);
        const f64 dy = std::stod(parts[2]);
        const f64 dz = std::stod(parts[3]);
        const Box start = parseBox(parts[4]);

        std::vector<Box> shapes;
        for (size_t i = 5; i < parts.size(); ++i) {
            shapes.push_back(parseBox(parts[i]));
        }

        const Box end = collide(start, dx, dy, dz, shapes);
        return json{{"unit", unit},
            {"dx", formatNumber(end.minX - start.minX)},
            {"dy", formatNumber(end.minY - start.minY)},
            {"dz", formatNumber(end.minZ - start.minZ)},
            {"maxX", formatNumber(end.maxX)},
            {"maxY", formatNumber(end.maxY)},
            {"maxZ", formatNumber(end.maxZ)}}
            .dump();
    }

    throw std::runtime_error("collide_sweep: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(collide_sweep, &writeList, &runLine)

} // namespace mc::diff
