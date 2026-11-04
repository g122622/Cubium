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
 * @brief 差分能力：流体高度与量值（`FlowingFluid.getOwnHeight`，纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.world.level.material.FlowingFluid`：
 *
 * ```java
 * // WaterFluid 与 LavaFluid 完全一致
 * public int getAmount(FluidState state) {
 *     return state.getValue(LEVEL);      // 流动液体：LEVEL ∈ [1, 8]
 * }
 * // 源头上方的重载
 * public int getAmount(FluidState state) {
 *     return 8;                          // 源头固定返回 8
 * }
 *
 * public float getOwnHeight(FluidState state) {
 *     return state.getAmount() / 9.0F;
 * }
 *
 * public float getHeight(FluidState state, BlockGetter level, BlockPos pos) {
 *     return hasSameAbove(state, level, pos) ? 1.0F : state.getOwnHeight();
 * }
 *
 * public VoxelShape getShape(FluidState state, BlockGetter level, BlockPos pos) {
 *     return state.getAmount() == 9 && hasSameAbove(state, level, pos)
 *         ? Shapes.block()
 *         : Shapes.box(0.0, 0.0, 0.0, 1.0, state.getHeight(level, pos), 1.0);
 * }
 *
 * // BlockStateProperties
 * LEVEL_FLOWING = IntegerProperty.create("level", 1, 8);
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`getOwnHeight` 的分母是 9，但 `getAmount` 最大只有 8。**
 *
 *    ```java
 *    return state.getAmount() / 9.0F;
 *    ```
 *
 *    源头（`isSource`）的 `getAmount()` 返回 **8**，所以源头高度是 `8/9 ≈ 0.8888889`，
 *    **不是 `1.0`**。
 *
 *    | `getAmount` | `getOwnHeight` |
 *    |---|---|
 *    | 1 | `0.111111` |
 *    | 2 | `0.222222` |
 *    | 3 | `0.333333` |
 *    | 4 | `0.444444` |
 *    | 5 | `0.555556` |
 *    | 6 | `0.666667` |
 *    | 7 | `0.777778` |
 *    | 8（源头）| `0.888889` |
 *
 *    **只有 8 个离散值，没有 0 也没有 1。** 这是流体渲染时「水面比方块顶低一点点」的原因。
 *
 * 2. **`> 8/9` 只出现在 `hasSameAbove` 为真时，此时直接返回 `1.0`。**
 *
 *    ```java
 *    return hasSameAbove(state, level, pos) ? 1.0F : state.getOwnHeight();
 *    ```
 *
 *    上方是同类流体时，当前格「满格」——但这个 `1.0` **不是**算出来的，是**特判**。所以
 *    `getHeight` 的取值范围是 `{1/9, 2/9, ..., 8/9, 1}`，共 **9 个值**。
 *
 * 3. **`LEVEL_FLOWING` 的范围是 `[1, 8]`，不是 `[0, 7]` 也不是 `[0, 8]`。**
 *
 *    ```java
 *    LEVEL_FLOWING = IntegerProperty.create("level", 1, 8);
 *    ```
 *
 *    **流体没有「0 级」**——最小的流动液体是 1 级（高度 `1/9`）。若 Cubium 用 `[0, 7]`，
 *    所有非源头流体的高度都会差 `1/9`。
 *
 *    （注意与 `LEVEL = IntegerProperty.create("level", 0, 15)` 区分——那是炼药锅、堆肥桶等
 *    方块的通用 `level`，范围完全不同。）
 *
 * 4. **水与岩浆的 `getAmount` / `getOwnHeight` 实现完全相同。**
 *
 *    `WaterFluid` 与 `LavaFluid` 都返回 `LEVEL`（流动）或 `8`（源头），`getOwnHeight` 都是
 *    `/ 9.0F`。差异只在**流动速度**（水 5 格、岩浆 2 格）与**是否可无限扩散**，这些属于
 *    `FluidSpread` 逻辑（已被 [`fluid_spread`](./fluid_spread.md) 覆盖），不在高度计算里。
 *
 * 单元格式：
 * - `amount|<level>` —— `getAmount`（`level` ∈ 1..8）
 * - `ownHeight|<level>` —— `getOwnHeight`
 * - `height|<level>|<sameAbove>` —— `getHeight`（含 `hasSameAbove` 的影响）
 *
 * 所有浮点按 `%.9g` 格式化后去掉尾随零。**这里用 `%.9g` 而非 `%.6g`**，因为
 * `n/9.0F` 在 `float` 下只有约 7 位有效数字，`%.6g` 会把 `1/9` 的 `0.111111` 与 `2/9` 的
 * `0.222222` 区分开，但会丢失 `float` 的末位精度；`%.9g` 能完整保留 `float` 的全部有效位。
 *
 * TODO: 未覆盖 `hasSameAbove` 的世界查询（需要 `BlockGetter`）、`getShape` 的 `VoxelShape`
 * 返回、`getFlow` / `getSlopeDistance` 的坡度衰减（属于流动逻辑）。
 */
