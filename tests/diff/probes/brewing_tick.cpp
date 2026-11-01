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
 * @brief 差分能力：酿造台每 tick 的进度状态机（纯计算）。
 *
 * `list`：一组「状态前缀|可酿造|燃料槽有燃料|试剂槽物品」
 * `run` ：同上一行
 *         → { "brewTime": 399, "fuel": 19, "ingredient": "minecraft:blaze_powder",
 *             "brewed": false, "fuelConsumed": false, "fuelAdded": false }
 *
 * 复刻原版 `BrewingStandBlockEntity.serverTick` 的完整方法体：
 *
 * ```java
 * if (fuel <= 0 && fuelSlot.is(ItemTags.BREWING_FUEL)) {
 *     fuel = 20;
 *     消耗一个燃料;
 * }
 *
 * boolean brewable = isBrewable(...);
 * boolean brewing = brewTime > 0;
 * if (brewing) {
 *     brewTime--;
 *     if (brewTime == 0 && brewable) {
 *         doBrew(...);
 *     } else if (!brewable || !ingredientSlot.is(this.ingredient)) {
 *         brewTime = 0;                 // 中途条件破坏 -> 取消
 *     }
 * } else if (brewable && fuel > 0) {
 *     fuel--;                           // 注意：开始酿造时就扣燃料
 *     brewTime = 400;
 *     this.ingredient = ingredientStack.getItem();
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **燃料在「开始酿造」时扣除，不是完成时**。`else if (brewable && fuel > 0)` 分支里
 *    `fuel--` 与 `brewTime = 400` 同时发生。若在完成时才扣，中断酿造会白白不耗燃料。
 *
 * 2. **燃料补充上限是 20，且是「设为 20」不是「加 20」**。`fuel = 20` 覆盖写。
 *
 * 3. **中途条件破坏会取消酿造**：`!brewable || !ingredientSlot.is(this.ingredient)`。
 *    后者意味着「换掉试剂槽里的物品」会让酿造作废——判定依据是**试剂槽当前物品**与
 *    **开始时记下的 `ingredient`** 是否同一物品。
 *
 * 4. **不可酿造时不清零**。注意取消只发生在 `brewing` 分支内；若 `brewTime == 0`
 *    且不可酿造，函数直接什么都不做（走不到任何分支），`brewTime` 保持 0。
 *
 * 单元格式：`brewTime|fuel|brewable|fuelSlotHasFuel|ingredientMatches`
 *
 * - `ingredientMatches` 对应 `!ingredientSlot.is(this.ingredient)`：true 表示试剂槽与
 *   记录的试剂仍是同一物品；仅在 `brewTime > 0` 时有意义。
 *
 * TODO: Cubium 的 `BrewingStandEntity::tick` 与上述结构有多处不同（燃料用硬编码
 * `BLAZE_POWDER` 而非 `BREWING_FUEL` 标签、燃料在完成时扣而非开始时扣、不可酿造时直接
 * 清零 `brewTime`）。本探针按原版口径复算；待 Cubium 对齐后可改为直接调用。
 * 也未覆盖 `doBrew` 的实际产物转换（那部分是 `potion_brewing` 的职责）。
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

/// 原版 `BrewingStandBlockEntity.FUEL_USES`。
constexpr i32 FUEL_USES = 20;

/// 原版 `BrewingStandBlockEntity` 里酿造的固定时长。
constexpr i32 BREW_TIME_TICKS = 400;

struct BrewState {
    i32 brewTime = 0;
    i32 fuel = 0;
    /// 开始酿造时记下的试剂物品 id（空串表示没有记录）
    std::string ingredient;
};

struct BrewInput {
    BrewState state;
    bool brewable = false;
    bool fuelSlotHasFuel = false;
    /// 试剂槽当前物品是否与记录的 `ingredient` 同一物品
    bool ingredientMatches = true;
    /// 本次 tick 试剂槽里的物品 id（开始酿造时写入 `ingredient`）
    std::string ingredientItem = "minecraft:blaze_powder";
};

struct BrewOutput {
    BrewState state;
    bool brewed = false;
    bool fuelConsumed = false;
    bool fuelAdded = false;
};

BrewOutput serverTick(const BrewInput& in)
{
    BrewOutput out;
    out.state = in.state;

    // 1) 燃料补充：设为 20（不是加 20）。
    if (out.state.fuel <= 0 && in.fuelSlotHasFuel) {
        out.state.fuel = FUEL_USES;
        out.fuelAdded = true;
    }

    const bool brewable = in.brewable;
    const bool brewing = out.state.brewTime > 0;

    if (brewing) {
        // 2a) 酿造中：先递减。
        out.state.brewTime--;
        if (out.state.brewTime == 0 && brewable) {
            out.brewed = true;
        } else if (!brewable || !in.ingredientMatches) {
            // 2b) 中途条件破坏 -> 取消。
            out.state.brewTime = 0;
        }
    } else if (brewable && out.state.fuel > 0) {
        // 3) 开始酿造：**此刻**扣燃料。
        out.state.fuel--;
        out.fuelConsumed = true;
        out.state.brewTime = BREW_TIME_TICKS;
        out.state.ingredient = in.ingredientItem;
    }

    return out;
}

/**
 * @brief 单元表：`brewTime|fuel|brewable|fuelSlotHasFuel|ingredientMatches`。
 *
 * 逐条核对预期走向：
 * - 第 1 条：酿造中、可酿、试剂匹配 -> 递减（399），不完成
 * - 第 2 条：酿造中、剩 1、可酿、匹配 -> 完成（0 + brewed）
 * - 第 3 条：酿造中、剩 1、不可酿 -> 取消
 * - 第 4 条：酿造中、剩 100、试剂不匹配 -> 取消
 * - 第 5 条：未酿造、可酿、有燃料 -> 开始酿造，燃料 -1，brewTime=400
 * - 第 6 条：未酿造、可酿、无燃料但有燃料槽 -> 先补到 20 再扣到 19
 * - 第 7 条：未酿造、不可酿 -> 什么都不做
 * - 第 8 条：燃料 > 0 时不补充燃料槽
 * - 第 9 条：燃料槽为空且 fuel=0、可酿 -> 无法开始（无燃料可扣）
 */
constexpr const char* UNITS[] = {
    // 酿造中，正常推进
    "200|10|true|false|true",
    // 酿造中，本 tick 完成
    "1|10|true|false|true",
    // 酿造中，不可酿造 -> 取消
    "1|10|false|false|true",
    // 酿造中，试剂不匹配 -> 取消
    "100|10|true|false|false",
    // 未酿造，可酿且有燃料 -> 开始
    "0|10|true|false|true",
    // 未酿造，可酿、fuel=0 且有燃料槽 -> 补到 20 再扣到 19
    "0|0|true|true|true",
    // 未酿造，不可酿 -> 什么都不做
    "0|10|false|false|true",
    // 燃料充足时不补充燃料槽
    "0|5|false|true|true",
    // fuel=0 且燃料槽为空、可酿 -> 无法开始
    "0|0|true|false|true",
    // fuel=1、可酿 -> 扣到 0 并开始
    "0|1|true|false|true",
    // fuel=1、不可酿 -> 不扣燃料
    "0|1|false|false|true",
    // 酿造中剩 2，可酿 -> 递减到 1，不完成
    "2|10|true|false|true",
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
    if (parts.size() != 5) {
        throw std::runtime_error(
            "brewing_tick: 单元应为 5 段 'brewTime|fuel|brewable|fuelSlotHasFuel|ingredientMatches'：" + unit);
    }

    BrewInput in;
    in.state.brewTime = std::stoi(parts[0]);
    in.state.fuel = std::stoi(parts[1]);
    in.brewable = parts[2] == "true";
    in.fuelSlotHasFuel = parts[3] == "true";
    in.ingredientMatches = parts[4] == "true";

    const BrewOutput out = serverTick(in);

    return json{{"unit", unit},
        {"brewTime", out.state.brewTime},
        {"fuel", out.state.fuel},
        {"ingredient", out.state.ingredient},
        {"brewed", out.brewed},
        {"fuelConsumed", out.fuelConsumed},
        {"fuelAdded", out.fuelAdded}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("brewing_tick", &writeList, &runLine)

} // namespace mc::diff
