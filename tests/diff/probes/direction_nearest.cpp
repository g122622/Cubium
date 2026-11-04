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
 * @brief 差分能力：`Direction.getApproximateNearest` 与 `Direction.getNearest`（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": "<direction>" }
 *
 * 复刻原版 `net.minecraft.core.Direction`：
 *
 * ```java
 * public static Direction getApproximateNearest(double x, double y, double z) {
 *     return getApproximateNearest((float) x, (float) y, (float) z);   // 先降精度
 * }
 *
 * public static Direction getApproximateNearest(float x, float y, float z) {
 *     Direction direction = NORTH;              // 默认 NORTH
 *     float f = Float.MIN_VALUE;                // 不是 -Infinity！
 *     for (Direction candidate : VALUES) {      // 按 data3d 排序：DOWN,UP,NORTH,SOUTH,WEST,EAST
 *         float dot = x * c.normal.getX() + y * c.normal.getY() + z * c.normal.getZ();
 *         if (dot > f) {                        // 严格大于
 *             f = dot;
 *             direction = candidate;
 *         }
 *     }
 *     return direction;
 * }
 *
 * public static @Nullable Direction getNearest(int x, int y, int z, @Nullable Direction fallback) {
 *     int i = Math.abs(x), j = Math.abs(y), k = Math.abs(z);
 *     if (i > k && i > j)      return x < 0 ? WEST : EAST;
 *     else if (k > i && k > j) return z < 0 ? NORTH : SOUTH;
 *     else if (j > i && j > k) return y < 0 ? DOWN : UP;
 *     else                     return fallback;   // 平局回退
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`getApproximateNearest` 的初始比较基准是 `Float.MIN_VALUE`（约 `1.4e-45`），不是
 *    `-Infinity`。**
 *
 *    `Float.MIN_VALUE` 是**最小正规格化正数**，不是最负的数（那是 `-Float.MAX_VALUE`）。
 *
 *    因此**当所有点积都 `<= Float.MIN_VALUE` 时，循环一次都不更新，返回默认值 `NORTH`**。
 *
 *    - 全零向量 `(0,0,0)`：所有点积都是 `0`，`0 > 1.4e-45` 为假 → **返回 `NORTH`**
 *    - 负方向 `(0,0,-1)`：`NORTH` 的点积是 `1`，会被选中
 *    - **`(0,-1,0)`（正下方）**：`DOWN` 的点积是 `1`，被选中
 *
 *    注意 `(0,0,-1)` 与 `(0,0,1)` 分别得 `NORTH` / `SOUTH`，但**全零向量也返回 `NORTH`**——
 *    无法从结果反推「是否为零向量」。
 *
 * 2. **平局由 `VALUES` 的枚举顺序决定。**
 *
 *    `VALUES = values()`，而枚举声明顺序是 `DOWN, UP, NORTH, SOUTH, WEST, EAST`。
 *    循环用**严格** `>` 比较，所以**先遍历到的胜出**。
 *
 *    - `(1,1,0)`：`EAST` 与 `UP` 的点积都是 `1`。`UP` (索引 1) 先于 `EAST` (索引 5)，
 *      但 `UP` 的点积来自 `y=1`，`EAST` 来自 `x=1`，**两者都等于 1**。
 *      遍历顺序：`DOWN`(0) → `UP`(1, dot=1, 更新) → `NORTH`(0) → `SOUTH`(0) → `WEST`(0) → `EAST`(1, 不 >1)
 *      → **返回 `UP`**
 *
 * 3. **`getApproximateNearest(double,...)` 先把参数降成 `float`。**
 *
 *    ```java
 *    return getApproximateNearest((float) x, (float) y, (float) z);
 *    ```
 *
 *    因此 `double` 版**没有更高精度**——`getApproximateNearest(1e-300)` 会降成 `0.0f`。
 *    两版行为完全一致，只是入口不同。
 *
 * 4. **`getNearest` 用严格 `>` 且平局回退到 `fallback`。**
 *
 *    ```java
 *    if (i > k && i > j) ... else return fallback;
 *    ```
 *
 *    **三个坐标绝对值相等时（包括全零）一律返回 `fallback`**，不做任何启发式。
 *
 *    返回 `null` 也是合法的（`fallback = null` 时），但本能力用 `"null"` 字符串表示。
 *
 * 单元格式：
 * - `approxF|<x>|<y>|<z>` —— `getApproximateNearest(float,float,float)`
 * - `approxD|<x>|<y>|<z>` —— `getApproximateNearest(double,double,double)`
 * - `nearest|<x>|<y>|<z>|<fallback>` —— `getNearest`，`fallback` 取 `north`/`east`/`null` 等
 *
 * 方向输出为**小写枚举名**（`down`/`up`/`north`/`south`/`west`/`east`），`null` 输出 `"null"`。
 *
 * TODO: 未覆盖 `Direction.orderedByNearest`（需要 `Entity`）、`Direction.rotate`（需要矩阵）、
 * `Direction.axisStepOrder`（返回 `ImmutableList<Axis>`）、`Direction.getFacingAxis`。
 */
#include <cmath>
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

/// 六个方向，按原版 `VALUES = values()` 的枚举声明顺序。
constexpr const char* DIRECTIONS[] = {"down", "up", "north", "south", "west", "east"};

/// 各方向的单位法向量（与 `DIRECTIONS` 一一对应）。
constexpr f32 NORMALS[6][3] = {
    {0.0F, -1.0F, 0.0F}, // down
    {0.0F, 1.0F, 0.0F},  // up
    {0.0F, 0.0F, -1.0F}, // north
    {0.0F, 0.0F, 1.0F},  // south
    {-1.0F, 0.0F, 0.0F}, // west
    {1.0F, 0.0F, 0.0F},  // east
};

/// 原版 `Direction.getApproximateNearest(float, float, float)`。
std::string getApproximateNearest(f32 x, f32 y, f32 z)
{
    // 原版初值：Direction.NORTH 与 Float.MIN_VALUE（最小正规格化正数）。
    i32 best = 2;
    f32 bestDot = 1.401298464324817E-45F;
    for (i32 i = 0; i < 6; ++i) {
        const f32 dot = x * NORMALS[i][0] + y * NORMALS[i][1] + z * NORMALS[i][2];
        if (dot > bestDot) {
            bestDot = dot;
            best = i;
        }
    }
    return DIRECTIONS[best];
}

/// 原版 `Direction.getNearest(int, int, int, Direction)`：平局返回 `fallback`。
std::string getNearest(i32 x, i32 y, i32 z, const std::optional<std::string>& fallback)
{
    const i32 i = std::abs(x);
    const i32 j = std::abs(y);
    const i32 k = std::abs(z);
    if (i > k && i > j) {
        return x < 0 ? "west" : "east";
    }
    if (k > i && k > j) {
        return z < 0 ? "north" : "south";
    }
    if (j > i && j > k) {
        return y < 0 ? "down" : "up";
    }
    return fallback.value_or("null");
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // approxF：轴向
        "approxF|1|0|0",
        "approxF|-1|0|0",
        "approxF|0|1|0",
        "approxF|0|-1|0",
        "approxF|0|0|1",
        "approxF|0|0|-1",
        // approxF：全零（关键：返回 NORTH）
        "approxF|0|0|0",
        // approxF：斜向
        "approxF|1|1|0",
        "approxF|1|0|1",
        "approxF|0|1|1",
        "approxF|1|1|1",
        "approxF|-1|-1|-1",
        // approxF：极端小值（低于 Float.MIN_VALUE）
        "approxF|1e-46|1e-46|1e-46",
        // approxD：与 float 版对照
        "approxD|1|0|0",
        "approxD|0|0|0",
        "approxD|1e-300|0|0",
        // nearest：三轴主导
        "nearest|5|1|1|north",
        "nearest|-5|1|1|north",
        "nearest|1|5|1|north",
        "nearest|1|-5|1|north",
        "nearest|1|1|5|north",
        "nearest|1|1|-5|north",
        // nearest：平局
        "nearest|1|1|0|north",
        "nearest|1|0|1|north",
        "nearest|0|1|1|north",
        "nearest|1|1|1|north",
        "nearest|0|0|0|north",
        "nearest|0|0|0|null",
        "nearest|-1|1|0|west",
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
        throw std::runtime_error("direction_nearest: 单元至少两段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "approxF" && parts.size() == 4) {
        const std::string result = getApproximateNearest(static_cast<f32>(std::stod(parts[1])),
            static_cast<f32>(std::stod(parts[2])),
            static_cast<f32>(std::stod(parts[3])));
        return json{{"unit", unit}, {"value", result}}.dump();
    }
    if (mode == "approxD" && parts.size() == 4) {
        // 原版 double 版先降精度到 float。
        const std::string result = getApproximateNearest(static_cast<f32>(std::stod(parts[1])),
            static_cast<f32>(std::stod(parts[2])),
            static_cast<f32>(std::stod(parts[3])));
        return json{{"unit", unit}, {"value", result}}.dump();
    }
    if (mode == "nearest" && parts.size() == 5) {
        std::optional<std::string> fallback;
        if (parts[4] != "null") {
            fallback = parts[4];
        }
        const std::string result = getNearest(std::stoi(parts[1]), std::stoi(parts[2]), std::stoi(parts[3]), fallback);
        return json{{"unit", unit}, {"value", result}}.dump();
    }

    throw std::runtime_error("direction_nearest: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(direction_nearest, &writeList, &runLine)

} // namespace mc::diff
