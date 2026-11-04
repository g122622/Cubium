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
 * @brief 差分能力：`Direction.Axis` / `Direction.Plane` 的判定与选取（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "result": "..." }
 *
 * 复刻原版 `Direction.Axis` 与 `Direction.Plane`：
 *
 * ```java
 * // Axis 枚举：X("x"), Y("y"), Z("z")
 * public int choose(int x, int y, int z) { return x; }   // X 取第 1 个
 * public int choose(int x, int y, int z) { return y; }   // Y 取第 2 个
 * public int choose(int x, int y, int z) { return z; }   // Z 取第 3 个
 * // boolean 与 double 版本结构相同
 *
 * public Direction getPositive() { return EAST; }   // X -> EAST
 * public Direction getNegative() { return WEST; }   // X -> WEST
 * // Y -> (UP, DOWN)，Z -> (SOUTH, NORTH)
 *
 * public Plane getPlane() {
 *     return switch (this) { case X, Z -> HORIZONTAL; case Y -> VERTICAL; };
 * }
 *
 * public Direction.Axis getAxis() {
 *     return switch (this) { case DOWN, UP -> Y; case NORTH, SOUTH -> Z; case WEST, EAST -> X; };
 * }
 *
 * // Plane
 * HORIZONTAL(new Direction[]{NORTH, EAST, SOUTH, WEST}, new Axis[]{X, Z}),
 * VERTICAL(new Direction[]{UP, DOWN}, new Axis[]{Y});
 *
 * public boolean test(@Nullable Direction d) {
 *     return d != null && d.getAxis().getPlane() == this;
 * }
 * ```
 *
 * ## 三个关键点
 *
 * 1. **`Axis.choose(x, y, z)` 是按轴取对应分量**：`X.choose` 返回**第 1 个**参数、`Y` 返回
 *    第 2 个、`Z` 返回第 3 个。这不是「选最大/最小」之类的运算，只是按轴投影。三个重载
 *    （`int` / `double` / `boolean`）结构完全一致。
 *
 * 2. **`getPositive` / `getNegative` 与 `AxisDirection` 相关但不同**：`Z` 轴的 `getPositive`
 *    是 `SOUTH` 而不是 `NORTH`——因为 Minecraft 的 `+Z` 指向南。这是最容易写反的一处。
 *
 * 3. **`Plane.test(null)` 返回 `false`**，且 `HORIZONTAL` 的轴集合是 `{X, Z}`（注意枚举
 *    顺序是 `X, Z` 而不是 `Z, X`）。`HORIZONTAL` 的方向数组顺序是 `NORTH, EAST, SOUTH,
 *    WEST`——这是迭代顺序，影响依赖遍历下标的代码。
 *
 * 单元格式：
 * - `choosei|<axis>` / `choosed|<axis>` / `chooseb|<axis>` —— 用固定三分量 `(10,20,30)` 取值
 * - `positive|<axis>` / `negative|<axis>` —— 该轴的正/负方向
 * - `planeof|<axis>` —— 轴所属平面
 * - `axisof|<dir>` —— 方向所属轴
 * - `test|<plane>|<dir>` / `test|<plane>|null` —— 平面判定
 *
 * TODO: 未覆盖 `Plane.getRandomDirection` / `getRandomAxis`（依赖随机源）、`Plane.iterator` /
 * `stream`（迭代顺序已在文档中记录，但未用单元钉住）。
 */
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

enum class Dir : u8 { Down = 0, Up = 1, North = 2, South = 3, West = 4, East = 5 };

enum class Axis : u8 { X = 0, Y = 1, Z = 2 };

enum class Plane : u8 { Horizontal = 0, Vertical = 1 };

const char* dirName(Dir dir)
{
    switch (dir) {
        case Dir::Down:
            return "down";
        case Dir::Up:
            return "up";
        case Dir::North:
            return "north";
        case Dir::South:
            return "south";
        case Dir::West:
            return "west";
        case Dir::East:
            return "east";
    }
    return "unknown";
}

const char* axisName(Axis axis)
{
    switch (axis) {
        case Axis::X:
            return "x";
        case Axis::Y:
            return "y";
        case Axis::Z:
            return "z";
    }
    return "unknown";
}

bool parseDir(const std::string& text, Dir& out)
{
    if (text == "down") {
        out = Dir::Down;
    } else if (text == "up") {
        out = Dir::Up;
    } else if (text == "north") {
        out = Dir::North;
    } else if (text == "south") {
        out = Dir::South;
    } else if (text == "west") {
        out = Dir::West;
    } else if (text == "east") {
        out = Dir::East;
    } else {
        return false;
    }
    return true;
}

bool parseAxis(const std::string& text, Axis& out)
{
    if (text == "x") {
        out = Axis::X;
    } else if (text == "y") {
        out = Axis::Y;
    } else if (text == "z") {
        out = Axis::Z;
    } else {
        return false;
    }
    return true;
}

bool parsePlane(const std::string& text, Plane& out)
{
    if (text == "horizontal") {
        out = Plane::Horizontal;
    } else if (text == "vertical") {
        out = Plane::Vertical;
    } else {
        return false;
    }
    return true;
}

