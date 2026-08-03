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

#include "ContainerWidget.hpp"
#include "client/ui/kagero/layout/algorithms/FlexLayout.hpp"
#include "client/ui/kagero/layout/algorithms/GridLayout.hpp"
#include "client/ui/kagero/layout/core/LayoutEngine.hpp"
#include "client/ui/kagero/layout/integration/WidgetLayoutAdaptor.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include "common/core/Types.hpp"

namespace mc::client::ui::kagero::widget {

void ContainerWidget::paint(PaintContext& ctx)
{
    if (!isVisible()) {
        return;
    }

    // 绘制背景和边框（透明色则跳过绘制）
    if (backgroundColor() != 0x00000000) {
        ctx.drawFilledRect(bounds(), backgroundColor());
    }
    if (borderColor() != 0x00000000) {
        ctx.drawBorder(bounds(), 1.0f, borderColor());
    }

    paintChildren(ctx);
}

void ContainerWidget::tick(f32 dt)
{
    tickChildren(dt);

    // 延迟布局：在 tick 中检查脏标记并执行布局，避免同一帧内重复布局
    if (m_layoutDirty) {
        relayout();
    }
}

// ========== 鼠标事件 ==========

bool ContainerWidget::onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods)
{
    return handleClickInChildren(mouseX, mouseY, button, mods);
}

bool ContainerWidget::onRelease(i32 mouseX, i32 mouseY, i32 button, i32 mods)
{
    return handleReleaseInChildren(mouseX, mouseY, button, mods);
}

bool ContainerWidget::onDoubleClick(i32 mouseX, i32 mouseY, i32 button, i32 mods)
{
    return handleDoubleClickInChildren(mouseX, mouseY, button, mods);
}

bool ContainerWidget::onRightClick(i32 mouseX, i32 mouseY, i32 mods)
{
    return handleRightClickInChildren(mouseX, mouseY, mods);
}

bool ContainerWidget::onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY, i32 button)
{
    return handleDragInChildren(mouseX, mouseY, deltaX, deltaY, button);
}

bool ContainerWidget::onDragStart(i32 mouseX, i32 mouseY, i32 button, i32 mods)
{
    return handleDragStartInChildren(mouseX, mouseY, button, mods);
}

bool ContainerWidget::onDragEnd(i32 mouseX, i32 mouseY, i32 button, bool dropped)
{
    return handleDragEndInChildren(mouseX, mouseY, button, dropped);
}

bool ContainerWidget::onScroll(i32 mouseX, i32 mouseY, f64 delta)
{
    return handleScrollInChildren(mouseX, mouseY, delta);
}

void ContainerWidget::updateHover(i32 mouseX, i32 mouseY)
{
    setHovered(isMouseOver(mouseX, mouseY));

    for (auto& child : m_children) {
        if (child->isVisible() && child->isActive()) {
            child->updateHover(mouseX, mouseY);
        }
    }
}

// ========== 键盘事件 ==========

bool ContainerWidget::onKey(i32 key, i32 scanCode, i32 action, i32 mods)
{
    return handleKeyInChildren(key, scanCode, action, mods);
}

bool ContainerWidget::onChar(u32 codePoint)
{
    return handleCharInChildren(codePoint);
}

// ========== 布局系统 ==========

void ContainerWidget::setLayoutType(ContainerLayoutType type)
{
    if (m_layoutType != type) {
        m_layoutType = type;
        requestLayout();
    }
}

void ContainerWidget::setFlexConfig(const layout::FlexConfig& config)
{
    m_flexConfig = config;
    // 仅在当前为 Flex 布局时才需要请求重新布局
    if (m_layoutType == ContainerLayoutType::Flex) {
        requestLayout();
    }
}

void ContainerWidget::setGridConfig(const layout::GridConfig& config)
{
    m_gridConfig = config;
    // 仅在当前为 Grid 布局时才需要请求重新布局
    if (m_layoutType == ContainerLayoutType::Grid) {
        requestLayout();
    }
}

void ContainerWidget::requestLayout()
{
    m_layoutDirty = true;
    // 向上传播布局请求，确保父容器也能重新布局
    if (parent() != nullptr) {
        if (auto* container = dynamic_cast<ContainerWidget*>(parent())) {
            container->requestLayout();
        }
    }
}

void ContainerWidget::relayout()
{
    if (!m_layoutDirty || m_layoutType == ContainerLayoutType::None) {
        return;
    }

    auto& engine = layout::LayoutEngine::instance();
    layout::WidgetLayoutAdaptor adaptor(this);

    switch (m_layoutType) {
        case ContainerLayoutType::Flex:
            engine.layoutFlex(&adaptor, bounds(), m_flexConfig);
            break;
        case ContainerLayoutType::Grid:
            engine.layoutGrid(&adaptor, bounds(), m_gridConfig);
            break;
        case ContainerLayoutType::Anchor:
            engine.layoutWith("anchor", &adaptor, bounds());
            break;
        default:
            break;
    }

    m_layoutDirty = false;

    // 递归布局子容器
    for (auto& child : m_children) {
        if (auto* container = dynamic_cast<ContainerWidget*>(child.get())) {
            if (container->layoutType() != ContainerLayoutType::None) {
                container->relayout();
            }
        }
    }
}

void ContainerWidget::onResize(i32 width, i32 height)
{
    Widget::onResize(width, height);
    requestLayout();
}

} // namespace mc::client::ui::kagero::widget