#include <cmath>
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

/// 原版 `BlockStateProperties.LEVEL_FLOWING` 的范围。
constexpr i32 LEVEL_FLOWING_MIN = 1;
constexpr i32 LEVEL_FLOWING_MAX = 8;

/// 原版源头 `getAmount` 的固定返回值。
constexpr i32 SOURCE_AMOUNT = 8;

/// 格式化浮点：`%.9g` 并去掉尾随零。
std::string formatNumber(f32 value)
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.9g", static_cast<f64>(value));
    std::string text(buffer);
    if (text.find('.') != std::string::npos && text.find('e') == std::string::npos &&
        text.find('E') == std::string::npos) {
        while (!text.empty() && text.back() == '0') {
            text.pop_back();
        }
        if (!text.empty() && text.back() == '.') {
            text.pop_back();
        }
    }
    return text;
}

/// 原版 `FluidState.getAmount`：流动液体返回 `LEVEL`，源头返回 8。
i32 getAmount(i32 level)
{
    if (level < LEVEL_FLOWING_MIN || level > LEVEL_FLOWING_MAX) {
        throw std::runtime_error("fluid_height: LEVEL 超出 [1,8]：" + std::to_string(level));
    }
    return level;
}

/// 原版 `FlowingFluid.getOwnHeight`：`amount / 9.0F`。
f32 getOwnHeight(i32 amount)
{
    return static_cast<f32>(amount) / 9.0F;
}

/// 原版 `FlowingFluid.getHeight`：上方同类流体时特判为 1.0。
f32 getHeight(i32 amount, bool sameAbove)
{
    return sameAbove ? 1.0F : getOwnHeight(amount);
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // amount：全部 8 个等级
        "amount|1",
        "amount|2",
        "amount|3",
        "amount|4",
        "amount|5",
        "amount|6",
        "amount|7",
        "amount|8",
        // ownHeight：全部 8 个等级
        "ownHeight|1",
        "ownHeight|2",
        "ownHeight|3",
        "ownHeight|4",
        "ownHeight|5",
        "ownHeight|6",
        "ownHeight|7",
        "ownHeight|8",
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
        throw std::runtime_error("fluid_height: 单元至少两段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "amount" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", getAmount(std::stoi(parts[1]))}}.dump();
    }
    if (mode == "ownHeight" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", formatNumber(getOwnHeight(getAmount(std::stoi(parts[1]))))}}.dump();
    }
    if (mode == "height" && parts.size() == 3) {
        const i32 amount = getAmount(std::stoi(parts[1]));
        const bool sameAbove = parts[2] == "true";
        return json{{"unit", unit}, {"value", formatNumber(getHeight(amount, sameAbove))}}.dump();
    }

    throw std::runtime_error("fluid_height: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(fluid_height, &writeList, &runLine)

} // namespace mc::diff
