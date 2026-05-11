#include "ScoreObjective.hpp"
#include "Scoreboard.hpp"
#include "ScoreCriteria.hpp"
#include "../../util/text/StringTextComponent.hpp"

namespace mc::scoreboard {

ScoreObjective::ScoreObjective(Scoreboard& scoreboard,
                               const std::string& name,
                               ScoreCriteria& criteria,
                               std::unique_ptr<text::ITextComponent> displayName,
                               RenderType renderType)
    : m_scoreboard(scoreboard)
    , m_name(name)
    , m_criteria(&criteria)
    , m_displayName(displayName ? std::move(displayName)
                                : std::make_unique<text::StringTextComponent>(name))
    , m_renderType(renderType)
{
}

text::ITextComponent* ScoreObjective::getDisplayName() const noexcept {
    return m_displayName.get();
}

std::unique_ptr<text::ITextComponent> ScoreObjective::getFormattedDisplayName() const {
    // TODO: 实现带悬浮提示的格式化显示名称
    // MC 原版会在名称周围加上方括号和悬浮提示
    return m_displayName ? m_displayName->shallowCopy() : nullptr;
}

void ScoreObjective::setDisplayName(std::unique_ptr<text::ITextComponent> displayName) {
    m_displayName = std::move(displayName);
    m_scoreboard.onObjectiveChanged(*this);
}

void ScoreObjective::setRenderType(RenderType renderType) {
    if (m_renderType != renderType) {
        m_renderType = renderType;
        m_scoreboard.onObjectiveChanged(*this);
    }
}

} // namespace mc::scoreboard
