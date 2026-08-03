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

#include "ScoreCriteriaRenderType.hpp"
#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>

namespace mc::scoreboard {

namespace {

// ============================================================================
// 常量定义
// ============================================================================

/// 队伍颜色数量（16种颜色）
constexpr size_t TEAM_COLOR_COUNT = 16;

// ============================================================================
// 查找表
// ============================================================================

/// 颜色名称到 DisplaySlot 的映射表
const std::unordered_map<std::string, DisplaySlot> s_colorSlotMap = {{"black", DisplaySlot::SidebarTeamBlack},
    {"dark_blue", DisplaySlot::SidebarTeamDarkBlue},
    {"dark_green", DisplaySlot::SidebarTeamDarkGreen},
    {"dark_aqua", DisplaySlot::SidebarTeamDarkAqua},
    {"dark_red", DisplaySlot::SidebarTeamDarkRed},
    {"dark_purple", DisplaySlot::SidebarTeamDarkPurple},
    {"gold", DisplaySlot::SidebarTeamGold},
    {"gray", DisplaySlot::SidebarTeamGray},
    {"dark_gray", DisplaySlot::SidebarTeamDarkGray},
    {"blue", DisplaySlot::SidebarTeamBlue},
    {"green", DisplaySlot::SidebarTeamGreen},
    {"aqua", DisplaySlot::SidebarTeamAqua},
    {"red", DisplaySlot::SidebarTeamRed},
    {"light_purple", DisplaySlot::SidebarTeamLightPurple},
    {"yellow", DisplaySlot::SidebarTeamYellow},
    {"white", DisplaySlot::SidebarTeamWhite}};

/// 颜色索引到名称的数组
const char* s_slotColorNames[TEAM_COLOR_COUNT] = {"black",
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

} // namespace

// ============================================================================
// 公共函数实现
// ============================================================================

std::string displaySlotToString(DisplaySlot slot)
{
    switch (slot) {
        case DisplaySlot::List:
            return "list";
        case DisplaySlot::Sidebar:
            return "sidebar";
        case DisplaySlot::BelowName:
            return "belowName";
        default: {
            // 队伍侧边栏
            const size_t index = static_cast<size_t>(slot) - static_cast<size_t>(DisplaySlot::SidebarTeamBlack);
            if (index < TEAM_COLOR_COUNT) {
                return std::string("sidebar.team.") + s_slotColorNames[index];
            }
            return "list";
        }
    }
}

std::optional<DisplaySlot> displaySlotFromString(const std::string& str)
{
    if (str == "list") {
        return DisplaySlot::List;
    }
    if (str == "sidebar") {
        return DisplaySlot::Sidebar;
    }
    if (str == "belowName") {
        return DisplaySlot::BelowName;
    }

    // 检查队伍侧边栏格式: sidebar.team.{color}
    const std::string prefix = "sidebar.team.";
    if (str.size() > prefix.size() && str.substr(0, prefix.size()) == prefix) {
        const std::string colorName = str.substr(prefix.size());
        auto it = s_colorSlotMap.find(colorName);
        if (it != s_colorSlotMap.end()) {
            return it->second;
        }
    }

    return std::nullopt;
}

} // namespace mc::scoreboard
