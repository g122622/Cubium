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
 * @brief 差分能力：信标的效果范围与时长（纯计算）。
 *
 * `list`：一组「层数|主副效果是否相同|是否选了副效果」
 * `run` ：同上一行
 *         → { "radius": 50, "durationTicks": 37 * 20, "amplifier": 1, "appliesSecondary": false }
 *
 * 复刻原版 `BeaconBlockEntity.applyEffects`：
 *
 * ```java
 * double d0 = levels * 10 + 10;                 // 效果范围半径（方块）
 * int i = 0;                                    // 主效果 amplifier
 * if (levels >= 4 && Objects.equals(primary, secondary)) {
 *     i = 1;                                    // 满级且主副相同时升到 II 级
 * }
 *
 * int j = (9 + levels * 2) * 20;                // 效果持续时长（tick）
 * AABB aabb = new AABB(pos).inflate(d0).expandTowards(0.0, level.getHeight(), 0.0);
 *
 * for (Player player : 范围内的玩家) {
 *     player.addEffect(new MobEffectInstance(primary, j, i, true, true));
 * }
 *
 * if (levels >= 4 && !Objects.equals(primary, secondary) && secondary != null) {
 *     for (Player player : 范围内的玩家) {
 *         player.addEffect(new MobEffectInstance(secondary, j, 0, true, true));
 *     }
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **范围半径 = `层数 × 10 + 10`**，即 1 级 20、2 级 30、3 级 40、4 级 50。写成
 *    `层数 × 10` 会少 10 格。
 *
 * 2. **时长 = `(9 + 层数 × 2) × 20` tick**，即 1 级 220、4 级 340 tick。注意常数是
 *    **9 不是 10**——这是个容易记错的偏移。
 *
 * 3. **升到 II 级需要「满级且主副效果相同」**。只有 4 级时才会把主效果的 amplifier
 *    设为 1；3 级及以下永远是 0。
 *
 * 4. **副效果单独施加，amplifier 固定为 0**。`levels >= 4 && 主副不同 && 副非空` 时才
 *    施加。若主副相同，走的是第 3 条的「主效果升 II」路径，不再单独施加副效果——两者
 *    **互斥**。
 *
 * ## 层数的判定（`updateBase`）
 *
 * ```java
 * int i = 0;
 * for (int j = 1; j <= 4; i = j++) {
 *     int k = y - j;
 *     if (k < level.getMinY()) break;          // 到世界底部就停
 *     if (第 j 层不全是 BEACON_BASE_BLOCKS) break;
 * }
 * return i;
 * ```
 *
 * 注意循环的更新表达式是 `i = j++`，所以 `i` 记录的是**最后一次成功的那一层的编号**。
 * 本能力把它抽象成输入 `levels`，不重复验证方块检测。
 *
 * 单元格式：`levels|sameEffect|hasSecondary`
 *
 * TODO: Cubium 目前只有客户端 `BeaconRenderer`，没有服务端信标方块实体。本能力提供的
 * 是待实现的行为基准。也未覆盖 `updateBase` 的方块检测（需要世界）与信标光束的可见性
 * （纯渲染，属客户端）。
 */
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 原版信标的最大层数。
constexpr i32 MAX_LEVELS = 4;

/// 每级增加的效果范围（格）。
constexpr i32 RADIUS_PER_LEVEL = 10;

/// 效果范围的基础值（格）。
constexpr i32 RADIUS_BASE = 10;

/// 效果时长公式里的基础秒数偏移（注意是 9，不是 10）。
constexpr i32 DURATION_BASE_SECONDS = 9;

/// 每级增加的时长（秒）。
constexpr i32 DURATION_PER_LEVEL_SECONDS = 2;

struct BeaconEffects {
    i32 radius = 0;
    i32 durationTicks = 0;
    i32 amplifier = 0;
    bool appliesSecondary = false;
};

/**
 * @brief 复刻 `BeaconBlockEntity.applyEffects` 里范围与时长那一段。
 */
BeaconEffects computeEffects(i32 levels, bool sameEffect, bool hasSecondary)
{
    BeaconEffects result;

    // 范围半径。
    result.radius = levels * RADIUS_PER_LEVEL + RADIUS_BASE;

    // 主效果 amplifier：满级且主副相同时升到 II。
    result.amplifier = (levels >= MAX_LEVELS && sameEffect) ? 1 : 0;

    // 时长（tick）。
    result.durationTicks = (DURATION_BASE_SECONDS + levels * DURATION_PER_LEVEL_SECONDS) * 20;

    // 副效果：满级、主副不同、副效果非空才单独施加。
    result.appliesSecondary = levels >= MAX_LEVELS && !sameEffect && hasSecondary;

    return result;
}

/**
 * @brief 单元表：`levels|sameEffect|hasSecondary`。
 *
 * 覆盖 1~4 级，以及「主副相同 / 不同 / 无副效果」三种组合。
 */
constexpr const char* UNITS[] = {
    // 1 级，无副效果
    "1|false|false",
    // 2 级，无副效果
    "2|false|false",
    // 3 级，无副效果
    "3|false|false",
    // 4 级，主副相同 -> 主效果升 II，不施加副
    "4|true|false",
    // 4 级，主副相同且「有副」 -> 仍走升 II，不施加副
    "4|true|true",
    // 4 级，主副不同且有副 -> 主 0 级 + 副 0 级
    "4|false|true",
    // 4 级，主副不同但无副 -> 不施加副
    "4|false|false",
    // 3 级，主副相同 -> 不满级，不升 II
    "3|true|false",
    // 3 级，主副不同且有副 -> 不满级，不施加副
    "3|false|true",
    // 0 级（无底座）：范围 10、时长 180
    "0|false|false",
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
    if (parts.size() != 3) {
        throw std::runtime_error("beacon_effects: 单元应为 3 段 'levels|sameEffect|hasSecondary'：" + unit);
    }

    const i32 levels = std::stoi(parts[0]);
    const bool sameEffect = parts[1] == "true";
    const bool hasSecondary = parts[2] == "true";

    const BeaconEffects effects = computeEffects(levels, sameEffect, hasSecondary);

    return json{{"unit", unit},
        {"levels", levels},
        {"radius", effects.radius},
        {"durationTicks", effects.durationTicks},
        {"amplifier", effects.amplifier},
        {"appliesSecondary", effects.appliesSecondary}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("beacon_effects", &writeList, &runLine)

} // namespace mc::diff
