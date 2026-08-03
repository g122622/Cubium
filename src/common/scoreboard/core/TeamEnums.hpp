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
 * @brief 从字符串解析 TeamVisibility
 *
 * @param str 字符串表示
 * @return 可见性枚举值，解析失败返回 Always
 */
[[nodiscard]] inline TeamVisibility teamVisibilityFromString(const std::string& str) noexcept
{
    if (str == "never") {
        return TeamVisibility::Never;
    } else if (str == "hideForOtherTeams") {
        return TeamVisibility::HideForOtherTeams;
    } else if (str == "hideForOwnTeam") {
        return TeamVisibility::HideForOwnTeam;
    }
    return TeamVisibility::Always;
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
 * @brief 从字符串解析 TeamCollisionRule
 *
 * @param str 字符串表示
 * @return 碰撞规则枚举值，解析失败返回 Always
 */
[[nodiscard]] inline TeamCollisionRule teamCollisionRuleFromString(const std::string& str) noexcept
{
    if (str == "never") {
        return TeamCollisionRule::Never;
    } else if (str == "pushOtherTeams") {
        return TeamCollisionRule::PushOtherTeams;
    } else if (str == "pushOwnTeam") {
        return TeamCollisionRule::PushOwnTeam;
    }
    return TeamCollisionRule::Always;
}

} // namespace mc::scoreboard
