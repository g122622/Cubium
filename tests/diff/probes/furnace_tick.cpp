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
 * @brief 差分能力：熔炉每 tick 的进度状态机（纯计算）。
 *
 * `list`：一组「状态前缀|原料是否可烧|燃料是否可用|产物能否接收」
 * `run` ：同上一行
 *         → { "litTimeRemaining": 100, "litTotalTime": 200, "cookingTimer": 5,
 *             "cookingTotalTime": 200, "smelted": false, "stateChanged": true }
 *
 * 复刻原版 `AbstractFurnaceBlockEntity.serverTick` 的完整方法体：
 *
 * ```java
 * boolean wasLit = isLit();
 * if (isLit()) { litTimeRemaining--; }
 *
 * if (isLit() || hasFuel && hasInput) {
 *     if (!isLit() && canBurn(...)) {
 *         litTimeRemaining = getBurnDuration(fuel);
 *         litTotalTime = litTimeRemaining;
 *         if (isLit()) { flag1 = true; 消耗一个燃料; }
 *     }
 *     if (isLit() && canBurn(...)) {
 *         cookingTimer++;
 *         if (cookingTimer == cookingTotalTime) {
 *             cookingTimer = 0;
 *             cookingTotalTime = getTotalCookTime(...);
 *             产出;
 *             flag1 = true;
 *         }
 *     } else {
 *         cookingTimer = 0;                       // 注意：这里直接归零
 *     }
 * } else if (!isLit() && cookingTimer > 0) {
 *     cookingTimer = Mth.clamp(cookingTimer - 2, 0, cookingTotalTime);   // 注意：这里减 2
 * }
 * ```
 *
 * ## 四个容易写错的地方
 *
 * 1. **「烹饪中但条件不满足」与「完全熄火」的处理不同**。前者在 `if (isLit() || ...)`
 *    块内，`cookingTimer` 直接**归零**；后者走 `else if`，`cookingTimer` **减 2**（不是归零）。
 *    把两处合并成一处会让熄火后的进度衰减完全错掉——减 2 意味着熄火后进度是「缓慢回退」
 *    而不是「立刻清零」。
 *
 * 2. **点燃与烹饪在同一 tick 内可连续发生**。先判断点燃、再判断烹饪，两个 `if` 是顺序
 *    关系而非互斥。所以「刚点燃的那一 tick」也会 `cookingTimer++`。
 *
 * 3. **熄灭判定在读减之后**。`wasLit` 在 tick 开头记录，比较发生在末尾，用来决定方块
 *    状态是否需要更新。
 *
 * 4. **`cookingTimer == cookingTotalTime` 是相等判断**，不是 `>=`。正常流程下不会跳过，
 *    但如果 `cookingTotalTime` 被其它路径降低过，`>=` 与 `==` 的行为会不同。
 *
 * 单元格式：`litTimeRemaining|litTotalTime|cookingTimer|cookingTotalTime|canBurn|hasInput|hasFuel|outputAcceptable`
 *
 * TODO: 本能力把「配方是否存在」「产物能否接收」抽象成 `canBurn` 布尔输入，未覆盖
 * `canBurn` 内部的产物堆叠判定（那段是 `ItemStack.isSameItemSameComponents` 的组合，
 * 已由 `item_same_components` 覆盖）。也未覆盖 `getTotalCookTime`（配方速度乘数）与
 * `getBurnDuration`（燃料表，已由 `fuel_values` 覆盖）。
 */
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

struct FurnaceState {
    i32 litTimeRemaining = 0;
    i32 litTotalTime = 0;
    i32 cookingTimer = 0;
    i32 cookingTotalTime = 200;
};

struct FurnaceInput {
    FurnaceState state;
    /// 配方存在且产物槽能接收（对应原版 `canBurn` 的返回值）
    bool canBurn = false;
    /// 原料槽非空（`flag2`）
    bool hasInput = false;
    /// 燃料槽非空（`flag3`）
    bool hasFuel = false;
    /// 点燃后一 tick 的燃料燃烧时长（`getBurnDuration`）
    i32 burnDuration = 200;
};

struct FurnaceOutput {
    FurnaceState state;
    bool smelted = false;
    bool stateChanged = false;
};

bool isLit(const FurnaceState& state)
{
    return state.litTimeRemaining > 0;
}

/**
 * @brief 复刻 `AbstractFurnaceBlockEntity.serverTick` 的完整方法体。
 */
