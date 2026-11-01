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
 * @brief 差分能力：`BoundingBox.orientBox` 的结构朝向变换（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": "minX,minY,minZ,maxX,maxY,maxZ" }
 *
 * 复刻原版 `net.minecraft.world.level.levelgen.structure.BoundingBox`：
 *
 * ```java
 * public static BoundingBox orientBox(
 *         int x, int y, int z,                      // 结构原点
 *         int offsetX, int offsetY, int offsetZ,    // 相对偏移
 *         int sizeX, int sizeY, int sizeZ,          // 尺寸
 *         Direction direction) {
 *     switch (direction) {
 *         case SOUTH:
 *         default:
 *             return new BoundingBox(
 *                 x + offsetX,           y + offsetY,           z + offsetZ,
 *                 x + sizeX - 1 + offsetX, y + sizeY - 1 + offsetY, z + sizeZ - 1 + offsetZ);
 *         case NORTH:
 *             return new BoundingBox(
 *                 x + offsetX,           y + offsetY,           z - sizeZ + 1 + offsetZ,
 *                 x + sizeX - 1 + offsetX, y + sizeY - 1 + offsetY, z + offsetZ);
 *         case WEST:
 *             return new BoundingBox(
 *                 x - sizeZ + 1 + offsetZ, y + offsetY,           z + offsetX,
 *                 x + offsetZ,             y + sizeY - 1 + offsetY, z + sizeX - 1 + offsetX);
 *         case EAST:
 *             return new BoundingBox(
 *                 x + offsetZ,             y + offsetY,           z + offsetX,
 *                 x + sizeZ - 1 + offsetZ, y + sizeY - 1 + offsetY, z + sizeX - 1 + offsetX);
 *     }
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`SOUTH`（以及 `UP`/`DOWN`）是「不旋转」的默认分支——X/Z 尺寸原样使用。**
 *
 *    ```java
 *    case SOUTH:
 *    default:
 *    ```
 *
 *    **`Direction.UP` 和 `Direction.DOWN` 也走这个分支**（`switch` 里没有它们的 case）。
 *
 * 2. **只有 `NORTH` 会让 Z 轴「反向」——尺寸加到 `minZ` 之前。**
 *
 *    ```java
 *    z - sizeZ + 1 + offsetZ,   // minZ
 *    z + offsetZ                // maxZ
 *    ```
 *
 *    注意 `minZ` 用的是 `-sizeZ + 1`，`maxZ` 只是 `z + offsetZ`。**两者相差 `sizeZ - 1`**。
 *
 * 3. **`WEST` / `EAST` 会交换 X 与 Z 的角色。**
 *
 *    - `WEST`：`minX` 用 `sizeZ`，`maxZ` 用 `sizeX`
 *    - `EAST`：`maxX` 用 `sizeZ`，`maxZ` 用 `sizeX`
 *
 *    **水平方向上的旋转必然交换 X/Z 尺寸**——因为结构转了 90°。
 *
 * 4. **`WEST` 与 `EAST` 的差别只在 X 轴的哪一端用 `-size + 1`。**
 *
 *    | 方向 | `minX` | `maxX` | `minZ` | `maxZ` |
 *    |---|---|---|---|---|
 *    | `WEST` | `x - sizeZ + 1 + offsetZ` | `x + offsetZ` | `z + offsetX` | `z + sizeX - 1 + offsetX` |
 *    | `EAST` | `x + offsetZ` | `x + sizeZ - 1 + offsetZ` | `z + offsetX` | `z + sizeX - 1 + offsetX` |
 *
 *    即 **`WEST` 在 X 上反向，`EAST` 在 X 上正向**；两者在 Z 上都正向。
 *
 * 单元格式：
 * - `<x>|<y>|<z>|<offsetX>|<offsetY>|<offsetZ>|<sizeX>|<sizeY>|<sizeZ>|<direction>`
 *
 * 方向取 `down`/`up`/`north`/`south`/`west`/`east`。
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

/// 六个方向，索引顺序与原版 `Direction` 枚举一致。
constexpr const char* DIRECTION_NAMES[] = {"down", "up", "north", "south", "west", "east"};

/// 一个整数包围盒（闭区间）。
struct Box {
    i64 minX = 0;
    i64 minY = 0;
    i64 minZ = 0;
    i64 maxX = 0;
    i64 maxY = 0;
    i64 maxZ = 0;
};

i32 directionIndex(const std::string& name)
{
    for (i32 i = 0; i < 6; ++i) {
        if (name == DIRECTION_NAMES[i]) {
            return i;
        }
    }
    throw std::runtime_error("structure_orient: 未知方向：" + name);
}

/// 原版 `BoundingBox.orientBox`。
Box orientBox(
    i64 x, i64 y, i64 z, i64 offsetX, i64 offsetY, i64 offsetZ, i64 sizeX, i64 sizeY, i64 sizeZ, i32 direction)
{
    // 0=down, 1=up, 2=north, 3=south, 4=west, 5=east
    switch (direction) {
        case 2: // NORTH
            return Box{x + offsetX,
                y + offsetY,
                z - sizeZ + 1 + offsetZ,
                x + sizeX - 1 + offsetX,
                y + sizeY - 1 + offsetY,
                z + offsetZ};
        case 4: // WEST
            return Box{x - sizeZ + 1 + offsetZ,
                y + offsetY,
                z + offsetX,
                x + offsetZ,
                y + sizeY - 1 + offsetY,
                z + sizeX - 1 + offsetX};
        case 5: // EAST
            return Box{x + offsetZ,
                y + offsetY,
                z + offsetX,
                x + sizeZ - 1 + offsetZ,
                y + sizeY - 1 + offsetY,
                z + sizeX - 1 + offsetX};
        case 3:  // SOUTH
        default: // SOUTH、UP、DOWN
            return Box{x + offsetX,
                y + offsetY,
                z + offsetZ,
                x + sizeX - 1 + offsetX,
                y + sizeY - 1 + offsetY,
                z + sizeZ - 1 + offsetZ};
    }
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // 原点，无偏移，尺寸 3x2x4，四个水平方向
        "0|0|0|0|0|0|3|2|4|south",
        "0|0|0|0|0|0|3|2|4|north",
        "0|0|0|0|0|0|3|2|4|west",
        "0|0|0|0|0|0|3|2|4|east",
        // UP / DOWN 走默认分支（与 SOUTH 相同）
        "0|0|0|0|0|0|3|2|4|up",
        "0|0|0|0|0|0|3|2|4|down",
        // 带偏移
        "10|64|-20|1|2|3|3|2|4|south",
        "10|64|-20|1|2|3|3|2|4|north",
        "10|64|-20|1|2|3|3|2|4|west",
        "10|64|-20|1|2|3|3|2|4|east",
        // 正方体尺寸（X/Z 交换后不变，验证不掩盖 bug）
        "0|0|0|1|1|1|2|2|2|west",
        "0|0|0|1|1|1|2|2|2|east",
        // 尺寸 1（单方块）
        "5|5|5|0|0|0|1|1|1|north",
        "5|5|5|0|0|0|1|1|1|west",
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
    if (parts.size() != 10) {
        throw std::runtime_error("structure_orient: 单元需要 10 段：" + unit);
    }

    const Box box = orientBox(std::stoll(parts[0]),
        std::stoll(parts[1]),
        std::stoll(parts[2]),
        std::stoll(parts[3]),
        std::stoll(parts[4]),
        std::stoll(parts[5]),
        std::stoll(parts[6]),
        std::stoll(parts[7]),
        std::stoll(parts[8]),
        directionIndex(parts[9]));

    const std::string value = std::to_string(box.minX) + "," + std::to_string(box.minY) + "," +
        std::to_string(box.minZ) + "," + std::to_string(box.maxX) + "," + std::to_string(box.maxY) + "," +
        std::to_string(box.maxZ);
    return json{{"unit", unit}, {"value", value}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("structure_orient", &writeList, &runLine)

} // namespace mc::diff
