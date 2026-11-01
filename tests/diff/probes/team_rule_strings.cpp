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
 * @brief 差分能力：`Team.Visibility` / `Team.CollisionRule` 的字符串双向映射。
 *
 * **本探针调用 Cubium 的真实实现**
 * （`mc::scoreboard::teamVisibilityToString` / `teamVisibilityFromString` /
 * `teamCollisionRuleToString` / `teamCollisionRuleFromString`）。
 *
 * Cubium 侧（`common/scoreboard/core/TeamEnums.hpp`）：
 *
 * ```cpp
 * enum class TeamVisibility : u8 { Always, Never, HideForOtherTeams, HideForOwnTeam };
 * enum class TeamCollisionRule : u8 { Always, Never, PushOtherTeams, PushOwnTeam };
 *
 * const char* teamVisibilityToString(TeamVisibility) noexcept;
 * TeamVisibility teamVisibilityFromString(const std::string&) noexcept;   // 未知 → Always
 * const char* teamCollisionRuleToString(TeamCollisionRule) noexcept;
 * TeamCollisionRule teamCollisionRuleFromString(const std::string&) noexcept; // 未知 → Always
 * ```
 *
 * 原版侧：
 *
 * ```java
 * public enum Visibility implements StringRepresentable {
 *     ALWAYS("always"),
 *     NEVER("never"),
 *     HIDE_FOR_OTHER_TEAMS("hideForOtherTeams"),
 *     HIDE_FOR_OWN_TEAM("hideForOwnTeam");
 * }
 * public enum CollisionRule implements StringRepresentable {
 *     ALWAYS("always"),
 *     NEVER("never"),
 *     PUSH_OTHER_TEAMS("pushOtherTeams"),
 *     PUSH_OWN_TEAM("pushOwnTeam");
 * }
 * ```
 *
 * ## 三个关键点
 *
 * 1. **序列化名是驼峰，不是下划线。**
 *
 *    | 枚举 | `getSerializedName()` |
 *    |---|---|
 *    | `ALWAYS` | `always` |
 *    | `NEVER` | `never` |
 *    | **`HIDE_FOR_OTHER_TEAMS`** | **`hideForOtherTeams`** |
 *    | **`HIDE_FOR_OWN_TEAM`** | **`hideForOwnTeam`** |
 *    | **`PUSH_OTHER_TEAMS`** | **`pushOtherTeams`** |
 *    | **`PUSH_OWN_TEAM`** | **`pushOwnTeam`** |
 *
 *    **不是 `hide_for_other_teams`**——枚举名是 `SNAKE_CASE`，序列化名却是 `camelCase`。
 *    这与 [`permission_level`](./permission_level.md) 的「枚举与名字都是复数」又不同。
 *
 *    单个队伍项是**单数**（`HIDE_FOR_OWN_TEAM` → `hideForOwnTeam`），
 *    而「其他队伍」是**复数**（`HIDE_FOR_OTHER_TEAMS` → `hideForOtherTeams`）——
 *    这一层单复数差异在枚举名里已体现，序列化名只是照搬。
 *
 * 2. **`fromString` 对未知输入回退到 `Always`，原版 `CODEC.byName` 返回 `null`。**
 *
 *    | 输入 | Cubium `fromString` | 原版 `byName` |
 *    |---|---|---|
 *    | `hideForOtherTeams` | `HideForOtherTeams` | `HIDE_FOR_OTHER_TEAMS` |
 *    | **`dunno`** | **`Always`** | **`null`** |
 *    | **``（空串）** | **`Always`** | **`null`** |
 *
 *    **可用「原版 null」与「Cubium Always」的对照抓出差异**——本能力把原版的 `null`
 *    输出为字面量 `null`，Cubium 输出 `Always`，两者必须一致才说明不带回退。
 *
 * 3. **`fromString` 大小写敏感。**
 *
 *    Cubium 用 `str == "hideForOtherTeams"` 精确比较，没有 `tolower`。
 *    原版 `StringRepresentable.fromEnum` 的 `byName` 同样是精确匹配。
 *
 * 单元格式：
 * - `visibilityToString|<enum>` —— `teamVisibilityToString`
 * - `visibilityFromString|<text>` —— `teamVisibilityFromString`（未命中应输出 `null`）
 * - `collisionToString|<enum>` —— `teamCollisionRuleToString`
 * - `collisionFromString|<text>` —— `teamCollisionRuleFromString`（未命中应输出 `null`）
 * - `count` —— 两个枚举各有的项数
 */
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/scoreboard/core/TeamEnums.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 可见性枚举项。
struct VisibilityRef {
    const char* javaName;
    scoreboard::TeamVisibility cubium;
};

