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
 * @brief 差分能力：`Team.CollisionRule` 与 `Team.Visibility`（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.world.scores.Team`：
 *
 * ```java
 * public enum CollisionRule implements StringRepresentable {
 *     ALWAYS("always", 0),
 *     NEVER("never", 1),
 *     PUSH_OTHER_TEAMS("pushOtherTeams", 2),
 *     PUSH_OWN_TEAM("pushOwnTeam", 3);
 *
 *     public final String name;
 *     public final int id;
 *
 *     private static final IntFunction<CollisionRule> BY_ID =
 *         ByIdMap.continuous(rule -> rule.id, values(), ByIdMap.OutOfBoundsStrategy.ZERO);
 *
 *     public Component getDisplayName() {
 *         return Component.translatable("team.collision." + this.name);
 *     }
 *
 *     public String getSerializedName() { return this.name; }
 * }
 *
 * public enum Visibility implements StringRepresentable {
 *     ALWAYS("always", 0),
 *     NEVER("never", 1),
 *     HIDE_FOR_OTHER_TEAMS("hideForOtherTeams", 2),
 *     HIDE_FOR_OWN_TEAM("hideForOwnTeam", 3);
 *
 *     public Component getDisplayName() {
 *         return Component.translatable("team.visibility." + this.name);
 *     }
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **两个枚举结构完全相同，但翻译键前缀不同。**
 *
 *    | 枚举 | 翻译键前缀 |
 *    |---|---|
 *    | `CollisionRule` | **`team.collision.`** |
 *    | `Visibility` | **`team.visibility.`** |
 *
 *    所以 `CollisionRule.ALWAYS` 的键是 `team.collision.always`，
 *    `Visibility.ALWAYS` 的键是 `team.visibility.always`——**同名字、不同键**。
 *
 * 2. **序列化名混合了全小写与 camelCase。**
 *
 *    | 枚举 | 序列化名 |
 *    |---|---|
 *    | `ALWAYS` | `always` |
 *    | `NEVER` | `never` |
 *    | **`PUSH_OTHER_TEAMS`** | **`pushOtherTeams`** |
 *    | **`PUSH_OWN_TEAM`** | **`pushOwnTeam`** |
 *    | **`HIDE_FOR_OTHER_TEAMS`** | **`hideForOtherTeams`** |
 *    | **`HIDE_FOR_OWN_TEAM`** | **`hideForOwnTeam`** |
 *
 *    **`always` / `never` 是全小写，其余四个是 camelCase**——同一枚举内两种风格。
 *
 *    且注意复数：`pushOtherTeams`（Teams 复数）但 `pushOwnTeam`（Team 单数）。
 *    同样的 `HIDE_FOR_OTHER_TEAMS`（复数）/ `HIDE_FOR_OWN_TEAM`（单数）。
 *
 * 3. **`BY_ID` 越界策略是 `ZERO`，返回 `ALWAYS`。**
 *
 *    | id | 结果 |
 *    |---|---|
 *    | `0` | `ALWAYS` |
 *    | `3` | 第 4 项 |
 *    | **`4` / `-1` / `1000`** | **`ALWAYS`**（越界）|
 *
 *    与 [`display_slot`](./display_slot.md) 的 `OutOfBoundsStrategy.ZERO` 完全一致。
 *
 * 4. **`id` 与序数一致（`0`…`3`），但 `BY_ID` 走 `rule -> rule.id` 而非序数。**
 *
 * 单元格式：
 * - `collisionName|<rule>` —— `CollisionRule.getSerializedName()`
 * - `collisionId|<rule>` —— `CollisionRule.id`
 * - `collisionById|<n>` —— `BY_ID.apply(n)`
 * - `collisionDisplay|<rule>` —— `getDisplayName()` 的翻译键
 * - `visibilityName|<visibility>` —— `Visibility.getSerializedName()`
 * - `visibilityId|<visibility>` —— `Visibility.id`
 * - `visibilityById|<n>` —— `BY_ID.apply(n)`
 * - `visibilityDisplay|<visibility>` —— `getDisplayName()` 的翻译键
 * - `collisionCount` / `visibilityCount` —— 枚举项数
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

/// 一个枚举项（`CollisionRule` 与 `Visibility` 结构相同）。
struct TeamRule {
    const char* enumName;
    const char* serializedName;
    i32 id;
};

constexpr TeamRule COLLISION_RULES[] = {
    {"ALWAYS", "always", 0},
    {"NEVER", "never", 1},
    {"PUSH_OTHER_TEAMS", "pushOtherTeams", 2},
    {"PUSH_OWN_TEAM", "pushOwnTeam", 3},
};

constexpr TeamRule VISIBILITIES[] = {
    {"ALWAYS", "always", 0},
    {"NEVER", "never", 1},
    {"HIDE_FOR_OTHER_TEAMS", "hideForOtherTeams", 2},
    {"HIDE_FOR_OWN_TEAM", "hideForOwnTeam", 3},
};

constexpr i32 RULE_COUNT = 4;

const TeamRule* findRule(const TeamRule* table, const std::string& name)
{
    for (i32 i = 0; i < RULE_COUNT; ++i) {
        if (name == table[i].enumName) {
            return &table[i];
        }
    }
    throw std::runtime_error("team_rule: 未知枚举项：" + name);
}

/// `ByIdMap.continuous(..., OutOfBoundsStrategy.ZERO)`：越界返回 `values()[0]`。
const TeamRule& byId(const TeamRule* table, i32 id)
{
    if (id >= 0 && id < RULE_COUNT) {
        return table[id];
    }
    return table[0];
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // collisionName：全小写 vs camelCase
        "collisionName|ALWAYS",
        "collisionName|NEVER",
        "collisionName|PUSH_OTHER_TEAMS",
        "collisionName|PUSH_OWN_TEAM",
        // collisionId
        "collisionId|ALWAYS",
        "collisionId|PUSH_OWN_TEAM",
        // collisionById：边界与越界
        "collisionById|0",
        "collisionById|3",
        "collisionById|4",
        "collisionById|-1",
        "collisionById|1000",
        // collisionDisplay：翻译键
        "collisionDisplay|ALWAYS",
        "collisionDisplay|PUSH_OTHER_TEAMS",
        // visibilityName：复数与单数
        "visibilityName|ALWAYS",
        "visibilityName|NEVER",
        "visibilityName|HIDE_FOR_OTHER_TEAMS",
        "visibilityName|HIDE_FOR_OWN_TEAM",
        // visibilityId
        "visibilityId|ALWAYS",
        "visibilityId|HIDE_FOR_OWN_TEAM",
        // visibilityById
        "visibilityById|0",
        "visibilityById|3",
        "visibilityById|4",
        "visibilityById|-1",
        // visibilityDisplay
        "visibilityDisplay|ALWAYS",
        "visibilityDisplay|HIDE_FOR_OWN_TEAM",
        // count
        "collisionCount",
        "visibilityCount",
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
        throw std::runtime_error("team_rule: 空单元");
    }

    const std::string mode = parts[0];
    std::string value;

    if (mode == "collisionCount" && parts.size() == 1) {
        value = std::to_string(RULE_COUNT);
    } else if (mode == "visibilityCount" && parts.size() == 1) {
        value = std::to_string(RULE_COUNT);
    } else if (parts.size() != 2) {
        throw std::runtime_error("team_rule: 单元段数不符：" + unit);
    } else if (mode == "collisionName") {
        value = findRule(COLLISION_RULES, parts[1])->serializedName;
    } else if (mode == "collisionId") {
        value = std::to_string(findRule(COLLISION_RULES, parts[1])->id);
    } else if (mode == "collisionById") {
        value = byId(COLLISION_RULES, static_cast<i32>(std::stoll(parts[1]))).enumName;
    } else if (mode == "collisionDisplay") {
        value = std::string("team.collision.") + findRule(COLLISION_RULES, parts[1])->serializedName;
    } else if (mode == "visibilityName") {
        value = findRule(VISIBILITIES, parts[1])->serializedName;
    } else if (mode == "visibilityId") {
        value = std::to_string(findRule(VISIBILITIES, parts[1])->id);
    } else if (mode == "visibilityById") {
        value = byId(VISIBILITIES, static_cast<i32>(std::stoll(parts[1]))).enumName;
    } else if (mode == "visibilityDisplay") {
        value = std::string("team.visibility.") + findRule(VISIBILITIES, parts[1])->serializedName;
    } else {
        throw std::runtime_error("team_rule: 无法识别的单元：" + unit);
    }

    return json{{"unit", unit}, {"value", value}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("team_rule", &writeList, &runLine)

} // namespace mc::diff
