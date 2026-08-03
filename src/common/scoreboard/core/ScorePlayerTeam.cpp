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

#include "ScorePlayerTeam.hpp"
#include "Scoreboard.hpp"
#include "common/core/Types.hpp"
#include "common/scoreboard/core/Team.hpp"
#include "common/scoreboard/core/TeamEnums.hpp"
#include "common/util/text/ComponentUtils.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/TextEvents.hpp"
#include "common/util/text/TextStyle.hpp"
#include <memory>
#include <string>
#include <utility>

namespace mc::scoreboard {

ScorePlayerTeam::ScorePlayerTeam(Scoreboard& scoreboard, const std::string& name)
    : m_scoreboard(scoreboard)
    , m_name(name)
    , m_displayName(std::make_unique<text::StringTextComponent>(name))
    , m_prefix(std::make_unique<text::StringTextComponent>(""))
    , m_suffix(std::make_unique<text::StringTextComponent>(""))
{}

const text::ITextComponent* ScorePlayerTeam::getDisplayName() const noexcept
{
    return m_displayName.get();
}

void ScorePlayerTeam::setDisplayName(std::unique_ptr<text::ITextComponent> displayName)
{
    m_displayName = std::move(displayName);
    m_scoreboard.onTeamChanged(*this);
}

bool ScorePlayerTeam::addMember(const std::string& playerName)
{
    auto result = m_members.insert(playerName);
    if (result.second) {
        m_scoreboard.onTeamChanged(*this);
        return true;
    }
    return false;
}

bool ScorePlayerTeam::removeMember(const std::string& playerName)
{
    auto it = m_members.find(playerName);
    if (it != m_members.end()) {
        m_members.erase(it);
        m_scoreboard.onTeamChanged(*this);
        return true;
    }
    return false;
}

bool ScorePlayerTeam::hasMember(const std::string& playerName) const
{
    return m_members.count(playerName) > 0;
}

void ScorePlayerTeam::clearMembers()
{
    if (!m_members.empty()) {
        m_members.clear();
        m_scoreboard.onTeamChanged(*this);
    }
}

const text::ITextComponent* ScorePlayerTeam::getPrefix() const noexcept
{
    return m_prefix.get();
}

void ScorePlayerTeam::setPrefix(std::unique_ptr<text::ITextComponent> prefix)
{
    m_prefix = prefix ? std::move(prefix) : std::make_unique<text::StringTextComponent>("");
    m_scoreboard.onTeamChanged(*this);
}

const text::ITextComponent* ScorePlayerTeam::getSuffix() const noexcept
{
    return m_suffix.get();
}

void ScorePlayerTeam::setSuffix(std::unique_ptr<text::ITextComponent> suffix)
{
    m_suffix = suffix ? std::move(suffix) : std::make_unique<text::StringTextComponent>("");
    m_scoreboard.onTeamChanged(*this);
}

void ScorePlayerTeam::setColor(TextFormatting color)
{
    if (m_color != color) {
        m_color = color;
        m_scoreboard.onTeamChanged(*this);
    }
}

void ScorePlayerTeam::setAllowFriendlyFire(bool allow)
{
    if (m_allowFriendlyFire != allow) {
        m_allowFriendlyFire = allow;
        m_scoreboard.onTeamChanged(*this);
    }
}

void ScorePlayerTeam::setSeeFriendlyInvisibles(bool see)
{
    if (m_seeFriendlyInvisibles != see) {
        m_seeFriendlyInvisibles = see;
        m_scoreboard.onTeamChanged(*this);
    }
}

void ScorePlayerTeam::setNameTagVisibility(TeamVisibility visibility)
{
    if (m_nameTagVisibility != visibility) {
        m_nameTagVisibility = visibility;
        m_scoreboard.onTeamChanged(*this);
    }
}

void ScorePlayerTeam::setDeathMessageVisibility(TeamVisibility visibility)
{
    if (m_deathMessageVisibility != visibility) {
        m_deathMessageVisibility = visibility;
        m_scoreboard.onTeamChanged(*this);
    }
}

void ScorePlayerTeam::setCollisionRule(TeamCollisionRule rule)
{
    if (m_collisionRule != rule) {
        m_collisionRule = rule;
        m_scoreboard.onTeamChanged(*this);
    }
}

std::unique_ptr<text::ITextComponent> ScorePlayerTeam::getFormattedDisplayName() const
{
    // 深拷贝显示名称，设置悬停事件显示队伍内部名称
    auto displayNameCopy = m_displayName->deepCopy();
    text::Style style = displayNameCopy->getStyle();
    style.setHoverEvent(text::HoverEvent::showText(m_name));
    displayNameCopy->setStyle(style);

    // 用方括号包裹
    auto result = text::ComponentUtils::wrapInSquareBrackets(std::move(displayNameCopy));

    // 如果颜色不是 Reset，将队伍颜色应用到包裹后的组件
    if (m_color != TextFormatting::Reset) {
        text::Style resultStyle = result->getStyle();
        resultStyle.setColor(m_color);
        result->setStyle(resultStyle);
    }

    return result;
}

std::unique_ptr<text::ITextComponent> ScorePlayerTeam::formatName(const text::ITextComponent& name) const
{
    // 创建空根组件，依次追加 prefix + name + suffix
    // 然后将队伍颜色应用到整个组件

    // 创建空根组件
    auto result = std::make_unique<text::StringTextComponent>("");

    // 追加前缀（深拷贝）
    if (m_prefix) {
        result->append(m_prefix->deepCopy());
    }

    // 追加名称（深拷贝，保留原有样式）
    result->append(name.deepCopy());

    // 追加后缀（深拷贝）
    if (m_suffix) {
        result->append(m_suffix->deepCopy());
    }

    // 如果颜色不是 Reset，将颜色应用到根组件
    // 子组件会通过样式继承机制继承此颜色
    if (m_color != TextFormatting::Reset) {
        text::Style style = result->getStyle();
        style.setColor(m_color);
        result->setStyle(style);
    }

    return result;
}

u8 ScorePlayerTeam::getFriendlyFlags() const noexcept
{
    u8 flags = 0;
    if (m_allowFriendlyFire) {
        flags |= 0x01;
    }
    if (m_seeFriendlyInvisibles) {
        flags |= 0x02;
    }
    return flags;
}

void ScorePlayerTeam::setFriendlyFlags(u8 flags)
{
    m_allowFriendlyFire = (flags & 0x01) != 0;
    m_seeFriendlyInvisibles = (flags & 0x02) != 0;
    m_scoreboard.onTeamChanged(*this);
}

} // namespace mc::scoreboard
