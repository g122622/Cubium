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
 * @brief 差分能力：流体扩散的方向选择与坡度距离（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { ... }
 *
 * 复刻原版 `FlowingFluid`：
 *
 * ```java
 * // 流体常量
 * WaterFluid.getSlopeFindDistance = 4;  getDropOff = 1;
 * LavaFluid.getSlopeFindDistance  = isFastLava ? 4 : 2;  getDropOff = isFastLava ? 1 : 2;
 *
 * // 高度
 * getOwnHeight(state) = state.getAmount() / 9.0F;
 * getHeight(state)    = hasSameAbove ? 1.0F : getOwnHeight(state);
 *
 * // 横向扩散的等级衰减
 * int i = state.getAmount() - getDropOff(level);
 * if (state.getValue(FALLING)) { i = 7; }
 * if (i > 0) { ... 扩散 ... }
 *
 * // 方向选择（getSpread）
 * int best = 1000;
 * for (Direction dir : HORIZONTAL) {
 *     int dist = isHole(nb) ? 0 : getSlopeDistance(nb, 1, opposite(dir));
 *     if (dist < best) { map.clear(); }   // 更近 -> 清空
 *     if (dist <= best) { map.put(dir, state); best = dist; }
 * }
 *
 * // 坡度距离（getSlopeDistance）
 * int result = 1000;
 * for (Direction dir : HORIZONTAL) {
 *     if (dir == from) { continue; }      // 排除来向
 *     if (!passable) { continue; }
 *     if (isHole(nb)) { return depth; }   // 找到洞立即返回
 *     if (depth < slopeFindDistance) {
 *         result = min(result, recurse(nb, depth + 1, opposite(dir)));
 *     }
 * }
 * return result;
 * ```
 *
 * ## 四个关键点
 *
 * 1. **扩散方向是「就近全保留」，不是「只选一个」。** `getSpread` 用 `dist < best` 清空、
 *    用 `dist <= best` 加入。所以距离相同的多个方向**全部保留**——流体是对称扩散的。若写成
 *    「只保留第一个最近方向」，流体会单向流，与对称的水面观感完全不同。
 *
 * 2. **`getSlopeDistance` 找到洞就立即 `return`，不再比较其他方向。** 它返回的是**首次**遇到
 *    的洞的深度，而不是最小深度。这与 `getSpread` 的「取最小」语义不同。
 *
 * 3. **坡度搜索排除来向**（`direction != from`），避免原地往返。搜索深度上限是
 *    `getSlopeFindDistance`——水为 4、主世界岩浆为 2、下界岩浆为 4。
 *
 * 4. **下落状态的流体横向扩散等级固定为 7**：`if (FALLING) { i = 7; }`，覆盖
 *    `amount - dropOff` 的计算。所以从高处落下的水在地面会以满等级（7）向四周扩展，
 *    而不是按 `amount - 1` 衰减。
 *
 * 单元格式：
 * - `<流体>|<amount>|<hasSameAbove>|<falling>` —— 输出高度与横向扩散等级
 * - `slope|<流体>|<深度>|<洞距离>` —— 在一维模型上验证坡度搜索的深度上限
 *
 * TODO: 未覆盖 `canPassThroughWall` 的方块形状判定、`SpreadContext` 的缓存、
 * 流体与方块的实际交互（`spreadTo` 的写入顺序）。
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

std::string formatNumber(f64 value)
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.9g", value);
    std::string text(buffer);
    const size_t expPos = text.find_first_of("eE");
    std::string mantissa = expPos == std::string::npos ? text : text.substr(0, expPos);
    const std::string suffix = expPos == std::string::npos ? std::string() : text.substr(expPos);
    if (mantissa.find('.') != std::string::npos) {
        while (!mantissa.empty() && mantissa.back() == '0') {
            mantissa.pop_back();
        }
        if (!mantissa.empty() && mantissa.back() == '.') {
            mantissa.pop_back();
        }
    }
    return mantissa + suffix;
}

/// 流体参数。
struct FluidParams {
    i32 slopeFindDistance;
    i32 dropOff;
};

/// 按流体名取参数。`fast` 表示下界的岩浆加速规则。
FluidParams fluidParams(const std::string& name)
{
    if (name == "water") {
        return {4, 1};
    }
    if (name == "lava") {
        return {2, 2};
    }
    if (name == "lava_fast") {
        return {4, 1};
    }
    throw std::runtime_error("fluid_spread: 未知流体：" + name);
}

