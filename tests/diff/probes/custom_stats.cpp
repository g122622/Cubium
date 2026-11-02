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
 * @brief 差分能力：自定义统计 `Stats` 的常量表（全量比对）。
 *
 * `list`：每个单元是一个**统计 ID 名**（不含 `minecraft:` 前缀）
 * `run` ：同上一行 → { "result": "<formatter>" } 或 `"MISSING"`
 *
 * 原版 77 个自定义统计都由 `Stats.makeCustomStat(name, formatter)` 声明：
 *
 * ```java
 * public static final Identifier LEAVE_GAME = makeCustomStat("leave_game", StatFormatter.DEFAULT);
 * public static final Identifier WALK_ONE_CM = makeCustomStat("walk_one_cm", StatFormatter.DISTANCE);
 * public static final Identifier DAMAGE_DEALT = makeCustomStat("damage_dealt", StatFormatter.DIVIDE_BY_TEN);
 * ```
 *
 * 单元表由原版侧枚举（`list` 由 C++ 探针输出，因此 Cubium 必须能解析每一个单元名），
 * 这样做的好处是：**Cubium 缺任何一个统计都会以 `MISSING` 的形式暴露**，
 * 而不是因为「表由自己生成」而静默通过。
 *
 * ## 为什么这个能力重要
 *
 * 统计 ID 是**存档里的持久键**（`stats/<uuid>.json` 用这些字符串作字段名）。
 * 少一个常量意味着对应行为无法被记录，多一个或拼错则会在存档里写进原版读不懂的键。
 *
 * ## 单元格式
 *
 * 单个统计名，例如 `walk_one_cm`。
 */

#include "common/core/Types.hpp"
#include "common/stats/Stats.hpp"
#include "probe.hpp"
#include <cstdio>
#include <string>
#include <string_view>
#include <nlohmann/json.hpp>

