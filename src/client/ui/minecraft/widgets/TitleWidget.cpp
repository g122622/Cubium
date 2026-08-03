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

#include "TitleWidget.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include "common/core/Types.hpp"
#include "common/network/protocol/TitleActions.hpp"
#include <optional>
#include <string>

namespace mc::client::ui::minecraft::widgets {

// ============================================================================
// 构造函数
// ============================================================================

TitleWidget::TitleWidget()
    : Widget("title")
{
    setVisible(true);
    setActive(true);
}

// ============================================================================
// 标题控制
// ============================================================================

void TitleWidget::setTitle(const std::string& text)
{
    m_title.text = text;
    m_title.elapsed = 0.0f;
    m_title.remainingTime = m_title.fadeInTime + m_title.stayTime + m_title.fadeOutTime;
    m_title.active = !text.empty();

    // 使用当前设置的时间（如果没有单独设置，使用默认值）
    if (m_title.fadeInTime == 0.0f && m_title.stayTime == 0.0f && m_title.fadeOutTime == 0.0f) {
        m_title.fadeInTime = m_defaultFadeIn;
        m_title.stayTime = m_defaultStay;
        m_title.fadeOutTime = m_defaultFadeOut;
        m_title.remainingTime = m_title.fadeInTime + m_title.stayTime + m_title.fadeOutTime;
    }
}

void TitleWidget::setSubtitle(const std::string& text)
{
    m_subtitle.text = text;
    m_subtitle.elapsed = 0.0f;
    m_subtitle.remainingTime = m_subtitle.fadeInTime + m_subtitle.stayTime + m_subtitle.fadeOutTime;
    m_subtitle.active = !text.empty();

    // 使用与主标题相同的时间
    if (m_subtitle.fadeInTime == 0.0f && m_subtitle.stayTime == 0.0f && m_subtitle.fadeOutTime == 0.0f) {
        m_subtitle.fadeInTime = m_title.fadeInTime > 0.0f ? m_title.fadeInTime : m_defaultFadeIn;
        m_subtitle.stayTime = m_title.stayTime > 0.0f ? m_title.stayTime : m_defaultStay;
        m_subtitle.fadeOutTime = m_title.fadeOutTime > 0.0f ? m_title.fadeOutTime : m_defaultFadeOut;
        m_subtitle.remainingTime = m_subtitle.fadeInTime + m_subtitle.stayTime + m_subtitle.fadeOutTime;
    }
}

void TitleWidget::setActionbar(const std::string& text)
{
    m_actionbar.text = text;
    m_actionbar.elapsed = 0.0f;
    m_actionbar.remainingTime = m_actionbar.fadeInTime + m_actionbar.stayTime + m_actionbar.fadeOutTime;
    m_actionbar.active = !text.empty();

    // 使用默认时间
    if (m_actionbar.fadeInTime == 0.0f && m_actionbar.stayTime == 0.0f && m_actionbar.fadeOutTime == 0.0f) {
        m_actionbar.fadeInTime = m_defaultFadeIn;
        m_actionbar.stayTime = m_defaultStay;
        m_actionbar.fadeOutTime = m_defaultFadeOut;
        m_actionbar.remainingTime = m_actionbar.fadeInTime + m_actionbar.stayTime + m_actionbar.fadeOutTime;
    }
}

void TitleWidget::setTimes(i32 fadeIn, i32 stay, i32 fadeOut)
{
    // 将 tick 转换为秒（1 tick = 50ms = 0.05s）
    constexpr f32 TICK_TO_SECONDS = 0.05f;

    m_defaultFadeIn = static_cast<f32>(fadeIn) * TICK_TO_SECONDS;
    m_defaultStay = static_cast<f32>(stay) * TICK_TO_SECONDS;
    m_defaultFadeOut = static_cast<f32>(fadeOut) * TICK_TO_SECONDS;

    // 如果标题正在显示，也更新其时间
    if (m_title.active) {
        m_title.fadeInTime = m_defaultFadeIn;
        m_title.stayTime = m_defaultStay;
        m_title.fadeOutTime = m_defaultFadeOut;
        m_title.remainingTime = m_title.fadeInTime + m_title.stayTime + m_title.fadeOutTime;
    }
    if (m_subtitle.active) {
        m_subtitle.fadeInTime = m_defaultFadeIn;
        m_subtitle.stayTime = m_defaultStay;
        m_subtitle.fadeOutTime = m_defaultFadeOut;
        m_subtitle.remainingTime = m_subtitle.fadeInTime + m_subtitle.stayTime + m_subtitle.fadeOutTime;
    }
    if (m_actionbar.active) {
        m_actionbar.fadeInTime = m_defaultFadeIn;
        m_actionbar.stayTime = m_defaultStay;
        m_actionbar.fadeOutTime = m_defaultFadeOut;
        m_actionbar.remainingTime = m_actionbar.fadeInTime + m_actionbar.stayTime + m_actionbar.fadeOutTime;
    }
}

void TitleWidget::clear()
{
    // 立即清除标题和副标题（不显示淡出动画）
    m_title.active = false;
    m_title.text = std::nullopt;
    m_subtitle.active = false;
    m_subtitle.text = std::nullopt;
}

void TitleWidget::reset()
{
    // 重置所有状态到默认值
    clear();

    m_actionbar.active = false;
    m_actionbar.text = std::nullopt;

    // 重置时间为默认值
    m_defaultFadeIn = 0.5f;  // 10 ticks
    m_defaultStay = 3.5f;    // 70 ticks
    m_defaultFadeOut = 1.0f; // 20 ticks
}

void TitleWidget::handleTitlePacket(
    TitleAction action, const std::optional<std::string>& text, i32 fadeIn, i32 stay, i32 fadeOut)
{
    switch (action) {
        case TitleAction::Title:
            // 设置主标题
            if (text.has_value() && !text->empty()) {
                setTitle(*text);
            } else {
                m_title.active = false;
                m_title.text = std::nullopt;
            }
            break;

        case TitleAction::Subtitle:
            // 设置副标题
            if (text.has_value() && !text->empty()) {
                setSubtitle(*text);
            } else {
                m_subtitle.active = false;
                m_subtitle.text = std::nullopt;
            }
            break;

        case TitleAction::Actionbar:
            // 设置动作栏
            if (text.has_value() && !text->empty()) {
                setActionbar(*text);
            } else {
                m_actionbar.active = false;
                m_actionbar.text = std::nullopt;
            }
            break;

        case TitleAction::Times:
            // 设置动画时间
            setTimes(fadeIn, stay, fadeOut);
            break;

        case TitleAction::Clear:
            // 清除标题（不显示淡出动画）
            clear();
            break;

        case TitleAction::Reset:
            // 重置到默认状态
            reset();
            break;
    }
}

// ============================================================================
// Widget 接口
// ============================================================================

void TitleWidget::tick(f32 dt)
{
    // 更新主标题
    if (m_title.active) {
        m_title.elapsed += dt;
        if (m_title.elapsed >= m_title.remainingTime) {
            m_title.active = false;
            m_title.text = std::nullopt;
        }
    }

    // 更新副标题
    if (m_subtitle.active) {
        m_subtitle.elapsed += dt;
        if (m_subtitle.elapsed >= m_subtitle.remainingTime) {
            m_subtitle.active = false;
            m_subtitle.text = std::nullopt;
        }
    }

    // 更新动作栏
    if (m_actionbar.active) {
        m_actionbar.elapsed += dt;
        if (m_actionbar.elapsed >= m_actionbar.remainingTime) {
            m_actionbar.active = false;
            m_actionbar.text = std::nullopt;
        }
    }
}

void TitleWidget::paint(kagero::widget::PaintContext& ctx)
{
    // 渲染动作栏（在快捷栏上方）
    _renderActionbar(ctx);

    // 渲染标题和副标题（屏幕中央）
    _renderTitle(ctx);
}

// ============================================================================
// 渲染方法
// ============================================================================

void TitleWidget::_renderTitle(kagero::widget::PaintContext& ctx)
{
    const f32 screenWidth = static_cast<f32>(width());
    const f32 screenHeight = static_cast<f32>(height());

    // 计算标题位置（屏幕上方 1/4 处）
    const f32 titleY = screenHeight * TITLE_Y_RATIO;

    // 渲染主标题
    if (m_title.active && m_title.text.has_value()) {
        f32 alpha = _calculateAlpha(m_title);
        if (alpha > 0.0f) {
            const std::string& text = *m_title.text;

            // 计算文本宽度
            f32 textWidth = ctx.getTextWidth(text);
            f32 x = (screenWidth - textWidth) / 2.0f;

            // 应用透明度到颜色
            u32 textColor = static_cast<u32>(static_cast<u8>(alpha * 255.0f)) << 24 | (TITLE_COLOR & 0x00FFFFFF);
            u32 shadowColor = static_cast<u32>(static_cast<u8>(alpha * TITLE_SHADOW_ALPHA * 255.0f)) << 24 |
                (SHADOW_COLOR & 0x00FFFFFF);

            // 绘制阴影（偏移 2 像素）
            ctx.drawText(text, static_cast<i32>(x + 2.0f), static_cast<i32>(titleY + 2.0f), shadowColor);

            // 绘制文本
            ctx.drawText(text, static_cast<i32>(x), static_cast<i32>(titleY), textColor);
        }
    }

    // 渲染副标题（主标题下方）
    if (m_subtitle.active && m_subtitle.text.has_value() && m_title.active) {
        f32 alpha = _calculateAlpha(m_subtitle);
        if (alpha > 0.0f) {
            const std::string& text = *m_subtitle.text;

            // 计算副标题位置（主标题下方）
            f32 subtitleY = titleY + 20.0f;

            // 计算文本宽度
            f32 textWidth = ctx.getTextWidth(text);
            f32 x = (screenWidth - textWidth) / 2.0f;

            // 应用透明度
            u32 textColor = static_cast<u32>(static_cast<u8>(alpha * 255.0f)) << 24 | (TITLE_COLOR & 0x00FFFFFF);
            u32 shadowColor = static_cast<u32>(static_cast<u8>(alpha * TITLE_SHADOW_ALPHA * 255.0f)) << 24 |
                (SHADOW_COLOR & 0x00FFFFFF);

            // 绘制阴影
            ctx.drawText(text, static_cast<i32>(x + 1.0f), static_cast<i32>(subtitleY + 1.0f), shadowColor);

            // 绘制文本
            ctx.drawText(text, static_cast<i32>(x), static_cast<i32>(subtitleY), textColor);
        }
    }
}

void TitleWidget::_renderActionbar(kagero::widget::PaintContext& ctx)
{
    if (!m_actionbar.active || !m_actionbar.text.has_value()) {
        return;
    }

    f32 alpha = _calculateAlpha(m_actionbar);
    if (alpha <= 0.0f) {
        return;
    }

    const f32 screenWidth = static_cast<f32>(width());
    const f32 screenHeight = static_cast<f32>(height());

    // 计算动作栏位置（快捷栏上方）
    const f32 actionbarY = screenHeight * ACTIONBAR_Y_RATIO;

    const std::string& text = *m_actionbar.text;

    // 计算文本宽度
    f32 textWidth = ctx.getTextWidth(text);
    f32 x = (screenWidth - textWidth) / 2.0f;

    // 应用透明度
    u32 textColor = static_cast<u32>(static_cast<u8>(alpha * 255.0f)) << 24 | (TITLE_COLOR & 0x00FFFFFF);
    u32 shadowColor =
        static_cast<u32>(static_cast<u8>(alpha * TITLE_SHADOW_ALPHA * 255.0f)) << 24 | (SHADOW_COLOR & 0x00FFFFFF);

    // 绘制阴影
    ctx.drawText(text, static_cast<i32>(x + 1.0f), static_cast<i32>(actionbarY + 1.0f), shadowColor);

    // 绘制文本
    ctx.drawText(text, static_cast<i32>(x), static_cast<i32>(actionbarY), textColor);
}

f32 TitleWidget::_calculateAlpha(const TitleState& state) const
{
    if (!state.active) {
        return 0.0f;
    }

    const f32 elapsed = state.elapsed;
    const f32 fadeIn = state.fadeInTime;
    const f32 stay = state.stayTime;
    const f32 fadeOut = state.fadeOutTime;

    // 淡入阶段
    if (elapsed < fadeIn && fadeIn > 0.0f) {
        return elapsed / fadeIn;
    }

    // 停留阶段
    if (elapsed < fadeIn + stay) {
        return 1.0f;
    }

    // 淡出阶段
    const f32 fadeOutStart = fadeIn + stay;
    if (elapsed < fadeOutStart + fadeOut && fadeOut > 0.0f) {
        return 1.0f - (elapsed - fadeOutStart) / fadeOut;
    }

    return 0.0f;
}

} // namespace mc::client::ui::minecraft::widgets
