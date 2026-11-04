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
 * @brief 差分能力：`AxisCycle` —— 三轴循环映射（纯计算）。
 *
 * `list`：全部单元
 * `run` ：一行 → { "value": ... }
 *
 * ## 为什么需要它
 *
 * `AxisCycle` 是 `VoxelShape.collide`、`DiscreteVoxelShape` 的边/面遍历、
 * 以及 `AABB.inflate` 等一大批几何算法的**底层坐标变换**。它没有分支、
 * 没有浮点，一旦这里错一个映射，上层所有几何结果都会整体偏移，而症状
 * 通常表现为「碰撞箱差了一点」这种极难定位的形式。
 *
 * ## 原版语义（`net/minecraft/core/AxisCycle.java`）
 *
 * ```java
 * NONE {
 *     int  cycle(x, y, z, axis) { return axis.choose(x, y, z); }
 *     Axis cycle(axis)          { return axis; }
 *     AxisCycle inverse()       { return this; }
 * }
 * FORWARD {
 *     int  cycle(x, y, z, axis) { return axis.choose(z, x, y); }
 *     Axis cycle(axis)          { return AXIS_VALUES[floorMod(axis.ordinal() + 1, 3)]; }
 *     AxisCycle inverse()       { return BACKWARD; }
 * }
 * BACKWARD {
 *     int  cycle(x, y, z, axis) { return axis.choose(y, z, x); }
 *     Axis cycle(axis)          { return AXIS_VALUES[floorMod(axis.ordinal() - 1, 3)]; }
 *     AxisCycle inverse()       { return FORWARD; }
 * }
 * ```
 *
 * 其中 `Axis.choose(x, y, z)` 是「按轴取分量」：`X→x, Y→y, Z→z`。
 *
 * **注意两个 `cycle` 重载语义不同**：`cycle(axis)` 是「轴序推进」（X→Y→Z→X），
 * 而 `cycle(x, y, z, axis)` 是「按循环重排三个分量后取指定轴」，对 FORWARD
 * 而言 X 取的是 **z** 而不是 y。这两者极易混淆。
 *
 * ## 单元格式
 *
 * | 单元 | 含义 |
 * |---|---|
 * | `axis:<cycle>|<axis>` | `cycle(Axis)` —— 轴序推进 |
 * | `inverse:<cycle>` | `inverse()` |
 * | `coord:<cycle>|<x>|<y>|<z>|<axis>` | `cycle(int,int,int,Axis)` —— 分量重排 |
 * | `between:<from>|<to>` | Cubium 侧 `AxisCycles::between`（原版无对应物，按定义等价于 `inverse(cycle-for-x-shift)`
 * 推出） |
 */

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 三个循环名。
constexpr const char* CYCLES[] = {"none", "forward", "backward"};
/// 三个轴名（下标与 `Axis` 枚举一致）。
constexpr const char* AXES[] = {"x", "y", "z"};

bool parseCycle(const std::string& name, AxisCycle& out)
{
    if (name == "none") {
        out = AxisCycle::NONE;
        return true;
    }
    if (name == "forward") {
        out = AxisCycle::FORWARD;
        return true;
    }
    if (name == "backward") {
        out = AxisCycle::BACKWARD;
        return true;
    }
    return false;
}

bool parseAxis(const std::string& name, Axis& out)
{
    if (name == "x") {
        out = Axis::X;
        return true;
    }
    if (name == "y") {
        out = Axis::Y;
        return true;
    }
    if (name == "z") {
        out = Axis::Z;
        return true;
    }
    return false;
}

std::string cycleName(AxisCycle cycle)
{
    return CYCLES[static_cast<size_t>(cycle)];
}

std::string axisName(Axis axis)
{
    return AXES[static_cast<size_t>(axis)];
}

std::vector<std::string> splitPipe(std::string_view text)
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

