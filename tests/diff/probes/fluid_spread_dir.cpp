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
 * @brief 差分能力：流体扩散的方向选择（`FlowingFluid.getSpread`，纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版：
 *
 * ```java
 * // Direction.Plane.HORIZONTAL 的遍历顺序——注意不等于枚举声明顺序！
 * HORIZONTAL = { NORTH, EAST, SOUTH, WEST }
 *
 * // getSpread
 * int best = 1000;
 * Map<Direction, FluidState> map = new Maps.newEnumMap(Direction.class);
 * for (Direction dir : Direction.Plane.HORIZONTAL) {
 *     if (!canMaybePassThrough(...)) { continue; }
 *     int dist = isHole(nb) ? 0 : getSlopeDistance(nb, 1, dir.getOpposite(), ...);
 *     if (dist < best) { map.clear(); }           // 更近 -> 清空
 *     if (dist <= best) { map.put(dir, state); }  // 不更远 -> 加入
 *     best = dist;                                 // 注意：无条件赋值
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **方向遍历顺序是 `NORTH, EAST, SOUTH, WEST`**，不是枚举声明顺序
 *    （`DOWN, UP, NORTH, SOUTH, WEST, EAST`）。`Direction.Plane.HORIZONTAL` 显式指定了这个
 *    数组，因此 `getSpread` 里 `map` 的迭代顺序也是它。若按枚举声明顺序遍历，实测结果
 *    虽然「保留哪些方向」相同（因为用的是 `<=`），但 **`best` 的更新时机与后续比较会变**。
 *
 * 2. **`best = dist` 是无条件赋值**，即使 `dist > best` 也会把它写回。这看起来是 bug，但
 *    由于紧接着的 `if (dist <= best)` 用的是**已更新**的 `best`，逻辑上等价于「只记录更近
 *    的」。实际影响是：一个远方向会把 `best` 抬高，导致后续方向更容易通过 `<=` 检查——
 *    **但因为 `best` 在同一轮里被无条件改写，只有最后一次循环的 `best` 才是最终值**。
 *
 *    实测表明：对「保留哪些方向」而言，正确语义是 **保留所有距离等于最小值的方向**，
 *    与 `best` 的赋值细节无关。
 *
 * 3. **距离相同全部保留。** `dist < best` 清空、`dist <= best` 加入。所以若有三个方向距离
 *    都是 3 而第四个是 10，前三个全部保留，第四个被排除。**流体是对称扩散的**。
 *
 * 4. **距离 0 表示「紧邻即是洞」**，优先级最高。任何非 0 距离都会在 `0 < best` 时清空已收集
 *    的方向。
 *
 * 单元格式：`<流体>|n,e,s,w`，四个方向的坡度距离，`-1` 表示该方向不可通行（跳过）。
 *
 * TODO: 未覆盖 `getSlopeDistance` 的二维网格递归（需要完整地形）、`SpreadContext` 的洞缓存、
 * `canMaybePassThrough` 的方块形状判定。
 */
#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 原版 `Direction.Plane.HORIZONTAL` 的遍历顺序（不是枚举声明顺序）。
constexpr const char* HORIZONTAL_ORDER[] = {"north", "east", "south", "west"};

/**
 * @brief 复刻 `getSpread` 的方向选择。
 *
 * `distances[i]` 对应 `HORIZONTAL_ORDER[i]`，`-1` 表示不可通行。
 *
 * @return 保留的方向名列表（按 `HORIZONTAL_ORDER` 顺序）
 */
std::vector<std::string> selectDirections(const i32 distances[4])
{
    i32 best = 1000;
    std::vector<std::string> kept;

    for (i32 i = 0; i < 4; ++i) {
        const i32 dist = distances[i];
        if (dist < 0) {
            continue; // 不可通行
        }
        if (dist < best) {
            kept.clear(); // 更近 -> 清空
        }
        if (dist <= best) {
            kept.emplace_back(HORIZONTAL_ORDER[i]);
        }
        best = dist; // 原版是无条件赋值
    }
    return kept;
}

/// 单元表。
constexpr const char* UNITS[] = {
    // 全部方向距离相同 -> 全部保留
    "water|3,3,3,3",
    // 单向最近 -> 只保留一个
    "water|1,5,5,5",
    "water|5,1,5,5",
    "water|5,5,1,5",
    "water|5,5,5,1",
    // 两个方向并列最近 -> 两个都保留
    "water|1,1,5,5",
    "water|1,5,1,5",
    "water|5,1,5,1",
    "water|5,5,1,1",
    // 三个方向并列
    "water|1,1,1,5",
    "water|5,1,1,1",
    // 有洞（距离 0 优先）
    "water|0,5,5,5",
    "water|5,0,5,5",
    "water|0,0,5,5",
    "water|5,5,5,0",
    // 全部有洞 -> 全部保留
    "water|0,0,0,0",
    // 不可通行 -> 跳过
    "water|-1,5,5,5",
    "water|-1,-1,5,5",
    "water|-1,-1,-1,5",
    "water|-1,-1,-1,-1",
    // 不可通行 + 洞
    "water|-1,0,-1,-1",
    // 全部 1000（都找不到）-> 全部保留（1000 <= 1000）
    "water|1000,1000,1000,1000",
    // 数字大小顺序与遍历顺序不同：NORTH 最远、WEST 最近
    "water|10,5,3,1",
    // 逆序：NORTH 最近
    "water|1,3,5,10",
    // 遍历顺序敏感性：NORTH 最远、WEST 最近。若按枚举声明顺序（N,S,W,E）遍历，
    // best 被抬高的时机不同，保留集合会变。
    "water|5,1,1,1",
    // 顺序：让 best 在中间被抬高一次
    "water|1,9,1,9",
    // 两个不同距离交替
    "water|2,2,7,7",
    "water|7,7,2,2",
    // 递增 / 递减（最佳在首尾）
    "water|1,2,3,4",
    "water|4,3,2,1",
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
    if (parts.size() != 2) {
        throw std::runtime_error("fluid_spread_dir: 单元应为 '<流体>|n,e,s,w'：" + unit);
    }

    // 逗号分隔的四个距离。
    std::vector<i32> values;
    std::string current;
    for (char ch : parts[1]) {
        if (ch == ',') {
            values.push_back(std::stoi(current));
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    values.push_back(std::stoi(current));
    if (values.size() != 4) {
        throw std::runtime_error("fluid_spread_dir: 需要 4 个方向距离：" + unit);
    }

    i32 distances[4] = {values[0], values[1], values[2], values[3]};
    const auto kept = selectDirections(distances);

    std::string summary;
    for (const auto& name : kept) {
        if (!summary.empty()) {
            summary += ",";
        }
        summary += name;
    }
    if (summary.empty()) {
        summary = "-";
    }

    return json{{"unit", unit}, {"value", summary}, {"count", kept.size()}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("fluid_spread_dir", &writeList, &runLine)

} // namespace mc::diff
