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
 * @brief 差分能力：`BlockPos.spiralAround` 的螺旋遍历顺序（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": "<pos>;<pos>;..." }
 *
 * 复刻原版 `net.minecraft.core.BlockPos`：
 *
 * ```java
 * public static Iterable<BlockPos.MutableBlockPos> spiralAround(
 *         BlockPos center, int radius, Direction first, Direction second) {
 *     Validate.validState(first.getAxis() != second.getAxis(),
 *         "The two directions cannot be on the same axis");
 *
 *     Direction[] directions = { first, second, first.getOpposite(), second.getOpposite() };
 *     MutableBlockPos cursor = center.mutable().move(second);   // 先向 second 走一格
 *     int legs = 4 * radius;
 *     int leg = -1;
 *     int legSize = 0;
 *     int legIndex = 0;
 *     int lastX = cursor.getX(), lastY = cursor.getY(), lastZ = cursor.getZ();
 *
 *     computeNext() {
 *         cursor.set(lastX, lastY, lastZ).move(directions[(leg + 4) % 4]);
 *         lastX = cursor.x; lastY = cursor.y; lastZ = cursor.z;
 *         if (legIndex >= legSize) {
 *             if (leg >= legs) return endOfData();
 *             leg++;
 *             legIndex = 0;
 *             legSize = leg / 2 + 1;
 *         }
 *         legIndex++;
 *         return cursor;
 *     }
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **起始位置先向 `second` 方向偏移一格，第一个产出点不是中心。**
 *
 *    ```java
 *    MutableBlockPos cursor = center.mutable().move(second);
 *    ```
 *
 *    但注意 `computeNext` 的第一件事是 `cursor.set(lastX,...).move(directions[(leg+4)%4])`，
 *    而 `leg = -1`，所以 `(leg+4)%4 = 3` → **`directions[3] = second.getOpposite()`**。
 *
 *    第一次移动实际是「从 `center + second` 再向 `-second` 走一格」，**回到中心**。
 *
 *    **所以第一个产出点就是中心本身。**
 *
 * 2. **段长度序列是 `leg/2 + 1`，即 1、1、2、2、3、3、4……——不是 1、2、3、4。**
 *
 *    | `leg` | `legSize = leg/2 + 1` |
 *    |---|---|
 *    | 0 | **1** |
 *    | 1 | **1** |
 *    | 2 | **2** |
 *    | 3 | **2** |
 *    | 4 | 3 |
 *    | 5 | 3 |
 *
 *    每两段长度相同，这是螺旋线「对角线扩展」的典型特征。
 *
 * 3. **方向循环是 `first, second, -first, -second`，不是 `first, second, -second, -first`。**
 *
 *    ```java
 *    Direction[] directions = { first, second, first.getOpposite(), second.getOpposite() };
 *    ```
 *
 *    若把第三、四个元素写成 `-second, -first`，会得到镜像的螺旋（逆时针 vs 顺时针）。
 *
 * 4. **终止条件是 `leg >= legs`，`legs = 4 * radius`——输出的点数与从「半径」的直觉不同。**
 *
 *    当 `radius = 0` 时 `legs = 0`，第一次调用时 `legIndex(0) >= legSize(0)` 成立，
 *    且 `leg(-1) >= legs(0)` 为假 → `leg` 变 0，`legSize` 变 1，产出 1 个点。
 *    第二次调用时 `legIndex(1) >= legSize(1)` 成立，`leg(0) >= legs(0)` 为真 → 结束。
 *
 *    **`radius = 0` 只产出中心 1 个点。**
 *
 * 单元格式：
 * - `<radius>|<centerX>|<centerY>|<centerZ>|<first>|<second>`
 *
 * 方向取 `down`/`up`/`north`/`south`/`west`/`east`。
 * 输出为 `x,y,z` 用 `;` 连接。
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

/// 六个方向的单位偏移，索引顺序与原版 `Direction` 枚举一致。
constexpr const char* DIRECTION_NAMES[] = {"down", "up", "north", "south", "west", "east"};

constexpr i32 DIRECTION_DELTAS[6][3] = {
    {0, -1, 0}, // down
    {0, 1, 0},  // up
    {0, 0, -1}, // north
    {0, 0, 1},  // south
    {-1, 0, 0}, // west
    {1, 0, 0},  // east
};

/// 方向的轴：0 = X, 1 = Y, 2 = Z。
constexpr i32 DIRECTION_AXIS[6] = {1, 1, 2, 2, 0, 0};

/// 方向索引 → 相反方向索引。
constexpr i32 DIRECTION_OPPOSITE[6] = {1, 0, 3, 2, 5, 4};

i32 directionIndex(const std::string& name)
{
    for (i32 i = 0; i < 6; ++i) {
        if (name == DIRECTION_NAMES[i]) {
            return i;
        }
    }
    throw std::runtime_error("blockpos_spiral: 未知方向：" + name);
}

/// 原版 `BlockPos.spiralAround` 的遍历顺序。
std::string spiralAround(i32 centerX, i32 centerY, i32 centerZ, i32 radius, i32 first, i32 second)
{
    if (DIRECTION_AXIS[first] == DIRECTION_AXIS[second]) {
        throw std::runtime_error("blockpos_spiral: 两个方向不能在同一轴上");
    }

    const i32 order[4] = {first, second, DIRECTION_OPPOSITE[first], DIRECTION_OPPOSITE[second]};
    const i32 legs = 4 * radius;

    // 起点：center + second。
    i32 lastX = centerX + DIRECTION_DELTAS[second][0];
    i32 lastY = centerY + DIRECTION_DELTAS[second][1];
    i32 lastZ = centerZ + DIRECTION_DELTAS[second][2];

    i32 leg = -1;
    i32 legSize = 0;
    i32 legIndex = 0;

    std::string out;
    while (true) {
        const i32 direction = order[(leg + 4) % 4];
        const i32 curX = lastX + DIRECTION_DELTAS[direction][0];
        const i32 curY = lastY + DIRECTION_DELTAS[direction][1];
        const i32 curZ = lastZ + DIRECTION_DELTAS[direction][2];
        lastX = curX;
        lastY = curY;
        lastZ = curZ;

        if (legIndex >= legSize) {
            if (leg >= legs) {
                break;
            }
            ++leg;
            legIndex = 0;
            legSize = leg / 2 + 1;
        }
        ++legIndex;

        if (!out.empty()) {
            out.push_back(';');
        }
        out += std::to_string(curX);
        out.push_back(',');
        out += std::to_string(curY);
        out.push_back(',');
        out += std::to_string(curZ);
    }
    return out;
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // radius 0：只产出中心
        "0|0|0|0|east|south",
        // radius 1~3
        "1|0|0|0|east|south",
        "2|0|0|0|east|south",
        "3|0|0|0|east|south",
        // 换方向组合
        "1|0|0|0|north|up",
        "2|0|0|0|north|up",
        // 非原点中心
        "1|10|64|-20|east|south",
        // 反向方向顺序（first/second 互换）
        "1|0|0|0|south|east",
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
    if (parts.size() != 6) {
        throw std::runtime_error("blockpos_spiral: 单元需要 6 段：" + unit);
    }

    const std::string result = spiralAround(std::stoi(parts[1]),
        std::stoi(parts[2]),
        std::stoi(parts[3]),
        std::stoi(parts[0]),
        directionIndex(parts[4]),
        directionIndex(parts[5]));
    return json{{"unit", unit}, {"value", result}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(blockpos_spiral, &writeList, &runLine)

} // namespace mc::diff
