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

#include "TeamKillCriteria.hpp"
#include "common/util/text/TextStyle.hpp"
#include <string>
#include <unordered_set>

namespace mc::scoreboard {

// 导入需要的函数
using text::toName;

namespace {

// 支持队伍击杀判据的颜色
const std::unordered_set<TextFormatting> s_supportedColors = {TextFormatting::Black,
    TextFormatting::DarkBlue,
    TextFormatting::DarkGreen,
    TextFormatting::DarkAqua,
    TextFormatting::DarkRed,
    TextFormatting::DarkPurple,
    TextFormatting::Gold,
    TextFormatting::Gray,
    TextFormatting::DarkGray,
    TextFormatting::Blue,
    TextFormatting::Green,
    TextFormatting::Aqua,
    TextFormatting::Red,
    TextFormatting::LightPurple,
    TextFormatting::Yellow,
    TextFormatting::White};

} // namespace

TeamKillCriteria::TeamKillCriteria(TextFormatting color, Type type)
    : m_color(color)
    , m_type(type)
{
    m_name = generateName(color, type);
}

std::string TeamKillCriteria::generateName(TextFormatting color, Type type)
{
    std::string colorName = toName(color);
    std::string prefix = (type == Type::TeamKill) ? "teamkill." : "killedByTeam.";
    return prefix + colorName;
}

bool TeamKillCriteria::matches(TextFormatting killerTeam, TextFormatting victimTeam) const
{
    if (m_type == Type::TeamKill) {
        // 玩家击杀指定颜色队伍的成员
        return victimTeam == m_color;
    } else {
        // 玩家被指定颜色队伍的成员击杀
        return killerTeam == m_color;
    }
}

bool TeamKillCriteria::isSupportedColor(TextFormatting color)
{
    return s_supportedColors.count(color) > 0;
}

} // namespace mc::scoreboard