FurnaceOutput serverTick(const FurnaceInput& in)
{
    FurnaceOutput out;
    out.state = in.state;

    const bool wasLit = isLit(out.state);
    bool changed = false;

    // 1) 先递减燃烧计时。
    if (isLit(out.state)) {
        out.state.litTimeRemaining--;
    }

    // 2) 主分支：燃烧中，或「有燃料且有原料」。
    if (isLit(out.state) || (in.hasFuel && in.hasInput)) {
        // 2a) 未燃烧且可以烧 -> 点燃。
        if (!isLit(out.state) && in.canBurn) {
            out.state.litTimeRemaining = in.burnDuration;
            out.state.litTotalTime = out.state.litTimeRemaining;
            if (isLit(out.state)) {
                changed = true;
            }
        }

        // 2b) 燃烧中且可以烧 -> 推进烹饪。
        if (isLit(out.state) && in.canBurn) {
            out.state.cookingTimer++;
            if (out.state.cookingTimer == out.state.cookingTotalTime) {
                out.state.cookingTimer = 0;
                out.smelted = true;
                changed = true;
            }
        } else {
            // 注意：这里直接归零，与下面的 -2 不同。
            out.state.cookingTimer = 0;
        }
    } else if (!isLit(out.state) && out.state.cookingTimer > 0) {
        // 3) 完全熄火：进度每次回退 2，不是归零。
        out.state.cookingTimer = std::clamp(out.state.cookingTimer - 2, 0, out.state.cookingTotalTime);
    }

    // 4) 燃烧状态变化需要同步到方块状态。
    if (wasLit != isLit(out.state)) {
        changed = true;
    }

    out.stateChanged = changed;
    return out;
}

/// 单元表：`lit|cook|canBurn|hasInput|hasFuel|burnDuration`（lit/cook 为剩余与当前值）。
constexpr const char* UNITS[] = {
    // 燃烧中、可烧、进度推进
    "100|200|5|200|true|true|true|200",
    // 燃烧中、可烧、恰好在本次 tick 完成（5 -> 6 == total）
    "100|200|5|6|true|true|true|200",
    // 燃烧中、不可烧 -> 进度归零
    "100|200|5|200|false|true|true|200",
    // 未燃烧、有燃料有原料、可烧 -> 点燃 + 同 tick 推进烹饪
    "0|0|0|200|true|true|true|200",
    // 未燃烧、有燃料无原料 -> 不点燃
    "0|0|5|200|true|false|true|200",
    // 未燃烧、无燃料有原料 -> 不点燃
    "0|0|5|200|true|true|false|200",
    // 完全熄火、有进度 -> 减 2
    "0|0|10|200|false|false|false|200",
    // 完全熄火、进度为 1 -> 减到 0
    "0|0|1|200|false|false|false|200",
    // 完全熄火、进度为 0 -> 保持 0
    "0|0|0|200|false|false|false|200",
    // 燃烧只剩 1 tick -> 本 tick 减到 0，且不可烧 -> 归零
    "1|200|5|200|false|true|true|200",
    // 燃烧只剩 1 tick，可烧 -> 本 tick 减到 0 后不再推进
    "1|200|5|200|true|true|true|200",
    // 未燃烧、无燃料但有原料、进度 > 0 -> 减 2 分支
    "0|0|100|200|false|true|false|200",
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
    if (parts.size() != 8) {
        throw std::runtime_error("furnace_tick: 单元应为 8 段 "
                                 "'lit|litTotal|cook|cookTotal|canBurn|hasInput|hasFuel|burnDuration'：" +
            unit);
    }

    FurnaceInput in;
    in.state.litTimeRemaining = std::stoi(parts[0]);
    in.state.litTotalTime = std::stoi(parts[1]);
    in.state.cookingTimer = std::stoi(parts[2]);
    in.state.cookingTotalTime = std::stoi(parts[3]);
    in.canBurn = parts[4] == "true";
    in.hasInput = parts[5] == "true";
    in.hasFuel = parts[6] == "true";
    in.burnDuration = std::stoi(parts[7]);

    const FurnaceOutput out = serverTick(in);

    return json{{"unit", unit},
        {"litTimeRemaining", out.state.litTimeRemaining},
        {"litTotalTime", out.state.litTotalTime},
        {"cookingTimer", out.state.cookingTimer},
        {"cookingTotalTime", out.state.cookingTotalTime},
        {"smelted", out.smelted},
        {"stateChanged", out.stateChanged}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(furnace_tick, &writeList, &runLine)

} // namespace mc::diff
