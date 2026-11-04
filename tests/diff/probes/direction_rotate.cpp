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
 * @brief 差分能力：`Direction` 的旋转与 Y 轴角度换算（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "result": "east" } 等
 *
 * 复刻原版 `Direction` 的三组运算：
 *
 * ```java
 * public Direction getClockWise(Axis a) {
 *     return switch (a) {
 *         case X -> this != WEST && this != EAST ? this.getClockWiseX() : this;
 *         case Y -> this != UP && this != DOWN ? this.getClockWise() : this;
 *         case Z -> this != NORTH && this != SOUTH ? this.getClockWiseZ() : this;
 *     };
 * }
 *
 * public Direction getClockWise() {
 *     return switch (this) {
 *         case NORTH -> EAST;  case SOUTH -> WEST;
 *         case WEST -> NORTH;  case EAST -> SOUTH;
 *         default -> throw new IllegalStateException(...);
 *     };
 * }
 *
 * public static Direction fromYRot(double rot) {
 *     return from2DDataValue(Mth.floor(rot / 90.0 + 0.5) & 3);
 * }
 *
 * public float toYRot() { return (this.data2d & 3) * 90; }
 * ```
 *
 * ## 三个关键点
 *
 * 1. **轴向旋转对「垂直于该轴的方向」返回自身**。`getClockWise(X)` 对 `EAST`/`WEST`
 *    （它们沿 X 轴）返回 `this`，而不是抛异常——因为绕 X 轴旋转不改变沿 X 轴的朝向。
 *    直接调用 `getClockWiseX()` 会在这些方向上抛 `IllegalStateException`，所以那层守卫
 *    是必需的，不是保险。同理 `getClockWise(Y)` 对 `UP`/`DOWN` 返回自身。
 *
 * 2. **顺时针与逆时针的表互为反函数**。`north→east→south→west→north` 是顺时针环；
 *    `getCounterClockWise` 走反向。两次顺时针等于两次逆时针，四次回到原值。
 *
 * 3. **`fromYRot` 用 `floor(x + 0.5)` 而非四舍五入到最近**，且最后 `& 3` 截到 0..3。
 *    注意 `Mth.floor` 在负数上与非负数的行为（见 `mth_rounding` 能力）。`toYRot` 的
 *    结果是 `{0, 90, 180, 270}`，不含负角度。
 *
 * ## 水平方向的 data2d 对应
 *
 * `SOUTH = 0`、`WEST = 1`、`NORTH = 2`、`EAST = 3`。所以 `toYRot`：`SOUTH→0`、
 * `WEST→90`、`NORTH→180`、`EAST→270`。这个顺序（南为 0 度）容易被想当然写成
 * 「北为 0 度」。
 *
 * 单元格式：
 * - `cw|<dir>` / `ccw|<dir>` —— 水平方向顺时针/逆时针
 * - `cwaxis|<dir>|<axis>` / `ccwaxis|<dir>|<axis>` —— 按轴向旋转
 * - `toyrot|<dir>` —— 水平方向的 Y 轴角度
 * - `fromyrot|<deg>` —— 由 Y 轴角度反推方向
 *
 * TODO: Cubium 的 `util/Direction.hpp` 已有 `getAxis` 等，但**没有** `getClockWise` /
 * `getCounterClockWise` / `toYRot` / `fromYRot`。本能力提供的是待实现的行为基准。
 * 也未覆盖 `Direction.Axis.choose` 与向量运算（`stepX`/`stepY`/`stepZ`），后者是简单表。
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

/// `EnumSet.allOf(Direction.class)` 的顺序：Down、Up、North、South、West、East。
constexpr Dir ALL_DIRS[] = {Dir::Down, Dir::Up, Dir::North, Dir::South, Dir::West, Dir::East};

/**
 * @brief 原版 `Direction.getClockWise()`（绕 Y 轴，仅水平方向合法）。
 */
bool clockWiseY(Dir dir, Dir& out)
{
    switch (dir) {
        case Dir::North:
            out = Dir::East;
            return true;
        case Dir::South:
            out = Dir::West;
            return true;
        case Dir::West:
            out = Dir::North;
            return true;
        case Dir::East:
            out = Dir::South;
            return true;
        default:
            return false;
    }
}

/**
 * @brief 原版 `Direction.getCounterClockWise()`。
 */
bool counterClockWiseY(Dir dir, Dir& out)
{
    switch (dir) {
        case Dir::North:
            out = Dir::West;
            return true;
        case Dir::South:
            out = Dir::East;
            return true;
        case Dir::West:
            out = Dir::South;
            return true;
        case Dir::East:
            out = Dir::North;
            return true;
        default:
            return false;
    }
}

