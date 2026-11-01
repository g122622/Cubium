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
 * @brief 差分能力：`BlockPos.betweenClosed` 的迭代顺序（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": "<pos>;<pos>;..." }
 *
 * 复刻原版 `net.minecraft.core.BlockPos`：
 *
 * ```java
 * public static Iterable<BlockPos> betweenClosed(
 *         int minX, int minY, int minZ, int maxX, int maxY, int maxZ) {
 *     int i = maxX - minX + 1;      // x 跨度
 *     int j = maxY - minY + 1;      // y 跨度
 *     int k = maxZ - minZ + 1;      // z 跨度
 *     int l = i * j * k;            // 总数
 *     return () -> new AbstractIterator<BlockPos>() {
 *         private int index;
 *         protected BlockPos computeNext() {
 *             if (index == l) return endOfData();
 *             int i1 = index % i;               // x 偏移
 *             int j1 = index / i;
 *             int k1 = j1 % j;                  // y 偏移
 *             int l1 = j1 / j;                  // z 偏移
 *             index++;
 *             return cursor.set(minX + i1, minY + k1, minZ + l1);
 *         }
 *     };
 * }
 *
 * public static Iterable<BlockPos> betweenClosed(BlockPos a, BlockPos b) {
 *     return betweenClosed(min(a.x,b.x), min(a.y,b.y), min(a.z,b.z),
 *                          max(a.x,b.x), max(a.y,b.y), max(a.z,b.z));
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **迭代顺序是 x 最快、z 最慢（x → y → z）。**
 *
 *    索引分解是：
 *    - `x` 偏移 = `index % i`
 *    - `y` 偏移 = `(index / i) % j`
 *    - `z` 偏移 = `index / (i * j)`
 *
 *    所以对 `(0,0,0)..(2,1,1)`（`i=3, j=2, k=2`，共 12 个），顺序是：
 *    `(0,0,0) (1,0,0) (2,0,0) (0,1,0) (1,1,0) (2,1,0) (0,0,1) ...`
 *
 *    **这与「先 z 后 x」的直觉相反**——MC 的区块内遍历通常是 x 最快。
 *
 * 2. **`betweenClosed(BlockPos, BlockPos)` 会自动归一化端点顺序。**
 *
 *    ```java
 *     Math.min(a.x, b.x), ... , Math.max(a.z, b.z)
 *    ```
 *
 *    所以 `betweenClosed((5,5,5), (0,0,0))` 与 `betweenClosed((0,0,0), (5,5,5))` **完全等价**。
 *
 * 3. **`betweenClosed` 是「闭区间」，两端都包含。**
 *
 *    跨度是 `max - min + 1`，总数 `i * j * k`——单点区间 `i = j = k = 1` 也迭代 1 次。
 *
 * 4. **六个参数版本不做任何归一化；`min > max` 时是无限迭代，不是空结果。**
 *
 *    `betweenClosed(int, int, int, int, int, int)` 假定 `min <= max`。
 *
 *    - 若某个跨度恰为 `0`（如 `minX = maxX + 1`），总数 `l = 0`，迭代器**立刻结束**。
 *    - 若**所有三个跨度都为负**（如 `(5,5,5)..(0,0,0)` 得 `(-5,-5,-5)`），乘积 `l = -125`。
 *      迭代器的终止条件是 `index == l`，而 `index` 从 `0` 递增永远不会等于负数 →
 *      **无限迭代直到 OOM**。
 *
 *    **这是原版的真实缺陷**，无法作为差分用例（会让 oracle 进程崩溃）。
 *
 * 单元格式：
 * - `iter|<minX>|<minY>|<minZ>|<maxX>|<maxY>|<maxZ>` —— 六参版本，输出全部坐标
 * - `pair|<ax>|<ay>|<az>|<bx>|<by>|<bz>` —— 两参版本（自动归一化）
 *
 * 输出格式：`x,y,z` 用 `;` 连接；空结果输出空串。
 * 数量上限由用例保证（最大 27 个），避免输出过长。
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

/// 原版 `BlockPos.betweenClosed(int, int, int, int, int, int)` 的迭代顺序。
std::string betweenClosed(i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ)
{
    const i64 spanX = static_cast<i64>(maxX) - static_cast<i64>(minX) + 1;
    const i64 spanY = static_cast<i64>(maxY) - static_cast<i64>(minY) + 1;
    const i64 spanZ = static_cast<i64>(maxZ) - static_cast<i64>(minZ) + 1;
    const i64 total = spanX * spanY * spanZ;

    std::string out;
    for (i64 index = 0; index < total; ++index) {
        const i64 offsetX = index % spanX;
        const i64 rest = index / spanX;
        const i64 offsetY = rest % spanY;
        const i64 offsetZ = rest / spanY;
        if (!out.empty()) {
            out.push_back(';');
        }
        out += std::to_string(static_cast<i64>(minX) + offsetX);
        out.push_back(',');
        out += std::to_string(static_cast<i64>(minY) + offsetY);
        out.push_back(',');
        out += std::to_string(static_cast<i64>(minZ) + offsetZ);
    }
    return out;
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // iter：单点
        "iter|0|0|0|0|0|0",
        // iter：x 方向（验证 x 最快）
        "iter|0|0|0|2|0|0",
        // iter：y 方向
        "iter|0|0|0|0|2|0",
        // iter：z 方向
        "iter|0|0|0|0|0|2",
        // iter：二维与三维（关键顺序）
        "iter|0|0|0|2|1|1",
        "iter|0|0|0|1|1|1",
        // iter：负坐标起点
        "iter|-1|-1|-1|0|0|0",
        // 注意：min > max 时原版会无限迭代（跨度乘积为负，index == l 永不成立），
        // 会导致 OOM，无法作为差分用例。见文档「未覆盖范围」。
        // pair：自动归一化
        "pair|0|0|0|2|1|1",
        "pair|2|1|1|0|0|0",
        "pair|5|5|5|5|5|5",
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
    if (parts.size() != 7) {
        throw std::runtime_error("blockpos_iterate: 单元需要 7 段：" + unit);
    }

    const std::string mode = parts[0];
    i32 a[3];
    i32 b[3];
    for (i32 i = 0; i < 3; ++i) {
        a[i] = std::stoi(parts[1 + i]);
        b[i] = std::stoi(parts[4 + i]);
    }

    if (mode == "iter") {
        return json{{"unit", unit}, {"value", betweenClosed(a[0], a[1], a[2], b[0], b[1], b[2])}}.dump();
    }
    if (mode == "pair") {
        // 两参版本：端点自动归一化。
        const i32 minX = a[0] < b[0] ? a[0] : b[0];
        const i32 minY = a[1] < b[1] ? a[1] : b[1];
        const i32 minZ = a[2] < b[2] ? a[2] : b[2];
        const i32 maxX = a[0] > b[0] ? a[0] : b[0];
        const i32 maxY = a[1] > b[1] ? a[1] : b[1];
        const i32 maxZ = a[2] > b[2] ? a[2] : b[2];
        return json{{"unit", unit}, {"value", betweenClosed(minX, minY, minZ, maxX, maxY, maxZ)}}.dump();
    }

    throw std::runtime_error("blockpos_iterate: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE("blockpos_iterate", &writeList, &runLine)

} // namespace mc::diff
