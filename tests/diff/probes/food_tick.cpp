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
 * @brief 差分能力：饥饿值每 tick 的状态机（纯计算）。
 *
 * `list`：一组「状态前缀|难度|自然再生|受伤|生命值」
 * `run` ：同上一行
 *         → { "foodLevel": 20, "saturationLevel": "3", "exhaustionLevel": "0",
 *             "tickTimer": 0, "heal": "0.5", "damage": "0" }
 *
 * 复刻原版 `FoodData.tick(ServerPlayer)` 的完整方法体：
 *
 * ```java
 * if (exhaustionLevel > 4.0F) {
 *     exhaustionLevel -= 4.0F;
 *     if (saturationLevel > 0.0F) saturationLevel = max(saturationLevel - 1.0F, 0.0F);
 *     else if (difficulty != PEACEFUL) foodLevel = max(foodLevel - 1, 0);
 * }
 *
 * if (regen && saturationLevel > 0 && hurt && foodLevel >= 20) {
 *     if (++tickTimer >= 10) { heal; addExhaustion(min(saturationLevel, 6.0F)); tickTimer = 0; }
 * } else if (regen && foodLevel >= 18 && hurt) {
 *     if (++tickTimer >= 80) { heal(1.0F); addExhaustion(6.0F); tickTimer = 0; }
 * } else if (foodLevel <= 0) {
 *     if (++tickTimer >= 80) { 按生命值与难度决定是否造成 1 点饥饿伤害; tickTimer = 0; }
 * } else {
 *     tickTimer = 0;
 * }
 * ```
 *
 * ## 四个互斥分支与两个阈值
 *
 * 1. **exhaustion 的消耗在任何分支之前**，且只用一次 `> 4.0F` 判定，不做循环。
 *    `exhaustionLevel -= 4.0F` 之后才决定扣饱和还是扣饥饿。
 * 2. **快再生优先**：需要 `foodLevel >= 20`（满）且饱和 > 0。每 10 tick 回血
 *    `min(saturation, 6) / 6` 点，并**把这部分饱和值作为 exhaustion 加回去**。
 * 3. **慢再生**：`foodLevel >= 18` 且受伤。每 80 tick 回 1 点，加 6 exhaustion。
 * 4. **饥饿伤害**：`foodLevel <= 0`。每 80 tick 一次，是否真的扣血取决于生命值与难度：
 *    `health > 10 || difficulty == HARD || (health > 1 && difficulty == NORMAL)`。
 * 5. **其余情况把 tickTimer 归零**——这一点容易漏，它意味着「条件不满足时计时器不保留」。
 *
 * 一个不易察觉的顺序细节：快再生的 `addExhaustion(min(saturation, 6))` 会立刻抬高
 * exhaustion，**下一次 tick** 就可能触发上面的消耗分支，从而把饱和扣掉。
 *
 * 单元格式：`food|sat|exh|timer|difficulty|regen|hurt|health`
 *
 * TODO: Cubium 的 `FoodStats::tick` 已存在（拆成 `_performFastRegeneration` 等私有方法），
 * 本探针按原版方法体逐行复算以便验证分支口径；待其与 `FoodStats` 行为对齐后可改为直接调用。
 */
#include <algorithm>
#include <cmath>
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

/// 原版 `addExhaustion` 的上限。
constexpr f32 MAX_EXHAUSTION = 40.0f;

std::string formatFloat(f64 value)
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.6g", value);
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

struct TickInput {
    i32 foodLevel = 20;
    f32 saturationLevel = 5.0f;
    f32 exhaustionLevel = 0.0f;
    i32 tickTimer = 0;
    /// "peaceful" / "easy" / "normal" / "hard"
    std::string difficulty;
    bool naturalRegeneration = true;
    bool hurt = false;
    f32 health = 20.0f;
};

struct TickOutput {
    i32 foodLevel = 20;
    f32 saturationLevel = 5.0f;
    f32 exhaustionLevel = 0.0f;
    i32 tickTimer = 0;
    f32 heal = 0.0f;
    f32 damage = 0.0f;
};

/// 原版 `FoodData.addExhaustion`。
void addExhaustion(TickOutput& state, f32 amount)
{
    state.exhaustionLevel = std::min(state.exhaustionLevel + amount, MAX_EXHAUSTION);
}

/**
 * @brief 复刻 `FoodData.tick(ServerPlayer)` 的完整方法体。
 */