/**
 * @brief 原版私有 `getClockWiseX()`（绕 X 轴，仅沿 Y/Z 的方向合法）。
 */
bool clockWiseX(Dir dir, Dir& out)
{
    switch (dir) {
        case Dir::Down:
            out = Dir::South;
            return true;
        case Dir::Up:
            out = Dir::North;
            return true;
        case Dir::North:
            out = Dir::Down;
            return true;
        case Dir::South:
            out = Dir::Up;
            return true;
        default:
            return false;
    }
}

bool counterClockWiseX(Dir dir, Dir& out)
{
    switch (dir) {
        case Dir::Down:
            out = Dir::North;
            return true;
        case Dir::Up:
            out = Dir::South;
            return true;
        case Dir::North:
            out = Dir::Up;
            return true;
        case Dir::South:
            out = Dir::Down;
            return true;
        default:
            return false;
    }
}

/**
 * @brief 原版私有 `getClockWiseZ()`。
 */
bool clockWiseZ(Dir dir, Dir& out)
{
    switch (dir) {
        case Dir::Down:
            out = Dir::West;
            return true;
        case Dir::Up:
            out = Dir::East;
            return true;
        case Dir::West:
            out = Dir::Up;
            return true;
        case Dir::East:
            out = Dir::Down;
            return true;
        default:
            return false;
    }
}

bool counterClockWiseZ(Dir dir, Dir& out)
{
    switch (dir) {
        case Dir::Down:
            out = Dir::East;
            return true;
        case Dir::Up:
            out = Dir::West;
            return true;
        case Dir::West:
            out = Dir::Down;
            return true;
        case Dir::East:
            out = Dir::Up;
            return true;
        default:
            return false;
    }
}

/**
 * @brief 原版 `getClockWise(Axis)`：垂直方向返回自身。
 */
bool clockWiseAxis(Dir dir, Axis axis, Dir& out)
{
    switch (axis) {
        case Axis::X:
            if (dir == Dir::West || dir == Dir::East) {
                out = dir;
                return true;
            }
            return clockWiseX(dir, out);
        case Axis::Y:
            if (dir == Dir::Up || dir == Dir::Down) {
                out = dir;
                return true;
            }
            return clockWiseY(dir, out);
        case Axis::Z:
            if (dir == Dir::North || dir == Dir::South) {
                out = dir;
                return true;
            }
            return clockWiseZ(dir, out);
    }
    return false;
}

bool counterClockWiseAxis(Dir dir, Axis axis, Dir& out)
{
    switch (axis) {
        case Axis::X:
            if (dir == Dir::West || dir == Dir::East) {
                out = dir;
                return true;
            }
            return counterClockWiseX(dir, out);
        case Axis::Y:
            if (dir == Dir::Up || dir == Dir::Down) {
                out = dir;
                return true;
            }
            return counterClockWiseY(dir, out);
        case Axis::Z:
            if (dir == Dir::North || dir == Dir::South) {
                out = dir;
                return true;
            }
            return counterClockWiseZ(dir, out);
    }
    return false;
}

/**
 * @brief 原版 `toYRot()`：`(data2d & 3) * 90`。
 *
 * `data2d` 只有水平方向有真实语义（SOUTH=0, WEST=1, NORTH=2, EAST=3）。
 * **垂直方向不会抛异常**——`UP`/`DOWN` 的 `data2d` 字段在构造时被赋为 `3`，
 * 因此 `toYRot()` 对它们都返回 `270`。这里必须复刻这一行为，而不是报错。
 */
bool toYRot(Dir dir, i32& out)
{
    switch (dir) {
        case Dir::South:
            out = 0;
            return true;
        case Dir::West:
            out = 90;
            return true;
        case Dir::North:
            out = 180;
            return true;
        case Dir::East:
            out = 270;
            return true;
        case Dir::Up:
        case Dir::Down:
            // 原版 UP/DOWN 的 data2d 为 3，故 (3 & 3) * 90 = 270。
            out = 270;
            return true;
    }
    return false;
}

/**
 * @brief 原版 `Mth.floor`：向零截断后比较。
 */
i32 mthFloor(f64 value)
{
    const i32 truncated = static_cast<i32>(value);
    return value < static_cast<f64>(truncated) ? truncated - 1 : truncated;
}

/**
 * @brief 原版 `from2DDataValue`：0=SOUTH, 1=WEST, 2=NORTH, 3=EAST。
 */
Dir from2DDataValue(i32 value)
{
    switch (value & 3) {
        case 0:
            return Dir::South;
        case 1:
            return Dir::West;
        case 2:
            return Dir::North;
        default:
            return Dir::East;
    }
}