constexpr VisibilityRef VISIBILITIES[] = {
    {"ALWAYS", scoreboard::TeamVisibility::Always},
    {"NEVER", scoreboard::TeamVisibility::Never},
    {"HIDE_FOR_OTHER_TEAMS", scoreboard::TeamVisibility::HideForOtherTeams},
    {"HIDE_FOR_OWN_TEAM", scoreboard::TeamVisibility::HideForOwnTeam},
};

/// 碰撞规则枚举项。
struct CollisionRef {
    const char* javaName;
    scoreboard::TeamCollisionRule cubium;
};

constexpr CollisionRef COLLISIONS[] = {
    {"ALWAYS", scoreboard::TeamCollisionRule::Always},
    {"NEVER", scoreboard::TeamCollisionRule::Never},
    {"PUSH_OTHER_TEAMS", scoreboard::TeamCollisionRule::PushOtherTeams},
    {"PUSH_OWN_TEAM", scoreboard::TeamCollisionRule::PushOwnTeam},
};

constexpr i32 RULE_COUNT = 4;

const VisibilityRef* findVisibility(const std::string& name)
{
    for (const VisibilityRef& entry : VISIBILITIES) {
        if (name == entry.javaName) {
            return &entry;
        }
    }
    return nullptr;
}

const CollisionRef* findCollision(const std::string& name)
{
    for (const CollisionRef& entry : COLLISIONS) {
        if (name == entry.javaName) {
            return &entry;
        }
    }
    return nullptr;
}

/// 把 Cubium 的可见性值转回原版枚举名。
std::string toJavaVisibility(scoreboard::TeamVisibility value)
{
    for (const VisibilityRef& entry : VISIBILITIES) {
        if (entry.cubium == value) {
            return entry.javaName;
        }
    }
    return "null";
}

/// 把 Cubium 的碰撞规则值转回原版枚举名。
std::string toJavaCollision(scoreboard::TeamCollisionRule value)
{
    for (const CollisionRef& entry : COLLISIONS) {
        if (entry.cubium == value) {
            return entry.javaName;
        }
    }
    return "null";
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        "count",
        // visibility：正向
        "visibilityToString|ALWAYS",
        "visibilityToString|NEVER",
        "visibilityToString|HIDE_FOR_OTHER_TEAMS",
        "visibilityToString|HIDE_FOR_OWN_TEAM",
        // collision：正向
        "collisionToString|ALWAYS",
        "collisionToString|NEVER",
        "collisionToString|PUSH_OTHER_TEAMS",
        "collisionToString|PUSH_OWN_TEAM",
        // visibility：反向命中
        "visibilityFromString|always",
        "visibilityFromString|never",
        "visibilityFromString|hideForOtherTeams",
        "visibilityFromString|hideForOwnTeam",
        // visibility：反向未命中（Cubium 会回退到 Always，原版返回 null）
        "visibilityFromString|dunno",
        "visibilityFromString|",
        "visibilityFromString|ALWAYS",
        "visibilityFromString|hide_for_other_teams",
        // collision：反向命中
        "collisionFromString|always",
        "collisionFromString|never",
        "collisionFromString|pushOtherTeams",
        "collisionFromString|pushOwnTeam",
        // collision：反向未命中
        "collisionFromString|dunno",
        "collisionFromString|",
        "collisionFromString|push_other_teams",
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
        throw std::runtime_error("team_rule_strings: 空单元");
    }

    const std::string mode = parts[0];
    std::string value;

    if (mode == "count" && parts.size() == 1) {
        value = std::to_string(RULE_COUNT);
    } else if (parts.size() != 2) {
        throw std::runtime_error("team_rule_strings: 单元段数不符：" + unit);
    } else if (mode == "visibilityToString") {
        const VisibilityRef* entry = findVisibility(parts[1]);
        if (entry == nullptr) {
            throw std::runtime_error("team_rule_strings: 未知可见性：" + parts[1]);
        }
        value = scoreboard::teamVisibilityToString(entry->cubium);
    } else if (mode == "visibilityFromString") {
        // 严格解析（等价原版 CODEC.byName）：未知值应为 nullopt。
        const auto parsed = scoreboard::teamVisibilityParse(parts[1]);
        value = parsed ? toJavaVisibility(*parsed) : "null";
    } else if (mode == "collisionToString") {
        const CollisionRef* entry = findCollision(parts[1]);
        if (entry == nullptr) {
            throw std::runtime_error("team_rule_strings: 未知碰撞规则：" + parts[1]);
        }
        value = scoreboard::teamCollisionRuleToString(entry->cubium);
    } else if (mode == "collisionFromString") {
        const auto parsed = scoreboard::teamCollisionRuleParse(parts[1]);
        value = parsed ? toJavaCollision(*parsed) : "null";
    } else {
        throw std::runtime_error("team_rule_strings: 无法识别的单元：" + unit);
    }

    return json{{"unit", unit}, {"value", value}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("team_rule_strings", &writeList, &runLine)

} // namespace mc::diff
