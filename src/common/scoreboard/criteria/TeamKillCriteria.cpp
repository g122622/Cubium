#include "TeamKillCriteria.hpp"
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
