#include "ScoreObjective.hpp"
#include "../../util/text/StringTextComponent.hpp"
#include "../../util/text/TextEvents.hpp"
#include "../../util/text/TextStyle.hpp"
#include "ScoreCriteria.hpp"
#include "Scoreboard.hpp"

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
    , m_renderType(renderType)
{}

text::ITextComponent* ScoreObjective::getDisplayName() const noexcept
{
    return m_displayName.get();
}

std::unique_ptr<text::ITextComponent> ScoreObjective::getFormattedDisplayName() const
{
    // 参考 MC 1.16.5: ScoreObjective.func_237498_g_()
    // 1. 深拷贝显示名称
    // 2. 添加悬停事件，悬停时显示目标内部名称
    // 3. 用方括号包裹

    if (!m_displayName) {
        // 如果没有显示名称，创建一个使用目标名称的组件
        auto displayName = std::make_unique<text::StringTextComponent>(m_name);

        // 设置悬停事件
        text::Style style;
        style.setHoverEvent(text::HoverEvent::showText(m_name));
        displayName->setStyle(style);

        // 用方括号包裹
        auto result = std::make_unique<text::StringTextComponent>("[");
        result->append(std::move(displayName));
        result->append(std::make_unique<text::StringTextComponent>("]"));

        return result;
    }

    // 深拷贝显示名称
    auto displayNameCopy = m_displayName->deepCopy();

    // 获取当前样式并设置悬停事件
    text::Style style = displayNameCopy->getStyle();
    style.setHoverEvent(text::HoverEvent::showText(m_name));
    displayNameCopy->setStyle(style);

    // 用方括号包裹：创建 "[" 组件，追加带悬停的显示名称，追加 "]"
    // 参考 MC 1.16.5: TextComponentUtils.wrapWithSquareBrackets()
    // 该方法使用翻译键 "chat.square_brackets"，即 "[%s]"
    // 由于翻译系统未完成，直接用 StringTextComponent 组合
    auto result = std::make_unique<text::StringTextComponent>("[");
    result->append(std::move(displayNameCopy));
    result->append(std::make_unique<text::StringTextComponent>("]"));

    return result;
}

void ScoreObjective::setDisplayName(std::unique_ptr<text::ITextComponent> displayName)
{
    m_displayName = std::move(displayName);
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
