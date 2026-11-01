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
 * @brief 差分能力：`Direction` 的数据值与平面判定（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.core.Direction`：
 *
 * ```java
 * // (3D id, 2D id, 名字, 轴方向, 轴, 法向量)
 * DOWN (0, 1, -1, "down",  NEGATIVE, Axis.Y, (0, -1, 0))
 * UP   (1, 0, -1, "up",    POSITIVE, Axis.Y, (0, 1, 0))
 * NORTH(2, 3,  2, "north", NEGATIVE, Axis.Z, (0, 0, -1))
 * SOUTH(3, 2,  0, "south", POSITIVE, Axis.Z, (0, 0, 1))
 * WEST (4, 5,  1, "west",  NEGATIVE, Axis.X, (-1, 0, 0))
 * EAST (5, 4,  3, "east",  POSITIVE, Axis.X, (1, 0, 0))
 *
 * public static Direction from3DDataValue(int id) {
 *     return BY_3D_DATA[Mth.abs(id % BY_3D_DATA.length)];
 * }
 * public static Direction from2DDataValue(int id) {
 *     return BY_2D_DATA[Mth.abs(id % BY_2D_DATA.length)];
 * }
 *
 * public enum Plane {
 *     HORIZONTAL(NORTH, EAST, SOUTH, WEST),   // 轴 X, Z
 *     VERTICAL(UP, DOWN);                     // 轴 Y
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **3D id 与 2D id 是两套不同的编号，且水平方向的顺序相反。**
 *
 *    | 方向 | 3D id | 2D id |
 *    |---|---|---|
 *    | `DOWN` | 0 | **-1**（不属于水平面）|
 *    | `UP` | 1 | **-1** |
 *    | `NORTH` | 2 | **3** |
 *    | `SOUTH` | 3 | **2** |
 *    | `WEST` | 4 | **5** |
 *    | `EAST` | 5 | **4** |
 *
 *    **`NORTH` 的 3D id 是 2、2D id 是 3；`SOUTH` 恰好反过来（3D=3、2D=2）。**
 *    `WEST`/`EAST` 同样相反。这不是笔误——2D id 是按「水平面枚举顺序」`NORTH,EAST,SOUTH,WEST`
 *    的某种历史排布，与 3D 的声明顺序不同。
 *
 *    `DOWN` 与 `UP` 的 2D id 是 **-1**（哨兵值），因为它们不在水平面上。
 *
 * 2. **`from3DDataValue` / `from2DDataValue` 用 `Mth.abs(id % len)`，不是越界取 0。**
 *
 *    ```java
 *    BY_3D_DATA[Mth.abs(id % 6)]
 *    BY_2D_DATA[Mth.abs(id % 4)]
 *    ```
 *
 *    - `from3DDataValue(-1)` → `abs(-1 % 6) = 1` → **`UP`**（不是 `DOWN`！）
 *    - `from3DDataValue(7)`  → `abs(7 % 6) = 1` → `UP`
 *    - `from3DDataValue(6)`  → `abs(6 % 6) = 0` → `DOWN`
 *
 *    **与 [`dye_color`](./dye_color.md)、[`equipment_slot`](./equipment_slot.md)、
 *    [`heightmap_types`](./heightmap_types.md) 的 `ByIdMap.ZERO` 策略完全不同**：那三个越界都返回
 *    第 0 项，这里负数会先取绝对值再取模，落到中间项。
 *
 * 3. **`Axis` 与 `Plane` 的成员顺序都不是「上下、南北、东西」。**
 *
 *    - `Axis` 声明顺序：`X, Y, Z`
 *    - `Plane.HORIZONTAL` 的成员顺序：**`NORTH, EAST, SOUTH, WEST`**（不是 `NORTH,SOUTH,WEST,EAST`）
 *    - `Plane.VERTICAL` 的成员顺序：**`UP, DOWN`**（不是 `DOWN, UP`，与 3D id 顺序相反）
 *
 *    `& 1 == 0` 之类的「偶数即水平」判据在 `HORIZONTAL` 的成员顺序下**不成立**，
 *    必须用 `getAxis().isHorizontal()`。
 *
 * 4. **`getOpposite()` 不是「3D id 加 1 或减 1」。**
 *
 *    | 方向 | 3D id | 反方向 | 反方向 3D id |
 *    |---|---|---|---|
 *    | `DOWN` | 0 | `UP` | 1 |
 *    | `UP` | 1 | `DOWN` | 0 |
 *    | `NORTH` | 2 | `SOUTH` | 3 |
 *    | `SOUTH` | 3 | `NORTH` | 2 |
 *    | `WEST` | 4 | `EAST` | 5 |
 *    | `EAST` | 5 | `WEST` | 4 |
 *
 *    恰好都是 `id ^ 1`（异或 1）——但这是巧合，不能依赖。
 *
 * 单元格式：
 * - `info|<方向>` —— `name,3dId,2dId,axis,axisDirection,nx,ny,nz,isHorizontal,opposite`
 * - `from3d|<id>` / `from2d|<id>` —— 由数据值反查方向
 * - `plane|<HORIZONTAL|VERTICAL>` —— 该平面的成员（逗号分隔，按平面内声明顺序）
 * - `axis|<方向>` —— 该方向的轴名
 *
 * TODO: 未覆盖 `Direction.getNearest`（需要 Vec3 与浮点比较）、`isFacingAngle`（浮点三角函数）、
 * `Direction.rotate` / `getClockWise` / `getCounterClockWise`（已在 `direction_rotate` 覆盖）。
 */
#include <cstdio>
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

/// 一个方向。
struct DirectionInfo {
    const char* name;
    i32 id3d;
    i32 id2d;
    const char* axis;
    const char* axisDirection;
    i32 nx;
    i32 ny;
    i32 nz;
};

/// 原版 6 个方向，按 3D id 升序（即枚举声明顺序）。
constexpr DirectionInfo DIRECTIONS[] = {
    {"DOWN", 0, -1, "Y", "NEGATIVE", 0, -1, 0},
    {"UP", 1, -1, "Y", "POSITIVE", 0, 1, 0},
    {"NORTH", 2, 2, "Z", "NEGATIVE", 0, 0, -1},
    {"SOUTH", 3, 0, "Z", "POSITIVE", 0, 0, 1},
    {"WEST", 4, 1, "X", "NEGATIVE", -1, 0, 0},
    {"EAST", 5, 3, "X", "POSITIVE", 1, 0, 0},
};

constexpr i32 DIRECTION_COUNT = 6;

/// `BY_2D_DATA` 的下标映射：2D id → `DIRECTIONS` 下标。
/// 原版 `BY_2D_DATA`：按 `data2d` 升序，即 2D id 0..3 对应 `SOUTH, WEST, NORTH, EAST`
/// （对应本表下标 3、4、2、5）。
constexpr i32 BY_2D_DATA[] = {3, 4, 2, 5};

constexpr i32 BY_2D_LENGTH = 4;

/// 原版 `Mth.abs`：注意 `abs(Integer.MIN_VALUE)` 仍是 `Integer.MIN_VALUE`。
i32 mthAbs(i32 value)
{
    return value < 0 ? static_cast<i32>(0u - static_cast<u32>(value)) : value;
}

/// 原版 `from3DDataValue`：`BY_3D_DATA[Mth.abs(id % 6)]`。
const DirectionInfo* from3D(i32 id, bool& valid)
{
    const i32 index = mthAbs(id % DIRECTION_COUNT);
    valid = index >= 0 && index < DIRECTION_COUNT;
    if (!valid) {
        return nullptr;
    }
    return &DIRECTIONS[index];
}

/// 原版 `from2DDataValue`：`BY_2D_DATA[Mth.abs(id % 4)]`。
const DirectionInfo* from2D(i32 id, bool& valid)
{
    const i32 index = mthAbs(id % BY_2D_LENGTH);
    valid = index >= 0 && index < BY_2D_LENGTH;
    if (!valid) {
        return nullptr;
    }
    return &DIRECTIONS[BY_2D_DATA[index]];
}

/// 方向名 → 方向。
const DirectionInfo* findDirection(const std::string& name)
{
    for (const auto& dir : DIRECTIONS) {
        if (name == dir.name) {
            return &dir;
        }
    }
    return nullptr;
}

/// 反方向。
const DirectionInfo* opposite(const DirectionInfo& dir)
{
    return &DIRECTIONS[dir.id3d ^ 1];
}

/// 平面成员（按原版 `Plane` 的声明顺序）。
std::vector<const char*> planeMembers(const std::string& plane)
{
    if (plane == "HORIZONTAL") {
        return {"NORTH", "EAST", "SOUTH", "WEST"};
    }
    if (plane == "VERTICAL") {
        return {"UP", "DOWN"};
    }
    throw std::runtime_error("direction_plane: 未知平面：" + plane);
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // info：全部 6 个方向
        "info|DOWN",
        "info|UP",
        "info|NORTH",
        "info|SOUTH",
        "info|WEST",
        "info|EAST",
        // plane
        "plane|HORIZONTAL",
        "plane|VERTICAL",
        // from3d：连续与越界
        "from3d|0",
        "from3d|1",
        "from3d|2",
        "from3d|5",
        "from3d|6",
        "from3d|7",
        "from3d|-1",
        "from3d|-2",
        "from3d|12",
        // from2d
        "from2d|0",
        "from2d|1",
        "from2d|2",
        "from2d|3",
        "from2d|4",
        "from2d|-1",
        "from2d|-2",
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
        throw std::runtime_error("direction_plane: 单元至少两段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "info" && parts.size() == 2) {
        const DirectionInfo* dir = findDirection(parts[1]);
        if (dir == nullptr) {
            return json{{"unit", unit}, {"value", "null"}}.dump();
        }
        const std::string value = std::string(dir->name) + "," + std::to_string(dir->id3d) + "," +
            std::to_string(dir->id2d) + "," + dir->axis + "," + dir->axisDirection + "," + std::to_string(dir->nx) +
            "," + std::to_string(dir->ny) + "," + std::to_string(dir->nz) + "," +
            (std::string(dir->axis) != "Y" ? "true" : "false") + "," + opposite(*dir)->name;
        return json{{"unit", unit}, {"value", value}}.dump();
    }
    if (mode == "from3d" && parts.size() == 2) {
        bool valid = false;
        const DirectionInfo* dir = from3D(std::stoi(parts[1]), valid);
        return json{{"unit", unit}, {"value", dir == nullptr ? "null" : dir->name}}.dump();
    }
    if (mode == "from2d" && parts.size() == 2) {
        bool valid = false;
        const DirectionInfo* dir = from2D(std::stoi(parts[1]), valid);
        return json{{"unit", unit}, {"value", dir == nullptr ? "null" : dir->name}}.dump();
    }
    if (mode == "plane" && parts.size() == 2) {
        const auto members = planeMembers(parts[1]);
        std::string value;
        for (const auto* name : members) {
            if (!value.empty()) {
                value += ",";
            }
            value += name;
        }
        return json{{"unit", unit}, {"value", value}}.dump();
    }
    if (mode == "axis" && parts.size() == 2) {
        const DirectionInfo* dir = findDirection(parts[1]);
        if (dir == nullptr) {
            return json{{"unit", unit}, {"value", "null"}}.dump();
        }
        return json{{"unit", unit}, {"value", std::string(dir->axis)}}.dump();
    }

    throw std::runtime_error("direction_plane: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE("direction_plane", &writeList, &runLine)

} // namespace mc::diff