void writeList(FILE* out)
{
    // 1) cycle(Axis)：3 循环 × 3 轴。
    for (const char* cycle : CYCLES) {
        for (const char* axis : AXES) {
            std::fprintf(out, "axis:%s|%s\n", cycle, axis);
        }
    }
    // 2) inverse()：3 循环。
    for (const char* cycle : CYCLES) {
        std::fprintf(out, "inverse:%s\n", cycle);
    }
    // 3) cycle(int,int,int,Axis)：3 循环 × 3 轴 × 若干分量组合。
    //
    // 分量刻意选成**互不相等**且含负数、零，任何「取错分量」都会立即显形
    // （用 1/1/1 这类相同值测试会让错误映射静默通过）。
    constexpr const char* COORDS[] = {
        "1|2|3",       // 基础
        "3|2|1",       // 反向排列
        "0|0|0",       // 全零（无法区分，用作对照）
        "-1|-2|-3",    // 负数
        "7|0|-5",      // 混合
        "100|200|300", // 大值
    };
    for (const char* cycle : CYCLES) {
        for (const char* coords : COORDS) {
            for (const char* axis : AXES) {
                std::fprintf(out, "coord:%s|%s|%s\n", cycle, coords, axis);
            }
        }
    }
    // 4) between(from, to)：3 × 3。
    for (const char* from : AXES) {
        for (const char* to : AXES) {
            std::fprintf(out, "between:%s|%s\n", from, to);
        }
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const std::vector<std::string> parts = splitPipe(unit);
    if (parts.empty()) {
        throw std::runtime_error("axis_cycle: 空单元");
    }

    // 单元的形态是 `<kind>:<参数>|<参数>|...`，前缀与第一个参数用 `:` 相连，
    // 因此必须先按 `:` 拆出 kind，再按 `|` 拆参数。
    const std::size_t colon = parts[0].find(':');
    const std::string kind = parts[0].substr(0, colon);
    std::vector<std::string> args;
    if (colon != std::string::npos) {
        args.push_back(parts[0].substr(colon + 1));
    }
    for (std::size_t i = 1; i < parts.size(); ++i) {
        args.push_back(parts[i]);
    }

    if (kind == "axis" && args.size() == 2) {
        AxisCycle cycle{};
        Axis axis{};
        if (!parseCycle(args[0], cycle) || !parseAxis(args[1], axis)) {
            throw std::runtime_error("axis_cycle: 无法解析 axis 单元：" + unit);
        }
        return json{{"unit", unit}, {"value", axisName(AxisCycles::cycle(cycle, axis))}}.dump();
    }

    if (kind == "inverse" && args.size() == 1) {
        AxisCycle cycle{};
        if (!parseCycle(args[0], cycle)) {
            throw std::runtime_error("axis_cycle: 无法解析 inverse 单元：" + unit);
        }
        return json{{"unit", unit}, {"value", cycleName(AxisCycles::inverse(cycle))}}.dump();
    }

    if (kind == "coord" && args.size() == 5) {
        AxisCycle cycle{};
        Axis axis{};
        if (!parseCycle(args[0], cycle) || !parseAxis(args[4], axis)) {
            throw std::runtime_error("axis_cycle: 无法解析 coord 单元：" + unit);
        }
        const i32 x = std::stoi(args[1]);
        const i32 y = std::stoi(args[2]);
        const i32 z = std::stoi(args[3]);
        return json{{"unit", unit}, {"value", AxisCycles::cycle(cycle, x, y, z, axis)}}.dump();
    }

    if (kind == "between" && args.size() == 2) {
        Axis from{};
        Axis to{};
        if (!parseAxis(args[0], from) || !parseAxis(args[1], to)) {
            throw std::runtime_error("axis_cycle: 无法解析 between 单元：" + unit);
        }
        return json{{"unit", unit}, {"value", cycleName(AxisCycles::between(from, to))}}.dump();
    }
    throw std::runtime_error("axis_cycle: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(axis_cycle, &writeList, &runLine)

} // namespace mc::diff
