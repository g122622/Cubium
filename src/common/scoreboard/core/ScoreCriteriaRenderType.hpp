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
#include <cstddef>
#include <optional>
#include <string>

namespace mc::scoreboard {

/**
 * @brief 分数渲染类型
 *
 * 控制分数在记分板中的显示方式。
 */
enum class RenderType : u8 {
    /// 显示为整数
    Integer = 0,

    /// 显示为心形（用于生命值等）
    Hearts = 1
};

/**
 * @brief 将 RenderType 转换为字符串
 *
 * @param renderType 渲染类型枚举值
 * @return 字符串表示
 */
[[nodiscard]] inline const char* renderTypeToString(RenderType renderType) noexcept
{
    return renderType == RenderType::Hearts ? "hearts" : "integer";
}

/**
 * @brief 严格解析 RenderType（对应原版 ObjectiveCriteria.RenderType.byId）
 *
 * 与原版 `CODEC.byName` 语义一致：未知输入返回 std::nullopt，**不回退**。
 * 存档读取等需要「显式拒绝/保留原值」的场景应使用本函数。
 *
 * @param str 字符串表示
 * @return 解析成功返回枚举值，失败返回 std::nullopt
 */
[[nodiscard]] inline std::optional<RenderType> renderTypeParse(const std::string& str) noexcept
{
    if (str == "integer") {
        return RenderType::Integer;
    }
    if (str == "hearts") {
        return RenderType::Hearts;
    }
    return std::nullopt;
}

/**
 * @brief 从字符串解析 RenderType（宽松入口）
 *
 * **未知输入回退到 Integer**——调用方若需要拒绝非法输入，请改用 renderTypeParse。
 *
 * @param str 字符串表示
 * @return 渲染类型枚举值，解析失败返回 Integer
 */
[[nodiscard]] inline RenderType renderTypeFromString(const std::string& str) noexcept
{
    return renderTypeParse(str).value_or(RenderType::Integer);
}

/**
 * @brief 显示槽位枚举
 *
 * 定义记分板可以显示的位置。
 *
 * 槽位说明：
 * - 0: list (Tab 列表)
 * - 1: sidebar (侧边栏)
 * - 2: belowName (名称下方)
 * - 3-18: sidebar.team.{color} (队伍专属侧边栏，16种颜色)
 */
enum class DisplaySlot : u8 {
    /// Tab 列表显示
    List = 0,

    /// 侧边栏显示
    Sidebar = 1,

    /// 名称下方显示
    BelowName = 2,

    // 队伍侧边栏（按颜色索引）
    SidebarTeamBlack = 3,
    SidebarTeamDarkBlue = 4,
    SidebarTeamDarkGreen = 5,
    SidebarTeamDarkAqua = 6,
    SidebarTeamDarkRed = 7,
    SidebarTeamDarkPurple = 8,
    SidebarTeamGold = 9,
    SidebarTeamGray = 10,
    SidebarTeamDarkGray = 11,
    SidebarTeamBlue = 12,
    SidebarTeamGreen = 13,
    SidebarTeamAqua = 14,
    SidebarTeamRed = 15,
    SidebarTeamLightPurple = 16,
    SidebarTeamYellow = 17,
    SidebarTeamWhite = 18
};

/// 显示槽位数量
constexpr size_t DISPLAY_SLOT_COUNT = 19;

/**
 * @brief 将 DisplaySlot 转换为字符串
 *
 * @param slot 显示槽位枚举值
 * @return 字符串表示
 */
[[nodiscard]] std::string displaySlotToString(DisplaySlot slot);

/**
 * @brief 从字符串解析 DisplaySlot
 *
 * @param str 字符串表示
 * @return 显示槽位枚举值，解析失败返回 nullopt
 */
[[nodiscard]] std::optional<DisplaySlot> displaySlotFromString(const std::string& str);

} // namespace mc::scoreboard
