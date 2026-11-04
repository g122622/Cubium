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
 * @brief 差分能力：`ObjectiveCriteria` 的判据名、只读标志与渲染类型（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.world.scores.criteria.ObjectiveCriteria`：
 *
 * ```java
 * public static final ObjectiveCriteria DUMMY              = registerCustom("dummy");
 * public static final ObjectiveCriteria TRIGGER            = registerCustom("trigger");
 * public static final ObjectiveCriteria DEATH_COUNT        = registerCustom("deathCount");
 * public static final ObjectiveCriteria KILL_COUNT_PLAYERS = registerCustom("playerKillCount");
 * public static final ObjectiveCriteria KILL_COUNT_ALL     = registerCustom("totalKillCount");
 * public static final ObjectiveCriteria HEALTH    = registerCustom("health", true, HEARTS);
 * public static final ObjectiveCriteria FOOD      = registerCustom("food", true, INTEGER);
 * public static final ObjectiveCriteria AIR       = registerCustom("air", true, INTEGER);
 * public static final ObjectiveCriteria ARMOR     = registerCustom("armor", true, INTEGER);
 * public static final ObjectiveCriteria EXPERIENCE = registerCustom("xp", true, INTEGER);
 * public static final ObjectiveCriteria LEVEL     = registerCustom("level", true, INTEGER);
 *
 * public static final ObjectiveCriteria[] TEAM_KILL = {
 *     registerCustom("teamkill." + ChatFormatting.BLACK.getName()),
 *     ... 共 16 个颜色
 * };
 * public static final ObjectiveCriteria[] KILLED_BY_TEAM = {
 *     registerCustom("killedByTeam." + ChatFormatting.BLACK.getName()),
 *     ... 共 16 个颜色
 * };
 *
 * public enum RenderType implements StringRepresentable {
 *     INTEGER("integer"),
 *     HEARTS("hearts");
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **判据名是 camelCase，不是 snake_case。**
 *
 *    | 常量 | 判据名 |
 *    |---|---|
 *    | `DUMMY` | `dummy` |
 *    | `TRIGGER` | `trigger` |
 *    | **`DEATH_COUNT`** | **`deathCount`** |
 *    | **`KILL_COUNT_PLAYERS`** | **`playerKillCount`** |
 *    | **`KILL_COUNT_ALL`** | **`totalKillCount`** |
 *    | `HEALTH` | `health` |
 *    | **`EXPERIENCE`** | **`xp`**（不是 `experience`！）|
 *
 *    **`EXPERIENCE` 的判据名是 `xp`**——与常量名风马牛不相及。
 *    若按常量名小写推导，会得到 `experience`——完全错误。
 *
 * 2. **只有 `HEALTH` 用 `RenderType.HEARTS`，其余全部 `INTEGER`。**
 *
 *    这也说明「心形渲染」是血量专属。
 *
 * 3. **`readOnly` 标志只对 6 个「世界状态派生」的判据为 `true`。**
 *
 *    | 判据 | `readOnly` |
 *    |---|---|
 *    | `dummy` / `trigger` / `deathCount` / `playerKillCount` / `totalKillCount` | `false` |
 *    | **`health` / `food` / `air` / `armor` / `xp` / `level`** | **`true`** |
 *    | `teamkill.*` / `killedByTeam.*` | `false` |
 *
 *    `readOnly` 表示「由游戏状态自动派生，不能被 `/scoreboard` 直接改」。
 *
 * 4. **`TEAM_KILL` 与 `KILLED_BY_TEAM` 是 16 个颜色后缀的两组，前缀不同。**
 *
 *    | 数组 | 前缀 | 示例 |
 *    |---|---|---|
 *    | `TEAM_KILL` | **`teamkill.`** | `teamkill.black` |
 *    | `KILLED_BY_TEAM` | **`killedByTeam.`** | `killedByTeam.black` |
 *
 *    注意 `killedByTeam` 是 **camelCase**，`teamkill` 是 **全小写无驼峰**——两组前缀风格也不一致。
 *
 * 单元格式：
 * - `name|<criteria>` —— `getName()`，`criteria` 用常量名
 * - `readOnly|<criteria>` —— `isReadOnly()`
 * - `renderType|<criteria>` —— `getDefaultRenderType().getSerializedName()`
 * - `teamKill|<index>` —— `TEAM_KILL[index].getName()`
 * - `killedByTeam|<index>` —— `KILLED_BY_TEAM[index].getName()`
 * - `teamKillCount` / `killedByTeamCount` —— 数组长度
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

/// 一个 `ObjectiveCriteria` 常量。
struct Criteria {
    const char* constant;
    const char* name;
    bool readOnly;
    const char* renderType;
};

constexpr Criteria CRITERIA[] = {
    {"DUMMY", "dummy", false, "integer"},
    {"TRIGGER", "trigger", false, "integer"},
    {"DEATH_COUNT", "deathCount", false, "integer"},
    {"KILL_COUNT_PLAYERS", "playerKillCount", false, "integer"},
    {"KILL_COUNT_ALL", "totalKillCount", false, "integer"},
    {"HEALTH", "health", true, "hearts"},
    {"FOOD", "food", true, "integer"},
    {"AIR", "air", true, "integer"},
    {"ARMOR", "armor", true, "integer"},
    {"EXPERIENCE", "xp", true, "integer"},
    {"LEVEL", "level", true, "integer"},
};

constexpr i32 CRITERIA_COUNT = 11;

/// 16 个颜色后缀，按 `ChatFormatting` 的颜色顺序。
constexpr const char* TEAM_COLORS[] = {"black",
    "dark_blue",
    "dark_green",
    "dark_aqua",
    "dark_red",
    "dark_purple",
    "gold",
    "gray",
    "dark_gray",
    "blue",
    "green",
    "aqua",
    "red",
    "light_purple",
    "yellow",
    "white"};

constexpr i32 TEAM_COLOR_COUNT = 16;

const Criteria* findCriteria(const std::string& constant)
{
    for (const Criteria& criteria : CRITERIA) {
        if (constant == criteria.constant) {
            return &criteria;
        }
    }
    throw std::runtime_error("objective_criteria: 未知常量：" + constant);
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // name：camelCase 与特例
        "name|DUMMY",
        "name|TRIGGER",
        "name|DEATH_COUNT",
        "name|KILL_COUNT_PLAYERS",
        "name|KILL_COUNT_ALL",
        "name|HEALTH",
        "name|FOOD",
        "name|AIR",
        "name|ARMOR",
        "name|EXPERIENCE",
        "name|LEVEL",
        // readOnly
        "readOnly|DUMMY",
        "readOnly|DEATH_COUNT",
        "readOnly|HEALTH",
        "readOnly|EXPERIENCE",
        "readOnly|LEVEL",
        // renderType
        "renderType|HEALTH",
        "renderType|DUMMY",
        "renderType|FOOD",
        "renderType|LEVEL",
        // teamKill / killedByTeam
        "teamKill|0",
        "teamKill|1",
        "teamKill|15",
        "killedByTeam|0",
        "killedByTeam|1",
        "killedByTeam|15",
        "teamKillCount",
        "killedByTeamCount",
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
    if (parts.empty()) {
        throw std::runtime_error("objective_criteria: 空单元");
    }

    const std::string mode = parts[0];
    std::string value;

    if (mode == "teamKillCount" && parts.size() == 1) {
        value = std::to_string(TEAM_COLOR_COUNT);
    } else if (mode == "killedByTeamCount" && parts.size() == 1) {
        value = std::to_string(TEAM_COLOR_COUNT);
    } else if (parts.size() != 2) {
        throw std::runtime_error("objective_criteria: 单元段数不符：" + unit);
    } else if (mode == "name") {
        value = findCriteria(parts[1])->name;
    } else if (mode == "readOnly") {
        value = findCriteria(parts[1])->readOnly ? "true" : "false";
    } else if (mode == "renderType") {
        value = findCriteria(parts[1])->renderType;
    } else if (mode == "teamKill" || mode == "killedByTeam") {
        const i32 index = static_cast<i32>(std::stoll(parts[1]));
        if (index < 0 || index >= TEAM_COLOR_COUNT) {
            throw std::runtime_error("objective_criteria: 索引越界：" + parts[1]);
        }
        const std::string prefix = mode == "teamKill" ? "teamkill." : "killedByTeam.";
        value = prefix + TEAM_COLORS[index];
    } else {
        throw std::runtime_error("objective_criteria: 无法识别的单元：" + unit);
    }

    return json{{"unit", unit}, {"value", value}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(objective_criteria, &writeList, &runLine)

} // namespace mc::diff