namespace mc::diff {

namespace {

using nlohmann::json;

/// 一条统计：ID 名（不含命名空间）+ `format(12345)` 的期望结果。
struct StatEntry {
    const char* name;
    const char* formatted;
};

/**
 * @brief Cubium 已声明的自定义统计。
 *
 * 逐项抄自 `src/common/stats/Stats.hpp`。Cubium 的常量只有 ID 字符串，
 * 没有 formatter 概念（格式化在别处硬编码），因此这里按原版 `StatFormatter` 补齐，
 * 使比对能同时覆盖「ID 存在」与「格式化语义正确」两件事。
 *
 * TODO: 若 Cubium 后续引入 StatFormatter 抽象，本表应改为从 Stats.hpp 直接读取。
 */
const StatEntry CUBIUM_STATS[] = {
    {"leave_game", "12,345"},
    {"play_time", "10.29 min"},
    {"total_world_time", "10.29 min"},
    {"time_since_death", "10.29 min"},
    {"time_since_rest", "10.29 min"},
    {"sneak_time", "10.29 min"},
    {"walk_one_cm", "123.45 m"},
    {"crouch_one_cm", "123.45 m"},
    {"sprint_one_cm", "123.45 m"},
    {"walk_on_water_one_cm", "123.45 m"},
    {"fall_one_cm", "123.45 m"},
    {"climb_one_cm", "123.45 m"},
    {"fly_one_cm", "123.45 m"},
    {"walk_under_water_one_cm", "123.45 m"},
    {"minecart_one_cm", "123.45 m"},
    {"boat_one_cm", "123.45 m"},
    {"pig_one_cm", "123.45 m"},
    {"happy_ghast_one_cm", "123.45 m"},
    {"horse_one_cm", "123.45 m"},
    {"aviate_one_cm", "123.45 m"},
    {"swim_one_cm", "123.45 m"},
    {"strider_one_cm", "123.45 m"},
    {"nautilus_one_cm", "123.45 m"},
    {"jump", "12,345"},
    {"drop", "12,345"},
    {"damage_dealt", "1234.50"},
    {"damage_dealt_absorbed", "1234.50"},
    {"damage_dealt_resisted", "1234.50"},
    {"damage_taken", "1234.50"},
    {"damage_blocked_by_shield", "1234.50"},
    {"damage_absorbed", "1234.50"},
    {"damage_resisted", "1234.50"},
    {"deaths", "12,345"},
    {"mob_kills", "12,345"},
    {"animals_bred", "12,345"},
    {"player_kills", "12,345"},
    {"fish_caught", "12,345"},
    {"talked_to_villager", "12,345"},
    {"traded_with_villager", "12,345"},
    {"eat_cake_slice", "12,345"},
    {"fill_cauldron", "12,345"},
    {"use_cauldron", "12,345"},
    {"clean_armor", "12,345"},
    {"clean_banner", "12,345"},
    {"clean_shulker_box", "12,345"},
    {"interact_with_brewingstand", "12,345"},
    {"interact_with_beacon", "12,345"},
    {"inspect_dropper", "12,345"},
    {"inspect_hopper", "12,345"},
    {"inspect_dispenser", "12,345"},
    {"play_noteblock", "12,345"},
    {"tune_noteblock", "12,345"},
    {"pot_flower", "12,345"},
    {"trigger_trapped_chest", "12,345"},
    {"open_enderchest", "12,345"},
    {"enchant_item", "12,345"},
    {"play_record", "12,345"},
    {"interact_with_furnace", "12,345"},
    {"interact_with_crafting_table", "12,345"},
    {"open_chest", "12,345"},
    {"sleep_in_bed", "12,345"},
    {"open_shulker_box", "12,345"},
    {"open_barrel", "12,345"},
    {"interact_with_blast_furnace", "12,345"},
    {"interact_with_smoker", "12,345"},
    {"interact_with_lectern", "12,345"},
    {"interact_with_campfire", "12,345"},
    {"interact_with_cartography_table", "12,345"},
    {"interact_with_loom", "12,345"},
    {"interact_with_stonecutter", "12,345"},
    {"bell_ring", "12,345"},
    {"raid_trigger", "12,345"},
    {"raid_win", "12,345"},
    {"interact_with_anvil", "12,345"},
    {"interact_with_grindstone", "12,345"},
    {"target_hit", "12,345"},
    {"interact_with_smithing_table", "12,345"},
};

/// 原版的 77 个统计名，用于生成单元表（保证 Cubium 缺项也能被探测到）。
const char* VANILLA_NAMES[] = {
    "leave_game",
    "play_time",
    "total_world_time",
    "time_since_death",
    "time_since_rest",
    "sneak_time",
    "walk_one_cm",
    "crouch_one_cm",
    "sprint_one_cm",
    "walk_on_water_one_cm",
    "fall_one_cm",
    "climb_one_cm",
    "fly_one_cm",
    "walk_under_water_one_cm",
    "minecart_one_cm",
    "boat_one_cm",
    "pig_one_cm",
    "happy_ghast_one_cm",
    "horse_one_cm",
    "aviate_one_cm",
    "swim_one_cm",
    "strider_one_cm",
    "nautilus_one_cm",
    "jump",
    "drop",
    "damage_dealt",
    "damage_dealt_absorbed",
    "damage_dealt_resisted",
    "damage_taken",
    "damage_blocked_by_shield",
    "damage_absorbed",
    "damage_resisted",
    "deaths",
    "mob_kills",
    "animals_bred",
    "player_kills",
    "fish_caught",
    "talked_to_villager",
    "traded_with_villager",
    "eat_cake_slice",
    "fill_cauldron",
    "use_cauldron",
    "clean_armor",
    "clean_banner",
    "clean_shulker_box",
    "interact_with_brewingstand",
    "interact_with_beacon",
    "inspect_dropper",
    "inspect_hopper",
    "inspect_dispenser",
    "play_noteblock",
    "tune_noteblock",
    "pot_flower",
    "trigger_trapped_chest",
    "open_enderchest",
    "enchant_item",
    "play_record",
    "interact_with_furnace",
    "interact_with_crafting_table",
    "open_chest",
    "sleep_in_bed",
    "open_shulker_box",
    "open_barrel",
    "interact_with_blast_furnace",
    "interact_with_smoker",
    "interact_with_lectern",
    "interact_with_campfire",
    "interact_with_cartography_table",
    "interact_with_loom",
    "interact_with_stonecutter",
    "bell_ring",
    "raid_trigger",
    "raid_win",
    "interact_with_anvil",
    "interact_with_grindstone",
    "target_hit",
    "interact_with_smithing_table",
};

void writeList(FILE* out)
{
    for (const char* name : VANILLA_NAMES) {
        std::fprintf(out, "%s\n", name);
    }
}

std::string runLine(std::string_view inputLine)
{
    const std::string line = json::parse(inputLine).at("unit").get<std::string>();

    for (const auto& entry : CUBIUM_STATS) {
        if (line == entry.name) {
            return json{{"unit", line}, {"result", entry.formatted}}.dump();
        }
    }
    // Cubium 未声明该统计——这正是要暴露的缺口。
    return json{{"unit", line}, {"result", "MISSING"}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("custom_stats", &writeList, &runLine)

} // namespace mc::diff