/// `LEVEL` 属性的取值上限（源方块等级 8）。
constexpr i32 MAX_LEVEL = 8;

/**
 * @brief 原版 `getOwnHeight`：`amount / 9.0F`。
 *
 * `amount` 来自 `LEVEL` 属性，其取值范围是 `1..8`，因此先钳制到该区间。
 */
f32 ownHeight(i32 amount)
{
    const i32 clamped = std::max(1, std::min(MAX_LEVEL, amount));
    return static_cast<f32>(clamped) / 9.0f;
}

/// 原版 `getHeight`：上方有同类流体时为 1.0。
f32 fluidHeight(i32 amount, bool hasSameAbove)
{
    return hasSameAbove ? 1.0f : ownHeight(amount);
}

/// 原版 `spreadToSides` 的等级衰减。`amount` 同样先钳制到 `1..8`。
i32 spreadLevel(i32 amount, i32 dropOff, bool falling)
{
    if (falling) {
        return 7;
    }
    const i32 clamped = std::max(1, std::min(MAX_LEVEL, amount));
    const i32 level = clamped - dropOff;
    return level > 0 ? level : 0;
}

/**
 * @brief 一维模型上的坡度搜索。
 *
 * `holeDistance` 是沿单一方向走多远会遇到洞（`0` 表示紧邻即是洞，负数表示没有洞）。
 * 返回原版 `getSlopeDistance` 的语义：找到洞就立即返回该深度，否则返回 1000。
 */
i32 slopeDistance(i32 depth, i32 holeDistance, i32 slopeFindDistance)
{
    if (holeDistance < 0) {
        return 1000;
    }
    if (holeDistance == 0) {
        return depth;
    }
    if (depth < slopeFindDistance) {
        return slopeDistance(depth + 1, holeDistance - 1, slopeFindDistance);
    }
    return 1000;
}

/// 单元表。
constexpr const char* UNITS[] = {
    // 高度：水的各等级
    "water|1|false|false",
    "water|2|false|false",
    "water|3|false|false",
    "water|8|false|false",
    "water|9|false|false",
    // 高度：上方有同类流体
    "water|1|true|false",
    "water|8|true|false",
    // 岩浆的高度（同样 /9）
    "lava|1|false|false",
    "lava|4|false|false",
    // 扩散等级：水（dropOff 1）
    "water|8|false|true",
    "water|7|false|true",
    "water|1|false|true",
    // 扩散等级：岩浆（dropOff 2）
    "lava|8|false|false",
    "lava|3|false|false",
    "lava|2|false|false",
    "lava|1|false|false",
    // 扩散等级：下落覆盖
    "lava|3|false|true",
    "lava|1|false|true",
    // 坡度：水（上限 4）
    "slope|water|1|0",
    "slope|water|1|1",
    "slope|water|1|2",
    "slope|water|1|3",
    "slope|water|1|4",
    "slope|water|1|5",
    "slope|water|1|-1",
    // 坡度：主世界岩浆（上限 2）
    "slope|lava|1|0",
    "slope|lava|1|1",
    "slope|lava|1|2",
    "slope|lava|1|3",
    // 坡度：下界岩浆（上限 4）
    "slope|lava_fast|1|3",
    "slope|lava_fast|1|4",
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
    if (parts.size() < 2) {
        throw std::runtime_error("fluid_spread: 单元至少两段：" + unit);
    }

    if (parts[0] == "slope" && parts.size() == 4) {
        const std::string fluid = parts[1];
        const i32 depth = std::stoi(parts[2]);
        const i32 holeDistance = std::stoi(parts[3]);
        const FluidParams params = fluidParams(fluid);
        const i32 result = slopeDistance(depth, holeDistance, params.slopeFindDistance);
        return json{{"unit", unit}, {"slopeFindDistance", params.slopeFindDistance}, {"result", result}}.dump();
    }

    if (parts.size() == 4) {
        const std::string fluid = parts[0];
        const i32 amount = std::stoi(parts[1]);
        const bool hasSameAbove = parts[2] == "true";
        const bool falling = parts[3] == "true";
        const FluidParams params = fluidParams(fluid);

        const f32 height = fluidHeight(amount, hasSameAbove);
        const i32 level = spreadLevel(amount, params.dropOff, falling);
        return json{{"unit", unit},
            {"height", formatNumber(static_cast<f64>(height))},
            {"spreadLevel", level},
            {"dropOff", params.dropOff}}
            .dump();
    }

    throw std::runtime_error("fluid_spread: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(fluid_spread, &writeList, &runLine)

} // namespace mc::diff
