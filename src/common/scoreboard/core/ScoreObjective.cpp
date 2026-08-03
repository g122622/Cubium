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

#include "common/scoreboard/core/ScoreObjective.hpp"
#include "common/scoreboard/core/ScoreCriteria.hpp"
#include "common/scoreboard/core/ScoreCriteriaRenderType.hpp"
#include "common/scoreboard/core/Scoreboard.hpp"
#include "common/util/text/ComponentUtils.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/TextEvents.hpp"
#include "common/util/text/TextStyle.hpp"
#include <memory>
#include <string>
#include <utility>

namespace mc::scoreboard {

ScoreObjective::ScoreObjective(Scoreboard& scoreboard,
    const std::string& name,
    ScoreCriteria& criteria,
    std::unique_ptr<text::ITextComponent> displayName,
    RenderType renderType)
    : m_scoreboard(scoreboard)
    , m_name(name)
    , m_criteria(&criteria)
    , m_displayName(displayName ? std::move(displayName) : std::make_unique<text::StringTextComponent>(name))
    , m_formattedDisplayName(createFormattedDisplayName())
    , m_renderType(renderType)
{}

text::ITextComponent* ScoreObjective::getDisplayName() const noexcept
{
    return m_displayName.get();
}

const text::ITextComponent& ScoreObjective::getFormattedDisplayName() const noexcept
{
    return *m_formattedDisplayName;
}

std::unique_ptr<text::ITextComponent> ScoreObjective::createFormattedDisplayName() const
{
    // 深拷贝显示名称，设置悬停事件显示目标内部名称，然后用方括号包裹
    auto displayNameCopy = m_displayName->deepCopy();
    text::Style style = displayNameCopy->getStyle();
    style.setHoverEvent(text::HoverEvent::showText(m_name));
    displayNameCopy->setStyle(style);

    return text::ComponentUtils::wrapInSquareBrackets(std::move(displayNameCopy));
}

void ScoreObjective::setDisplayName(std::unique_ptr<text::ITextComponent> displayName)
{
    m_displayName = std::move(displayName);
    m_formattedDisplayName = createFormattedDisplayName();
    m_scoreboard.onObjectiveChanged(*this);
}

void ScoreObjective::setRenderType(RenderType renderType)
{
    if (m_renderType != renderType) {
        m_renderType = renderType;
        m_scoreboard.onObjectiveChanged(*this);
    }
}

} // namespace mc::scoreboard
