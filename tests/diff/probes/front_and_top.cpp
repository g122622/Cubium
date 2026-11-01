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
 * @brief 差分能力：`FrontAndTop` 的查找表与序列化名（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.core.FrontAndTop`：
 *
 * ```java
 * public enum FrontAndTop {
 *     DOWN_NORTH("down_north", Direction.DOWN, Direction.NORTH),
 *     DOWN_SOUTH("down_south", Direction.DOWN, Direction.SOUTH),
 *     DOWN_WEST("down_west",   Direction.DOWN, Direction.WEST),
 *     UP_EAST("up_east",       Direction.UP,   Direction.EAST),
 *     UP_NORTH("up_north",     Direction.UP,   Direction.NORTH),
 *     UP_SOUTH("up_south",     Direction.UP,   Direction.SOUTH),
 *     UP_WEST("up_west",       Direction.UP,   Direction.WEST),
 *     WEST_UP("west_up",       Direction.WEST, Direction.UP),
 *     EAST_UP("east_up",       Direction.EAST, Direction.UP),
 *     NORTH_UP("north_up",     Direction.NORTH, Direction.UP),
 *     SOUTH_UP("south_up",     Direction.SOUTH, Direction.UP);
 *
 *     private static final FrontAndTop[] BY_TOP_FRONT =
 *         new FrontAndTop[NUM_DIRECTIONS * NUM_DIRECTIONS];
 *     static { for (FrontAndTop v : values()) BY_TOP_FRONT[lookupKey(v.front, v.top)] = v; }
 *
 *     private static int lookupKey(Direction front, Direction top) {
 *         return front.ordinal() * NUM_DIRECTIONS + top.ordinal();
 *     }
 *
 *     public static FrontAndTop fromFrontAndTop(Direction front, Direction top) {
 *         return BY_TOP_FRONT[lookupKey(front, top)];
 *     }
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **构造函数参数顺序是 `(name, front, top)`，但枚举常量名是 `FRONT_TOP` 形式。**
 *
 *    ```java
 *    DOWN_SOUTH("down_south", Direction.DOWN, Direction.SOUTH),
 *    //  name = "down_south"   front = DOWN   top = SOUTH
 *    ```
 *
 *    **`DOWN_SOUTH` 的含义是「front = DOWN，top = SOUTH」**，不是「top = DOWN，front = SOUTH」。
 *    这与直觉相反——`BlockState.getStateForPlacement` 里用 `fromFrontAndTop(facing, up)`
 *    时，第一个参数是朝外的那一面（`front`），第二个是顶面（`top`）。
 *
 * 2. **合法组合只有 12 种，`fromFrontAndTop` 对非法组合返回 `null`，不抛异常。**
 *
 *    查找表大小是 `6 * 6 = 36`，只填了 12 个槽位。未填充的槽位默认 `null`。
 *
 *    例如 `fromFrontAndTop(NORTH, SOUTH)`（同轴）返回 `null`。
 *
 * 3. **`top` 只能取 `UP`、`DOWN`、`NORTH`、`SOUTH`、`WEST`、`EAST` 中的一个，但组合受限。**
 *
 *    观察枚举表：
 *
 *    | `top` | 允许的 `front` |
 *    |---|---|
 *    | `NORTH` | `DOWN` |
 *    | `SOUTH` | `DOWN` |
 *    | `WEST` | `DOWN` |
 *    | `EAST` | `UP` |
 *    | `NORTH` | `UP` |
 *    | `SOUTH` | `UP` |
 *    | `WEST` | `UP` |
 *    | `UP` | `WEST` |
 *    | `UP` | `EAST` |
 *    | `UP` | `NORTH` |
 *    | `UP` | `SOUTH` |
 *
 *    **`top = EAST` 不存在**（`UP_EAST` 是 `front=UP, top=EAST`，等等——
 *    见第 1 条的参数顺序，`UP_EAST("up_east", Direction.UP, Direction.EAST)` 表示
 *    `front=UP, top=EAST`）。
 *
 *    **`top = DOWN` 也完全不存在**——没有任何枚举的 `top` 是 `DOWN`。
 *
 * 4. **`getSerializedName()` 恒等于常量名的小写形式，但不完全等于 `FRONT_TOP` 的拼接。**
 *
 *    12 个名字都是 `front` 与 `top` 的小写拼接，用 `_` 分隔。可以直接由 `front`/`top` 拼出，
 *    但**只在合法组合上成立**。
 *
 * 单元格式：
 * - `serial|<front>|<top>` —— `fromFrontAndTop(...).getSerializedName()`；非法输出 `"<null>"`
 * - `front|<front>|<top>` —— 返回的枚举的 `front()`；非法输出 `"<null>"`
 * - `top|<front>|<top>` —— 返回的枚举的 `top()`；非法输出 `"<null>"`
 *
 * 方向取 `down`/`up`/`north`/`south`/`west`/`east`。
 */
#include <cstdio>
#include <optional>
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

/// 六个方向，索引顺序与原版 `Direction` 枚举一致。
constexpr const char* DIRECTION_NAMES[] = {"down", "up", "north", "south", "west", "east"};

constexpr i32 DIRECTION_COUNT = 6;

/// 原版 `FrontAndTop` 的 12 个合法组合：`{front 索引, top 索引}`。
struct Entry {
    const char* name;
    i32 front;
    i32 top;
};

constexpr Entry ENTRIES[] = {
    {"down_east", 0, 5},  // DOWN_EAST:  front=DOWN,  top=EAST
    {"down_north", 0, 2}, // DOWN_NORTH: front=DOWN,  top=NORTH
    {"down_south", 0, 3}, // DOWN_SOUTH: front=DOWN,  top=SOUTH
    {"down_west", 0, 4},  // DOWN_WEST:  front=DOWN,  top=WEST
    {"up_east", 1, 5},    // UP_EAST:    front=UP,    top=EAST
    {"up_north", 1, 2},   // UP_NORTH:   front=UP,    top=NORTH
    {"up_south", 1, 3},   // UP_SOUTH:   front=UP,    top=SOUTH
    {"up_west", 1, 4},    // UP_WEST:    front=UP,    top=WEST
    {"west_up", 4, 1},    // WEST_UP:    front=WEST,  top=UP
    {"east_up", 5, 1},    // EAST_UP:    front=EAST,  top=UP
    {"north_up", 2, 1},   // NORTH_UP:   front=NORTH, top=UP
    {"south_up", 3, 1},   // SOUTH_UP:   front=SOUTH, top=UP
};

/// 按 `front.ordinal() * 6 + top.ordinal()` 查表；未命中返回 `nullptr`。
const Entry* fromFrontAndTop(i32 front, i32 top)
{
    const i32 key = front * DIRECTION_COUNT + top;
    for (const Entry& entry : ENTRIES) {
        if (entry.front * DIRECTION_COUNT + entry.top == key) {
            return &entry;
        }
    }
    return nullptr;
}

i32 directionIndex(const std::string& name)
{
    for (i32 i = 0; i < DIRECTION_COUNT; ++i) {
        if (name == DIRECTION_NAMES[i]) {
            return i;
        }
    }
    throw std::runtime_error("front_and_top: 未知方向：" + name);
}

void writeList(FILE* out)
{
    // 覆盖全部 36 种组合，再加若干显式命名用例。
    for (i32 front = 0; front < DIRECTION_COUNT; ++front) {
        for (i32 top = 0; top < DIRECTION_COUNT; ++top) {
            std::fprintf(out, "serial|%s|%s\n", DIRECTION_NAMES[front], DIRECTION_NAMES[top]);
        }
    }
    // 显式验证 front()/top() 的返回。
    constexpr const char* EXTRA[] = {
        "front|down|north",
        "front|up|east",
        "front|west|up",
        "top|down|north",
        "top|up|east",
        "top|west|up",
        "front|north|south",
        "top|north|south",
    };
    for (const auto* unit : EXTRA) {
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
    if (parts.size() != 3) {
        throw std::runtime_error("front_and_top: 单元需要 3 段：" + unit);
    }

    const std::string mode = parts[0];
    const i32 front = directionIndex(parts[1]);
    const i32 top = directionIndex(parts[2]);
    const Entry* entry = fromFrontAndTop(front, top);

    if (mode == "serial") {
        return json{{"unit", unit}, {"value", entry ? std::string(entry->name) : "<null>"}}.dump();
    }
    if (mode == "front" || mode == "top") {
        if (!entry) {
            return json{{"unit", unit}, {"value", "<null>"}}.dump();
        }
        const i32 index = mode == "front" ? entry->front : entry->top;
        return json{{"unit", unit}, {"value", DIRECTION_NAMES[index]}}.dump();
    }

    throw std::runtime_error("front_and_top: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE("front_and_top", &writeList, &runLine)

} // namespace mc::diff
