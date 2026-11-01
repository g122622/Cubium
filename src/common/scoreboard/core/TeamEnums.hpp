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

#pragma once

#include "common/core/Types.hpp"
#include <optional>
#include <string>

namespace mc::scoreboard {

/**
 * @brief 队伍名称标签可见性
 *
 * 控制队伍成员名称标签的显示方式。
 */
enum class TeamVisibility : u8 {
    /// 总是显示
    Always = 0,

    /// 从不显示
    Never = 1,

    /// 对其他队伍隐藏
    HideForOtherTeams = 2,

    /// 对自己队伍隐藏
    HideForOwnTeam = 3
};

/**
 * @brief 队伍碰撞规则
 *
 * 控制队伍成员之间的碰撞行为。
 */
enum class TeamCollisionRule : u8 {
    /// 总是碰撞
    Always = 0,

    /// 从不碰撞
    Never = 1,

    /// 推动其他队伍
    PushOtherTeams = 2,

    /// 推动自己队伍
    PushOwnTeam = 3
};

/**
 * @brief 将 TeamVisibility 转换为字符串
 *
 * @param visibility 可见性枚举值
 * @return 字符串表示
 */
[[nodiscard]] inline const char* teamVisibilityToString(TeamVisibility visibility) noexcept
{
    switch (visibility) {
        case TeamVisibility::Always:
            return "always";
        case TeamVisibility::Never:
            return "never";
        case TeamVisibility::HideForOtherTeams:
            return "hideForOtherTeams";
        case TeamVisibility::HideForOwnTeam:
            return "hideForOwnTeam";
        default:
            return "always";
    }
}

/**
 * @brief 严格解析 TeamVisibility（对应原版 CODEC.byName）
 *
 * 与原版语义一致：未知输入返回 std::nullopt，**不回退**。
 * 存档读取等需要「显式拒绝/保留原值」的场景应使用本函数。
 *
 * @param str 字符串表示
 * @return 解析成功返回枚举值，失败返回 std::nullopt
 */
[[nodiscard]] inline std::optional<TeamVisibility> teamVisibilityParse(const std::string& str) noexcept
{
    if (str == "always") {
        return TeamVisibility::Always;
    } else if (str == "never") {
        return TeamVisibility::Never;
    } else if (str == "hideForOtherTeams") {
        return TeamVisibility::HideForOtherTeams;
    } else if (str == "hideForOwnTeam") {
        return TeamVisibility::HideForOwnTeam;
    }
    return std::nullopt;
}

/**
 * @brief 从字符串解析 TeamVisibility（宽松入口）
 *
 * **未知输入回退到 Always**——调用方若需要拒绝非法输入，请改用 teamVisibilityParse。
 *
 * @param str 字符串表示
 * @return 可见性枚举值，解析失败返回 Always
 */
[[nodiscard]] inline TeamVisibility teamVisibilityFromString(const std::string& str) noexcept
{
    return teamVisibilityParse(str).value_or(TeamVisibility::Always);
}

/**
 * @brief 将 TeamCollisionRule 转换为字符串
 *
 * @param rule 碰撞规则枚举值
 * @return 字符串表示
 */
[[nodiscard]] inline const char* teamCollisionRuleToString(TeamCollisionRule rule) noexcept
{
    switch (rule) {
        case TeamCollisionRule::Always:
            return "always";
        case TeamCollisionRule::Never:
            return "never";
        case TeamCollisionRule::PushOtherTeams:
            return "pushOtherTeams";
        case TeamCollisionRule::PushOwnTeam:
            return "pushOwnTeam";
        default:
            return "always";
    }
}

/**
 * @brief 严格解析 TeamCollisionRule（对应原版 CODEC.byName）
 *
 * @param str 字符串表示
 * @return 解析成功返回枚举值，失败返回 std::nullopt
 */
[[nodiscard]] inline std::optional<TeamCollisionRule> teamCollisionRuleParse(const std::string& str) noexcept
{
    if (str == "always") {
        return TeamCollisionRule::Always;
    } else if (str == "never") {
        return TeamCollisionRule::Never;
    } else if (str == "pushOtherTeams") {
        return TeamCollisionRule::PushOtherTeams;
    } else if (str == "pushOwnTeam") {
        return TeamCollisionRule::PushOwnTeam;
    }
    return std::nullopt;
}

/**
 * @brief 从字符串解析 TeamCollisionRule（宽松入口）
 *
 * @param str 字符串表示
 * @return 碰撞规则枚举值，解析失败返回 Always
 */
[[nodiscard]] inline TeamCollisionRule teamCollisionRuleFromString(const std::string& str) noexcept
{
    return teamCollisionRuleParse(str).value_or(TeamCollisionRule::Always);
}

} // namespace mc::scoreboard