TickOutput tick(const TickInput& in)
{
    TickOutput out;
    out.foodLevel = in.foodLevel;
    out.saturationLevel = in.saturationLevel;
    out.exhaustionLevel = in.exhaustionLevel;
    out.tickTimer = in.tickTimer;

    // 1) exhaustion 消耗：任何分支之前，只判定一次。
    if (out.exhaustionLevel > 4.0f) {
        out.exhaustionLevel -= 4.0f;
        if (out.saturationLevel > 0.0f) {
            out.saturationLevel = std::max(out.saturationLevel - 1.0f, 0.0f);
        } else if (in.difficulty != "peaceful") {
            out.foodLevel = std::max(out.foodLevel - 1, 0);
        }
    }

    // 2) 四个互斥分支。
    if (in.naturalRegeneration && out.saturationLevel > 0.0f && in.hurt && out.foodLevel >= 20) {
        ++out.tickTimer;
        if (out.tickTimer >= 10) {
            const f32 amount = std::min(out.saturationLevel, 6.0f);
            out.heal = amount / 6.0f;
            addExhaustion(out, amount);
            out.tickTimer = 0;
        }
    } else if (in.naturalRegeneration && out.foodLevel >= 18 && in.hurt) {
        ++out.tickTimer;
        if (out.tickTimer >= 80) {
            out.heal = 1.0f;
            addExhaustion(out, 6.0f);
            out.tickTimer = 0;
        }
    } else if (out.foodLevel <= 0) {
        ++out.tickTimer;
        if (out.tickTimer >= 80) {
            // 原版：health > 10 || difficulty == HARD || (health > 1 && difficulty == NORMAL)
            if (in.health > 10.0f || in.difficulty == "hard" || (in.health > 1.0f && in.difficulty == "normal")) {
                out.damage = 1.0f;
            }
            out.tickTimer = 0;
        }
    } else {
        out.tickTimer = 0;
    }

    return out;
}

/**
 * @brief 单元表：`food|sat|exh|timer|difficulty|regen|hurt|health`。
 *
 * 覆盖四个分支的触发与不触发、两个 tick 阈值（10 / 80）的边界、饥饿伤害的三种
 * 生命值-难度组合，以及 exhaustion 消耗与再生在同一 tick 内的叠加。
 */
constexpr const char* UNITS[] = {
    // 快再生：timer 9 -> 10 触发
    "20|3|0|9|normal|true|true|10",
    // 快再生：timer 8 -> 9 不触发
    "20|3|0|8|normal|true|true|10",
    // 快再生：饱和 > 6 时只回 1 点（min(sat, 6) = 6）
    "20|10|0|9|normal|true|true|10",
    // 快再生条件不满足：未满 20
    "19|3|0|9|normal|true|true|10",
    // 快再生条件不满足：未受伤
    "20|3|0|9|normal|true|false|10",
    // 快再生条件不满足：自然再生关闭
    "20|3|0|9|normal|false|true|10",
    // 慢再生：food 18、timer 79 -> 80 触发
    "18|0|0|79|normal|true|true|10",
    // 慢再生：timer 78 不触发
    "18|0|0|78|normal|true|true|10",
    // 慢再生：food 17 不满足
    "17|0|0|79|normal|true|true|10",
    // 饥饿伤害：生命 > 10，任意难度都扣血
    "0|0|0|79|easy|true|false|20",
    // 饥饿伤害：生命 = 10、HARD 扣血
    "0|0|0|79|hard|true|false|10",
    // 饥饿伤害：生命 = 10、NORMAL 扣血（health > 1 成立）
    "0|0|0|79|normal|true|false|10",
    // 饥饿伤害：生命 = 1、NORMAL 不扣血（需 > 1）
    "0|0|0|79|normal|true|false|1",
    // 饥饿伤害：生命 = 1、HARD 扣血（难度直接放行）
    "0|0|0|79|hard|true|false|1",
    // 饥饿伤害：生命 = 5、EASY 不扣血
    "0|0|0|79|easy|true|false|5",
    // exhaustion 消耗：有饱和则扣饱和不扣饥饿
    "20|3|5|0|normal|true|false|20",
    // exhaustion 消耗：无饱和则扣饥饿
    "20|0|5|0|normal|true|false|20",
    // exhaustion 消耗：无饱和且 PEACEFUL，不扣任何东西
    "20|0|5|0|peaceful|true|false|20",
    // exhaustion 恰为 4.0 不触发消耗
    "20|3|4|0|normal|true|false|20",
    // 默认分支：条件全不满足 -> timer 归零
    "15|2|0|50|normal|true|false|20",
    // exhaustion 消耗与快再生在同一 tick 内叠加
    "20|8|3|9|normal|true|true|10",
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
        throw std::runtime_error("food_tick: 单元应为 8 段 'food|sat|exh|timer|difficulty|regen|hurt|health'：" + unit);
    }

    TickInput in;
    in.foodLevel = std::stoi(parts[0]);
    in.saturationLevel = static_cast<f32>(std::stod(parts[1]));
    in.exhaustionLevel = static_cast<f32>(std::stod(parts[2]));
    in.tickTimer = std::stoi(parts[3]);
    in.difficulty = parts[4];
    in.naturalRegeneration = parts[5] == "true";
    in.hurt = parts[6] == "true";
    in.health = static_cast<f32>(std::stod(parts[7]));

    const TickOutput out = tick(in);

    return json{{"unit", unit},
        {"foodLevel", out.foodLevel},
        {"saturationLevel", formatFloat(static_cast<f64>(out.saturationLevel))},
        {"exhaustionLevel", formatFloat(static_cast<f64>(out.exhaustionLevel))},
        {"tickTimer", out.tickTimer},
        {"heal", formatFloat(static_cast<f64>(out.heal))},
        {"damage", formatFloat(static_cast<f64>(out.damage))}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(food_tick, &writeList, &runLine)

} // namespace mc::diff