/// 单元表。
constexpr const char* UNITS[] = {
    // 水平顺时针：四个方向各一条
    "cw|north",
    "cw|east",
    "cw|south",
    "cw|west",
    // 水平逆时针
    "ccw|north",
    "ccw|east",
    "ccw|south",
    "ccw|west",
    // 水平旋转对 UP/DOWN 应报错
    "cw|up",
    "cw|down",
    // 按轴向旋转：垂直方向返回自身
    "cwaxis|east|x",
    "cwaxis|west|x",
    "cwaxis|up|y",
    "cwaxis|down|y",
    "cwaxis|north|z",
    "cwaxis|south|z",
    // 按轴向旋转：非法轴会报错
    "cwaxis|up|x",
    "cwaxis|north|y",
    // 按轴向旋转：合法方向
    "cwaxis|up|z",
    "cwaxis|north|x",
    "cwaxis|east|z",
    "ccwaxis|east|x",
    "ccwaxis|up|y",
    "ccwaxis|up|z",
    "ccwaxis|north|x",
    // toYRot：四个水平方向
    "toyrot|south",
    "toyrot|west",
    "toyrot|north",
    "toyrot|east",
    // toYRot：垂直方向应报错
    "toyrot|up",
    "toyrot|down",
    // fromYRot：四个基准角度
    "fromyrot|0",
    "fromyrot|90",
    "fromyrot|180",
    "fromyrot|270",
    // fromYRot：边界（+0.5 后恰好落整）
    "fromyrot|45",
    "fromyrot|44.9",
    "fromyrot|45.1",
    "fromyrot|-45",
    "fromyrot|-44.9",
    // fromYRot：超过一周
    "fromyrot|360",
    "fromyrot|-90",
    "fromyrot|720",
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
    if (parts.empty()) {
        throw std::runtime_error("direction_rotate: 空单元");
    }

    const std::string mode = parts[0];

    if ((mode == "cw" || mode == "ccw") && parts.size() == 2) {
        Dir dir{};
        if (!parseDir(parts[1], dir)) {
            throw std::runtime_error("direction_rotate: 未知方向 " + parts[1]);
        }
        Dir result{};
        const bool ok = (mode == "cw") ? clockWiseY(dir, result) : counterClockWiseY(dir, result);
        if (!ok) {
            return json{{"unit", unit}, {"mode", mode}, {"error", true}}.dump();
        }
        return json{{"unit", unit}, {"mode", mode}, {"result", dirName(result)}}.dump();
    }

    if ((mode == "cwaxis" || mode == "ccwaxis") && parts.size() == 3) {
        Dir dir{};
        Axis axis{};
        if (!parseDir(parts[1], dir) || !parseAxis(parts[2], axis)) {
            throw std::runtime_error("direction_rotate: 无法解析 " + unit);
        }
        Dir result{};
        const bool ok = (mode == "cwaxis") ? clockWiseAxis(dir, axis, result) : counterClockWiseAxis(dir, axis, result);
        if (!ok) {
            return json{{"unit", unit}, {"mode", mode}, {"error", true}}.dump();
        }
        return json{{"unit", unit}, {"mode", mode}, {"result", dirName(result)}}.dump();
    }

    if (mode == "toyrot" && parts.size() == 2) {
        Dir dir{};
        if (!parseDir(parts[1], dir)) {
            throw std::runtime_error("direction_rotate: 未知方向 " + parts[1]);
        }
        i32 rot = 0;
        if (!toYRot(dir, rot)) {
            return json{{"unit", unit}, {"mode", mode}, {"error", true}}.dump();
        }
        return json{{"unit", unit}, {"mode", mode}, {"result", rot}}.dump();
    }

    if (mode == "fromyrot" && parts.size() == 2) {
        const f64 deg = std::stod(parts[1]);
        // 必须阻止编译器把 `deg / 90.0 + 0.5` 改写成倒数乘法：在 `-ffast-math` + `-mavx2`
        // 下，`-45 / 90.0` 会被算成 `-0.50000000000000001908`，加 0.5 得到 `-1.9e-17`，
        // `Mth.floor` 于是返回 -1（`& 3` 后是 3），而原版是精确的 0。
        // 用 volatile 强制除法结果落成独立的 f64，让中间值与原版一致。
        volatile f64 quotient = deg / 90.0;
        const f64 shifted = quotient + 0.5;
        const i32 idx = mthFloor(shifted) & 3;
        return json{{"unit", unit}, {"mode", mode}, {"result", dirName(from2DDataValue(idx))}}.dump();
    }

    throw std::runtime_error("direction_rotate: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(direction_rotate, &writeList, &runLine)

} // namespace mc::diff