/// 原版 `Direction.Axis.choose(int, int, int)`：按轴取对应分量。
i32 chooseInt(Axis axis, i32 x, i32 y, i32 z)
{
    switch (axis) {
        case Axis::X:
            return x;
        case Axis::Y:
            return y;
        case Axis::Z:
            return z;
    }
    return 0;
}

/// 原版 `Direction.Axis.getPositive()`。
Dir getPositive(Axis axis)
{
    switch (axis) {
        case Axis::X:
            return Dir::East;
        case Axis::Y:
            return Dir::Up;
        case Axis::Z:
            // 注意：+Z 指向南，不是北。
            return Dir::South;
    }
    return Dir::Up;
}

/// 原版 `Direction.Axis.getNegative()`。
Dir getNegative(Axis axis)
{
    switch (axis) {
        case Axis::X:
            return Dir::West;
        case Axis::Y:
            return Dir::Down;
        case Axis::Z:
            return Dir::North;
    }
    return Dir::Down;
}

/// 原版 `Direction.Axis.getPlane()`。
Plane getPlane(Axis axis)
{
    return axis == Axis::Y ? Plane::Vertical : Plane::Horizontal;
}

/// 原版 `Direction.getAxis()`。
Axis getAxis(Dir dir)
{
    switch (dir) {
        case Dir::Down:
        case Dir::Up:
            return Axis::Y;
        case Dir::North:
        case Dir::South:
            return Axis::Z;
        case Dir::West:
        case Dir::East:
            return Axis::X;
    }
    return Axis::Y;
}

/// 原版 `Direction.Plane.test`：null 返回 false。
bool planeTest(Plane plane, bool hasDir, Dir dir)
{
    if (!hasDir) {
        return false;
    }
    return getPlane(getAxis(dir)) == plane;
}

/// 单元表。
constexpr const char* UNITS[] = {
    // choose：轴决定取第几个分量
    "choosei|x",
    "choosei|y",
    "choosei|z",
    // positive / negative：Z 轴正方向是 SOUTH
    "positive|x",
    "positive|y",
    "positive|z",
    "negative|x",
    "negative|y",
    "negative|z",
    // planeof
    "planeof|x",
    "planeof|y",
    "planeof|z",
    // axisof：六个方向
    "axisof|down",
    "axisof|up",
    "axisof|north",
    "axisof|south",
    "axisof|west",
    "axisof|east",
    // test：水平面
    "test|horizontal|north",
    "test|horizontal|east",
    "test|horizontal|south",
    "test|horizontal|west",
    "test|horizontal|up",
    "test|horizontal|down",
    // test：垂直面
    "test|vertical|up",
    "test|vertical|down",
    "test|vertical|north",
    "test|vertical|east",
    // test：null
    "test|horizontal|null",
    "test|vertical|null",
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
    if (parts.size() < 2) {
        throw std::runtime_error("axis_plane: 单元至少两段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "choosei" && parts.size() == 2) {
        Axis axis{};
        if (!parseAxis(parts[1], axis)) {
            throw std::runtime_error("axis_plane: 未知轴 " + parts[1]);
        }
        // 固定三分量 (10, 20, 30)，用返回值区分取了哪一个。
        return json{{"unit", unit}, {"result", chooseInt(axis, 10, 20, 30)}}.dump();
    }

    if ((mode == "positive" || mode == "negative") && parts.size() == 2) {
        Axis axis{};
        if (!parseAxis(parts[1], axis)) {
            throw std::runtime_error("axis_plane: 未知轴 " + parts[1]);
        }
        const Dir dir = (mode == "positive") ? getPositive(axis) : getNegative(axis);
        return json{{"unit", unit}, {"result", dirName(dir)}}.dump();
    }

    if (mode == "planeof" && parts.size() == 2) {
        Axis axis{};
        if (!parseAxis(parts[1], axis)) {
            throw std::runtime_error("axis_plane: 未知轴 " + parts[1]);
        }
        return json{{"unit", unit}, {"result", getPlane(axis) == Plane::Vertical ? "vertical" : "horizontal"}}.dump();
    }

    if (mode == "axisof" && parts.size() == 2) {
        Dir dir{};
        if (!parseDir(parts[1], dir)) {
            throw std::runtime_error("axis_plane: 未知方向 " + parts[1]);
        }
        return json{{"unit", unit}, {"result", axisName(getAxis(dir))}}.dump();
    }

    if (mode == "test" && parts.size() == 3) {
        Plane plane{};
        if (!parsePlane(parts[1], plane)) {
            throw std::runtime_error("axis_plane: 未知平面 " + parts[1]);
        }
        if (parts[2] == "null") {
            return json{{"unit", unit}, {"result", planeTest(plane, false, Dir::Up)}}.dump();
        }
        Dir dir{};
        if (!parseDir(parts[2], dir)) {
            throw std::runtime_error("axis_plane: 未知方向 " + parts[2]);
        }
        return json{{"unit", unit}, {"result", planeTest(plane, true, dir)}}.dump();
    }

    throw std::runtime_error("axis_plane: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(axis_plane, &writeList, &runLine)

} // namespace mc::diff
