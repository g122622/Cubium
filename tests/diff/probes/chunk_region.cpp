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
 * @brief 差分能力：`ChunkPos` 的区域换算与有效性判定（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.world.level.ChunkPos`：
 *
 * ```java
 * private static final int REGION_BITS = 5;
 * public static final int REGION_SIZE = 32;
 * private static final int REGION_MASK = 31;
 * public static final int REGION_MAX_INDEX = 31;
 *
 * public static final int MAX_COORDINATE_VALUE =
 *     SectionPos.blockToSectionCoord(BlockPos.MAX_HORIZONTAL_COORDINATE) - SAFETY_MARGIN_CHUNKS;
 *
 * public static boolean isValid(int x, int z) {
 *     return Mth.absMax(x, z) <= MAX_COORDINATE_VALUE;
 * }
 *
 * public int getRegionX() { return this.x >> 5; }
 * public int getRegionZ() { return this.z >> 5; }
 * public int getRegionLocalX() { return this.x & 31; }
 * public int getRegionLocalZ() { return this.z & 31; }
 *
 * public static ChunkPos minFromRegion(int x, int z) { return new ChunkPos(x << 5, z << 5); }
 * public static ChunkPos maxFromRegion(int x, int z) {
 *     return new ChunkPos((x << 5) + 31, (z << 5) + 31);
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`isValid` 用的是 `Mth.absMax(x, z)`——两个坐标中绝对值较大的那个。**
 *
 *    ```java
 *    return Mth.absMax(x, z) <= MAX_COORDINATE_VALUE;
 *    ```
 *
 *    **这不是「两个坐标分别都在范围内」**，而是「较大的那个在范围内」。两者在数值上等价
 *    （若 `max(|x|,|z|) <= V`，则 `|x| <= V` 且 `|z| <= V`），但用 `absMax` 更简洁且避免两次比较。
 *
 *    `Mth.absMax(a, b) = Math.max(Math.abs(a), Math.abs(b))`。
 *
 * 2. **`isValid` 未纳入本能力——其阈值依赖 `ChunkPyramid` 的依赖链长度。**
 *
 *    ```java
 *    private static final int SAFETY_MARGIN_CHUNKS =
 *        (32 + ChunkPyramid.GENERATION_PYRAMID.getStepTo(ChunkStatus.FULL)
 *                   .accumulatedDependencies().size() + 1) * 2;
 *    public static final int MAX_COORDINATE_VALUE =
 *        SectionPos.blockToSectionCoord(BlockPos.MAX_HORIZONTAL_COORDINATE) - SAFETY_MARGIN_CHUNKS;
 *    ```
 *
 *    `ChunkStatus` 的依赖链长度是**生成器拓扑的函数**，随版本变化。若要纳入，必须先实现
 *    `ChunkPyramid` 本身——那是「结构生成器」领域的能力，不属于本能力的范围。
 *
 *    **本能力的行为基准是区域换算部分**（`getRegionX` 等），那是纯位运算，可静态验证。
 *
 * 3. **区域换算是算术右移 `>> 5`，对负数向下取整。**
 *
 *    | `x` | `x >> 5` |
 *    |---|---|
 *    | `0` | `0` |
 *    | `31` | `0` |
 *    | `32` | `1` |
 *    | **`-1`** | **`-1`** |
 *    | **`-32`** | **`-1`** |
 *    | **`-33`** | **`-2`** |
 *
 *    **`-1 >> 5 = -1`，不是 `0`**——这是算术右移与整数除法的关键差别。
 *
 * 4. **`getRegionLocalX` 用 `& 31`，对负数得到正的结果。**
 *
 *    | `x` | `x & 31` |
 *    |---|---|
 *    | `-1` | **`31`** |
 *    | `-32` | **`0`** |
 *    | `-33` | **`31`** |
 *
 *    `& 31` 是「取模 32 的非负余数」——`-1 & 31 = 31`，而 `-1 % 32 = -1`。
 *
 * 单元格式：
 * - `regionX|<x>` / `regionZ|<z>` —— `getRegionX` / `getRegionZ`
 * - `localX|<x>` / `localZ|<z>` —— `getRegionLocalX` / `getRegionLocalZ`
 * - `minFromRegion|<rx>|<rz>` —— `minFromRegion`，输出 `"x,z"`
 * - `maxFromRegion|<rx>|<rz>` —— `maxFromRegion`，输出 `"x,z"`
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

/// 原版 `ChunkPos.REGION_BITS`。
constexpr i32 REGION_BITS = 5;
/// 原版 `ChunkPos.REGION_SIZE`。
constexpr i32 REGION_SIZE = 32;
/// 原版 `ChunkPos.REGION_MAX_INDEX`。
constexpr i32 REGION_MAX_INDEX = 31;

/// 原版 `ChunkPos.getRegionX` / `getRegionZ`：算术右移。
i32 regionCoord(i32 value)
{
    return value >> REGION_BITS;
}

/// 原版 `ChunkPos.getRegionLocalX` / `getRegionLocalZ`：按位与。
i32 regionLocal(i32 value)
{
    return value & REGION_MAX_INDEX;
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // regionX：正数
        "regionX|0",
        "regionX|31",
        "regionX|32",
        "regionX|63",
        "regionX|64",
        // regionX：负数（算术右移关键）
        "regionX|-1",
        "regionX|-32",
        "regionX|-33",
        "regionX|-64",
        // regionZ 对称
        "regionZ|32",
        "regionZ|-33",
        // localX：按位与
        "localX|0",
        "localX|31",
        "localX|32",
        "localX|-1",
        "localX|-32",
        "localX|-33",
        // localZ 对称
        "localZ|-1",
        "localZ|33",
        // minFromRegion / maxFromRegion
        "minFromRegion|0|0",
        "minFromRegion|1|-1",
        "minFromRegion|-1|-2",
        "maxFromRegion|0|0",
        "maxFromRegion|1|-1",
        "maxFromRegion|-1|-2",
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
        throw std::runtime_error("chunk_region: 单元至少两段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "regionX" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", regionCoord(static_cast<i32>(std::stoll(parts[1])))}}.dump();
    }
    if (mode == "regionZ" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", regionCoord(static_cast<i32>(std::stoll(parts[1])))}}.dump();
    }
    if (mode == "localX" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", regionLocal(static_cast<i32>(std::stoll(parts[1])))}}.dump();
    }
    if (mode == "localZ" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", regionLocal(static_cast<i32>(std::stoll(parts[1])))}}.dump();
    }
    if (mode == "minFromRegion" && parts.size() == 3) {
        const i32 rx = static_cast<i32>(std::stoll(parts[1]));
        const i32 rz = static_cast<i32>(std::stoll(parts[2]));
        const std::string value = std::to_string(rx << REGION_BITS) + "," + std::to_string(rz << REGION_BITS);
        return json{{"unit", unit}, {"value", value}}.dump();
    }
    if (mode == "maxFromRegion" && parts.size() == 3) {
        const i32 rx = static_cast<i32>(std::stoll(parts[1]));
        const i32 rz = static_cast<i32>(std::stoll(parts[2]));
        const std::string value = std::to_string((rx << REGION_BITS) + REGION_MAX_INDEX) + "," +
            std::to_string((rz << REGION_BITS) + REGION_MAX_INDEX);
        return json{{"unit", unit}, {"value", value}}.dump();
    }
    throw std::runtime_error("chunk_region: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(chunk_region, &writeList, &runLine)

} // namespace mc::diff
